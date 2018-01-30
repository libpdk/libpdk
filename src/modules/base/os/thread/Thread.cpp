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
// Created by softboy on 2017/01/25.

#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/ReadWriteLock.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/EventLoop.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"

namespace pdk {
namespace os {
namespace thread {

using pdk::kernel::AbstractEventDispatcher;
using pdk::kernel::internal::CoreApplicationPrivate;
using pdk::kernel::EventLoop;

namespace internal {

ThreadData::ThreadData(int initialRefCount)
   : m_loopLevel(0),
     m_scopeLevel(0),
     m_thread(0),
     m_threadId(0),
     m_eventDispatcher(0),
     m_quitNow(false),
     m_canWait(true),
     m_isAdopted(false),
     m_requiresCoreApplication(true),
     m_ref(initialRefCount)
{
}

ThreadData::~ThreadData()
{
   PDK_ASSERT(m_ref.load() == 0);
   // In the odd case that pdk is running on a secondary thread, the main
   // thread instance will have been dereffed asunder because of the deref in
   // ThreadData::current() and the deref in the pthread_destroy. To avoid
   // crashing during CoreApplicationData's global static cleanup we need to
   // safeguard the main thread here.. This fix is a bit crude, but it solves
   // the problem...
   if (this->m_thread == CoreApplicationPrivate::sm_theMainThread) {
      CoreApplicationPrivate::sm_theMainThread = nullptr;
      ThreadData::clearCurrentThreadData();
   }
   Thread *tempPtr = m_thread;
   m_thread = nullptr;
   delete tempPtr;
   for (size_t i = 0; i < m_postEventList.size(); ++i) {
      const PostEvent &postEvent = m_postEventList.at(i);
      if (postEvent.m_event) {
         --postEvent.m_receiver->getImplPtr()->m_postedEvents;
         postEvent.m_event->m_posted = false;
         delete postEvent.m_event;
      }
   }
}

void ThreadData::ref()
{
   (void) m_ref.ref();
   PDK_ASSERT(m_ref.load() != 0);
}

void ThreadData::deref()
{
   if (m_ref.deref()) {
      delete this;
   }
}

AdoptedThread::AdoptedThread(ThreadData *data)
   : Thread(*new ThreadPrivate(data))
{
   getImplPtr()->m_running = true;
   getImplPtr()->m_finished = false;
   init();
   // fprintf(stderr, "new AdoptedThread = %p\n", this);
}

AdoptedThread::~AdoptedThread()
{
   // fprintf(stderr, "~AdoptedThread = %p\n", this);
}

void AdoptedThread::run()
{
   // this function should never be called
   // qFatal("AdoptedThread::run(): Internal error, this implementation should never be called.");
}

ThreadPrivate::ThreadPrivate(ThreadData *d)
   : ObjectPrivate(),
     m_data(d),
     m_running(false),
     m_finished(false),
     m_isInFinish(false),
     m_interruptionRequested(false),
     m_exited(false),
     m_returnCode(-1),
     m_stackSize(0),
     m_priority(Thread::Priority::InheritPriority)
{
#ifdef PDK_OS_WIN
   m_handle = nullptr;
   m_waiters = 0;
   m_terminationEnabled = true;
   m_terminatePending = false;
#endif
   if (!m_data) {
      m_data = new ThreadData;
   }
}

ThreadPrivate::~ThreadPrivate()
{
   m_data->deref();
}

} // internal

Thread *Thread::getCurrentThread()
{
   ThreadData *data = ThreadData::current();
   PDK_ASSERT(data != 0);
   return data->m_thread;
}

Thread::Thread(Object *parent)
   : Object(*(new ThreadPrivate), parent)
{
   PDK_D(Thread);
   implPtr->m_data->m_thread = this;
}

Thread::Thread(ThreadPrivate &dd, Object *parent)
   : Object(dd, parent)
{
   PDK_D(Thread);
   // fprintf(stderr, "ThreadData %p taken from private data for thread %p\n", d->data, this);
   implPtr->m_data->m_thread = this;
}

Thread::~Thread()
{
   PDK_D(Thread);
   {
      std::unique_lock locker(implPtr->m_mutex);
      if (implPtr->m_isInFinish) {
         locker.unlock();
         wait();
         locker.lock();
      }
      if (implPtr->m_running && !implPtr->m_finished && !implPtr->m_data->m_isAdopted) {
         // qFatal("Thread: Destroyed while thread is still running");
      }
      implPtr->m_data->m_thread = nullptr;
   }
}

bool Thread::isFinished() const
{
   PDK_D(const Thread);
   std::scoped_lock locker(implPtr->m_mutex);
   return implPtr->m_finished || implPtr->m_isInFinish;
}

bool Thread::isRunning() const
{
   PDK_D(const Thread);
   std::scoped_lock locker(implPtr->m_mutex);
   return implPtr->m_running && !implPtr->m_isInFinish;
}

void Thread::setStackSize(uint stackSize)
{
   PDK_D(Thread);
   std::scoped_lock locker(implPtr->m_mutex);
   PDK_ASSERT_X(!implPtr->m_running, "Thread::setStackSize",
                "cannot change stack size while the thread is running");
   implPtr->m_stackSize = stackSize;
}

uint Thread::getStackSize() const
{
   PDK_D(const Thread);
   std::scoped_lock locker(implPtr->m_mutex);
   return implPtr->m_stackSize;
}

int Thread::exec()
{
   PDK_D(Thread);
   std::unique_lock locker(implPtr->m_mutex);
   implPtr->m_data->m_quitNow = false;
   if (implPtr->m_exited) {
      implPtr->m_exited = false;
      return implPtr->m_returnCode;
   }
   locker.unlock();
   EventLoop eventLoop;
   int returnCode = eventLoop.exec();
   locker.lock();
   implPtr->m_exited = false;
   implPtr->m_returnCode = -1;
   return returnCode;
}

void Thread::exit(int returnCode)
{
   PDK_D(Thread);
   std::scoped_lock locker(implPtr->m_mutex);
   implPtr->m_exited = true;
   implPtr->m_returnCode = returnCode;
   implPtr->m_data->m_quitNow = true;
   for (size_t i = 0; i < implPtr->m_data->m_eventLoops.size(); ++i) {
      EventLoop *eventLoop = implPtr->m_data->m_eventLoops.at(i);
      eventLoop->exit(returnCode);
   }
}

void Thread::quit()
{
   exit();
}

void Thread::run()
{
   (void) exec();
}

void Thread::setPriority(Priority priority)
{
   PDK_D(Thread);
   std::scoped_lock locker(implPtr->m_mutex);
   if (!implPtr->m_running) {
      // qWarning("Thread::setPriority: Cannot set priority, thread is not running");
      return;
   }
   implPtr->setPriority(priority);
}

Thread::Priority Thread::getPriority() const
{
   PDK_D(const Thread);
   std::scoped_lock locker(implPtr->m_mutex);
   return implPtr->m_priority;
}

int Thread::getLoopLevel() const
{
   PDK_D(const Thread);
   return implPtr->m_data->m_eventLoops.size();
}

AbstractEventDispatcher *Thread::getEventDispatcher() const
{
   PDK_D(const Thread);
   return implPtr->m_data->m_eventDispatcher.load();
}

void Thread::setEventDispatcher(AbstractEventDispatcher *eventDispatcher)
{
   PDK_D(Thread);
   if (implPtr->m_data->hasEventDispatcher()) {
      // qWarning("Thread::setEventDispatcher: An event dispatcher has already been created for this thread");
   } else {
      eventDispatcher->moveToThread(this);
      if (eventDispatcher->getThread() == this) {
         implPtr->m_data->m_eventDispatcher = eventDispatcher;
      } else {
         // qWarning("Thread::setEventDispatcher: Could not move event dispatcher to target thread");
      }
   }
}

bool Thread::event(Event *event)
{
   if (event->getType() == Event::Type::Quit) {
      quit();
      return true;
   } else {
      return Object::event(event);
   }
}

void Thread::requestInterruption()
{
   PDK_D(Thread);
   std::scoped_lock locker(implPtr->m_mutex);
   if (!implPtr->m_running || implPtr->m_finished || implPtr->m_isInFinish) {
      return;
   }
   if (this == CoreApplicationPrivate::sm_theMainThread) {
      // qWarning("Thread::requestInterruption has no effect on the main thread");
      return;
   }
   implPtr->m_interruptionRequested = true;
}

bool Thread::isInterruptionRequested() const
{
   PDK_D(const Thread);
   std::scoped_lock locker(implPtr->m_mutex);
   if (!implPtr->m_running || implPtr->m_finished || implPtr->m_isInFinish) {
      return false;  
   }
   return implPtr->m_interruptionRequested;
}

namespace internal {

DaemonThread::DaemonThread(Object *parent)
   : Thread(parent)
{
   
}

DaemonThread::~DaemonThread()
{
}

} // internal

} // thread
} // os
} // pdk
