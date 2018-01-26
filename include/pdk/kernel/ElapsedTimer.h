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

#ifndef PDK_KERNEL_ELAPSED_TIMER_H
#define PDK_KERNEL_ELAPSED_TIMER_H

#include "pdk/global/Global.h"

namespace pdk {
namespace kernel {

class PDK_CORE_EXPORT ElapsedTimer
{
public:
   enum class ClockType
   {
      SystemTime,
      MonotonicClock,
      TickCounter,
      MachAbsoluteTime,
      PerformanceCounter
   };
   
   constexpr ElapsedTimer()
      : m_t1(PDK_INT64_C(0x8000000000000000)),
        m_t2(PDK_INT64_C(0x8000000000000000))
   {
   }
   
   static ClockType getClockType() noexcept;
   static bool isMonotonic() noexcept;
   
   void start() noexcept;
   pdk::pint64 restart() noexcept;
   void invalidate() noexcept;
   bool isValid() const noexcept;
   
   pdk::pint64 nsecsElapsed() const noexcept;
   pdk::pint64 elapsed() const noexcept;
   bool hasExpired(pdk::pint64 timeout) const noexcept;
   
   pdk::pint64 msecsSinceReference() const noexcept;
   pdk::pint64 msecsTo(const ElapsedTimer &other) const noexcept;
   pdk::pint64 secsTo(const ElapsedTimer &other) const noexcept;
   
   bool operator==(const ElapsedTimer &other) const noexcept
   {
      return m_t1 == other.m_t1 && m_t2 == other.m_t2;
   }
   
   bool operator!=(const ElapsedTimer &other) const noexcept
   {
      return !(*this == other);
   }
   
   friend bool PDK_CORE_EXPORT operator<(const ElapsedTimer &v1, const ElapsedTimer &v2) noexcept;
   
private:
   pdk::pint64 m_t1;
   pdk::pint64 m_t2;
};

} // kernel
} // pdk

#endif // PDK_KERNEL_ELAPSED_TIMER_H
