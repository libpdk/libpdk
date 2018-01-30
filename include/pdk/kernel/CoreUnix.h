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

#ifndef PDK_KERNEL_CORE_UNIX_H
#define PDK_KERNEL_CORE_UNIX_H

#include "pdk/global/Global.h"
#include "pdk/global/PlatformDefs.h"
#include "pdk/base/os/thread/Atomic.h"

#ifndef PDK_OS_UNIX
# error "pdk/kernel/CoreUnix.h included on a non-Unix system"
#endif

#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef PDK_OS_NACL
#elif !defined (PDK_OS_VXWORKS)
# if !defined(PDK_OS_HPUX) || defined(__ia64)
#  include <sys/select.h>
# endif
#  include <sys/time.h>
#else
#  include <selectLib.h>
#endif

#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#if !defined(PDK_POSIX_IPC) && !defined(PDK_NO_SHAREDMEMORY)
#  include <sys/ipc.h>
#endif

#if defined(PDK_OS_VXWORKS)
#  include <ioLib.h>
#endif

#ifdef PDK_NO_NATIVE_POLL
#  include "pdk/kernel/internal/PollPrivate.h"
#else
#  include <poll.h>
#endif

struct sockaddr;

#define PDK_EINTR_LOOP(var, cmd)           \
   do {                                    \
   var = cmd;                              \
   } while (var == -1 && errno == EINTR)


