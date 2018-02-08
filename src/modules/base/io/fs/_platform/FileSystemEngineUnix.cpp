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
// Created by softboy on 2018/02/07.

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/kernel/CoreUnix.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/global/Logging.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"

#include <pwd.h>
#include <cstdlib> // for realpath()
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>

#if PDK_HAS_INCLUDE(<paths.h>)
# include <paths.h>
#endif
#ifndef _PATH_TMP           // from <paths.h>
# define _PATH_TMP          "/tmp"
#endif

#if defined(PDK_OS_MAC)
# include "pdk/kernel/internal/CoreMacPrivate.h"
# include <CoreFoundation/CFBundle.h>
#endif

#ifdef PDK_OS_OSX
#include <CoreServices/CoreServices.h>
#endif

#if defined(PDK_OS_DARWIN)
# include <copyfile.h>
// We cannot include <Foundation/Foundation.h> (it's an Objective-C header), but
// we need these declarations:
PDK_FORWARD_DECLARE_OBJC_CLASS(NSString);
extern "C" NSString *NSTemporaryDirectory();
#endif

#if defined(PDK_OS_LINUX)
#  include <sys/ioctl.h>
#  include <sys/syscall.h>
#  include <sys/sendfile.h>
#  include <linux/fs.h>

// in case linux/fs.h is too old and doesn't define it:
#ifndef FICLONE
#  define FICLONE       _IOW(0x94, 9, int)
#endif
#  if !PDK_CONFIG(renameat2) && defined(SYS_renameat2)
namespace {
int renameat2(int oldfd, const char *oldpath, int newfd, const char *newpath, unsigned flags)
{ 
   return syscall(SYS_renameat2, oldfd, oldpath, newfd, newpath, flags);
}
}
#  endif

#  if !PDK_CONFIG(statx) && defined(SYS_statx) && PDK_HAS_INCLUDE(<linux/stat.h>)
#    include <linux/stat.h>
namespace 
{
int statx(int dirfd, const char *pathname, int flag, unsigned mask, struct statx *statxbuf)
{
   return syscall(SYS_statx, dirfd, pathname, flag, mask, statxbuf); 
}
}
#  endif
#endif

#ifndef STATX_BASIC_STATS
struct statx { mode_t stx_mode; };
#endif

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::MessageLogger;
using pdk::os::thread::BasicAtomicInteger;
using pdk::os::thread::BasicAtomicInt;
using pdk::ds::VarLengthArray;

namespace {

void do_empty_file_entry_warning(const char *file, int line, const char *function)
{
   MessageLogger(file, line, function).warning("Empty filename passed to function");
   errno = EINVAL;
}
#define empty_file_entry_warning() do_empty_file_entry_warning(PDK_MESSAGELOG_FILE, PDK_MESSAGELOG_LINE, PDK_MESSAGELOG_FUNC)

namespace getfiletimes {
#if !PDK_CONFIG(futimens) && (PDK_CONFIG(futimes))
template <typename T>
static inline typename std::enable_if<(&T::st_atim, &T::st_mtim, true)>::type
get(const T *p, struct timeval *access, struct timeval *modification)
{
   access->tv_sec = p->st_atim.tv_sec;
   access->tv_usec = p->st_atim.tv_nsec / 1000;
   modification->tv_sec = p->st_mtim.tv_sec;
   modification->tv_usec = p->st_mtim.tv_nsec / 1000;
}

template <typename T>
static inline typename std::enable_if<(&T::st_atimespec, &T::st_mtimespec, true)>::type 
get(const T *p, struct timeval *access, struct timeval *modification)
{
   access->tv_sec = p->st_atimespec.tv_sec;
   access->tv_usec = p->st_atimespec.tv_nsec / 1000;
   modification->tv_sec = p->st_mtimespec.tv_sec;
   modification->tv_usec = p->st_mtimespec.tv_nsec / 1000;
}

#  ifndef st_atimensec
// if "st_atimensec" is defined, this would expand to invalid C++
template <typename T>
static inline typename std::enable_if<(&T::st_atimensec, &T::st_mtimensec, true)>::type 
get(const T *p, struct timeval *access, struct timeval *modification)
{
   access->tv_sec = p->st_atime;
   access->tv_usec = p->st_atimensec / 1000;
   modification->tv_sec = p->st_mtime;
   modification->tv_usec = p->st_mtimensec / 1000;
}
#  endif
#endif

pdk::pint64 timespec_to_msecs(const timespec &spec)
{
   return (pdk::pint64(spec.tv_sec) * 1000) + (spec.tv_nsec / 1000000);
}

// fallback set
PDK_DECL_UNUSED pdk::pint64 atime(const PDK_STATBUF &statBuffer, ulong)
{
   return pdk::pint64(statBuffer.st_atime) * 1000;
}

PDK_DECL_UNUSED pdk::pint64 birthtime(const PDK_STATBUF &, ulong)
{
   return PDK_INT64_C(0);
}

PDK_DECL_UNUSED pdk::pint64 ctime(const PDK_STATBUF &statBuffer, ulong)
{
   return pdk::pint64(statBuffer.st_ctime) * 1000;
}

PDK_DECL_UNUSED pdk::pint64 mtime(const PDK_STATBUF &statBuffer, ulong)
{
   return pdk::pint64(statBuffer.st_mtime) * 1000;
}

// Xtim, POSIX.1-2008
template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_atim, true), pdk::pint64>::type
atime(const T &statBuffer, int)
{
   return timespec_to_msecs(statBuffer.st_atim);
}

template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_birthtim, true), pdk::pint64>::type
birthtime(const T &statBuffer, int)
{
   return timespec_to_msecs(statBuffer.st_birthtim);
}

template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_ctim, true), pdk::pint64>::type
ctime(const T &statBuffer, int)
{
   return timespec_to_msecs(statBuffer.st_ctim);
}

template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_mtim, true), pdk::pint64>::type
mtime(const T &statBuffer, int)
{
   return timespec_to_msecs(statBuffer.st_mtim);
}

#ifndef st_mtimespec
// Xtimespec
template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_atimespec, true), pdk::pint64>::type
atime(const T &statBuffer, int)
{
   return timespec_to_msecs(statBuffer.st_atimespec);
}

template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_birthtimespec, true), pdk::pint64>::type
birthtime(const T &statBuffer, int)
{
   return timespec_to_msecs(statBuffer.st_birthtimespec);
}

template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_ctimespec, true), pdk::pint64>::type
ctime(const T &statBuffer, int)
{
   return timespec_to_msecs(statBuffer.st_ctimespec);
}

template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_mtimespec, true), pdk::pint64>::type
mtime(const T &statBuffer, int)
{
   return timespec_to_msecs(statBuffer.st_mtimespec);
}
#endif

