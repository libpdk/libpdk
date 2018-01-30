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

// ask for the latest POSIX, just in case

#define _POSIX_C_SOURCE 200809L

#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/kernel/DeadlineTimer.h"
#include "pdk/kernel/internal/DeadlineTimerPrivate.h"
#include "pdk/kernel/CoreUnix.h"
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <mach/mach_time.h>

namespace pdk {
namespace kernel {


#ifdef __LP64__
using LargeInt = __int128_t;
#else
using LargeInt = pdk::pint64;
#endif

namespace {

static mach_timebase_info_data_t sg_info = {0,0};

pdk::pint64 absolute_to_nsecs(pdk::pint64 cpuTime)
{
   if (sg_info.denom == 0) {
      mach_timebase_info(&sg_info);
   }
   // don't do multiplication & division if those are equal
   // (mathematically it would be the same, but it's computationally expensive)
   if (sg_info.numer == sg_info.denom) {
      return cpuTime;
   }
   pdk::pint64 nsecs = LargeInt(cpuTime) * sg_info.numer / sg_info.denom;
   return nsecs;
}

pdk::pint64 absolute_to_msecs(pdk::pint64 cpuTime)
{
   return absolute_to_nsecs(cpuTime) / 1000000;
}

}

timespec get_time() noexcept
{
   timespec tv;
   uint64_t cpuTime = mach_absolute_time();
   uint64_t nsecs = absolute_to_nsecs(cpuTime);
   tv.tv_sec = nsecs / 1000000000ull;
   tv.tv_nsec = nsecs - (tv.tv_sec * 1000000000ull);
   return tv;
}

void nanosleep(timespec amount)
{
   // Mac doesn't have clock_nanosleep, but it does have nanosleep.
   // nanosleep is POSIX.1-1993
   
   int r;
   PDK_EINTR_LOOP(r, ::nanosleep(&amount, &amount));
}

ElapsedTimer::ClockType ElapsedTimer::getClockType() noexcept
{
   return ClockType::MachAbsoluteTime;
}

bool ElapsedTimer::isMonotonic() noexcept
{
   return true;
}

void ElapsedTimer::start() noexcept
{
   m_t1 = mach_absolute_time();
   m_t2 = 0;
}

pdk::pint64 ElapsedTimer::restart() noexcept
{
   pdk::pint64 old = m_t1;
   m_t1 = mach_absolute_time();
   m_t2 = 0;
   return absolute_to_msecs(m_t1 - old);
}

pdk::pint64 ElapsedTimer::nsecsElapsed() const noexcept
{
   uint64_t cpuTime = mach_absolute_time();
   return absolute_to_nsecs(cpuTime - m_t1);
}

pdk::pint64 ElapsedTimer::elapsed() const noexcept
{
   uint64_t cpuTime = mach_absolute_time();
   return absolute_to_msecs(cpuTime - m_t1);
}

pdk::pint64 ElapsedTimer::msecsSinceReference() const noexcept
{
   return absolute_to_msecs(m_t1);
}

pdk::pint64 ElapsedTimer::msecsTo(const ElapsedTimer &other) const noexcept
{
   return absolute_to_msecs(other.m_t1 - m_t1);
}

pdk::pint64 ElapsedTimer::secsTo(const ElapsedTimer &other) const noexcept
{
   return msecsTo(other) / 1000;
}

DeadlineTimer DeadlineTimer::getCurrent(pdk::TimerType timerType) noexcept
{
   PDK_STATIC_ASSERT(!internal::DeadlineTimerNanosecondsInT2);
   DeadlineTimer result;
   result.m_type = timerType;
   result.m_t1 = absolute_to_nsecs(mach_absolute_time());
   return result;
}

bool operator<(const ElapsedTimer &lhs, const ElapsedTimer &rhs) noexcept
{
   return lhs.m_t1 < rhs.m_t1;
}

} // kernel
} // pdk
