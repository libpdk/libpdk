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
#include "pdk/stdext/utility/Algorithms.h"

#include <errno.h>
#include <cstdio>
#include <cstdlib>

// TODO remove me 
#define PDK_NO_EVENTFD

#ifndef PDK_NO_EVENTFD
#  include <sys/eventfd.h>
#endif

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
   switch (type) {
   case SocketNotifier::Type::Read:
      return "Read";
      break;
   case SocketNotifier::Type::Write:
      return "Write";
      break;
   case SocketNotifier::Type::Exception:
      return "Exception";
      break;
   }
   PDK_UNREACHABLE();
}

#if defined(PDK_OS_VXWORKS)
void init_thread_pipe_fd(int fd)
{
   int ret = fcntl(fd, F_SETFD, FD_CLOEXEC);
   if (ret == -1) {
      perror("EventDispatcherUNIXPrivate: Unable to init thread pipe");
   }
   int flags = fcntl(fd, F_GETFL);
   if (flags == -1) {
      perror("EventDispatcherUNIXPrivate: Unable to get flags on thread pipe");
   }
   ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
   if (ret == -1) {
      perror("EventDispatcherUNIXPrivate: Unable to set flags on thread pipe");
   }
}
#endif

}

ThreadPipe::ThreadPipe()
{
   m_fds[0] = -1;
   m_fds[1] = -1;
#if defined(PDK_OS_VXWORKS)
   m_name[0] = '\0';
#endif
}

ThreadPipe::~ThreadPipe()
{
   if (m_fds[0] >= 0) {
      close(m_fds[0]);
   }
   if (m_fds[1] >= 0) {
      close(m_fds[1]);
   }
#if defined(PDK_OS_VXWORKS)
   pipeDevDelete(name, true);
#endif
}

bool ThreadPipe::init()
{
#if defined(PDK_OS_VXWORKS)
   std::snprintf(m_name, sizeof(m_name), "/pipe/pdk_%08x", static_cast<int>(taskIdSelf()));
   // make sure there is no pipe with this name
   pipeDevDelete(name, true);
   // create the pipe
   if (pipeDevCreate(name, 128 /*maxMsg*/, 1 /*maxLength*/) != OK) {
      perror("ThreadPipe: Unable to create thread pipe device %s", name);
      return false;
   }
   if ((m_fds[0] = open(name, O_RDWR, 0)) < 0) {
      perror("ThreadPipe: Unable to open pipe device %s", name);
      return false;
   }
   initThreadPipeFD(fds[0]);
   m_fds[1] = m_fds[0];
#else
#  ifndef PDK_NO_EVENTFD
   if ((m_fds[0] = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) >= 0)
      return true;
#  endif
   if (safe_pipe(m_fds, O_NONBLOCK) == -1) {
      perror("ThreadPipe: Unable to create pipe");
      return false;
   }
#endif
   return true;
}

pollfd ThreadPipe::prepare() const
{
   return make_pollfd(m_fds[0], POLLIN);
}

void ThreadPipe::wakeUp()
{
   if (m_wakeUps.testAndSetAcquire(0, 1)) {
#ifndef PDK_NO_EVENTFD
      if (m_fds[1] == -1) {
         // eventfd
         eventfd_t value = 1;
         int ret;
         EINTR_LOOP(ret, eventfd_write(m_fds[0], value));
         return;
      }
#endif
      char c = 0;
      safe_write(m_fds[1], &c, 1);
   }
}

int ThreadPipe::check(const pollfd &pfd)
{
   PDK_ASSERT(pfd.fd == m_fds[0]);
   char c[16];
   const int readyRead = pfd.revents & POLLIN;
   if (readyRead) {
      // consume the data on the thread pipe so that
      // poll doesn't immediately return next time
#if defined(PDK_OS_VXWORKS)
      ::read(m_fds[0], c, sizeof(c));
      ::ioctl(m_fds[0], FIOFLUSH, 0);
#else
#  ifndef PDK_NO_EVENTFD
      if (m_fds[1] == -1) {
         // eventfd
         eventfd_t value;
         eventfd_read(m_fds[0], &value);
      } else
#  endif
      {
         while (::read(m_fds[0], c, sizeof(c)) > 0) {}
      }
#endif
      if (!m_wakeUps.testAndSetRelease(1, 0)) {
         // hopefully, this is dead code
         // qWarning("ThreadPipe: internal error, wakeUps.testAndSetRelease(1, 0) failed!");
      }
   }
   return readyRead;
}

