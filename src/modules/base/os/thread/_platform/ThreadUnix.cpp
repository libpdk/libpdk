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
// Created by softboy on 2017/01/25.

#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/EventdispatcherUnix.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"

#include <sched.h>
#include <errno.h>

#ifdef PDK_OS_BSD4
#include <sys/sysctl.h>
#endif
#ifdef PDK_OS_VXWORKS
#  if (_WRS_VXWORKS_MAJOR > 6) || ((_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR >= 6))
#    include <vxCpuLib.h>
#    include <cpuset.h>
#    define PDK_VXWORKS_HAS_CPUSET
#  endif
#endif // PDK_OS_VXWORKS

#ifdef PDK_OS_HPUX
#include <sys/pstat.h>
#endif // PDK_OS_HPUX

#if defined(PDK_OS_LINUX) && !defined(PDK_LINUXBASE)
#include <sys/prctl.h>
#endif

#if defined(PDK_OS_LINUX) && !defined(SCHED_IDLE)
// from linux/sched.h
# define SCHED_IDLE    5
#endif

#if defined(PDK_OS_DARWIN) || !defined(PDK_OS_OPENBSD) && defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && (_POSIX_THREAD_PRIORITY_SCHEDULING-0 >= 0)
#define PDK_HAS_THREAD_PRIORITY_SCHEDULING
#endif

namespace pdk {
namespace os {
namespace thread {



} // thread
} // os
} // pdk