#ifndef st_mtimensec
// Xtimensec
template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_atimensec, true), pdk::pint64>::type
atime(const T &statBuffer, int)
{
   return statBuffer.st_atime * PDK_INT64_C(1000) + statBuffer.st_atimensec / 1000000;
}

template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_birthtimensec, true), pdk::pint64>::type
birthtime(const T &statBuffer, int)
{ 
   return statBuffer.st_birthtime * PDK_INT64_C(1000) + statBuffer.st_birthtimensec / 1000000;
}

template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_ctimensec, true), pdk::pint64>::type
ctime(const T &statBuffer, int)
{
   return statBuffer.st_ctime * PDK_INT64_C(1000) + statBuffer.st_ctimensec / 1000000;
}

template <typename T>
PDK_DECL_UNUSED static typename std::enable_if<(&T::st_mtimensec, true), pdk::pint64>::type
mtime(const T &statBuffer, int)
{
   return statBuffer.st_mtime * PDK_INT64_C(1000) + statBuffer.st_mtimensec / 1000000;
}
#endif

} // namespace GetFileTimes

#if defined(_DEXTRA_FIRST)
void fill_stat64_from_stat32(struct stat64 *statBuf64, const struct stat &statBuf32)
{
   statBuf64->st_mode = statBuf32.st_mode;
   statBuf64->st_size = statBuf32.st_size;
#if _POSIX_VERSION >= 200809L
   statBuf64->st_ctim = statBuf32.st_ctim;
   statBuf64->st_mtim = statBuf32.st_mtim;
   statBuf64->st_atim = statBuf32.st_atim;
#else
   statBuf64->st_ctime = statBuf32.st_ctime;
   statBuf64->st_mtime = statBuf32.st_mtime;
   statBuf64->st_atime = statBuf32.st_atime;
#endif
   statBuf64->st_uid = statBuf32.st_uid;
   statBuf64->st_gid = statBuf32.st_gid;
}
#endif

} // anonymous namespace

#ifdef STATX_BASIC_STATS
namespace {
int pdk_real_statx(int fd, const char *pathname, int flags, struct statx *statxBuffer)
{
#ifdef PDK_ATOMIC_INT8_IS_SUPPORTED
   static BasicAtomicInteger<pdk::pint8> statxTested  = PDK_BASIC_ATOMIC_INITIALIZER(0);
#else
   static BasicAtomicInt statxTested = PDK_BASIC_ATOMIC_INITIALIZER(0);
#endif
   
   if (statxTested.load() == -1)
      return -ENOSYS;
   
   unsigned mask = STATX_BASIC_STATS | STATX_BTIME;
   int ret = statx(fd, pathname, flags, mask, statxBuffer);
   if (ret == -1 && errno == ENOSYS) {
      statxTested.store(-1);
      return -ENOSYS;
   }
   statxTested.store(1);
   return ret == -1 ? -errno : 0;
}

int pdk_statx(const char *pathname, struct statx *statxBuffer)
{
   return pdk_real_statx(AT_FDCWD, pathname, 0, statxBuffer);
}

int pdk_lstatx(const char *pathname, struct statx *statxBuffer)
{
   return pdk_real_statx(AT_FDCWD, pathname, AT_SYMLINK_NOFOLLOW, statxBuffer);
}

int pdk_fstatx(int fd, struct statx *statxBuffer)
{
   return pdk_real_statx(fd, "", AT_EMPTY_PATH, statxBuffer);
}

} // anonymous namespace

inline void FileSystemMetaData::fillFromStatxBuf(const struct statx &statxBuffer)
{
   // Permissions
   if (statxBuffer.stx_mode & S_IRUSR) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OwnerReadPermission;
   }
   if (statxBuffer.stx_mode & S_IWUSR) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OwnerWritePermission;
   }
   
   if (statxBuffer.stx_mode & S_IXUSR) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OwnerExecutePermission;
   }
   
   if (statxBuffer.stx_mode & S_IRGRP) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::GroupReadPermission;
   }
   
   if (statxBuffer.stx_mode & S_IWGRP) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::GroupWritePermission;
   }
   
   if (statxBuffer.stx_mode & S_IXGRP) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::GroupExecutePermission;
   }
   
   if (statxBuffer.stx_mode & S_IROTH) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OtherReadPermission;
   }
   
   if (statxBuffer.stx_mode & S_IWOTH) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OtherWritePermission;
   }
   
   if (statxBuffer.stx_mode & S_IXOTH) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OtherExecutePermission;
   }
   
   // Type
   if (S_ISLNK(statxBuffer.stx_mode)) {
      m_entryFlags |= FileSystemMetaData::LinkType;
   }
   
   if ((statxBuffer.stx_mode & S_IFMT) == S_IFREG) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::FileType;
   } else if ((statxBuffer.stx_mode & S_IFMT) == S_IFDIR) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::DirectoryType;
   } else if ((statxBuffer.stx_mode & S_IFMT) != S_IFBLK) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::SequentialType;
   }
   
   // Attributes
   m_entryFlags |= FileSystemMetaData::MetaDataFlag::ExistsAttribute; // inode exists
   if (statxBuffer.stx_nlink == 0) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::WasDeletedAttribute;
   }
   
   m_size = pdk::pint64(statxBuffer.stx_size);
   
   // Times
   auto toMSecs = [](struct statx_timestamp ts)
   {
      return pdk::pint64(ts.tv_sec) * 1000 + (ts.tv_nsec / 1000000);
   };
   m_accessTime = toMSecs(statxBuffer.stx_atime);
   m_metadataChangeTime = toMSecs(statxBuffer.stx_ctime);
   m_modificationTime = toMSecs(statxBuffer.stx_mtime);
   if (statxBuffer.stx_mask & STATX_BTIME) {
      m_birthTime = toMSecs(statxBuffer.stx_btime);
   } else {
      m_birthTime = 0;
   }
   m_userId = statxBuffer.stx_uid;
   m_groupId = statxBuffer.stx_gid;
}
#else
namespace {
int pdk_statx(const char *, struct statx *)
{ 
   return -ENOSYS;
}

int pdk_lstatx(const char *, struct statx *)
{
   return -ENOSYS;
}

int pdk_fstatx(int, struct statx *)
{ 
   return -ENOSYS;
}

} // anonymous namespace
inline void FileSystemMetaData::fillFromStatxBuf(const struct statx &)
{}
#endif

