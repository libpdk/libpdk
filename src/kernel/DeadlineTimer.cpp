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
// Created by softboy on 2018/01/26.

#include "pdk/kernel/DeadlineTimer.h"
#include "pdk/kernel/internal/DeadlineTimerPrivate.h"
#include <utility>

namespace pdk {
namespace kernel {

namespace {

PDK_DECL_CONST_FUNCTION static inline std::pair<pdk::pint64, pdk::pint64> 
to_secs_and_nsecs(pdk::pint64 nsecs)
{
   pdk::pint64 secs = nsecs / (1000*1000*1000);
   if (nsecs < 0) {
      --secs;
   }
   nsecs -= secs * 1000*1000*1000;
   return std::make_pair(secs, nsecs);
}

}

DeadlineTimer::DeadlineTimer(pdk::pint64 msecs, pdk::TimerType type) noexcept
   : m_t2(0)
{
   setRemainingTime(msecs, type);
}

void DeadlineTimer::setRemainingTime(pdk::pint64 msecs, pdk::TimerType timerType) noexcept
{
   if (msecs == -1) {
      *this = DeadlineTimer(ForeverConstant::Forever, timerType);
   } else {
      setPreciseRemainingTime(0, msecs * 1000 * 1000, timerType);
   }
}

void DeadlineTimer::setPreciseRemainingTime(pdk::pint64 secs, pdk::pint64 nsecs, 
                                            pdk::TimerType timerType) noexcept
{
   if (secs == -1) {
      *this = DeadlineTimer(ForeverConstant::Forever, timerType);
      return;
   }
   
   *this = getCurrent(timerType);
   if (internal::DeadlineTimerNanosecondsInT2) {
      m_t1 += secs + to_secs_and_nsecs(nsecs).first;
      m_t2 += to_secs_and_nsecs(nsecs).second;
      if (m_t2 > 1000*1000*1000) {
         m_t2 -= 1000*1000*1000;
         ++m_t1;
      }
   } else {
      m_t1 += secs * 1000 * 1000 * 1000 + nsecs;
   }
}

bool DeadlineTimer::hasExpired() const noexcept
{
   if (isForever()) {
      return false;
   }
   return *this <= getCurrent(getTimerType());
}

void DeadlineTimer::setTimerType(pdk::TimerType timerType)
{
   m_type = timerType;
}

pdk::pint64 DeadlineTimer::getRemainingTime() const noexcept
{
   pdk::pint64 ns = getRemainingTimeNSecs();
   return ns <= 0 ? ns : ns / (1000 * 1000);
}

pdk::pint64 DeadlineTimer::getRemainingTimeNSecs() const noexcept
{
   if (isForever()) {
      return -1;
   }
   pdk::pint64 raw = rawRemainingTimeNSecs();
   return raw < 0 ? 0 : raw;
}

pdk::pint64 DeadlineTimer::rawRemainingTimeNSecs() const noexcept
{
   DeadlineTimer now = getCurrent(getTimerType());
   if (internal::DeadlineTimerNanosecondsInT2) {
      return (m_t1 - now.m_t1) * (1000*1000*1000) + m_t2 - now.m_t2;
   }
   return m_t1 - now.m_t1;
}

pdk::pint64 DeadlineTimer::getDeadline() const noexcept
{
   if (isForever()) {
      return m_t1;
   }
   return getDeadlineNSecs() / (1000 * 1000);
}

pdk::pint64 DeadlineTimer::getDeadlineNSecs() const noexcept
{
   if (isForever()) {
      return m_t1;
   }
   if (internal::DeadlineTimerNanosecondsInT2) {
      return m_t1 * 1000 * 1000 * 1000 + m_t2;
   }
   return m_t1;
}

void DeadlineTimer::setDeadline(pdk::pint64 msecs, pdk::TimerType timerType) noexcept
{
   if (msecs == (std::numeric_limits<pdk::pint64>::max)()) {
      setPreciseDeadline(msecs, 0, timerType);    // msecs == MAX implies Forever
   } else {
      setPreciseDeadline(msecs / 1000, msecs % 1000 * 1000 * 1000, timerType);
   }
}

void DeadlineTimer::setPreciseDeadline(pdk::pint64 secs, pdk::pint64 nsecs, 
                                       pdk::TimerType timerType) noexcept
{
   m_type = timerType;
   if (secs == (std::numeric_limits<pdk::pint64>::max)() || nsecs == (std::numeric_limits<pdk::pint64>::max)()) {
      *this = DeadlineTimer(ForeverConstant::Forever, timerType);
   } else if (internal::DeadlineTimerNanosecondsInT2) {
      m_t1 = secs + to_secs_and_nsecs(nsecs).first;
      m_t2 = to_secs_and_nsecs(nsecs).second;
   } else {
      m_t1 = secs * (1000*1000*1000) + nsecs;
   }
}

DeadlineTimer DeadlineTimer::addNSecs(DeadlineTimer dt, pdk::pint64 nsecs) noexcept
{
   if (dt.isForever() || nsecs == (std::numeric_limits<pdk::pint64>::max)()) {
      dt = DeadlineTimer(ForeverConstant::Forever, dt.getTimerType());
   } else if (internal::DeadlineTimerNanosecondsInT2) {
      dt.m_t1 += to_secs_and_nsecs(nsecs).first;
      dt.m_t2 += to_secs_and_nsecs(nsecs).second;
      if (dt.m_t2 > 1000*1000*1000) {
         dt.m_t2 -= 1000*1000*1000;
         ++dt.m_t1;
      }
   } else {
      dt.m_t1 += nsecs;
   }
   return dt;
}

} // kernel
} // pdk
