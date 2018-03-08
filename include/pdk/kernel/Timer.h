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
#include "pdk/kernel/internal/ObjectDefsPrivate.h"
#include <chrono>

namespace pdk {
namespace kernel {

namespace internal {
class TimerPrivate;
} // internal

using internal::TimerPrivate;
using internal::SlotObjectBase;
using internal::SlotObject;
using internal::FunctorSlotObject;
using pdk::stdext::FunctionPointer;

class PDK_CORE_EXPORT Timer : public Object
{
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
   
   inline void setSingleShot(bool singleShot);
   inline bool isSingleShot() const
   {
      return m_single;
   }
   
   static void singleShot(int msec, const Object *receiver, const char *member);
   static void singleShot(int msec, pdk::TimerType timerType, const Object *receiver, const char *member);
   
   // singleShot to a Object slot
   template <typename Duration, typename Func>
   static inline void singleShot(Duration interval, const typename FunctionPointer<Func>::ObjectType *receiver, Func slot)
   {
      singleShot(interval, defaultTypeFor(interval), receiver, slot);
   }
   
   template <typename Duration, typename Func>
   static inline void singleShot(Duration interval, pdk::TimerType timerType, 
                                 const typename FunctionPointer<Func>::ObjectType *receiver,
                                 Func slot)
   {
      typedef FunctionPointer<Func> SlotType;
      //compilation error if the slot has arguments.
      PDK_STATIC_ASSERT_X(int(SlotType::ArgumentCount) == 0,
                          "The slot must not have any arguments.");
      singleShotImpl(interval, timerType, receiver,
                     new SlotObject<Func, typename SlotType::ArgTypes, void>(slot));
   }
   
   // singleShot to a functor or function pointer (without context)
   template <typename Duration, typename Func>
   static inline typename std::enable_if<!FunctionPointer<Func>::IsPointerToMemberFunction &&
   !std::is_same<const char*, Func>::value, void>::type
   singleShot(Duration interval, Func slot)
   {
      singleShot(interval, defaultTypeFor(interval), nullptr, std::move(slot));
   }
   
   template <typename Duration, typename Func>
   static inline typename std::enable_if<!FunctionPointer<Func>::IsPointerToMemberFunction &&
   !std::is_same<const char*, Func>::value, void>::type
   singleShot(Duration interval, pdk::TimerType timerType, Func slot)
   {
      singleShot(interval, timerType, nullptr, std::move(slot));
   }
   
   // singleShot to a functor or function pointer (with context)
   template <typename Duration, typename Func>
   static inline typename std::enable_if<!FunctionPointer<Func>::IsPointerToMemberFunction &&
   !std::is_same<const char*, Func>::value, void>::Type
   singleShot(Duration interval, Object *context, Func slot)
   {
      singleShot(interval, defaultTypeFor(interval), context, std::move(slot));
   }
   
   template <typename Duration, typename Func>
   static inline typename std::enable_if<!FunctionPointer<Func>::IsPointerToMemberFunction &&
   !std::is_same<const char*, Func>::value, void>::Type
   singleShot(Duration interval, pdk::TimerType timerType, Object *context, Func slot)
   {
      //compilation error if the slot has arguments.
      using SlotType = FunctionPointer<Func>;
      PDK_STATIC_ASSERT_X(int(SlotType::ArgumentCount) <= 0,  "The slot must not have any arguments.");
      
      singleShotImpl(interval, timerType, context,
                     new FunctorSlotObject<Func, 0,
                     std::tuple<void>(), void>(std::move(slot)));
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
   static void singleShot(std::chrono::milliseconds value, const Object *receiver, 
                          const char *member)
   {
      singleShot(int(value.count()), receiver, member);
   }
   
   PDK_ALWAYS_INLINE
   static void singleShot(std::chrono::milliseconds value, pdk::TimerType timerType, 
                          const Object *receiver, const char *member)
   {
      singleShot(int(value.count()), timerType, receiver, member);
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
                              const Object *receiver, SlotObjectBase *slotObj);
   
   static pdk::TimerType defaultTypeFor(std::chrono::milliseconds interval)
   {
      return defaultTypeFor(int(interval.count()));
   }
   
   static void singleShotImpl(std::chrono::milliseconds interval, pdk::TimerType timerType,
                              const Object *receiver, SlotObjectBase *slotObj)
   {
      singleShotImpl(int(interval.count()), timerType, receiver, slotObj);
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
