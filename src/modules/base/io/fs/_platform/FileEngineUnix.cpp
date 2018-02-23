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
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"

#ifndef PDK_NO_FSFILEENGINE

#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/ds/VarLengthArray.h"

#include <sys/mman.h>
#include <cstdlib>
#include <climits>
#include <cerrno>

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::pdk_error_string;

namespace {

inline int open_mode_to_open_flags(IoDevice::OpenModes mode)
{
   int oflags = PDK_OPEN_RDONLY;
#ifdef PDK_LARGEFILE_SUPPORT
   oflags |= PDK_OPEN_LARGEFILE;
#endif
   
   if ((mode & File::OpenMode::ReadWrite) == File::OpenMode::ReadWrite) {
      oflags = PDK_OPEN_RDWR | PDK_OPEN_CREAT;
   } else if (mode & File::OpenMode::WriteOnly) {
      oflags = PDK_OPEN_WRONLY | PDK_OPEN_CREAT;
   }
   
   if (mode & File::OpenMode::Append) {
      oflags |= PDK_OPEN_APPEND;
   } else if (mode & File::OpenMode::WriteOnly) {
      if ((mode & File::OpenMode::Truncate) || !(mode & File::OpenMode::ReadOnly)) {
         oflags |= PDK_OPEN_TRUNC;
      }
   }
   return oflags;
}

inline String msg_open_directory()
{
   const char message[] = PDK_TRANSLATE_NOOP("IoDevice", "file to open is a directory");
#if PDK_CONFIG(translation)
   return IoDevice::tr(message);
#else
   return Latin1String(message);
#endif
}

} // anonymous namespace


bool FileEnginePrivate::nativeOpen(IoDevice::OpenModes openMode)
{
   PDK_Q(FileEngine);
   PDK_ASSERT_X(openMode & IoDevice::OpenMode::Unbuffered, "FileEngine::open",
                "FileEngine no longer supports buffered mode; upper layer must buffer");
   if (openMode & IoDevice::OpenMode::Unbuffered) {
      int flags = open_mode_to_open_flags(openMode);
      
      // Try to open the file in unbuffered mode.
      do {
         m_fd = PDK_OPEN(m_fileEntry.getNativeFilePath().getConstRawData(), flags, 0666);
      } while (m_fd == -1 && errno == EINTR);
      
      // On failure, return and report the error.
      if (m_fd == -1) {
         apiPtr->setError(errno == EMFILE ? File::FileError::ResourceError : File::FileError::OpenError,
                          pdk_error_string(errno));
         return false;
      }
      
      if (!(openMode & IoDevice::OpenMode::WriteOnly)) {
         // we don't need this check if we tried to open for writing because then
         // we had received EISDIR anyway.
         if (FileSystemEngine::fillMetaData(m_fd, m_metaData)
             && m_metaData.isDirectory()) {
            apiPtr->setError(File::FileError::OpenError, msg_open_directory());
            PDK_CLOSE(m_fd);
            return false;
         }
      }
      
      // Seek to the end when in Append mode.
      if (flags & pdk::as_integer<File::OpenMode>(File::OpenMode::Append)) {
         int ret;
         do {
            ret = PDK_LSEEK(m_fd, 0, SEEK_END);
         } while (ret == -1 && errno == EINTR);
         
         if (ret == -1) {
            apiPtr->setError(errno == EMFILE ? File::FileError::ResourceError : File::FileError::OpenError,
                             pdk_error_string(int(errno)));
            return false;
         }
      }
      
      m_fh = 0;
   }
   
   m_closeFileHandle = true;
   return true;
}

bool FileEnginePrivate::nativeClose()
{
   return closeFdFh();
}

bool FileEnginePrivate::nativeFlush()
{
   return m_fh ? flushFh() : m_fd != -1;
}

