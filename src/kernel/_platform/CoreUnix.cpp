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
// Created by softboy on 2018/01/30.

#include "pdk/global/Global.h"
#include "pdk/kernel/CoreUnix.h"
#include "pdk/kernel/ElapsedTimer.h"

#include <cstdlib>

#ifdef PDK_OS_MAC
#include <mach/mach_time.h>
#endif

namespace pdk {
namespace kernel {

ByteArray pdk_readlink(const char *path)
{
#ifndef PATH_MAX
   // suitably large value that won't consume too much memory
#  define PATH_MAX  1024*1024
#endif
   
   ByteArray buf(256, pdk::Uninitialized);
   
   ssize_t len = ::readlink(path, buf.getRawData(), buf.size());
   while (len == buf.size()) {
      // readlink(2) will fill our buffer and not necessarily terminate with NUL;
      if (buf.size() >= PATH_MAX) {
         errno = ENAMETOOLONG;
         return ByteArray();
      }
      
      // double the size and try again
      buf.resize(buf.size() * 2);
      len = ::readlink(path, buf.getRawData(), buf.size());
   }
   if (len == -1) {
      return ByteArray();
   }
   buf.resize(len);
   return buf;
}

#ifdef PDK_CONFIG_POLL_POLLTS
#define ppoll pollts
#endif

namespace {

inline bool time_update(struct timespec *tv, const struct timespec &start,
                        const struct timespec &timeout)
{
   // clock source is (hopefully) monotonic, so we can recalculate how much timeout is left;
   // if it isn't monotonic, we'll simply hope that it hasn't jumped, because we have no alternative
   struct timespec now = get_time();
   *tv = timeout + start - now;
   return tv->tv_sec >= 0;
}

#ifdef PDK_CONFIG_POLL_POLL
inline int timespec_to_millisecs(const struct timespec *ts)
{
   if (ts == nullptr) ? -1 :
      (ts->tv_sec * 1000) + (ts->tv_nsec / 1000000);
}
#endif

} // anonymous

// defined src/kernel/Poll.cpp file
int pdk_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts);

namespace {

inline int pdk_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
{
#if defined(PDK_CONFIG_POLL_PPOLL) || defined(PDK_CONFIG_POLL_POLLTS)
   return ::ppoll(fds, nfds, timeout_ts, nullptr);
#elif defined(PDK_CONFIG_POLL_POLL)
   return ::poll(fds, nfds, timespec_to_millisecs(timeout_ts));
#else
   return pdk_poll(fds, nfds, timeout_ts);
#endif
}

} // anonymous

int safe_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
{
   if (!timeout_ts) {
      // no timeout -> block forever
      int ret;
      PDK_EINTR_LOOP(ret, pdk_ppoll(fds, nfds, nullptr));
      return ret;
   }
   timespec start = get_time();
   timespec timeout = *timeout_ts;
   // loop and recalculate the timeout as needed
   while(true) {
      const int ret = pdk_ppoll(fds, nfds, &timeout);
      if (ret != -1 || errno != EINTR) {
         return ret;
      }
      // recalculate the timeout
      if (!time_update(&timeout, start, *timeout_ts)) {
         // timeout during update
         // or clock reset, fake timeout error
         return 0;
      }
   }
}

} // kernel
} // pdk