//static
bool FileSystemEngine::fillMetaData(int fd, FileSystemMetaData &data)
{
   data.m_entryFlags &= ~pdk::as_integer<FileSystemMetaData::MetaDataFlag>(FileSystemMetaData::MetaDataFlag::PosixStatFlags);
   data.m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::PosixStatFlags;
   
   union {
      struct statx statxBuffer;
      PDK_STATBUF statBuffer;
   };
   
   int ret = pdk_fstatx(fd, &statxBuffer);
   if (ret != -ENOSYS) {
      if (ret == 0) {
         data.fillFromStatxBuf(statxBuffer);
         return true;
      }
      return false;
   }
   
   if (PDK_FSTAT(fd, &statBuffer) == 0) {
      data.fillFromStatBuf(statBuffer);
      return true;
   }
   return false;
}

void FileSystemMetaData::fillFromStatBuf(const PDK_STATBUF &statBuffer)
{
   // Permissions
   if (statBuffer.st_mode & S_IRUSR) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OwnerReadPermission;
   }
   if (statBuffer.st_mode & S_IWUSR) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OwnerWritePermission;
   }
   if (statBuffer.st_mode & S_IXUSR) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OwnerExecutePermission;
   }
   if (statBuffer.st_mode & S_IRGRP) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::GroupReadPermission;
   }
   if (statBuffer.st_mode & S_IWGRP) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::GroupWritePermission;
   } 
   if (statBuffer.st_mode & S_IXGRP) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::GroupExecutePermission;
   }
   if (statBuffer.st_mode & S_IROTH) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OtherReadPermission;
   }
   if (statBuffer.st_mode & S_IWOTH) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OtherWritePermission;
   }
   if (statBuffer.st_mode & S_IXOTH) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::OtherExecutePermission;
   }
   // Type
   if ((statBuffer.st_mode & S_IFMT) == S_IFREG) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::FileType;
   } else if ((statBuffer.st_mode & S_IFMT) == S_IFDIR) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::DirectoryType;
   } else if ((statBuffer.st_mode & S_IFMT) != S_IFBLK) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::SequentialType;
   }
   // Attributes
   m_entryFlags |= FileSystemMetaData::MetaDataFlag::ExistsAttribute; // inode exists
   if (statBuffer.st_nlink == 0) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::WasDeletedAttribute;
   }
   m_size = statBuffer.st_size;
#ifdef UF_HIDDEN
   if (statBuffer.st_flags & UF_HIDDEN) {
      m_entryFlags |= FileSystemMetaData::MetaDataFlag::HiddenAttribute;
      m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::HiddenAttribute;
   }
#endif
   
   // Times
   m_accessTime = getfiletimes::atime(statBuffer, 0);
   m_birthTime = getfiletimes::birthtime(statBuffer, 0);
   m_metadataChangeTime = getfiletimes::ctime(statBuffer, 0);
   m_modificationTime = getfiletimes::mtime(statBuffer, 0);
   
   m_userId = statBuffer.st_uid;
   m_groupId = statBuffer.st_gid;
}

void FileSystemMetaData::fillFromDirEnt(const PDK_DIRENT &entry)
{
#if defined(_DEXTRA_FIRST)
   m_knownFlagsMask = 0;
   m_entryFlags = 0;
   for (dirent_extra *extra = _DEXTRA_FIRST(&entry); _DEXTRA_VALID(extra, &entry);
        extra = _DEXTRA_NEXT(extra)) {
      if (extra->d_type == _DTYPE_STAT || extra->d_type == _DTYPE_LSTAT) {
         const struct dirent_extra_stat * const extra_stat =
               reinterpret_cast<struct dirent_extra_stat *>(extra);
         // Remember whether this was a link or not, this saves an lstat() call later.
         if (extra->d_type == _DTYPE_LSTAT) {
            m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::LinkType;
            if (S_ISLNK(extra_stat->d_stat.st_mode)) {
               entryFlags |= FileSystemMetaData::MetaDataFlag::LinkType;
            }
         }
         
         // For symlinks, the extra type _DTYPE_LSTAT doesn't work for filling out the meta data,
         // as we need the stat() information there, not the lstat() information.
         // In this case, don't use the extra information.
         // Unfortunately, readdir() never seems to return extra info of type _DTYPE_STAT, so for
         // symlinks, we always incur the cost of an extra stat() call later.
         if (S_ISLNK(extra_stat->d_stat.st_mode) && extra->d_type == _DTYPE_LSTAT) {
            continue;
         }
         
#if defined(PDK_USE_XOPEN_LFS_EXTENSIONS) && defined(PDK_LARGEFILE_SUPPORT)
         // Even with large file support, d_stat is always of type struct stat, not struct stat64,
         // so it needs to be converted
         struct stat64 statBuf;
         fill_stat64_from_stat32(&statBuf, extra_stat->d_stat);
         fillFromStatBuf(statBuf);
#else
         fillFromStatBuf(extra_stat->d_stat);
#endif
         m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::PosixStatFlags;
         if (!S_ISLNK(extra_stat->d_stat.st_mode)) {
            m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::ExistsAttribute;
            m_entryFlags |= FileSystemMetaData::MetaDataFlag::ExistsAttribute;
         }
      }
   }
#elif defined(_DIRENT_HAVE_D_TYPE) || defined(PDK_OS_BSD4)
   // BSD4 includes OS X and iOS
   // ### This will clear all entry flags and knownFlagsMask
   switch (entry.d_type)
   {
   case DT_DIR:
      m_knownFlagsMask = FileSystemMetaData::MetaDataFlag::LinkType
            | FileSystemMetaData::MetaDataFlag::FileType
            | FileSystemMetaData::MetaDataFlag::DirectoryType
            | FileSystemMetaData::MetaDataFlag::SequentialType
            | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      
      m_entryFlags = FileSystemMetaData::MetaDataFlag::DirectoryType
            | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      
      break;
      
   case DT_BLK:
      m_knownFlagsMask = FileSystemMetaData::MetaDataFlag::LinkType
            | FileSystemMetaData::MetaDataFlag::FileType
            | FileSystemMetaData::MetaDataFlag::DirectoryType
            | FileSystemMetaData::MetaDataFlag::BundleType
            | FileSystemMetaData::MetaDataFlag::AliasType
            | FileSystemMetaData::MetaDataFlag::SequentialType
            | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      
      m_entryFlags = FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      
      break;
      
   case DT_CHR:
   case DT_FIFO:
   case DT_SOCK:
      // ### System attribute
      m_knownFlagsMask = FileSystemMetaData::MetaDataFlag::LinkType
            | FileSystemMetaData::MetaDataFlag::FileType
            | FileSystemMetaData::MetaDataFlag::DirectoryType
            | FileSystemMetaData::MetaDataFlag::BundleType
            | FileSystemMetaData::MetaDataFlag::AliasType
            | FileSystemMetaData::MetaDataFlag::SequentialType
            | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      
      m_entryFlags = FileSystemMetaData::MetaDataFlag::SequentialType
            | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      
      break;
      
   case DT_LNK:
      m_knownFlagsMask = FileSystemMetaData::MetaDataFlag::LinkType;
      m_entryFlags = FileSystemMetaData::MetaDataFlag::LinkType;
      break;
      
   case DT_REG:
      m_knownFlagsMask = FileSystemMetaData::MetaDataFlag::LinkType
            | FileSystemMetaData::MetaDataFlag::FileType
            | FileSystemMetaData::MetaDataFlag::DirectoryType
            | FileSystemMetaData::MetaDataFlag::BundleType
            | FileSystemMetaData::MetaDataFlag::SequentialType
            | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      
      m_entryFlags = FileSystemMetaData::MetaDataFlag::FileType
            | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      
      break;
      
   case DT_UNKNOWN:
   default:
      clear();
   }
#else
   PDK_UNUSED(entry);
#endif
}

