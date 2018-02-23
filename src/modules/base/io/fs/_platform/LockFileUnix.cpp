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
// Created by softboy on 2018/02/23.

#include "pdk/base/io/fs/internal/LockFilePrivate.h"
#include "pdk/base/io/fs/TemporaryFile.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/internal/TemporaryFilePrivate.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/utils/Cache.h"
#include "pdk/kernel/CoreUnix.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"

#include "pdk/global/GlobalStatic.h"
#include <mutex>

#if !defined(PDK_OS_INTEGRITY)
#include <sys/file.h>
#endif

#include <sys/types.h> // kill
#include <sys/signal.h> // kill
#include <unistd.h> // gethostname

#if defined(PDK_OS_OSX)
#   include <libproc.h>
#elif defined(PDK_OS_LINUX)
#   include <unistd.h>
#   include <cstdio>
#elif defined(PDK_OS_HAIKU)
#   include <kernel/OS.h>
#elif defined(PDK_OS_BSD4) && !defined(PDK_PLATFORM_UIKIT)
#   include <sys/cdefs.h>
#   include <sys/param.h>
#   include <sys/sysctl.h>
# if !defined(PDK_OS_NETBSD)
#   include <sys/user.h>
# endif
#endif

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::io::fs::LockFile;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;

namespace
{

pdk::pint64 pdk_write_loop(int fd, const char *data, pdk::pint64 len)
{
   pdk::pint64 pos = 0;
   while (pos < len) {
      const pdk::pint64 ret = pdk::kernel::safe_write(fd, data + pos, len - pos);
      if (-1 == ret) {
         return pos;
      }
      pos += ret;
   }
   return pos;
}

/*
 * Details about file locking on Unix.
 *
 * There are three types of advisory locks on Unix systems:
 *  1) POSIX process-wide locks using fcntl(F_SETLK)
 *  2) BSD flock(2) system call
 *  3) Linux-specific file descriptor locks using fcntl(F_OFD_SETLK)
 * There's also a mandatory locking feature by POSIX, which is deprecated on
 * Linux and users are advised not to use it.
 *
 * The first problem is that the POSIX API is braindead. POSIX.1-2008 says:
 *
 *   All locks associated with a file for a given process shall be removed when
 *   a file descriptor for that file is closed by that process or the process
 *   holding that file descriptor terminates.
 *
 * The Linux manpage is clearer:
 *
 *  * If a process closes _any_ file descriptor referring to a file, then all
 *    of the process's locks on that file are released, regardless of the file
 *    descriptor(s) on which the locks were obtained. This is bad: [...]
 *
 *  * The threads in a process share locks. In other words, a multithreaded
 *    program can't use record locking to ensure that threads don't
 *    simultaneously access the same region of a file.
 *
 * So in order to use POSIX locks, we'd need a global mutex that stays locked
 * while the QLockFile is locked. For that reason, Qt does not use POSIX
 * advisory locks anymore.
 *
 * The next problem is that POSIX leaves undefined the relationship between
 * locks with fcntl(), flock() and lockf(). In some systems (like the BSDs),
 * all three use the same record set, while on others (like Linux) the locks
 * are independent, except if locking over NFS mounts, in which case they're
 * actually the same. Therefore, it's a very bad idea to mix them in the same
 * process.
 *
 * We therefore use only flock(2).
 */
bool set_native_locks(int fd)
{
#if defined(LOCK_EX) && defined(LOCK_NB)
   if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
      return false;
   }
#else
   PDK_UNUSED(fd);
#endif
   return true;
}

} // anonymous namespace

LockFile::LockError LockFilePrivate::tryLockSys()
{
   const ByteArray lockFilename = File::encodeName(m_fileName);
   const int fd = pdk::kernel::safe_open(lockFilename.getConstRawData(), O_WRONLY | O_CREAT | O_EXCL, 0666);
   if (fd < 0) {
      switch (errno) {
      case EEXIST:
         return LockFile::LockError::LockFailedError;
      case EACCES:
      case EROFS:
         return LockFile::LockError::PermissionError;
      default:
         return LockFile::LockError::UnknownError;
      }
   }
   // Ensure nobody else can delete the file while we have it
   if (!set_native_locks(fd)) {
      const int errnoSaved = errno;
      warning_stream() << "set_native_locks failed:" << pdk::pdk_error_string(errnoSaved);
   }
   ByteArray fileData = lockFileContents();
   if (pdk_write_loop(fd, fileData.getConstRawData(), fileData.size()) < fileData.size()) {
      pdk::kernel::safe_close(fd);
      if (!File::remove(m_fileName)) {
         warning_stream("LockFile: Could not remove our own lock file %s.", pdk_printable(m_fileName));
      }
      return LockFile::LockError::UnknownError;
   }
   // We hold the lock, continue.
   m_fileHandle = fd;
   // Sync to disk if possible. Ignore errors (e.g. not supported).
#if defined(_POSIX_SYNCHRONIZED_IO) && _POSIX_SYNCHRONIZED_IO > 0
   fdatasync(m_fileHandle);
#else
   fsync(m_fileHandle);
#endif
   return LockFile::LockError::NoError;
}

