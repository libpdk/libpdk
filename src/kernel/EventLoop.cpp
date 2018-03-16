// @copyright 2017-2018 zzu_softboy <zzu_softboy@163.com>
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Created by softboy on 2018/01/23.

#include "pdk/kernel/internal/EventLoopPrivate.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/internal/AbstractEventDispatcherPrivate.h"
#include "pdk/kernel/EventLoop.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/global/Logging.h"
#include "pdk/kernel/ElapsedTimer.h"

namespace pdk {
namespace kernel {

using pdk::os::thread::internal::ThreadPrivate;

EventLoop::EventLoop(Object *parent)
   : Object(*new EventLoopPrivate, parent)
{
   PDK_D(EventLoop);
   if (!CoreApplication::getInstance() && CoreApplicationPrivate::threadRequiresCoreApplication()) {
      //warning_stream("EventLoop: Cannot be used without CoreApplication");
   } else if (!implPtr->m_threadData->m_eventDispatcher.load()) {
      ThreadPrivate::createEventDispatcher(implPtr->m_threadData);
   }
}

EventLoop::~EventLoop()
{}

bool EventLoop::processEvents(ProcessEventsFlags flags)
{
   PDK_D(EventLoop);
   if (!implPtr->m_threadData->m_eventDispatcher.load()) {
      return false;
   }
   return implPtr->m_threadData->m_eventDispatcher.load()->processEvents(flags);
}

int EventLoop::exec(ProcessEventsFlags flags)
{
   PDK_D(EventLoop);
   //we need to protect from race condition with Thread::exit
   std::unique_lock<std::mutex> locker(static_cast<ThreadPrivate *>(ObjectPrivate::get(implPtr->m_threadData->m_thread))->m_mutex);
   if (implPtr->m_threadData->m_quitNow) {
      return -1;
   }
   if (implPtr->m_inExec) {
      warning_stream("EventLoop::exec: instance %p has already called exec()", (void *)this);
      return -1;
   }
   struct LoopReference
   {
      EventLoopPrivate *m_implPtr;
      std::unique_lock<std::mutex> &m_locker;
      bool m_exceptionCaught;
      LoopReference(EventLoopPrivate *implPtr, std::unique_lock<std::mutex> &locker)
         : m_implPtr(implPtr),
           m_locker(locker),
           m_exceptionCaught(true)
      {
         m_implPtr->m_inExec = true;
         m_implPtr->m_exit.storeRelease(false);
         ++m_implPtr->m_threadData->m_loopLevel;
         m_implPtr->m_threadData->m_eventLoops.push_back(m_implPtr->getApiPtr());
         m_locker.unlock();
      }
      
      ~LoopReference()
      {
         if (m_exceptionCaught) {
            warning_stream("pdk has caught an exception thrown from an event handler. Throwing\n"
                           "exceptions from an event handler is not supported in pdk.\n"
                           "You must not let any exception whatsoever propagate through pdk code.\n"
                           "If that is not possible, in pdk 5 you must at least reimplement\n"
                           "CoreApplication::notify() and catch all exceptions there.\n");
         }
         m_locker.lock();
         EventLoop *eventLoop = m_implPtr->m_threadData->m_eventLoops.back();
         m_implPtr->m_threadData->m_eventLoops.pop_back();
         PDK_ASSERT_X(eventLoop == m_implPtr->getApiPtr(), "EventLoop::exec()", "internal error");
         PDK_UNUSED(eventLoop); // --release warning
         m_implPtr->m_inExec = false;
         --m_implPtr->m_threadData->m_loopLevel;
      }
   };
   LoopReference ref(implPtr, locker);
   // remove posted quit events when entering a new event loop
   CoreApplication *app = CoreApplication::getInstance();
   if (app && app->getThread() == getThread()) {
      CoreApplication::removePostedEvents(app, Event::Type::Quit);
   }
   while (!implPtr->m_exit.loadAcquire()) {
      processEvents(flags | WaitForMoreEvents | EventLoopExec);
   }
   ref.m_exceptionCaught = false;
   return implPtr->m_returnCode.load();
}

void EventLoop::processEvents(ProcessEventsFlags flags, int maxTime)
{
   PDK_D(EventLoop);
   if (!implPtr->m_threadData->m_eventDispatcher.load()) {
      return;
   }
   ElapsedTimer start;
   start.start();
   while (processEvents(flags & ~WaitForMoreEvents)) {
      if (start.elapsed() > maxTime) {
         break;
      }
   }
}

void EventLoop::exit(int returnCode)
{
   PDK_D(EventLoop);
   if (!implPtr->m_threadData->m_eventDispatcher.load()) {
      return;
   }
   implPtr->m_returnCode.store(returnCode);
   implPtr->m_exit.storeRelease(true);
   implPtr->m_threadData->m_eventDispatcher.load()->interrupt();
}

bool EventLoop::isRunning() const
{
   PDK_D(const EventLoop);
   return !implPtr->m_exit.loadAcquire();
}

void EventLoop::wakeUp()
{
   PDK_D(EventLoop);
   if (!implPtr->m_threadData->m_eventDispatcher.load()) {
      return;
   }
   implPtr->m_threadData->m_eventDispatcher.load()->wakeUp();
}

bool EventLoop::event(Event *event)
{
   if (event->getType() == Event::Type::Quit) {
      quit();
      return true;
   } else {
      return Object::event(event);
   }
}

void EventLoop::quit()
{
   exit(0);
}

namespace internal {

class EventLoopLockerPrivate
{
public:
   explicit EventLoopLockerPrivate(EventLoopPrivate *loop)
      : m_loop(loop), 
        m_type(EventLoop)
   {
      m_loop->ref();
   }
   
   explicit EventLoopLockerPrivate(ThreadPrivate *thread)
      : m_thread(thread),
        m_type(Thread)
   {
      m_thread->ref();
   }
   
   explicit EventLoopLockerPrivate(CoreApplicationPrivate *app)
      : m_app(app),
        m_type(Application)
   {
      m_app->ref();
   }
   
   ~EventLoopLockerPrivate()
   {
      switch (m_type)
      {
      case EventLoop:
         m_loop->deref();
         break;
      case Thread:
         m_thread->deref();
         break;
      default:
         m_app->deref();
         break;
      }
   }
   
private:
   union {
      EventLoopPrivate *m_loop;
      ThreadPrivate *m_thread;
      CoreApplicationPrivate *m_app;
   };
   enum Type {
      EventLoop,
      Thread,
      Application
   };
   const Type m_type;
};

} // internal

EventLoopLocker::EventLoopLocker()
   : m_implPtr(new EventLoopLockerPrivate(dynamic_cast<CoreApplicationPrivate *>(ObjectPrivate::get(CoreApplication::getInstance()))))
{}

EventLoopLocker::EventLoopLocker(EventLoop *loop)
   : m_implPtr(new EventLoopLockerPrivate(dynamic_cast<EventLoopPrivate *>(ObjectPrivate::get(loop))))
{}

EventLoopLocker::EventLoopLocker(Thread *thread)
   : m_implPtr(new EventLoopLockerPrivate(dynamic_cast<ThreadPrivate *>(ObjectPrivate::get(thread))))
{}

EventLoopLocker::~EventLoopLocker()
{
   delete m_implPtr;
}

} // kernel
} // pdk