FileSystemEntry FileSystemEngine::getLinkTarget(const FileSystemEntry &link, FileSystemMetaData &data)
{
   if (PDK_UNLIKELY(link.isEmpty())) {
      return empty_file_entry_warning(), link;
   }
   
   ByteArray s = pdk::kernel::pdk_readlink(link.getNativeFilePath().getConstRawData());
   if (s.length() > 0) {
      String ret;
      if (!data.hasFlags(FileSystemMetaData::MetaDataFlag::DirectoryType)) {
         fillMetaData(link, data, FileSystemMetaData::MetaDataFlag::DirectoryType);
      }
      if (data.isDirectory() && s[0] != '/') {
         Dir parent(link.getFilePath());
         parent.cdUp();
         ret = parent.getPath();
         if (!ret.isEmpty() && !ret.endsWith(Latin1Character('/'))) {
            ret += Latin1Character('/');
         }
      }
      ret += File::decodeName(s);
      
      if (!ret.startsWith(Latin1Character('/'))) {
         ret.prepend(getAbsoluteName(link).getPath() + Latin1Character('/'));
      }
      ret = Dir::cleanPath(ret);
      if (ret.size() > 1 && ret.endsWith(Latin1Character('/'))) {
         ret.chop(1);
      }
      return FileSystemEntry(ret);
   }
   return FileSystemEntry();
}

FileSystemEntry FileSystemEngine::getCanonicalName(const FileSystemEntry &entry, FileSystemMetaData &data)
{
   if (PDK_UNLIKELY(entry.isEmpty())) {
      return empty_file_entry_warning(), entry;
   }
   if (entry.isRoot()) {
      return entry;
   }
#if !defined(PDK_OS_MAC) && _POSIX_VERSION < 200809L
   // realpath(X,0) is not supported
   PDK_UNUSED(data);
   return FileSystemEntry(slowCanonicalized(getAbsoluteName(entry).getFilePath()));
#else
   char *ret = 0;
# if defined(PDK_OS_DARWIN)
   ret = (char*)malloc(PATH_MAX + 1);
   if (ret && realpath(entry.getNativeFilePath().getConstRawData(), (char*)ret) == 0) {
      const int savedErrno = errno; // errno is checked below, and free() might change it
      free(ret);
      errno = savedErrno;
      ret = 0;
   }
# else
#  if _POSIX_VERSION >= 200801L
   ret = realpath(entry.getNativeFilePath().getConstRawData(), (char*)0);
#  else
   ret = (char*)malloc(PATH_MAX + 1);
   if (realpath(entry.getNativeFilePath().getConstRawData(), (char*)ret) == 0) {
      const int savedErrno = errno; // errno is checked below, and free() might change it
      free(ret);
      errno = savedErrno;
      ret = 0;
   }
#  endif
# endif
   if (ret) {
      data.m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      data.m_entryFlags |= FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      String canonicalPath = Dir::cleanPath(File::decodeName(ret));
      free(ret);
      return FileSystemEntry(canonicalPath);
   } else if (errno == ENOENT) { // file doesn't exist
      data.m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      data.m_entryFlags &= ~(pdk::as_integer<FileSystemMetaData::MetaDataFlag>(FileSystemMetaData::MetaDataFlag::ExistsAttribute));
      return FileSystemEntry();
   }
   return entry;
#endif
}

//static
FileSystemEntry FileSystemEngine::getAbsoluteName(const FileSystemEntry &entry)
{
   if (PDK_UNLIKELY(entry.isEmpty())) {
      return empty_file_entry_warning(), entry;
   }
   if (entry.isAbsolute() && entry.isClean()) {
      return entry;
   }
   ByteArray orig = entry.getNativeFilePath();
   ByteArray result;
   if (orig.isEmpty() || !orig.startsWith('/')) {
      FileSystemEntry cur(getCurrentPath());
      result = cur.getNativeFilePath();
   }
   if (!orig.isEmpty() && !(orig.length() == 1 && orig[0] == '.')) {
      if (!result.isEmpty() && !result.endsWith('/')) {
         result.append('/');
      }
      result.append(orig);
   }
   if (result.length() == 1 && result[0] == '/') {
      return FileSystemEntry(result, FileSystemEntry::FromNativePath());
   }
   const bool isDir = result.endsWith('/');
   
   /* as long as Dir::cleanPath() operates on a String we have to convert to a string here.
     * ideally we never convert to a string since that loses information. Please fix after
     * we get a ByteArray version of Dir::cleanPath()
     */
   FileSystemEntry resultingEntry(result, FileSystemEntry::FromNativePath());
   String stringVersion = Dir::cleanPath(resultingEntry.getFilePath());
   if (isDir) {
      stringVersion.append(Latin1Character('/'));
   }
   return FileSystemEntry(stringVersion);
}

//static
ByteArray FileSystemEngine::getId(const FileSystemEntry &entry)
{
   if (PDK_UNLIKELY(entry.isEmpty())) {
      return empty_file_entry_warning(), ByteArray();
   }
   PDK_STATBUF statResult;
   if (PDK_STAT(entry.getNativeFilePath().getConstRawData(), &statResult)) {
      if (errno != ENOENT) {
         pdk::errno_warning("stat() failed for '%s'", entry.getNativeFilePath().getConstRawData());
      }
      return ByteArray();
   }
   ByteArray result = ByteArray::number(pdk::puint64(statResult.st_dev), 16);
   result += ':';
   result += ByteArray::number(pdk::puint64(statResult.st_ino));
   return result;
}

//static
ByteArray FileSystemEngine::getId(int id)
{
   PDK_STATBUF statResult;
   if (PDK_FSTAT(id, &statResult)) {
      pdk::errno_warning("fstat() failed for fd %d", id);
      return ByteArray();
   }
   ByteArray result = ByteArray::number(pdk::puint64(statResult.st_dev), 16);
   result += ':';
   result += ByteArray::number(pdk::puint64(statResult.st_ino));
   return result;
}