bool LockFilePrivate::removeStaleLock()
{
   const ByteArray lockFileName = File::encodeName(m_fileName);
   const int fd = pdk::kernel::safe_open(lockFileName.getConstRawData(), O_WRONLY, 0666);
   if (fd < 0) { // gone already?
      return false;
   }
   bool success = set_native_locks(fd) && (::unlink(lockFileName) == 0);
   close(fd);
   return success;
}

bool LockFilePrivate::isProcessRunning(pdk::pint64 pid, const String &appName)
{
   if (::kill(pid, 0) == -1 && errno == ESRCH) {
      return false; // PID doesn't exist anymore
   }
   const String processName = processNameByPid(pid);
   if (!processName.isEmpty()) {
      FileInfo fileInfo(appName);
      if (fileInfo.isSymLink()) {
         fileInfo.setFile(fileInfo.getSymLinkTarget());
      }
      if (processName != fileInfo.getFileName()) {
         return false; // PID got reused by a different application.
      }
   }
   return true;
}

String LockFilePrivate::processNameByPid(pdk::pint64 pid)
{
#if defined(PDK_OS_OSX)
   char name[1024];
   proc_name(pid, name, sizeof(name) / sizeof(char));
   return File::decodeName(name);
#elif defined(PDK_OS_LINUX)
   if (pdk::kernel::pdk_have_linux_procfs()) {
      return String();
   }
   char exePath[64];
   sprintf(exePath, "/proc/%lld/exe", pid);
   ByteArray buf = pdk::kernel::pdk_readlink(exePath);
   if (buf.isEmpty()) {
      // The pid is gone. Return some invalid process name to fail the test.
      return StringLiteral("/ERROR/");
   }
   return FileInfo(File::decodeName(buf)).getFileName();
#elif defined(PDK_OS_HAIKU)
   thread_info info;
   if (get_thread_info(pid, &info) != B_OK) {
      return String();
   }
   return File::decodeName(info.name);
#elif defined(PDK_OS_BSD4) && !defined(PDK_PLATFORM_UIKIT)
# if defined(PDK_OS_NETBSD)
   struct kinfo_proc2 kp;
   int mib[6] = { CTL_KERN, KERN_PROC2, KERN_PROC_PID, (int)pid, sizeof(struct kinfo_proc2), 1 };
# elif defined(PDK_OS_OPENBSD)
   struct kinfo_proc kp;
   int mib[6] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)pid, sizeof(struct kinfo_proc), 1 };
# else
   struct kinfo_proc kp;
   int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)pid };
# endif
   size_t len = sizeof(kp);
   u_int mib_len = sizeof(mib)/sizeof(u_int);
   
   if (sysctl(mib, mib_len, &kp, &len, NULL, 0) < 0) {
      return String();
   }
   
# if defined(PDK_OS_OPENBSD) || defined(PDK_OS_NETBSD)
   if (kp.p_pid != pid) {
      return String();
   } 
   String name = File::decodeName(kp.p_comm);
# else
   if (kp.ki_pid != pid) {
      return String();
   }
   String name = File::decodeName(kp.ki_comm);
# endif
   return name;
#else
   PDK_UNUSED(pid);
   return String();
#endif
}

} // internal

void LockFile::unlock()
{
   PDK_D(LockFile);
   if (!implPtr->m_isLocked) {
      return;
   }
   close(implPtr->m_fileHandle);
   implPtr->m_fileHandle = -1;
   if (!File::remove(implPtr->m_fileName)) {
      warning_stream() << "Could not remove our own lock file" << implPtr->m_fileName << "maybe permissions changed meanwhile?";
      // This is bad because other users of this lock file will now have to wait for the stale-lock-timeout...
   }
   implPtr->m_lockError = LockFile::LockError::NoError;
   implPtr->m_isLocked = false;
}

} // fs
} // io
} // pdk