bool FileEnginePrivate::nativeSyncToDisk()
{
   PDK_Q(FileEngine);
#if defined(_POSIX_SYNCHRONIZED_IO) && _POSIX_SYNCHRONIZED_IO > 0
   const int ret = fdatasync(getNativeHandle());
#else
   const int ret = fsync(getNativeHandle());
#endif
   if (ret != 0)
      apiPtr->setError(File::FileError::WriteError, pdk_error_string(errno));
   return ret == 0;
}

pdk::pint64 FileEnginePrivate::nativeRead(char *data, pdk::pint64 len)
{
   PDK_Q(FileEngine);
   
   if (m_fh && getNativeIsSequential()) {
      size_t readBytes = 0;
      int oldFlags = fcntl(PDK_FILENO(m_fh), F_GETFL);
      for (int i = 0; i < 2; ++i) {
         // Unix: Make the underlying file descriptor non-blocking
         if ((oldFlags & O_NONBLOCK) == 0)
            fcntl(PDK_FILENO(m_fh), F_SETFL, oldFlags | O_NONBLOCK);
         
         // Cross platform stdlib read
         size_t read = 0;
         do {
            read = fread(data + readBytes, 1, size_t(len - readBytes), m_fh);
         } while (read == 0 && !feof(m_fh) && errno == EINTR);
         if (read > 0) {
            readBytes += read;
            break;
         } else {
            if (readBytes)
               break;
            readBytes = read;
         }
         
         // Unix: Restore the blocking state of the underlying socket
         if ((oldFlags & O_NONBLOCK) == 0) {
            fcntl(PDK_FILENO(m_fh), F_SETFL, oldFlags);
            if (readBytes == 0) {
               int readByte = 0;
               do {
                  readByte = fgetc(m_fh);
               } while (readByte == -1 && errno == EINTR);
               if (readByte != -1) {
                  *data = uchar(readByte);
                  readBytes += 1;
               } else {
                  break;
               }
            }
         }
      }
      // Unix: Restore the blocking state of the underlying socket
      if ((oldFlags & O_NONBLOCK) == 0) {
         fcntl(PDK_FILENO(m_fh), F_SETFL, oldFlags);
      }
      if (readBytes == 0 && !feof(m_fh)) {
         // if we didn't read anything and we're not at EOF, it must be an error
         apiPtr->setError(File::FileError::ReadError, pdk_error_string(int(errno)));
         return -1;
      }
      return readBytes;
   }
   
   return readFdFh(data, len);
}

pdk::pint64 FileEnginePrivate::nativeReadLine(char *data, pdk::pint64 maxlen)
{
   return readLineFdFh(data, maxlen);
}

pdk::pint64 FileEnginePrivate::nativeWrite(const char *data, pdk::pint64 len)
{
   return writeFdFh(data, len);
}

pdk::pint64 FileEnginePrivate::getNativePos() const
{
   return getPosFdFh();
}

bool FileEnginePrivate::nativeSeek(pdk::pint64 pos)
{
   return seekFdFh(pos);
}

int FileEnginePrivate::getNativeHandle() const
{
   return m_fh ? fileno(m_fh) : m_fd;
}

bool FileEnginePrivate::getNativeIsSequential() const
{
   return isSequentialFdFh();
}

bool FileEngine::remove()
{
   PDK_D(FileEngine);
   SystemError error;
   bool ret = FileSystemEngine::removeFile(implPtr->m_fileEntry, error);
   implPtr->m_metaData.clear();
   if (!ret) {
      setError(File::FileError::RemoveError, error.toString());
   }
   return ret;
}

bool FileEngine::copy(const String &newName)
{
   PDK_D(FileEngine);
   SystemError error;
   bool ret = FileSystemEngine::copyFile(implPtr->m_fileEntry, FileSystemEntry(newName), error);
   if (!ret) {
      setError(File::FileError::CopyError, error.toString());
   }
   return ret;
}