namespace pdk {
namespace kernel {

// Internal operator functions for timespecs
inline timespec &normalized_timespec(timespec &t)
{
   while (t.tv_nsec >= 1000000000) {
      ++t.tv_sec;
      t.tv_nsec -= 1000000000;
   }
   while (t.tv_nsec < 0) {
      --t.tv_sec;
      t.tv_nsec += 1000000000;
   }
   return t;
}

inline bool operator<(const timespec &lhs, const timespec &rhs)
{
   return lhs.tv_sec < rhs.tv_sec || (lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec < rhs.tv_nsec);
}

inline bool operator==(const timespec &lhs, const timespec &rhs)
{
   return lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec == rhs.tv_nsec;
}

inline bool operator!=(const timespec &lhs, const timespec &rhs)
{
   return !(lhs == rhs);
}

inline timespec &operator+=(timespec &lhs, const timespec &rhs)
{
   lhs.tv_sec += rhs.tv_sec;
   lhs.tv_nsec += rhs.tv_nsec;
   return normalized_timespec(lhs);
}

inline timespec operator+(const timespec &lhs, const timespec &rhs)
{
   timespec tmp;
   tmp.tv_sec = lhs.tv_sec + rhs.tv_sec;
   tmp.tv_nsec = lhs.tv_nsec + rhs.tv_nsec;
   return normalized_timespec(tmp);
}

inline timespec operator-(const timespec &lhs, const timespec &rhs)
{
   timespec tmp;
   tmp.tv_sec = lhs.tv_sec - (rhs.tv_sec - 1);
   tmp.tv_nsec = lhs.tv_nsec - (rhs.tv_nsec + 1000000000);
   return normalized_timespec(tmp);
}

inline timespec operator*(const timespec &lhs, int mul)
{
   timespec tmp;
   tmp.tv_sec = lhs.tv_sec * mul;
   tmp.tv_nsec = lhs.tv_nsec * mul;
   return normalized_timespec(tmp);
}

inline timeval timespec_to_timeval(const timespec &ts)
{
   timeval tv;
   tv.tv_sec = ts.tv_sec;
   tv.tv_usec = ts.tv_nsec / 1000;
   return tv;
}

inline void ignore_sigpipe()
{
   // Set to ignore SIGPIPE once only.
   static pdk::os::thread::BasicAtomicInt atom = PDK_BASIC_ATOMIC_INITIALIZER(0);
   if (!atom.load()) {
      // More than one thread could turn off SIGPIPE at the same time
      // But that's acceptable because they all would be doing the same
      // action
      struct sigaction noaction;
      std::memset(&noaction, 0, sizeof(noaction));
      noaction.sa_handler = SIG_IGN;
      ::sigaction(SIGPIPE, &noaction, 0);
      atom.store(1);
   }
}

namespace {

// don't call PDK_OPEN or ::open
// call pdk::kernel::safe_open
inline int safe_open(const char *pathname, int flags, mode_t mode = 0777)
{
#ifdef O_CLOEXEC
   flags |= O_CLOEXEC;
#endif
   int fd;
   PDK_EINTR_LOOP(fd, PDK_OPEN(pathname, flags, mode));
   // unknown flags are ignored, so we have no way of verifying if
   // O_CLOEXEC was accepted
   if (fd != -1) {
      ::fcntl(fd, F_SETFD, FD_CLOEXEC);
   }
   return fd;
}
#undef PDK_OPEN
#define PDK_OPEN         pdk::kernel::safe_open

#ifndef PDK_OS_VXWORKS // no POSIX pipes in VxWorks
// don't call ::pipe
// call pdk::kernel::safe_pipe
inline int safe_pipe(int pipefd[2], int flags = 0)
{
   PDK_ASSERT((flags & ~O_NONBLOCK) == 0);
   
#ifdef PDK_THREADSAFE_CLOEXEC
   // use pipe2
   flags |= O_CLOEXEC;
   return ::pipe2(pipefd, flags); // pipe2 is documented not to return EINTR
#else
   int ret = ::pipe(pipefd);
   if (ret == -1) {
      return -1;
   }
   ::fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
   ::fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);
   // set non-block too?
   if (flags & O_NONBLOCK) {
      ::fcntl(pipefd[0], F_SETFL, ::fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
      ::fcntl(pipefd[1], F_SETFL, ::fcntl(pipefd[1], F_GETFL) | O_NONBLOCK);
   }
   return 0;
#endif
}

#endif // PDK_OS_VXWORKS

// don't call dup or fcntl(F_DUPFD)
inline int safe_dup(int oldfd, int atleast = 0, int flags = FD_CLOEXEC)
{
   PDK_ASSERT(flags == FD_CLOEXEC || flags == 0);
#ifdef F_DUPFD_CLOEXEC
   int cmd = F_DUPFD;
   if (flags & FD_CLOEXEC) {
      cmd = F_DUPFD_CLOEXEC;
   }
   return ::fcntl(oldfd, cmd, atleast);
#else
   // use F_DUPFD
   int ret = ::fcntl(oldfd, F_DUPFD, atleast);
   
   if (flags && ret != -1)
      ::fcntl(ret, F_SETFD, flags);
   return ret;
#endif
}

// don't call dup2
// call qt_safe_dup2
inline int safe_dup2(int oldfd, int newfd, int flags = FD_CLOEXEC)
{
   PDK_ASSERT(flags == FD_CLOEXEC || flags == 0);
   int ret;
#ifdef PDK_THREADSAFE_CLOEXEC
   // use dup3
   EINTR_LOOP(ret, ::dup3(oldfd, newfd, flags ? O_CLOEXEC : 0));
   return ret;
#else
   PDK_EINTR_LOOP(ret, ::dup2(oldfd, newfd));
   if (ret == -1) {
      return -1;
   }
   if (flags) {
      ::fcntl(newfd, F_SETFD, flags);
   }
   return 0;
#endif
}

inline pdk::pint64 safe_read(int fd, void *data, pdk::pint64 maxlen)
{
   pdk::pint64 ret = 0;
   PDK_EINTR_LOOP(ret, PDK_READ(fd, data, maxlen));
   return ret;
}
#undef PDK_READ
#define PDK_READ pdk::kernel::safe_read

inline pdk::pint64 safe_write(int fd, const void *data, pdk::pint64 len)
{
   pdk::pint64 ret = 0;
   PDK_EINTR_LOOP(ret, PDK_WRITE(fd, data, len));
   return ret;
}
#undef PDK_WRITE
#define PDK_WRITE pdk::kernel::safe_write

inline pdk::pint64 safe_write_nosignal(int fd, const void *data, pdk::pint64 len)
{
   ignore_sigpipe();
   return safe_write(fd, data, len);
}

inline int safe_close(int fd)
{
   int ret;
   PDK_EINTR_LOOP(ret, PDK_CLOSE(fd));
   return ret;
}

#undef PDK_CLOSE
#define PDK_CLOSE pdk::kernel::safe_close

// - VxWorks & iOS/tvOS/watchOS don't have processes
#if !defined(PDK_OS_VXWORKS) && !defined(PDK_NO_PROCESS)
inline int execve(const char *filename, char *const argv[],
                  char *const envp[])
{
   int ret;
   PDK_EINTR_LOOP(ret, ::execve(filename, argv, envp));
   return ret;
}

inline int safe_execv(const char *path, char *const argv[])
{
   int ret;
   PDK_EINTR_LOOP(ret, ::execv(path, argv));
   return ret;
}

inline int safe_execvp(const char *file, char *const argv[])
{
   int ret;
   PDK_EINTR_LOOP(ret, ::execvp(file, argv));
   return ret;
}

inline pid_t safe_waitpid(pid_t pid, int *status, int options)
{
   int ret;
   PDK_EINTR_LOOP(ret, ::waitpid(pid, status, options));
   return ret;
}

#endif // PDK_OS_VXWORKS

}

#if !defined(_POSIX_MONOTONIC_CLOCK)
#  define _POSIX_MONOTONIC_CLOCK -1
#endif

// in pdk/kernel/ElapsedtimerMac.cpp or pdk/kernel/TimestampUnix.cpp
timespec get_time() noexcept;
void nanosleep(timespec amount);

PDK_CORE_EXPORT int safe_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts);

namespace {

inline int poll_msecs(struct pollfd *fds, nfds_t nfds, int timeout)
{
   timespec ts;
   timespec *pts = nullptr;
   if (timeout >= 0) {
      ts.tv_sec = timeout / 1000;
      ts.tv_nsec = (timeout % 1000) * 1000 * 1000;
      pts = &ts;
   }
   return safe_poll(fds, nfds, pts);
}

inline struct pollfd make_pollfd(int fd, short events)
{
   struct pollfd pfd = { fd, events, 0 };
   return pfd;
}

}

// according to X/OPEN we have to define semun ourselves
// we use prefix as on some systems sem.h will have it
struct semid_ds;
union semun
{
   int m_val;                    /* value for SETVAL */
   struct semid_ds *m_buf;       /* buffer for IPC_STAT, IPC_SET */
   unsigned short *m_array;      /* array for GETALL, SETALL */
};

} // kernel
} // pdk

#endif // PDK_KERNEL_CORE_UNIX_H