//static
String FileSystemEngine::resolveUserName(uint userId)
{
#if !defined(PDK_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(PDK_OS_OPENBSD)
   int sizeMax = sysconf(_SC_GETPW_R_SIZE_MAX);
   if (sizeMax == -1) {
      sizeMax = 1024;
   }
   VarLengthArray<char, 1024> buf(sizeMax);
#endif
   
#if !defined(PDK_OS_INTEGRITY)
   struct passwd *pw = 0;
#if !defined(PDK_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(PDK_OS_OPENBSD) && !defined(PDK_OS_VXWORKS)
   struct passwd entry;
   getpwuid_r(userId, &entry, buf.getRawData(), buf.size(), &pw);
#else
   pw = getpwuid(userId);
#endif
   if (pw)
      return File::decodeName(ByteArray(pw->pw_name));
#endif
   return String();
}

//static
String FileSystemEngine::resolveGroupName(uint groupId)
{
#if !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD)
   int sizeMax = sysconf(_SC_GETPW_R_SIZE_MAX);
   if (sizeMax == -1) {
      sizeMax = 1024;
   }
   VarLengthArray<char, 1024> buf(sizeMax);
#endif
   
#if !defined(PDK_OS_INTEGRITY)
   struct group *gr = 0;
#if !defined(PDK_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(PDK_OS_OPENBSD) && !defined(PDK_OS_VXWORKS)
   sizeMax = sysconf(_SC_GETGR_R_SIZE_MAX);
   if (sizeMax == -1) {
      sizeMax = 1024;
   }
   buf.resize(sizeMax);
   struct group entry;
   // Some large systems have more members than the POSIX max size
   // Loop over by doubling the buffer size (upper limit 250k)
   for (unsigned size = sizeMax; size < 256000; size += size)
   {
      buf.resize(size);
      // ERANGE indicates that the buffer was too small
      if (!getgrgid_r(groupId, &entry, buf.getRawData(), buf.size(), &gr)
          || errno != ERANGE) {
         break;
      }
   }
#else
   gr = getgrgid(groupId);
#endif
   if (gr) {
      return File::decodeName(ByteArray(gr->gr_name));
   }
#endif
   return String();
}

//static
bool FileSystemEngine::fillMetaData(const FileSystemEntry &entry, FileSystemMetaData &data,
                                    FileSystemMetaData::MetaDataFlags what)
{
   if (PDK_UNLIKELY(entry.isEmpty())) {
      return empty_file_entry_warning(), false;
   }
   
   
#if defined(PDK_OS_DARWIN)
   if (what & FileSystemMetaData::MetaDataFlag::BundleType) {
      if (!data.hasFlags(FileSystemMetaData::MetaDataFlag::DirectoryType)) {
         what |= FileSystemMetaData::MetaDataFlag::DirectoryType;
      }
   }
#endif
#ifdef UF_HIDDEN
   if (what & FileSystemMetaData::MetaDataFlag::HiddenAttribute) {
      // OS X >= 10.5: st_flags & UF_HIDDEN
      what |= FileSystemMetaData::MetaDataFlag::PosixStatFlags;
   }
#endif // defined(PDK_OS_DARWIN)
   
   // if we're asking for any of the stat(2) flags, then we're getting them all
   if (what & FileSystemMetaData::MetaDataFlag::PosixStatFlags) {
      what |= FileSystemMetaData::MetaDataFlag::PosixStatFlags;
   }
   data.m_entryFlags &= ~what;
   const ByteArray nativeFilePath = entry.getNativeFilePath();
   int entryErrno = 0; // innocent until proven otherwise
   
   // first, we may try lstat(2). Possible outcomes:
   //  - success and is a symlink: filesystem entry exists, but we need stat(2)
   //    -> statResult = -1;
   //  - success and is not a symlink: filesystem entry exists and we're done
   //    -> statResult = 0
   //  - failure: really non-existent filesystem entry
   //    -> entryExists = false; statResult = 0;
   //    both stat(2) and lstat(2) may generate a number of different errno
   //    conditions, but of those, the only ones that could happen and the
   //    entry still exist are EACCES, EFAULT, ENOMEM and EOVERFLOW. If we get
   //    EACCES or ENOMEM, then we have no choice on how to proceed, so we may
   //    as well conclude it doesn't exist; EFAULT can't happen and EOVERFLOW
   //    shouldn't happen because we build in _LARGEFIE64.
   union {
      PDK_STATBUF statBuffer;
      struct statx statxBuffer;
   };
   int statResult = -1;
   if (what & FileSystemMetaData::MetaDataFlag::LinkType) {
      mode_t mode = 0;
      statResult = pdk_lstatx(nativeFilePath, &statxBuffer);
      if (statResult == -ENOSYS) {
         // use lstst(2)
         statResult = PDK_LSTAT(nativeFilePath, &statBuffer);
         if (statResult == 0) {
            mode = statBuffer.st_mode;
         }
      } else if (statResult == 0) {
         statResult = 1; // record it was statx(2) that succeeded
         mode = statxBuffer.stx_mode;
      }
      if (statResult >= 0) {
         if (S_ISLNK(mode)) {
            // it's a symlink, we don't know if the file "exists"
            data.m_entryFlags |= FileSystemMetaData::MetaDataFlag::LinkType;
            statResult = -1;    // force stat(2) below
         } else {
            // it's a reagular file and it exists
            if (statResult) {
               data.fillFromStatxBuf(statxBuffer);
            } else {
               data.fillFromStatBuf(statBuffer);
            }
            
            data.m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::PosixStatFlags
                  | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
            data.m_entryFlags |= FileSystemMetaData::MetaDataFlag::ExistsAttribute;
         }
      } else {
         // it doesn't exist
         entryErrno = errno;
         data.m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      }
      
      data.m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::LinkType;
   }
   
   // second, we try a regular stat(2)
   if (statResult == -1 && (what & FileSystemMetaData::MetaDataFlag::PosixStatFlags)) {
      if (entryErrno == 0 && statResult == -1) {
         data.m_entryFlags &= ~pdk::as_integer<FileSystemMetaData::MetaDataFlag>(FileSystemMetaData::MetaDataFlag::PosixStatFlags);
         statResult = pdk_statx(nativeFilePath, &statxBuffer);
         if (statResult == -ENOSYS) {
            // use stat(2)
            statResult = PDK_STAT(nativeFilePath, &statBuffer);
            if (statResult == 0) {
               data.fillFromStatBuf(statBuffer);
            }
         } else if (statResult == 0) {
            data.fillFromStatxBuf(statxBuffer);
         }
      }
      
      if (statResult != 0) {
         entryErrno = errno;
         data.m_birthTime = 0;
         data.m_metadataChangeTime = 0;
         data.m_modificationTime = 0;
         data.m_accessTime = 0;
         data.m_size = 0;
         data.m_userId = (uint) -2;
         data.m_groupId = (uint) -2;
      }
      
      // reset the mask
      data.m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::PosixStatFlags
            | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
   }
   
   // third, we try access(2)
   if (what & (FileSystemMetaData::MetaDataFlag::UserPermissions | FileSystemMetaData::MetaDataFlag::ExistsAttribute)) {
      // calculate user permissions
      auto checkAccess = [&](FileSystemMetaData::MetaDataFlag flag, int mode) {
         if (entryErrno != 0 || (what & flag) == 0)
            return;
         if (PDK_ACCESS(nativeFilePath, mode) == 0) {
            // access ok (and file exists)
            data.m_entryFlags |= flag | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
         } else if (errno != EACCES && errno != EROFS) {
            entryErrno = errno;
         }
      };
      
      checkAccess(FileSystemMetaData::MetaDataFlag::UserReadPermission, R_OK);
      checkAccess(FileSystemMetaData::MetaDataFlag::UserWritePermission, W_OK);
      checkAccess(FileSystemMetaData::MetaDataFlag::UserExecutePermission, X_OK);
      
      // if we still haven't found out if the file exists, try F_OK
      if (entryErrno == 0 && (data.m_entryFlags & FileSystemMetaData::MetaDataFlag::ExistsAttribute) == 0) {
         if (PDK_ACCESS(nativeFilePath, F_OK) == -1) {
            entryErrno = errno;
         } else {
            data.m_entryFlags |= FileSystemMetaData::MetaDataFlag::ExistsAttribute;
         }
      }
      
      data.m_knownFlagsMask |= (what & FileSystemMetaData::MetaDataFlag::UserPermissions) |
            FileSystemMetaData::MetaDataFlag::ExistsAttribute;
   }
   
   if (what & FileSystemMetaData::MetaDataFlag::HiddenAttribute
       && !data.isHidden()) {
      String fileName = entry.getFileName();
      if (fileName.size() > 0 && fileName.at(0) == Latin1Character('.')) {
         data.m_entryFlags |= FileSystemMetaData::MetaDataFlag::HiddenAttribute;
      }
      data.m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::HiddenAttribute;
   }
   
   if (entryErrno != 0) {
      // // don't clear link: could be broken symlink
      what &= ~pdk::as_integer<FileSystemMetaData::MetaDataFlag>(FileSystemMetaData::MetaDataFlag::LinkType);
      data.clearFlags(what);
      return false;
   }
   return true;
}

