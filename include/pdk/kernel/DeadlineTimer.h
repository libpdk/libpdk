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

#ifndef PDK_KERNEL_DEADLINE_TIMER_H
#define PDK_KERNEL_DEADLINE_TIMER_H

#include "pdk/kernel/ElapsedTimer.h"
#include <limits>
#include <chrono>

namespace pdk {
namespace kernel {

class PDK_CORE_EXPORT DeadlineTimer
{
public:
   enum class ForeverConstant
   {
      Forever
   };
   
public:
   constexpr DeadlineTimer(pdk::TimerType type = pdk::TimerType::CoarseTimer) noexcept
      : m_t1(0),
        m_t2(0),
        m_type(type)
   {}
   
   constexpr DeadlineTimer(ForeverConstant, pdk::TimerType type = pdk::TimerType::CoarseTimer) noexcept
      : m_t1(std::numeric_limits<pdk::pint64>::max()),
        m_t2(0),
        m_type(type)
   {}
   
   explicit DeadlineTimer(pdk::pint64 msecs, pdk::TimerType type = pdk::TimerType::CoarseTimer) noexcept;
   
   void swap(DeadlineTimer &other) noexcept
   { 
      std::swap(m_t1, other.m_t1);
      std::swap(m_t2, other.m_t2);
      std::swap(m_type, other.m_type);
   }
   
   constexpr bool isForever() const noexcept
   {
      return m_t1 == (std::numeric_limits<pdk::pint64>::max)();
   }
   
   bool hasExpired() const noexcept;
   
   pdk::TimerType getTimerType() const noexcept
   {
      return m_type;
   }
   
   void setTimerType(pdk::TimerType type);
   
   pdk::pint64 getRemainingTime() const noexcept;
   pdk::pint64 getRemainingTimeNSecs() const noexcept;
   void setRemainingTime(pdk::pint64 msecs, pdk::TimerType type = pdk::TimerType::CoarseTimer) noexcept;
   void setPreciseRemainingTime(pdk::pint64 secs, pdk::pint64 nsecs = 0,
                                pdk::TimerType type = pdk::TimerType::CoarseTimer) noexcept;
   
   pdk::pint64 getDeadline() const noexcept PDK_DECL_PURE_FUNCTION;
   pdk::pint64 getDeadlineNSecs() const noexcept PDK_DECL_PURE_FUNCTION;
   void setDeadline(pdk::pint64 msecs, pdk::TimerType timerType = pdk::TimerType::CoarseTimer) noexcept;
   void setPreciseDeadline(pdk::pint64 secs, pdk::pint64 nsecs = 0,
                           pdk::TimerType type = pdk::TimerType::CoarseTimer) noexcept;
   
   static DeadlineTimer addNSecs(DeadlineTimer dt, pdk::pint64 nsecs) noexcept PDK_DECL_PURE_FUNCTION;
   static DeadlineTimer getCurrent(pdk::TimerType timerType = pdk::TimerType::CoarseTimer) noexcept;
   
   friend bool operator==(DeadlineTimer lhs, DeadlineTimer rhs) noexcept
   {
      return lhs.m_t1 == rhs.m_t1 && lhs.m_t2 == rhs.m_t2;
   }
   
   friend bool operator!=(DeadlineTimer lhs, DeadlineTimer rhs) noexcept
   {
      return !(lhs == rhs);
   }
   
   friend bool operator<(DeadlineTimer lhs, DeadlineTimer rhs) noexcept
   {
      return lhs.m_t1 < rhs.m_t1 || (lhs.m_t1 == rhs.m_t1 && lhs.m_t2 < rhs.m_t2);
   }
   
   friend bool operator<=(DeadlineTimer lhs, DeadlineTimer rhs) noexcept
   {
      return lhs == rhs || lhs < rhs;
   }
   
   friend bool operator>(DeadlineTimer lhs, DeadlineTimer rhs) noexcept
   {
      return rhs < lhs;
   }
   
   friend bool operator>=(DeadlineTimer lhs, DeadlineTimer rhs) noexcept
   {
      return !(lhs < rhs);
   }
   
   friend DeadlineTimer operator+(DeadlineTimer dt, pdk::pint64 msecs)
   {
      return DeadlineTimer::addNSecs(dt, msecs * 1000 * 1000);
   }
   