bool FileEngine::renameOverwrite(const String &newName)
{
   PDK_D(FileEngine);
   SystemError error;
   bool ret = FileSystemEngine::renameOverwriteFile(implPtr->m_fileEntry, FileSystemEntry(newName), error);
   if (!ret) {
      setError(File::FileError::RenameError, error.toString());
   }
   return ret;
}

bool FileEngine::rename(const String &newName)
{
   PDK_D(FileEngine);
   SystemError error;
   bool ret = FileSystemEngine::renameFile(implPtr->m_fileEntry, FileSystemEntry(newName), error);
   
   if (!ret) {
      setError(File::FileError::RenameError, error.toString());
   }
   
   return ret;
}

bool FileEngine::link(const String &newName)
{
   PDK_D(FileEngine);
   SystemError error;
   bool ret = FileSystemEngine::createLink(implPtr->m_fileEntry, FileSystemEntry(newName), error);
   if (!ret) {
      setError(File::FileError::RenameError, error.toString());
   }
   return ret;
}

pdk::pint64 FileEnginePrivate::nativeSize() const
{
   return sizeFdFh();
}

bool FileEngine::mkdir(const String &name, bool createParentDirectories) const
{
   return FileSystemEngine::createDirectory(FileSystemEntry(name), createParentDirectories);
}

bool FileEngine::rmdir(const String &name, bool recurseParentDirectories) const
{
   return FileSystemEngine::removeDirectory(FileSystemEntry(name), recurseParentDirectories);
}

bool FileEngine::caseSensitive() const
{
   return true;
}

bool FileEngine::setCurrentPath(const String &path)
{
   return FileSystemEngine::setCurrentPath(FileSystemEntry(path));
}

String FileEngine::getCurrentPath(const String &)
{
   return FileSystemEngine::getCurrentPath().getFilePath();
}

String FileEngine::getHomePath()
{
   return FileSystemEngine::getHomePath();
}

String FileEngine::getRootPath()
{
   return FileSystemEngine::getRootPath();
}

String FileEngine::getTempPath()
{
   return FileSystemEngine::getTempPath();
}

FileInfoList FileEngine::getDrives()
{
   FileInfoList ret;
   ret.push_back(FileInfo(getRootPath()));
   return ret;
}

bool FileEnginePrivate::doStat(FileSystemMetaData::MetaDataFlags flags) const
{
   if (!m_triedStat || !m_metaData.hasFlags(flags)) {
      m_triedStat = 1;
      int localFd = m_fd;
      if (m_fh && m_fileEntry.isEmpty())
         localFd = PDK_FILENO(m_fh);
      if (localFd != -1)
         FileSystemEngine::fillMetaData(localFd, m_metaData);
      
      if (m_metaData.missingFlags(flags) && !m_fileEntry.isEmpty()) {
         FileSystemEngine::fillMetaData(m_fileEntry, m_metaData, m_metaData.missingFlags(flags));
      }
   }
   return m_metaData.exists();
}

bool FileEnginePrivate::isSymlink() const
{
   if (!m_metaData.hasFlags(FileSystemMetaData::MetaDataFlag::LinkType)) {
      FileSystemEngine::fillMetaData(m_fileEntry, m_metaData, FileSystemMetaData::MetaDataFlag::LinkType);
   }
   return m_metaData.isLink();
}