// static
bool FileSystemEngine::cloneFile(int srcfd, int dstfd, const FileSystemMetaData &knownData)
{
   PDK_STATBUF statBuffer;
   if (knownData.hasFlags(FileSystemMetaData::MetaDataFlag::PosixStatFlags) &&
       knownData.isFile()) {
      statBuffer.st_size = knownData.getSize();
      statBuffer.st_mode = S_IFREG;
   } else if (knownData.hasFlags(FileSystemMetaData::MetaDataFlag::PosixStatFlags) &&
              knownData.isDirectory()) {
      return false;   // fcopyfile(3) returns success on directories
   } else if (PDK_FSTAT(srcfd, &statBuffer) == -1) {
      return false;
   } else if (!S_ISREG((statBuffer.st_mode))) {
      // not a regular file, let File do the copy
      return false;
   }
   
#if defined(PDK_OS_LINUX)
   if (statBuffer.st_size == 0) {
      // empty file? we're done.
      return true;
   }
   
   // first, try FICLONE (only works on regular files and only on certain fs)
   if (::ioctl(dstfd, FICLONE, srcfd) == 0)
      return true;
   
   // Second, try sendfile (it can send to some special types too).
   // sendfile(2) is limited in the kernel to 2G - 4k
   auto sendfileSize = [](PDK_OFF_T size) { return size_t(std::min<pdk::pint64>(0x7ffff000, size)); };
   
   ssize_t n = ::sendfile(dstfd, srcfd, NULL, sendfileSize(statBuffer.st_size));
   if (n == -1) {
      // if we got an error here, give up and try at an upper layer
      return false;
   }
   
   statBuffer.st_size -= n;
   while (statBuffer.st_size) {
      n = ::sendfile(dstfd, srcfd, NULL, sendfileSize(statBuffer.st_size));
      if (n == 0) {
         // uh oh, this is probably a real error (like ENOSPC), but we have
         // no way to notify File of partial success, so just erase any work
         // done (hopefully we won't get any errors, because there's nothing
         // we can do about them)
         n = ftruncate(dstfd, 0);
         n = lseek(srcfd, 0, SEEK_SET);
         n = lseek(dstfd, 0, SEEK_SET);
         return false;
      }
      if (n == 0) {
         return true;
      }
      statBuffer.st_size -= n;
   }
   
   return true;
#elif defined(PDK_OS_DARWIN)
   // try fcopyfile
   return fcopyfile(srcfd, dstfd, nullptr, COPYFILE_DATA | COPYFILE_STAT) == 0;
#else
   PDK_UNUSED(dstfd);
   return false;
#endif
}