   friend DeadlineTimer operator+(pdk::pint64 msecs, DeadlineTimer dt)
   {
      return dt + msecs;
   }
   
   friend DeadlineTimer operator-(DeadlineTimer dt, pdk::pint64 msecs)
   {
      return dt + (-msecs);
   }
   
   friend pdk::pint64 operator-(DeadlineTimer dt1, DeadlineTimer dt2)
   {
      return (dt1.getDeadlineNSecs() - dt2.getDeadlineNSecs()) / (1000 * 1000);
   }
   
   DeadlineTimer &operator+=(pdk::pint64 msecs)
   {
      *this = *this + msecs;
      return *this;
   }
   
   DeadlineTimer &operator-=(pdk::pint64 msecs)
   {
      *this = *this + (-msecs);
      return *this;
   }
   
   template <class Clock, class Duration>
   DeadlineTimer(std::chrono::time_point<Clock, Duration> deadline,
                 pdk::TimerType type = pdk::TimerType::CoarseTimer)
      : m_t2(0)
   {
      setDeadline(deadline, type);
   }
   
   template <class Clock, class Duration>
   DeadlineTimer &operator=(std::chrono::time_point<Clock, Duration> deadline)
   {
      setDeadline(deadline);
      return *this;
   }
   
   template <class Clock, class Duration>
   void setDeadline(std::chrono::time_point<Clock, Duration> deadline,
                    pdk::TimerType type = pdk::TimerType::CoarseTimer)
   {
      setRemainingTime(deadline == deadline.max() ? Duration::max() : deadline - Clock::now(), type);
   }
   
   template <class Clock, class Duration = typename Clock::duration>
   std::chrono::time_point<Clock, Duration> getDeadline() const
   {
      auto val = std::chrono::nanoseconds(rawRemainingTimeNSecs()) + Clock::now();
      return std::chrono::time_point_cast<Duration>(val);
   }
   
   template <class Rep, class Period>
   DeadlineTimer(std::chrono::duration<Rep, Period> remaining, 
                 pdk::TimerType type = pdk::TimerType::CoarseTimer)
      : m_t2(0)
   {
      setRemainingTime(remaining, type);
   }
   
   template <class Rep, class Period>
   DeadlineTimer &operator=(std::chrono::duration<Rep, Period> remaining)
   {
      setRemainingTime(remaining);
      return *this;
   }
   
   template <class Rep, class Period>
   void setRemainingTime(std::chrono::duration<Rep, Period> remaining,
                         pdk::TimerType type = pdk::TimerType::CoarseTimer)
   {
      if (remaining == remaining.max()) {
         *this = DeadlineTimer(ForeverConstant::Forever, type);
      } else {
         setPreciseRemainingTime(0, std::chrono::nanoseconds(remaining).count(), type);
      }
   }
   
   std::chrono::nanoseconds getRemainingTimeAsDuration() const noexcept
   {
      if (isForever()) {
         return std::chrono::nanoseconds::max();
      }
      pdk::pint64 nsecs = rawRemainingTimeNSecs();
      if (nsecs <= 0) {
         return std::chrono::nanoseconds::zero();
      }
      return std::chrono::nanoseconds(nsecs);
   }
   
   template <class Rep, class Period>
   friend DeadlineTimer operator+(DeadlineTimer dt, std::chrono::duration<Rep, Period> value)
   {
      return DeadlineTimer::addNSecs(dt, std::chrono::duration_cast<std::chrono::nanoseconds>(value).count());
   }
   
   template <class Rep, class Period>
   friend DeadlineTimer operator+(std::chrono::duration<Rep, Period> value, DeadlineTimer dt)
   {
      return dt + value;
   }
   
   template <class Rep, class Period>
   friend DeadlineTimer operator+=(DeadlineTimer &dt, std::chrono::duration<Rep, Period> value)
   {
      return dt = dt + value;
   }
   
private:
   pdk::pint64 rawRemainingTimeNSecs() const noexcept;
   
private:
   pdk::pint64 m_t1;
   unsigned m_t2;
   pdk::TimerType m_type;
};



} // kernel
} // pdk

PDK_DECLARE_SHARED(pdk::kernel::DeadlineTimer)

#endif // PDK_KERNEL_DEADLINE_TIMER_H
