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

#ifndef PDK_KERNEL_ABSTRACT_EVENT_DISPATCHER_H
#define PDK_KERNEL_ABSTRACT_EVENT_DISPATCHER_H

#include "pdk/kernel/Object.h"
#include "pdk/kernel/EventLoop.h"
#include <list>

namespace pdk {

// forward declare class with namespace
namespace ds {
class ByteArray;
} // ds

namespace kernel {

// forward declare class with namespace
namespace internal {
class AbstractEventDispatcherPrivate;
} // internal

class AbstractNativeEventFilter;
class SocketNotifier;
using pdk::ds::ByteArray;
using internal::AbstractEventDispatcherPrivate;

class PDK_CORE_EXPORT AbstractEventDispatcher : public Object
{
   PDK_DECLARE_PRIVATE(AbstractEventDispatcher);
public:
   struct TimerInfo
   {
      inline TimerInfo(int id, int interval, pdk::TimerType timerType)
         : m_timerId(id),
           m_interval(interval),
           m_timerType(timerType)
      {}
      int m_timerId;
      int m_interval;
      pdk::TimerType m_timerType;
   };
   
   explicit AbstractEventDispatcher(Object *parent = nullptr);
   ~AbstractEventDispatcher();
   
   static AbstractEventDispatcher *getInstance(Thread *thread = nullptr);
   virtual bool processEvents(EventLoop::ProcessEventsFlags flags) = 0;
   virtual void registerSocketNotifier(SocketNotifier *notifier) = 0;
   virtual void unregisterSocketNotifier(SocketNotifier *notifier) = 0;
   int registerTimer(int interval, pdk::TimerType timerType, Object *object);
   virtual void registerTimer(int timerId, int interval, pdk::TimerType timerType, Object *object) = 0;
   virtual bool unregisterTimer(int timerId) = 0;
   virtual bool unregisterTimers(Object *object) = 0;
   virtual std::list<TimerInfo> registeredTimers(Object *object) const = 0;
   virtual int remainingTime(int timerId) = 0;
   
#ifdef PDK_OS_WIN
   virtual bool registerEventNotifier(WinEventNotifier *notifier) = 0;
   virtual void unregisterEventNotifier(WinEventNotifier *notifier) = 0;
#endif
   
   virtual void wakeUp() = 0;
   virtual void interrupt() = 0;
   virtual void flush() = 0;
   virtual void startingUp();
   virtual void closingDown();
   
   void installNativeEventFilter(AbstractNativeEventFilter *filterObj);
   void removeNativeEventFilter(AbstractNativeEventFilter *filterObj);
   bool filterNativeEvent(const ByteArray &eventType, void *message, long *result);
   
   // signal
   // void aboutToBlock();
   // void awake();
protected:
   virtual bool hasPendingEvents() = 0;
   AbstractEventDispatcher(AbstractEventDispatcherPrivate &,
                           Object *parent);
};

} // kernel
} // pdk

#endif // PDK_KERNEL_ABSTRACT_EVENT_DISPATCHER_H
