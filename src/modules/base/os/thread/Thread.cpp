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

namespace internal {

ThreadData::ThreadData(int initialRefCount)
   : m_ref(initialRefCount),
     m_loopLevel(0),
     m_scopeLevel(0),
     m_thread(0),
     m_threadId(0),
     m_eventDispatcher(0),
     m_quitNow(false),
     m_canWait(true),
     m_isAdopted(false),
     m_requiresCoreApplication(true)
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
   for (int i = 0; i < m_postEventList.size(); ++i) {
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
}

void ThreadData::deref()
{
}

AdoptedThread::AdoptedThread(ThreadData *data)
   : Thread(*new ThreadPrivate(data))
{
}

AdoptedThread::~AdoptedThread()
{
}

void AdoptedThread::run()
{
}

ThreadPrivate::ThreadPrivate(ThreadData *d)
   : ObjectPrivate(),
     m_running(false),
     m_finished(false),
     m_isInFinish(false),
     m_interruptionRequested(false),
     m_exited(false),
     m_returnCode(-1),
     m_stackSize(0),
     m_priority(Thread::Priority::InheritPriority),
     m_data(d)
{
}

ThreadPrivate::~ThreadPrivate()
{
}

} // internal

Thread *Thread::getCurrentThread()
{
}

Thread::Thread(Object *parent)
   : Object(*(new ThreadPrivate), parent)
{
}

Thread::Thread(ThreadPrivate &dd, Object *parent)
   : Object(dd, parent)
{
}

Thread::~Thread()
{
}

bool Thread::isFinished() const
{
}

bool Thread::isRunning() const
{
}

void Thread::setStackSize(uint stackSize)
{
}

uint Thread::getStackSize() const
{
}

int Thread::exec()
{
}

void Thread::exit(int returnCode)
{
}

void Thread::quit()
{}

void Thread::run()
{
}

void Thread::setPriority(Priority priority)
{
}

Thread::Priority Thread::getPriority() const
{
}

int Thread::getLoopLevel() const
{
}

AbstractEventDispatcher *Thread::getEventDispatcher() const
{
}

void Thread::setEventDispatcher(AbstractEventDispatcher *eventDispatcher)
{
}

bool Thread::event(Event *event)
{
}

void Thread::requestInterruption()
{ 
}

bool Thread::isInterruptionRequested() const
{
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
