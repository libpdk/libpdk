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
// Created by softboy on 2018/01/25.

#ifndef PDK_KERNEL_EVENT_DISPATCHER_UNIX_H
#define PDK_KERNEL_EVENT_DISPATCHER_UNIX_H

#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/internal/AbstractEventDispatcherPrivate.h"
#include "pdk/kernel/CoreUnix.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/kernel/TimerInfoUnix.h"
#include "pdk/base/os/thread/Atomic.h"

#include <list>
#include <vector>
#include <map>

namespace pdk {
namespace kernel {

namespace internal {
class EventDispatcherUNIXPrivate;
} // internal

using pdk::os::thread::AtomicInt;

struct PDK_CORE_EXPORT SocketNotifierSetUNIX final
{
   inline SocketNotifierSetUNIX() noexcept;
   inline bool isEmpty() const noexcept;
   inline short events() const noexcept;
   SocketNotifier *m_notifiers[3];
};

struct ThreadPipe
{
   ThreadPipe();
   ~ThreadPipe();
   
   bool init();
   pollfd prepare() const;
   
   void wakeUp();
   int check(const pollfd &pfd);
   
   // note for eventfd(7) support:
   // if m_fds[1] is -1, then eventfd(7) is in use and is stored in fds[0]
   int m_fds[2];
   AtomicInt m_wakeUps;
   
#if defined(PDK_OS_VXWORKS)
   static constexpr int NAME_LENGTH = 20;
   char m_name[NAME_LENGTH];
#endif
};

using internal::EventDispatcherUNIXPrivate;

class PDK_CORE_EXPORT EventDispatcherUNIX : public AbstractEventDispatcher
{
   PDK_DECLARE_PRIVATE(EventDispatcherUNIX);
   
public:
   explicit EventDispatcherUNIX(Object *parent = 0);
   ~EventDispatcherUNIX();
   
   bool processEvents(EventLoop::ProcessEventsFlags flags) override;
   bool hasPendingEvents() override;
   
   void registerSocketNotifier(SocketNotifier *notifier) final;
   void unregisterSocketNotifier(SocketNotifier *notifier) final;
   
   void registerTimer(int timerId, int interval, pdk::TimerType timerType, Object *object) final;
   bool unregisterTimer(int timerId) final;
   bool unregisterTimers(Object *object) final;
   std::list<TimerInfo> getRegisteredTimers(Object *object) const final;
   
   int remainingTime(int timerId) final;
   
   void wakeUp() final;
   void interrupt() final;
   void flush() override;
   
protected:
   EventDispatcherUNIX(EventDispatcherUNIXPrivate &dd, Object *parent = nullptr);
};

namespace internal {

class PDK_CORE_EXPORT EventDispatcherUNIXPrivate : public AbstractEventDispatcherPrivate
{
   PDK_DECLARE_PUBLIC(EventDispatcherUNIX);
   
public:
   EventDispatcherUNIXPrivate();
   ~EventDispatcherUNIXPrivate();
   
   int getActivateTimers();
   
   void markPendingSocketNotifiers();
   int getActivateSocketNotifiers();
   void setSocketNotifierPending(SocketNotifier *notifier);
   
   ThreadPipe m_threadPipe;
   std::vector<pollfd> m_pollfds;
   
   std::map<int, SocketNotifierSetUNIX> m_socketNotifiers;
   std::list<SocketNotifier *> m_pendingNotifiers;
   
   TimerInfoList m_timerList;
   AtomicInt m_interrupt; // bool
};

} // internal

inline SocketNotifierSetUNIX::SocketNotifierSetUNIX() noexcept
{
   m_notifiers[0] = 0;
   m_notifiers[1] = 0;
   m_notifiers[2] = 0;
}

inline bool SocketNotifierSetUNIX::isEmpty() const noexcept
{
   return !m_notifiers[0] && !m_notifiers[1] && !m_notifiers[2];
}

inline short SocketNotifierSetUNIX::events() const noexcept
{
   short result = 0;
   if (m_notifiers[0]) {
      result |= POLLIN;
   }
   if (m_notifiers[1]) {
      result |= POLLOUT;
   }
   if (m_notifiers[2]) {
      result |= POLLPRI;
   }
   return result;
}

} // kernel
} // pdk

#endif // PDK_KERNEL_EVENT_DISPATCHER_UNIX_H