AbstractFileEngine::FileFlags FileEngine::getFileFlags(FileFlags type) const
{
   PDK_D(const FileEngine);
   
   if (type & FileFlag::Refresh) {
      implPtr->m_metaData.clear();
   }
   AbstractFileEngine::FileFlags ret = 0;
   
   if (type & FileFlag::FlagsMask) {
      ret |= FileFlag::LocalDiskFlag;
   }  
   
   bool exists;
   {
      FileSystemMetaData::MetaDataFlags queryFlags = 0;
      
      queryFlags |= FileSystemMetaData::MetaDataFlags(uint(type))
            & FileSystemMetaData::MetaDataFlag::Permissions;
      
      if (type & FileFlag::TypesMask)
         queryFlags |= FileSystemMetaData::MetaDataFlag::AliasType
               | FileSystemMetaData::MetaDataFlag::LinkType
               | FileSystemMetaData::MetaDataFlag::FileType
               | FileSystemMetaData::MetaDataFlag::DirectoryType
               | FileSystemMetaData::MetaDataFlag::BundleType
               | FileSystemMetaData::MetaDataFlag::WasDeletedAttribute;
      
      if (type & FileFlag::FlagsMask) {
         queryFlags |= FileSystemMetaData::MetaDataFlag::HiddenAttribute
               | FileSystemMetaData::MetaDataFlag::ExistsAttribute;
      } else if (type & FileFlag::ExistsFlag) {
         queryFlags |= FileSystemMetaData::MetaDataFlag::WasDeletedAttribute;
      }
      queryFlags |= FileSystemMetaData::MetaDataFlag::LinkType;
      exists = implPtr->doStat(queryFlags);
   }
   
   if (!exists && !implPtr->m_metaData.isLink()) {
      return ret;
   }
   if (exists && (type & FileFlag::PermsMask)) {
      ret |= FileFlags(uint(implPtr->m_metaData.permissions()));
   }
   if (type & FileFlag::TypesMask) {
      if (implPtr->m_metaData.isAlias()) {
         ret |= FileFlag::LinkType;
      } else {
         if ((type & FileFlag::LinkType) && implPtr->m_metaData.isLink()) {
            ret |= FileFlag::LinkType;
         }
         if (exists) {
            if (implPtr->m_metaData.isFile()) {
               ret |= FileFlag::FileType;
            } else if (implPtr->m_metaData.isDirectory()) {
               ret |= FileFlag::DirectoryType;
               if ((type & FileFlag::BundleType) && implPtr->m_metaData.isBundle()) {
                  ret |= FileFlag::BundleType;
               }
            }
         }
      }
   }
   
   if (type & FileFlag::FlagsMask) {
      // the inode existing does not mean the file exists
      if (!implPtr->m_metaData.wasDeleted()) {
         ret |= FileFlag::ExistsFlag;
      }
      if (implPtr->m_fileEntry.isRoot()) {
         ret |= FileFlag::RootFlag;
      }
      else if (implPtr->m_metaData.isHidden()) {
         ret |= FileFlag::HiddenFlag;
      }
   }
   return ret;
}

ByteArray FileEngine::getId() const
{
   PDK_D(const FileEngine);
   if (implPtr->m_fd != -1) {
      return FileSystemEngine::getId(implPtr->m_fd);
   }
   return FileSystemEngine::getId(implPtr->m_fileEntry);
}

String FileEngine::getFileName(FileName file) const
{
   PDK_D(const FileEngine);
//   if (file == FileName::BundleName) {
//      return FileSystemEngine::bundleName(implPtr->m_fileEntry);
//   } else
   if (file == FileName::BaseName) {
      return implPtr->m_fileEntry.getFileName();
   } else if (file == FileName::PathName) {
      return implPtr->m_fileEntry.getPath();
   } else if (file == FileName::AbsoluteName || file == FileName::AbsolutePathName) {
      FileSystemEntry entry(FileSystemEngine::getAbsoluteName(implPtr->m_fileEntry));
      if (file == FileName::AbsolutePathName) {
         return entry.getPath();
      }
      return entry.getFilePath();
   } else if (file == FileName::CanonicalName || file == FileName::CanonicalPathName) {
      FileSystemEntry entry(FileSystemEngine::getCanonicalName(implPtr->m_fileEntry, implPtr->m_metaData));
      if (file == FileName::CanonicalPathName) {
         return entry.getPath();
      }
      return entry.getFilePath();
   } else if (file == FileName::LinkName) {
      if (implPtr->isSymlink()) {
         FileSystemEntry entry = FileSystemEngine::getLinkTarget(implPtr->m_fileEntry, implPtr->m_metaData);
         return entry.getFilePath();
      }
      return String();
   }
   return implPtr->m_fileEntry.getFilePath();
}