namespace internal {

EventDispatcherUNIXPrivate::EventDispatcherUNIXPrivate()
{
   if (PDK_UNLIKELY(m_threadPipe.init() == false)) {
      // qFatal("QEventDispatcherUNIXPrivate(): Can not continue without a thread pipe");
   }
}

EventDispatcherUNIXPrivate::~EventDispatcherUNIXPrivate()
{
   pdk::stdext::delete_all(m_timerList);
}

void EventDispatcherUNIXPrivate::setSocketNotifierPending(SocketNotifier *notifier)
{
   PDK_ASSERT(notifier);
   auto iter = std::find(m_pendingNotifiers.cbegin(),
                         m_pendingNotifiers.cend(), notifier);
   if (iter != m_pendingNotifiers.cend()) {
      return;
   }
   m_pendingNotifiers.push_back(notifier);
}

int EventDispatcherUNIXPrivate::getActivateTimers()
{
   return m_timerList.getActivateTimers();
}

void EventDispatcherUNIXPrivate::markPendingSocketNotifiers()
{
   for (const pollfd &pfd : pdk::as_const(m_pollfds)) {
      if (pfd.fd < 0 || pfd.revents == 0) {
         continue;
      }
      auto iter = m_socketNotifiers.find(pfd.fd);
      PDK_ASSERT(iter != m_socketNotifiers.end());
      const SocketNotifierSetUNIX &snSet = iter->second;
      static const struct
      {
         SocketNotifier::Type m_type;
         short m_flags;
      } notifierFlags[] = {
         {SocketNotifier::Type::Read,      POLLIN | POLLHUP | POLLERR},
         {SocketNotifier::Type::Write,     POLLOUT | POLLHUP | POLLERR},
         {SocketNotifier::Type::Exception, POLLPRI | POLLHUP | POLLERR}
      };
      for (const auto &nflag : notifierFlags) {
         SocketNotifier *notifier = snSet.m_notifiers[static_cast<int>(nflag.m_type)];
         if (!notifier) {
            continue;
         }
         if (pfd.revents & POLLNVAL) {
//            qWarning("SocketNotifier: Invalid socket %d with type %s, disabling...",
//                     it.key(), socketType(n.type));
            notifier->setEnabled(false);
         }
         if (pfd.revents & nflag.m_flags) {
            setSocketNotifierPending(notifier);
         }
      }
   }
   m_pollfds.clear();
}

int EventDispatcherUNIXPrivate::getActivateSocketNotifiers()
{
   markPendingSocketNotifiers();
   if (m_pendingNotifiers.empty()) {
      return 0;
   }
   int nActivated = 0;
   Event event(Event::Type::SocketActive);
   while (!m_pendingNotifiers.empty()) {
      SocketNotifier *notifier = m_pendingNotifiers.front();
      m_pendingNotifiers.pop_front();
      CoreApplication::sendEvent(notifier, &event);
      ++nActivated;
   }
   return nActivated;
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

void EventDispatcherUNIX::registerTimer(int timerId, int interval, pdk::TimerType timerType, Object *object)
{
#ifndef PDK_NO_DEBUG
   if (timerId < 1 || interval < 0 || !object) {
      //qWarning("EventDispatcherUNIX::registerTimer: invalid arguments");
      return;
   } else if (object->thread() != thread() || thread() != Thread::getCurrentThread()) {
      //qWarning("EventDispatcherUNIX::registerTimer: timers cannot be started from another thread");
      return;
   }
#endif
   PDK_D(EventDispatcherUNIX);
   implPtr->m_timerList.registerTimer(timerId, interval, timerType, object);
}

bool EventDispatcherUNIX::unregisterTimer(int timerId)
{
#ifndef PDK_NO_DEBUG
   if (timerId < 1) {
      // qWarning("EventDispatcherUNIX::unregisterTimer: invalid argument");
      return false;
   } else if (thread() != Thread::getCurrentThread()) {
      // qWarning("EventDispatcherUNIX::unregisterTimer: timers cannot be stopped from another thread");
      return false;
   }
#endif
   PDK_D(EventDispatcherUNIX);
   return implPtr->m_timerList.unregisterTimer(timerId);
}

bool EventDispatcherUNIX::unregisterTimers(Object *object)
{
#ifndef PDK_NO_DEBUG
   if (!object) {
      // qWarning("QEventDispatcherUNIX::unregisterTimers: invalid argument");
      return false;
   } else if (object->thread() != thread() || thread() != Thread::getCurrentThread()) {
      // qWarning("EventDispatcherUNIX::unregisterTimers: timers cannot be stopped from another thread");
      return false;
   }
#endif
   PDK_D(EventDispatcherUNIX);
   return implPtr->m_timerList.unregisterTimers(object);
}

std::list<EventDispatcherUNIX::TimerInfo>
EventDispatcherUNIX::getRegisteredTimers(Object *object) const
{
   if (!object) {
      // qWarning("QEventDispatcherUNIX:registeredTimers: invalid argument");
      return std::list<TimerInfo>();
   }
   PDK_D(const EventDispatcherUNIX);
   return implPtr->m_timerList.getRegisteredTimers(object);
}

void EventDispatcherUNIX::registerSocketNotifier(SocketNotifier *notifier)
{
   PDK_ASSERT(notifier);
   int sockfd = notifier->getSocket();
   SocketNotifier::Type type = notifier->getType();
#ifndef PDK_NO_DEBUG
   if (notifier->thread() != thread() || thread() != Thread::getCurrentThread()) {
      // qWarning("SocketNotifier: socket notifiers cannot be enabled from another thread");
      return;
   }
#endif
   PDK_D(EventDispatcherUNIX);
   SocketNotifierSetUNIX &snSet = implPtr->m_socketNotifiers[sockfd];
   int typeValue = static_cast<int>(type);
   if (snSet.m_notifiers[typeValue] && snSet.m_notifiers[typeValue] != notifier) {
//      qWarning("%s: Multiple socket notifiers for same socket %d and type %s",
//                       Q_FUNC_INFO, sockfd, socketType(type));
   }
   snSet.m_notifiers[typeValue] = notifier;
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

