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

#include "pdk/kernel/internal/CoreUnixPrivate.h"

namespace pdk {
namespace kernel {

#define PDK_POLL_READ_MASK   (POLLIN | POLLRDNORM)
#define PDK_POLL_WRITE_MASK  (POLLOUT | POLLWRNORM | POLLWRBAND)
#define PDK_POLL_EXCEPT_MASK (POLLPRI | POLLRDBAND)
#define PDK_POLL_ERROR_MASK  (POLLERR | POLLNVAL)
#define PDK_POLL_EVENTS_MASK (PDK_POLL_READ_MASK | PDK_POLL_WRITE_MASK | PDK_POLL_EXCEPT_MASK)

namespace {

inline int pdk_poll_prepare(struct pollfd *fds, nfds_t nfds,
                            fd_set *read_fds, fd_set *write_fds, 
                            fd_set *except_fds)
{
   int max_fd = -1;
   FD_ZERO(read_fds);
   FD_ZERO(write_fds);
   FD_ZERO(except_fds);
   for (nfds_t i = 0; i < nfds; i++) {
      if (fds[i].fd >= FD_SETSIZE) {
         errno = EINVAL;
         return -1;
      }
      if ((fds[i].fd < 0) || (fds[i].revents & PDK_POLL_ERROR_MASK)) {
         continue;
      }
      if (fds[i].events & PDK_POLL_READ_MASK) {
         FD_SET(fds[i].fd, read_fds);
      }
      if (fds[i].events & PDK_POLL_WRITE_MASK) {
         FD_SET(fds[i].fd, write_fds);
      }
      if (fds[i].events & PDK_POLL_EXCEPT_MASK) {
         FD_SET(fds[i].fd, except_fds);
      }
      if (fds[i].events & PDK_POLL_EVENTS_MASK) {
         max_fd = std::max(max_fd, fds[i].fd);
      }
   }
   return max_fd + 1;
}

inline void pdk_poll_examine_ready_read(struct pollfd &pfd)
{
   int res;
   char data;
   
   PDK_EINTR_LOOP(res, ::recv(pfd.fd, &data, sizeof(data), MSG_PEEK));
   const int error = (res < 0) ? errno : 0;
   if (res == 0) {
      pfd.revents |= POLLHUP;
   } else if (res > 0 || error == ENOTSOCK || error == ENOTCONN) {
      pfd.revents |= PDK_POLL_READ_MASK & pfd.events;
   } else {
      switch (error) {
      case ESHUTDOWN:
      case ECONNRESET:
      case ECONNABORTED:
      case ENETRESET:
         pfd.revents |= POLLHUP;
         break;
      default:
         pfd.revents |= POLLERR;
         break;
      }
   }
}

inline int pdk_poll_sweep(struct pollfd *fds, nfds_t nfds,
                          fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
   int result = 0;
   for (nfds_t i = 0; i < nfds; i++) {
      if (fds[i].fd < 0) {
         continue;
      }
      if (FD_ISSET(fds[i].fd, read_fds)) {
         pdk_poll_examine_ready_read(fds[i]);
      }
      if (FD_ISSET(fds[i].fd, write_fds)) {
         fds[i].revents |= PDK_POLL_WRITE_MASK & fds[i].events;
      }
      if (FD_ISSET(fds[i].fd, except_fds)) {
         fds[i].revents |= PDK_POLL_EXCEPT_MASK & fds[i].events;
      }
      if (fds[i].revents != 0) {
         result++;
      }
   }
   return result;
}

inline bool pdk_poll_is_bad_fd(int fd)
{
   int ret;
   PDK_EINTR_LOOP(ret, fcntl(fd, F_GETFD));
   return (ret == -1 && errno == EBADF);
}

int pdk_poll_mark_bad_fds(struct pollfd *fds, const nfds_t nfds)
{
   int n_marked = 0;
   for (nfds_t i = 0; i < nfds; i++) {
      if (fds[i].fd < 0) {
         continue;
      }
      if (fds[i].revents & PDK_POLL_ERROR_MASK) {
         continue;
      }
      if (pdk_poll_is_bad_fd(fds[i].fd)) {
         fds[i].revents |= POLLNVAL;
         n_marked++;
      }
   }
   return n_marked;
}

} // anonymous namespace

int pdk_poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts)
{
   if (!fds && nfds) {
      errno = EFAULT;
      return -1;
   }
   fd_set read_fds, write_fds, except_fds;
   struct timeval tv, *ptv = 0;
   if (timeout_ts) {
      tv = timespec_to_timeval(*timeout_ts);
      ptv = &tv;
   }
   int n_bad_fds = 0;
   for (nfds_t i = 0; i < nfds; i++) {
      fds[i].revents = 0;
      if (fds[i].fd < 0) {
         continue;
      }
      if (fds[i].events & PDK_POLL_EVENTS_MASK) {
         continue;
      }
      if (pdk_poll_is_bad_fd(fds[i].fd)) {
         // Mark bad file descriptors that have no event flags set
         // here, as we won't be passing them to select below and therefore
         // need to do the check ourselves
         fds[i].revents = POLLNVAL;
         n_bad_fds++;
      }
   }
   
   while(true) {
      const int max_fd = pdk_poll_prepare(fds, nfds, &read_fds, &write_fds, &except_fds);
      if (max_fd < 0) {
         return max_fd;
      }
      if (n_bad_fds > 0) {
         tv.tv_sec = 0;
         tv.tv_usec = 0;
         ptv = &tv;
      }
      const int ret = ::select(max_fd, &read_fds, &write_fds, &except_fds, ptv);
      if (ret == 0) {
         return n_bad_fds;
      }
      if (ret > 0) {
         return pdk_poll_sweep(fds, nfds, &read_fds, &write_fds, &except_fds);
      }
      if (errno != EBADF) {
         return -1;
      }
      // We have at least one bad file descriptor that we waited on, find out which and try again
      n_bad_fds += pdk_poll_mark_bad_fds(fds, nfds);
   }
}

} // kernel
} // pdk