bool FileEngine::isRelativePath() const
{
   PDK_D(const FileEngine);
   return implPtr->m_fileEntry.getFilePath().length() 
         ? implPtr->m_fileEntry.getFilePath().at(0) != Latin1Character('/') : true;
}

uint FileEngine::getOwnerId(FileOwner own) const
{
   PDK_D(const FileEngine);
   static const uint nobodyID = (uint) -2;
   if (implPtr->doStat(FileSystemMetaData::MetaDataFlag::OwnerIds)) {
      return implPtr->m_metaData.getOwnerId(own);
   }
   return nobodyID;
}

String FileEngine::getOwner(FileOwner own) const
{
   if (own == FileOwner::OwnerUser) {
      return FileSystemEngine::resolveUserName(getOwnerId(own));
   }
   return FileSystemEngine::resolveGroupName(getOwnerId(own));
}

bool FileEngine::setPermissions(uint perms)
{
   PDK_D(FileEngine);
   SystemError error;
   bool ok;
   if (implPtr->m_fd != -1) {
      ok = FileSystemEngine::setPermissions(implPtr->m_fd, File::Permissions(perms), error, 0);
   } else {
      ok = FileSystemEngine::setPermissions(implPtr->m_fileEntry, File::Permissions(perms), error, 0);
   } 
   if (!ok) {
      setError(File::FileError::PermissionsError, error.toString());
      return false;
   }
   return true;
}

bool FileEngine::setSize(pdk::pint64 size)
{
   PDK_D(FileEngine);
   bool ret = false;
   if (implPtr->m_fd != -1) {
      ret = PDK_FTRUNCATE(implPtr->m_fd, size) == 0;
   } else if (implPtr->m_fh) {
      ret = PDK_FTRUNCATE(PDK_FILENO(implPtr->m_fh), size) == 0;
   } else {
      ret = PDK_TRUNCATE(implPtr->m_fileEntry.getNativeFilePath().getConstRawData(), size) == 0;
   }
   if (!ret) {
      setError(File::FileError::ResizeError, pdk_error_string(errno));
   }
   return ret;
}

bool FileEngine::setFileTime(const DateTime &newDate, FileTime time)
{
   PDK_D(FileEngine);
   
   if (implPtr->m_openMode == IoDevice::OpenMode::NotOpen) {
      setError(File::FileError::PermissionsError, pdk_error_string(EACCES));
      return false;
   }
   
   SystemError error;
   if (!FileSystemEngine::setFileTime(implPtr->getNativeHandle(), newDate, time, error)) {
      setError(File::FileError::PermissionsError, error.toString());
      return false;
   }
   implPtr->m_metaData.clearFlags(FileSystemMetaData::MetaDataFlag::Times);
   return true;
}

