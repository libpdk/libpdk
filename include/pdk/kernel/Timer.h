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

#ifndef PDK_KERNEL_TIMER_H
#define PDK_KERNEL_TIMER_H

#include "pdk/global/Global.h"
#include "pdk/kernel/BasicTimer.h"
#include "pdk/kernel/Object.h"
#include <chrono>

namespace pdk {
namespace kernel {

namespace internal {
class TimerPrivate;
} // internal

using internal::TimerPrivate;
using pdk::stdext::FunctionPointer;
using pdk::kernel::signal::Signal;

class PDK_CORE_EXPORT Timer : public Object
{
public:
   using TimeoutHandlerType = void();
   using SlotFuncType = void (Object *);
public:
   explicit Timer(Object *parent = nullptr);
   ~Timer();
   inline bool isActive() const
   {
      return m_id >= 0;
   }
   
   int getTimerId() const
   {
      return m_id;
   }
   
   void setInterval(int msec);
   int getInterval() const
   {
      return m_interval;
   }
   
   int getRemainingTime() const;
   void setTimerType(pdk::TimerType type)
   {
      m_type = pdk::as_integer<pdk::TimerType>(type);
   }
   
   pdk::TimerType getTimerType() const
   {
      return pdk::TimerType(m_type);
   }
   
   PDK_DEFINE_SIGNAL_BINDER(Timeout)
   PDK_DEFINE_SIGNAL_EMITTER(Timeout)
   
   public:
      inline void setSingleShot(bool singleShot);
   inline bool isSingleShot() const
   {
      return m_single;
   }
   
   static void singleShot(int msec, const std::function<TimeoutHandlerType> &callable);
   static void singleShot(int msec, pdk::TimerType timerType, const std::function<TimeoutHandlerType> &callable);
   
   // singleShot to a Object slot
   template <typename Duration>
   static inline void singleShot(Duration interval, pdk::TimerType timerType, 
                                 const Object *receiver, const std::function<SlotFuncType> &slotFunc)
   {
      singleShotImpl(interval, timerType, receiver, slotFunc);
   }
   
   template <typename Duration>
   static inline void singleShot(Duration interval, const Object *receiver, const std::function<SlotFuncType> &slotFunc)
   {
      singleShot(interval, defaultTypeFor(interval), receiver, slotFunc);
   }
   
   template<typename Duration>
   void singleShot(Duration interval, pdk::TimerType timerType, const std::function<TimeoutHandlerType> &callable)
   {
      singleShotImpl(interval, timerType, callable);
   }
   
   template<typename Duration>
   void singleShot(Duration interval, const std::function<TimeoutHandlerType> &callable)
   {
      singleShot(interval, defaultTypeFor(interval), callable);
   }
   
public:
   void start(int msec);
   void start();
   void stop();
   // SIGNALS:
   // void timeout(QPrivateSignal);
   
public:
   PDK_ALWAYS_INLINE
   void setInterval(std::chrono::milliseconds value)
   {
      setInterval(int(value.count()));
   }
   
   PDK_ALWAYS_INLINE
   std::chrono::milliseconds intervalAsDuration() const
   {
      return std::chrono::milliseconds(getInterval());
   }
   
   PDK_ALWAYS_INLINE
   std::chrono::milliseconds remainingTimeAsDuration() const
   {
      return std::chrono::milliseconds(getRemainingTime());
   }
   
   PDK_ALWAYS_INLINE
   static void singleShot(std::chrono::milliseconds value, const std::function<TimeoutHandlerType> &callable)
   {
      singleShot(int(value.count()), callable);
   }
   
   PDK_ALWAYS_INLINE
   static void singleShot(std::chrono::milliseconds value, pdk::TimerType timerType, const std::function<TimeoutHandlerType> &callable)
   {
      singleShot(int(value.count()), timerType, callable);
   }
   
   PDK_ALWAYS_INLINE
   static void singleShot(std::chrono::milliseconds value, const Object *receiver, 
                          const std::function<SlotFuncType> &slotFunc)
   {
      singleShot(int(value.count()), defaultTypeFor(value), receiver, slotFunc);
   }
   
   PDK_ALWAYS_INLINE
   static void singleShot(std::chrono::milliseconds value, pdk::TimerType timerType, const Object *receiver, 
                          const std::function<SlotFuncType> &slotFunc)
   {
      singleShot(int(value.count()), timerType, receiver, slotFunc);
   }
   
   PDK_ALWAYS_INLINE
   void start(std::chrono::milliseconds value)
   {
      start(int(value.count()));
   }
   
protected:
   void timerEvent(TimerEvent *) override;
   
private:
   PDK_DISABLE_COPY(Timer);
   
   inline int startTimer(int)
   {
      return -1;   
   }
   inline void killTimer(int){}
   
   static constexpr pdk::TimerType defaultTypeFor(int msecs) noexcept
   {
      return msecs >= 2000 ? pdk::TimerType::CoarseTimer : pdk::TimerType::PreciseTimer;
   }
   
   static void singleShotImpl(int msec, pdk::TimerType timerType,
                              const Object *receiver, const std::function<SlotFuncType> &slotFunc);
   static void singleShotImpl(int msec, pdk::TimerType timerType,
                              const std::function<TimeoutHandlerType> &callable);
   
   static pdk::TimerType defaultTypeFor(std::chrono::milliseconds interval)
   {
      return defaultTypeFor(int(interval.count()));
   }
   
   static void singleShotImpl(std::chrono::milliseconds interval, pdk::TimerType timerType,
                              const Object *receiver, const std::function<SlotFuncType> &slotFunc)
   {
      singleShotImpl(int(interval.count()), timerType, receiver, slotFunc);
   }
   
   static void singleShotImpl(std::chrono::milliseconds interval, pdk::TimerType timerType,
                              const std::function<TimeoutHandlerType> &callable)
   {
      singleShotImpl(int(interval.count()), timerType, callable);
   }
   
   int m_id;
   int m_interval;
   uint m_single : 1;
   uint m_nullTimer : 1;
   uint m_type : 2;
   // uint m_reserved : 28
};

inline void Timer::setSingleShot(bool singleShot)
{
   m_single = singleShot;
}

} // kernel
} // pdk

#endif // PDK_KERNEL_TIMER_H
