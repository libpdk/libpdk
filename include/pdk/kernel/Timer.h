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
#include "pdk/kernel/internal/FunctionTrait.h"
#include <chrono>

namespace pdk {
namespace kernel {

namespace internal {
class TimerPrivate;
} // internal

using internal::TimerPrivate;

class PDK_CORE_EXPORT Timer : public Object
{
public:
   explicit Timer(Object *parent = nullptr);
   ~Timer();
   inline bool isActive() const
   {}
   int getTimerId() const
   {}
   void setInterval(int msec);
   int getInterval() const
   {}
   
   int getRemainingTime() const;
   void setTimerType(pdk::TimerType atype)
   {}
   
   pdk::TimerType getTimerType() const
   {}
   
   inline void setSingleShot(bool singleShot);
   inline bool isSingleShot() const
   {}
   
   static void singleShot(int msec, const Object *receiver, const char *member);
   static void singleShot(int msec, pdk::TimerType timerType, const Object *receiver, const char *member);
   
   // singleShot to a QObject slot
   template <typename Duration, typename Func1>
   static inline void singleShot(Duration interval, const typename internal::FunctionPointer<Func1>::ObjectType *receiver, Func1 slot)
   {
      
   }
   template <typename Duration, typename Func1>
   static inline void singleShot(Duration interval, pdk::TimerType timerType, 
                                 const typename internal::FunctionPointer<Func1>::ObjectType *receiver,
                                 Func1 slot)
   {
      
   }
   
   // singleShot to a functor or function pointer (without context)
   template <typename Duration, typename Func1>
   static inline typename std::enable_if<!internal::FunctionPointer<Func1>::IsPointerToMemberFunction &&
   !std::is_same<const char*, Func1>::value, void>::type
   singleShot(Duration interval, Func1 slot)
   {
      
   }
   
   template <typename Duration, typename Func1>
   static inline typename std::enable_if<!internal::FunctionPointer<Func1>::IsPointerToMemberFunction &&
   !std::is_same<const char*, Func1>::value, void>::type
   singleShot(Duration interval, pdk::TimerType timerType, Func1 slot)
   {
      
   }
   
   // singleShot to a functor or function pointer (with context)
   template <typename Duration, typename Func1>
   static inline typename std::enable_if<!internal::FunctionPointer<Func1>::IsPointerToMemberFunction &&
   !std::is_same<const char*, Func1>::value, void>::Type
   singleShot(Duration interval, Object *context, Func1 slot)
   {
      
   }
   
   template <typename Duration, typename Func1>
   static inline typename std::enable_if<!internal::FunctionPointer<Func1>::IsPointerToMemberFunction &&
   !std::is_same<const char*, Func1>::value, void>::Type
   singleShot(Duration interval, pdk::TimerType timerType, Object *context, Func1 slot)
   {
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
   }
   
   PDK_ALWAYS_INLINE
   std::chrono::milliseconds intervalAsDuration() const
   {
   }
   
   PDK_ALWAYS_INLINE
   std::chrono::milliseconds remainingTimeAsDuration() const
   {
   }
   
   PDK_ALWAYS_INLINE
   static void singleShot(std::chrono::milliseconds value, const Object *receiver, 
                          const char *member)
   {
   }
   
   PDK_ALWAYS_INLINE
   static void singleShot(std::chrono::milliseconds value, pdk::TimerType timerType, 
                          const Object *receiver, const char *member)
   {
   }
   
   PDK_ALWAYS_INLINE
   void start(std::chrono::milliseconds value)
   {
   }
   
protected:
   void timerEvent(TimerEvent *) override;
   
private:
   PDK_DISABLE_COPY(Timer);
   
   inline int startTimer(int){}
   inline void killTimer(int){}
   
   static constexpr pdk::TimerType defaultTypeFor(int msecs) noexcept
   {}
   static void singleShotImpl(int msec, pdk::TimerType timerType,
                              const Object *receiver/*, QtPrivate::QSlotObjectBase *slotObj*/);
   
   static pdk::TimerType defaultTypeFor(std::chrono::milliseconds interval)
   {}
   
   static void singleShotImpl(std::chrono::milliseconds interval, pdk::TimerType timerType,
                              const Object *receiver/*, QtPrivate::QSlotObjectBase *slotObj*/)
   {
   }
   
};

} // kernel
} // pdk


#endif // PDK_KERNEL_TIMER_H