uchar *FileEnginePrivate::map(pdk::pint64 offset, pdk::pint64 size, File::MemoryMapFlags flags)
{
#if defined(PDK_OS_LINUX) && PDK_PROCESSOR_WORDSIZE == 4
   // The Linux mmap2 system call on 32-bit takes a page-shifted 32-bit
   // integer so the maximum offset is 1 << (32+12) (the shift is always 12,
   // regardless of the actual page size). Unfortunately, the mmap64()
   // function is known to be broken in all Linux libcs (glibc, uclibc, musl
   // and Bionic): all of them do the right shift, but don't confirm that the
   // result fits into the 32-bit parameter to the kernel.
   
   static pdk::pint64 MaxFileOffset = (PDK_INT64_C(1) << (32+12)) - 1;
#else
   static pdk::pint64 MaxFileOffset = std::numeric_limits<PDK_OFF_T>::max();
#endif
   
   PDK_Q(FileEngine);
   if (m_openMode == IoDevice::OpenMode::NotOpen) {
      apiPtr->setError(File::FileError::PermissionsError, pdk_error_string(int(EACCES)));
      return 0;
   }
   
   if (offset < 0 || offset > MaxFileOffset
       || size < 0 || pdk::puint64(size) > pdk::puint64(size_t(-1))) {
      apiPtr->setError(File::FileError::UnspecifiedError, pdk_error_string(int(EINVAL)));
      return 0;
   }
   
   // If we know the mapping will extend beyond EOF, fail early to avoid
   // undefined behavior. Otherwise, let mmap have its say.
   if (doStat(FileSystemMetaData::MetaDataFlag::SizeAttribute)
       && (PDK_OFF_T(size) > m_metaData.getSize() - PDK_OFF_T(offset))) {
      // warning_stream("FileEngine::map: Mapping a file beyond its size is not portable");
   }
   
   int access = 0;
   if (m_openMode & IoDevice::OpenMode::ReadOnly) {
      access |= PROT_READ;
   }
   if (m_openMode & IoDevice::OpenMode::WriteOnly) {
      access |= PROT_WRITE;
   }
   
   int sharemode = MAP_SHARED;
   if (flags & FileDevice::MemoryMapFlag::MapPrivateOption) {
      sharemode = MAP_PRIVATE;
      access |= PROT_WRITE;
   }
   
#if defined(PDK_OS_INTEGRITY)
   int pageSize = sysconf(_SC_PAGESIZE);
#else
   int pageSize = getpagesize();
#endif
   int extra = offset % pageSize;
   
   if (pdk::puint64(size + extra) > pdk::puint64((size_t)-1)) {
      apiPtr->setError(File::FileError::UnspecifiedError, pdk_error_string(int(EINVAL)));
      return 0;
   }
   
   size_t realSize = (size_t)size + extra;
   PDK_OFF_T realOffset = PDK_OFF_T(offset);
   realOffset &= ~(PDK_OFF_T(pageSize - 1));
   
   void *mapAddress = PDK_MMAP(nullptr, realSize, access, sharemode, getNativeHandle(), realOffset);
   if (MAP_FAILED != mapAddress) {
      uchar *address = extra + static_cast<uchar*>(mapAddress);
      m_maps[address] = std::pair<int, size_t>(extra, realSize);
      return address;
   }
   
   switch(errno) {
   case EBADF:
      apiPtr->setError(File::FileError::PermissionsError, pdk_error_string(int(EACCES)));
      break;
   case ENFILE:
   case ENOMEM:
      apiPtr->setError(File::FileError::ResourceError, pdk_error_string(int(errno)));
      break;
   case EINVAL:
      // size are out of bounds
   default:
      apiPtr->setError(File::FileError::UnspecifiedError, pdk_error_string(int(errno)));
      break;
   }
   return 0;
}

bool FileEnginePrivate::unmap(uchar *ptr)
{
#if !defined(PDK_OS_INTEGRITY)
   PDK_Q(FileEngine);
   if (m_maps.find(ptr) != m_maps.end()) {
      apiPtr->setError(File::FileError::PermissionsError, pdk_error_string(EACCES));
      return false;
   }
   
   uchar *start = ptr - m_maps[ptr].first;
   size_t len = m_maps[ptr].second;
   if (-1 == munmap(start, len)) {
      apiPtr->setError(File::FileError::UnspecifiedError, pdk_error_string(errno));
      return false;
   }
   m_maps.erase(ptr);
   return true;
#else
   return false;
#endif
}

bool FileEngine::cloneTo(AbstractFileEngine *target)
{
   PDK_D(FileEngine);
   if ((target->getFileFlags(FileFlag::LocalDiskFlag) & FileFlag::LocalDiskFlag) == 0) {
      return false;
   }
   int srcfd = implPtr->getNativeHandle();
   int dstfd = target->getHandle();
   return FileSystemEngine::cloneFile(srcfd, dstfd, implPtr->m_metaData);
}

} // internal
} // fs
} // io
} // pdk

#endif // PDK_NO_FSFILEENGINE
