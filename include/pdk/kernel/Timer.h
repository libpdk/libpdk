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
#include "pdk/kernel/internal/SingleShotTimerPrivate.h"
#include "pdk/kernel/Object.h"
#include "pdk/global/Logging.h"
#include "pdk/kernel/CallableInvoker.h"
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
   PDK_DEFINE_SIGNAL_ENUMS(Timeout);
   
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
   
   template <typename MemberFuncPointer,
             typename = std::enable_if_t<std::is_member_function_pointer<MemberFuncPointer>::value>>
   static void singleShot(int msec, const Object *receiver, MemberFuncPointer memberFunc);
   
   template <typename MemberFuncPointer,
             typename = std::enable_if_t<std::is_member_function_pointer<MemberFuncPointer>::value>>
   static void singleShot(int msec, pdk::TimerType timerType, const Object *receiver, 
                          MemberFuncPointer memberFunc);
   
   template <typename MemberFuncPointer,
             typename = std::enable_if_t<std::is_member_function_pointer<MemberFuncPointer>::value>>
   static void singleShot(std::chrono::milliseconds interval, const Object *receiver, MemberFuncPointer memberFunc)
   {
      singleShot(int(interval.count()), receiver, memberFunc);
   }
   
   template <typename MemberFuncPointer,
             typename = std::enable_if_t<std::is_member_function_pointer<MemberFuncPointer>::value>>
   static void singleShot(std::chrono::milliseconds interval, pdk::TimerType timerType, const Object *receiver, 
                          MemberFuncPointer memberFunc)
   {
      singleShot(int(interval.count()), timerType, receiver, memberFunc);
   }
   
   template <typename Duration,
             typename MemberFuncPointer,
             typename = std::enable_if_t<std::is_member_function_pointer<MemberFuncPointer>::value>>
   static inline void singleShot(Duration interval, const Object *receiver, MemberFuncPointer memberFunc)
   {
      singleShot(interval, defaultTypeFor(interval), receiver, memberFunc);
   }
   
   template <typename Duration,
             typename MemberFuncPointer,
             typename = std::enable_if_t<std::is_member_function_pointer<MemberFuncPointer>::value>>
   static inline void singleShot(Duration interval, pdk::TimerType timerType, const Object *receiver, 
                                 MemberFuncPointer memberFunc)
   {
      PDK_STATIC_ASSERT_X(int(pdk::stdext::CallableInfoTrait<MemberFuncPointer>::argNum) == 0,
                          "The slot function must not have any arguments.");
      singleShotImpl(interval, timerType, receiver, memberFunc);
   }
   
   template <typename Duration,
             typename SlotFuncType,
             typename = std::enable_if_t<pdk::stdext::IsCallable<SlotFuncType>::value>>
   static inline void singleShot(Duration interval, const SlotFuncType &slotFunc)
   {
      singleShot(interval, defaultTypeFor(interval), nullptr, slotFunc);
   }
   
   template <typename Duration,
             typename SlotFuncType,
             typename = std::enable_if_t<pdk::stdext::IsCallable<SlotFuncType>::value>>
   static inline void singleShot(Duration interval, pdk::TimerType timerType, const SlotFuncType &slotFunc)
   {
      singleShot(interval, timerType, nullptr, slotFunc);
   }
   
   template <typename Duration,
             typename SlotFuncType,
             typename = std::enable_if_t<pdk::stdext::IsCallable<SlotFuncType>::value>>
   static inline void singleShot(Duration interval, const Object *context, const SlotFuncType &slotFunc)
   {
      singleShot(interval, defaultTypeFor(interval), context, slotFunc);
   }
   
   template <typename Duration,
             typename SlotFuncType,
             typename = std::enable_if_t<pdk::stdext::IsCallable<SlotFuncType>::value>>
   static inline void singleShot(Duration interval, pdk::TimerType timerType, const Object *context, const SlotFuncType &slotFunc)
   {
      if constexpr(!std::is_pointer<SlotFuncType>::value) {
         PDK_STATIC_ASSERT_X(
                  pdk::stdext::CallableInfoTrait<decltype(&SlotFuncType::operator())>::argNum == 0,
                  "The slot must not have any arguments.");
      } else {
         PDK_STATIC_ASSERT_X(
                  pdk::stdext::CallableInfoTrait<SlotFuncType>::argNum == 0,
                  "The slot must not have any arguments.");
      }
      
      singleShotImpl(interval, timerType, context, slotFunc);
   }
   
