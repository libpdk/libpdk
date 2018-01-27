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
// Created by softboy on 2018/01/27.

#include "pdk/global/PlatformDefs.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/SocketNotifier.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/kernel/EventdispatcherUnix.h"
#include "pdk/kernel/CoreUnix.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"

#include <errno.h>
#include <cstdio>
#include <cstdlib>

//#ifndef PDK_NO_EVENTFD
//#  include <sys/eventfd.h>
//#endif

// VxWorks doesn't correctly set the _POSIX_... options
#if defined(PDK_OS_VXWORKS)
#  if defined(_POSIX_MONOTONIC_CLOCK) && (_POSIX_MONOTONIC_CLOCK <= 0)
#    undef _POSIX_MONOTONIC_CLOCK
#    define _POSIX_MONOTONIC_CLOCK 1
#  endif
#  include <pipeDrv.h>
#  include <sys/time.h>
#endif

#if (_POSIX_MONOTONIC_CLOCK-0 <= 0)
#  include <sys/times.h>
#endif

namespace pdk {
namespace kernel {

namespace {

const char *socketType(SocketNotifier::Type type)
{
}

#if defined(PDK_OS_VXWORKS)
static void init_thread_pipe_fd(int fd)
{
}
#endif

}

ThreadPipe::ThreadPipe()
{
}

ThreadPipe::~ThreadPipe()
{
}

bool ThreadPipe::init()
{
   return true;
}

pollfd ThreadPipe::prepare() const
{
}

void ThreadPipe::wakeUp()
{
}

int ThreadPipe::check(const pollfd &pfd)
{
}

namespace internal {

EventDispatcherUNIXPrivate::EventDispatcherUNIXPrivate()
{
}

EventDispatcherUNIXPrivate::~EventDispatcherUNIXPrivate()
{
}

void EventDispatcherUNIXPrivate::setSocketNotifierPending(SocketNotifier *notifier)
{
}

int EventDispatcherUNIXPrivate::activateTimers()
{
}

void EventDispatcherUNIXPrivate::markPendingSocketNotifiers()
{
}

int EventDispatcherUNIXPrivate::activateSocketNotifiers()
{
}

} // internal

EventDispatcherUNIX::EventDispatcherUNIX(Object *parent)
    : AbstractEventDispatcher(*new EventDispatcherUNIXPrivate, parent)
{}

EventDispatcherUNIX::EventDispatcherUNIX(EventDispatcherUNIXPrivate &dd, Object *parent)
    : AbstractEventDispatcher(dd, parent)
{}

EventDispatcherUNIX::~EventDispatcherUNIX()
{}

void EventDispatcherUNIX::registerTimer(int timerId, int interval, pdk::TimerType timerType, Object *obj)
{
}

bool EventDispatcherUNIX::unregisterTimer(int timerId)
{
}

bool EventDispatcherUNIX::unregisterTimers(Object *object)
{
}

std::list<EventDispatcherUNIX::TimerInfo>
EventDispatcherUNIX::registeredTimers(Object *object) const
{
}

void EventDispatcherUNIX::registerSocketNotifier(SocketNotifier *notifier)
{
}

void EventDispatcherUNIX::unregisterSocketNotifier(SocketNotifier *notifier)
{
}

bool EventDispatcherUNIX::processEvents(EventLoop::ProcessEventsFlags flags)
{
}

bool EventDispatcherUNIX::hasPendingEvents()
{
}

int EventDispatcherUNIX::remainingTime(int timerId)
{
}

void EventDispatcherUNIX::wakeUp()
{
}

void EventDispatcherUNIX::interrupt()
{
}

void EventDispatcherUNIX::flush()
{}

} // kernel
} // pdk

