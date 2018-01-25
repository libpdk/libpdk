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

uint Thread::stackSize() const
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