// Note: if \a shouldMkdirFirst is false, we assume the caller did try to mkdir
// before calling this function.
static bool createDirectoryWithParents(const ByteArray &nativeName, bool shouldMkdirFirst = true)
{
   // helper function to check if a given path is a directory, since mkdir can
   // fail if the dir already exists (it may have been created by another
   // thread or another process)
   const auto isDir = [](const ByteArray &nativeName) {
      PDK_STATBUF st;
      return PDK_STAT(nativeName.getConstRawData(), &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
   };
   
   if (shouldMkdirFirst && PDK_MKDIR(nativeName, 0777) == 0) {
      return true;
   }  
   if (errno == EEXIST) {
      return isDir(nativeName);
   }  
   if (errno != ENOENT) {
      return false;
   }
   
   // mkdir failed because the parent dir doesn't exist, so try to create it
   int slash = nativeName.lastIndexOf('/');
   if (slash < 1)
      return false;
   
   ByteArray parentNativeName = nativeName.left(slash);
   if (!createDirectoryWithParents(parentNativeName))
      return false;
   
   // try again
   if (PDK_MKDIR(nativeName, 0777) == 0) {
      return true;
   } 
   return errno == EEXIST && isDir(nativeName);
}

//static
bool FileSystemEngine::createDirectory(const FileSystemEntry &entry, bool createParents)
{
   String dirName = entry.getFilePath();
   if (PDK_UNLIKELY(dirName.isEmpty())){
      return empty_file_entry_warning(), false;
   }
   // Darwin doesn't support trailing /'s, so remove for everyone
   while (dirName.size() > 1 && dirName.endsWith(Latin1Character('/'))) {
      dirName.chop(1);
   }
   // try to mkdir this directory
   ByteArray nativeName = File::encodeName(dirName);
   if (PDK_MKDIR(nativeName, 0777) == 0) {
      return true;
   }
   if (!createParents) {
      return false;
   }
   return createDirectoryWithParents(nativeName, false);
}

//static
bool FileSystemEngine::removeDirectory(const FileSystemEntry &entry, bool removeEmptyParents)
{
   if (PDK_UNLIKELY(entry.isEmpty())) {
      return empty_file_entry_warning(), false;
   }
   if (removeEmptyParents) {
      String dirName = Dir::cleanPath(entry.getFilePath());
      for (int oldslash = 0, slash = dirName.length(); slash > 0; oldslash = slash) {
         const ByteArray chunk = File::encodeName(dirName.left(slash));
         PDK_STATBUF st;
         if (PDK_STAT(chunk.getConstRawData(), &st) != -1) {
            if ((st.st_mode & S_IFMT) != S_IFDIR) {
               return false;
            }
            if (::rmdir(chunk.getConstRawData()) != 0) {
               return oldslash != 0;
            }
         } else {
            return false;
         }
         slash = dirName.lastIndexOf(Dir::getSeparator(), oldslash-1);
      }
      return true;
   }
   return rmdir(File::encodeName(entry.getFilePath()).getConstRawData()) == 0;
}

//static
bool FileSystemEngine::createLink(const FileSystemEntry &source, const FileSystemEntry &target, SystemError &error)
{
   if (PDK_UNLIKELY(source.isEmpty() || target.isEmpty())) {
      return empty_file_entry_warning(), false;
   }
   if (::symlink(source.getNativeFilePath().getConstRawData(), target.getNativeFilePath().getConstRawData()) == 0) {
      return true;
   }
   error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
   return false;
}

//static
bool FileSystemEngine::copyFile(const FileSystemEntry &source, const FileSystemEntry &target, SystemError &error)
{
   PDK_UNUSED(source);
   PDK_UNUSED(target);
   error = SystemError(ENOSYS, SystemError::ErrorScope::StandardLibraryError); //Function not implemented
   return false;
}

//static
bool FileSystemEngine::renameFile(const FileSystemEntry &source, const FileSystemEntry &target, SystemError &error)
{
   FileSystemEntry::NativePath srcPath = source.getNativeFilePath();
   FileSystemEntry::NativePath tgtPath = target.getNativeFilePath();
   if (PDK_UNLIKELY(srcPath.isEmpty() || tgtPath.isEmpty())) {
      return empty_file_entry_warning(), false;
   }
#if defined(RENAME_NOREPLACE) && (PDK_CONFIG(renameat2) || defined(SYS_renameat2))
   if (renameat2(AT_FDCWD, srcPath, AT_FDCWD, tgtPath, RENAME_NOREPLACE) == 0)
      return true;
   
   // If we're using syscall(), check for ENOSYS;
   // if renameat2 came from libc, we don't accept ENOSYS.
   // We can also get EINVAL for some non-local filesystems.
   if ((PDK_CONFIG(renameat2) || errno != ENOSYS) && errno != EINVAL) {
      error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
      return false;
   }
#endif
#if defined(PDK_OS_DARWIN) && defined(RENAME_EXCL)
   if (__builtin_available(macOS 10.12, *)) {
      if (renameatx_np(AT_FDCWD, srcPath, AT_FDCWD, tgtPath, RENAME_EXCL) == 0)
         return true;
      if (errno != ENOTSUP) {
         error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
         return false;
      }
   }
#endif
   
   if (::link(srcPath, tgtPath) == 0) {
      if (::unlink(srcPath) == 0)
         return true;
      
      // if we managed to link but can't unlink the source, it's likely
      // it's in a directory we don't have write access to; fail the
      // renaming instead
      int savedErrno = errno;
      
      // this could fail too, but there's nothing we can do about it now
      ::unlink(tgtPath);
      
      error = SystemError(savedErrno, SystemError::ErrorScope::StandardLibraryError);
      return false;
   } else {
      // man 2 link on Linux has:
      // EPERM  The filesystem containing oldpath and newpath does not
      //        support the creation of hard links.
      errno = EPERM;
   }
   
   switch (errno) {
   case EACCES:
   case EEXIST:
   case ENAMETOOLONG:
   case ENOENT:
   case ENOTDIR:
   case EROFS:
   case EXDEV:
      // accept the error from link(2) (especially EEXIST) and don't retry
      break;
      
   default:
      // fall back to rename()
      // ### Race condition. If a file is moved in after this, it /will/ be
      // overwritten.
      if (::rename(srcPath, tgtPath) == 0) {
         return true;
      }
   }
   error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
   return false;
}

//static
bool FileSystemEngine::renameOverwriteFile(const FileSystemEntry &source, const FileSystemEntry &target, 
                                           SystemError &error)
{
   if (PDK_UNLIKELY(source.isEmpty() || target.isEmpty())) {
      return empty_file_entry_warning(), false;
   }  
   if (::rename(source.getNativeFilePath().getConstRawData(), 
                target.getNativeFilePath().getConstRawData()) == 0) {
      return true;
   }
   error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
   return false;
}

//static
bool FileSystemEngine::removeFile(const FileSystemEntry &entry, SystemError &error)
{
   if (PDK_UNLIKELY(entry.isEmpty())) {
      return empty_file_entry_warning(), false;
   }
   if (unlink(entry.getNativeFilePath().getConstRawData()) == 0) {
      return true;
   } 
   error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
   return false;
}

static mode_t toMode_t(File::Permissions permissions)
{
   mode_t mode = 0;
   if (permissions & (pdk::as_integer<File::Permission>(File::Permission::ReadOwner) | 
                      pdk::as_integer<File::Permission>(File::Permission::ReadUser))) {
      mode |= S_IRUSR;
   }
   if (permissions & (pdk::as_integer<File::Permission>(File::Permission::WriteOwner) | 
                      pdk::as_integer<File::Permission>(File::Permission::WriteUser))) {
      mode |= S_IWUSR;
   }      
   if (permissions & (pdk::as_integer<File::Permission>(File::Permission::ExeOwner) | 
                      pdk::as_integer<File::Permission>(File::Permission::ExeUser))) {
      mode |= S_IXUSR;
   }
   if (permissions & File::Permission::ReadGroup) {
      mode |= S_IRGRP;
   }
   if (permissions & File::Permission::WriteGroup) {
      mode |= S_IWGRP;
   } 
   if (permissions & File::Permission::ExeGroup) {
      mode |= S_IXGRP;
   }
   if (permissions & File::Permission::ReadOther) {
      mode |= S_IROTH;
   }
   if (permissions & File::Permission::WriteOther) {
      mode |= S_IWOTH;
   } 
   if (permissions & File::Permission::ExeOther) {
      mode |= S_IXOTH;
   }
   return mode;
}

//static
bool FileSystemEngine::setPermissions(const FileSystemEntry &entry, File::Permissions permissions, SystemError &error, FileSystemMetaData *data)
{
   if (PDK_UNLIKELY(entry.isEmpty()))
      return empty_file_entry_warning(), false;
   
   mode_t mode = toMode_t(permissions);
   bool success = ::chmod(entry.getNativeFilePath().getConstRawData(), mode) == 0;
   if (success && data) {
      data->m_entryFlags &= ~pdk::as_integer<FileSystemMetaData::MetaDataFlag>(FileSystemMetaData::MetaDataFlag::Permissions);
      data->m_entryFlags |= FileSystemMetaData::MetaDataFlag(uint(permissions));
      data->m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::Permissions;
   }
   if (!success) {
      error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
   }
   return success;
}

//static
bool FileSystemEngine::setPermissions(int fd, File::Permissions permissions, SystemError &error, FileSystemMetaData *data)
{
   mode_t mode = toMode_t(permissions);
   
   bool success = ::fchmod(fd, mode) == 0;
   if (success && data) {
      data->m_entryFlags &= ~pdk::as_integer<FileSystemMetaData::MetaDataFlag>(FileSystemMetaData::MetaDataFlag::Permissions);
      data->m_entryFlags |= FileSystemMetaData::MetaDataFlag(uint(permissions));
      data->m_knownFlagsMask |= FileSystemMetaData::MetaDataFlag::Permissions;
   }
   if (!success) {
      error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
   }
   return success;
}

//static
bool FileSystemEngine::setFileTime(int fd, const DateTime &newDate, AbstractFileEngine::FileTime time, SystemError &error)
{
   if (!newDate.isValid() || time == AbstractFileEngine::FileTime::BirthTime ||
       time == AbstractFileEngine::FileTime::MetadataChangeTime) {
      error = SystemError(EINVAL, SystemError::ErrorScope::StandardLibraryError);
      return false;
   }
   
#if PDK_CONFIG(futimens)
   struct timespec ts[2];
   
   ts[0].tv_sec = ts[1].tv_sec = 0;
   ts[0].tv_nsec = ts[1].tv_nsec = UTIME_OMIT;
   
   const pdk::pint64 msecs = newDate.toMSecsSinceEpoch();
   
   if (time == AbstractFileEngine::FileTime::AccessTime) {
      ts[0].tv_sec = msecs / 1000;
      ts[0].tv_nsec = (msecs % 1000) * 1000000;
   } else if (time == AbstractFileEngine::FileTime::ModificationTime) {
      ts[1].tv_sec = msecs / 1000;
      ts[1].tv_nsec = (msecs % 1000) * 1000000;
   }
   
   if (futimens(fd, ts) == -1) {
      error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
      return false;
   }
   
   return true;
#elif PDK_CONFIG(futimes)
   struct timeval tv[2];
   PDK_STATBUF st;
   
   if (QT_FSTAT(fd, &st) == -1) {
      error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
      return false;
   }
   
   getfiletimes::get(&st, &tv[0], &tv[1]);
   
   const pdk::pint64 msecs = newDate.toMSecsSinceEpoch();
   
   if (time == AbstractFileEngine::FileTime::AccessTime) {
      tv[0].tv_sec = msecs / 1000;
      tv[0].tv_usec = (msecs % 1000) * 1000;
   } else if (time == AbstractFileEngine::FileTime::ModificationTime) {
      tv[1].tv_sec = msecs / 1000;
      tv[1].tv_usec = (msecs % 1000) * 1000;
   }
   
   if (futimes(fd, tv) == -1) {
      error = SystemError(errno, SystemError::ErrorScope::StandardLibraryError);
      return false;
   }
   
   return true;
#else
   PDK_UNUSED(fd);
   error = SystemError(ENOSYS, SystemError::ErrorScope::StandardLibraryError);
   return false;
#endif
}

String FileSystemEngine::getHomePath()
{
   String home = File::decodeName(pdk::pdk_getenv("HOME"));
   if (home.isEmpty()) {
      home = getRootPath();
   }
   return Dir::cleanPath(home);
}

String FileSystemEngine::getRootPath()
{
   return Latin1String("/");
}

String FileSystemEngine::getTempPath()
{
#ifdef PDK_UNIX_TEMP_PATH_OVERRIDE
   return Latin1String(PDK_UNIX_TEMP_PATH_OVERRIDE);
#else
   String temp = File::decodeName(pdk::pdk_getenv("TMPDIR"));
   if (temp.isEmpty()) {
      if (false) {
#if defined(PDK_OS_DARWIN)
      } else if (NSString *nsPath = NSTemporaryDirectory()) {
         temp = String::fromCFString((CFStringRef)nsPath);
#endif
      } else {
         temp = Latin1String(_PATH_TMP);
      }
   }
   return Dir::cleanPath(temp);
#endif
}

bool FileSystemEngine::setCurrentPath(const FileSystemEntry &path)
{
   int r;
   r = PDK_CHDIR(path.getNativeFilePath().getConstRawData());
   return r >= 0;
}

FileSystemEntry FileSystemEngine::getCurrentPath()
{
   FileSystemEntry result;
#if defined(__GLIBC__) && !defined(PATH_MAX)
   char *currentName = ::get_current_dir_name();
   if (currentName) {
      result = FileSystemEntry(ByteArray(currentName), FileSystemEntry::FromNativePath());
      ::free(currentName);
   }
#else
   char currentName[PATH_MAX+1];
   if (::getcwd(currentName, PATH_MAX)) {
#if defined(PDK_OS_VXWORKS) && defined(VXWORKS_VXSIM)
      ByteArray dir(currentName);
      if (dir.indexOf(':') < dir.indexOf('/')) {
          dir.remove(0, dir.indexOf(':')+1);
      }
      pdk::strncpy(currentName, dir.getConstRawData(), PATH_MAX);
#endif
      result = FileSystemEntry(ByteArray(currentName), FileSystemEntry::FromNativePath());
   }
# if defined(PDK_DEBUG)
   if (result.isEmpty())
      warning_stream("FileSystemEngine::getCurrentPath: getcwd() failed");
# endif
#endif
   return result;
}

} // internal
} // fs
} // io
} // pdk
