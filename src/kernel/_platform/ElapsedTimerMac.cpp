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
      mach_timebase_info(&info);
   }
   // don't do multiplication & division if those are equal
   // (mathematically it would be the same, but it's computationally expensive)
   if (info.numer == info.denom) {
      return cpuTime;
   }
   pdk::pint64 nsecs = LargeInt(cpuTime) * info.numer / info.denom;
   return nsecs;
}

pdk::pint64 absolute_to_msecs(pdk::pint64 cpuTime)
{
   return absolute_to_nsecs(cpuTime) / 1000000;
}

}

ElapsedTimer::ClockType ElapsedTimer::getClockType() noexcept
{
   return ClockType::MachAbsoluteTime;
}

bool ElapsedTimer::isMonotonic() noexcept
{
   return true;
}

} // kernel
} // pdk