public:
   void start(int msec);
   void start();
   void stop();
   
public:
   void setInterval(std::chrono::milliseconds value)
   {
      setInterval(int(value.count()));
   }
   
   std::chrono::milliseconds intervalAsDuration() const
   {
      return std::chrono::milliseconds(getInterval());
   }
   
   std::chrono::milliseconds remainingTimeAsDuration() const
   {
      return std::chrono::milliseconds(getRemainingTime());
   }
   
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
   
   template <typename SlotFunc,
             typename = std::enable_if_t<pdk::stdext::IsCallable<SlotFunc>::value ||
                                         std::is_member_function_pointer<SlotFunc>::value>>
   static void singleShotImpl(int msec, pdk::TimerType timerType,
                              const Object *context, SlotFunc slotFunc);
   
   static pdk::TimerType defaultTypeFor(std::chrono::milliseconds interval)
   {
      return defaultTypeFor(int(interval.count()));
   }
   
   template <typename SlotFunc,
             typename = std::enable_if_t<pdk::stdext::IsCallable<SlotFunc>::value>>
   static void singleShotImpl(std::chrono::milliseconds interval, pdk::TimerType timerType,
                              const Object *context, const SlotFunc &slotFunc)
   {
      singleShotImpl(int(interval.count()), timerType, context, slotFunc);
   }
   
   template <typename SlotFunc,
             typename = std::enable_if_t<std::is_member_function_pointer<SlotFunc>::value>>
   static void singleShotImpl(std::chrono::milliseconds interval, pdk::TimerType timerType,
                              const Object *receiver, SlotFunc slotFunc)
   {
      singleShotImpl(int(interval.count()), timerType, receiver, slotFunc);
   }
   
   int m_id;
   int m_interval;
   uint m_single: 1;
   uint m_nullTimer: 1;
   uint m_type: 2;
   // uint m_reserved : 28
};

inline void Timer::setSingleShot(bool singleShot)
{
   m_single = singleShot;
}

template <typename MemberFuncPointer, typename>
void Timer::singleShot(int msec, const Object *receiver, MemberFuncPointer memberFunc)
{
   singleShot(msec, 
              msec >= 2000 ? pdk::TimerType::CoarseTimer : pdk::TimerType::PreciseTimer,
              receiver, memberFunc);
}

template <typename MemberFuncPointer, typename>
void Timer::singleShot(int msec, pdk::TimerType timerType, const Object *receiver, 
                       MemberFuncPointer memberFunc)
{
   if (PDK_UNLIKELY(msec < 0)) {
      warning_stream("Timer::singleShot: Timers cannot have negative timeouts");
      return;
   }
   if (receiver) {
      if (msec == 0) {
         if (!memberFunc) {
            warning_stream("Timer::singleShot: Invalid slot specification");
            return;
         }
         CallableInvoker::invokeAsync(const_cast<Object *>(receiver), memberFunc);
         return;
      }
      (void) new internal::SingleShotTimer(msec, timerType, receiver, memberFunc);
   }
}

template <typename SlotFunc, typename>
void Timer::singleShotImpl(int msec, pdk::TimerType timerType,
                           const Object *context, SlotFunc slotFunc)
{
   new internal::SingleShotTimer(msec, timerType, context, slotFunc);
}

} // kernel
} // pdk

#endif // PDK_KERNEL_TIMER_H
