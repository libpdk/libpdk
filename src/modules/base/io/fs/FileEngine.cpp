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

#ifndef PDK_NO_FSFILEENGINE

#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileEngineIteratorPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/io/fs/DirIterator.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/lang/String.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"

#include <set>

#include <errno.h>
#if defined(PDK_OS_UNIX)
#include "pdk/kernel/CoreUnix.h"
#endif

#include <cstdio>
#include <cstdlib>
#if defined(PDK_OS_MAC)
# include "pdk/kernel/internal/CoreMacPrivate.h"
#endif

#ifdef PDK_OS_WIN
#  ifndef S_ISREG
#    define S_ISREG(x)   (((x) & S_IFMT) == S_IFREG)
#  endif
#  ifndef S_ISCHR
#    define S_ISCHR(x)   (((x) & S_IFMT) == S_IFCHR)
#  endif
#  ifndef S_ISFIFO
#    define S_ISFIFO(x) false
#  endif
#  ifndef S_ISSOCK
#    define S_ISSOCK(x) false
#  endif
#  ifndef INVALID_FILE_ATTRIBUTES
#    define INVALID_FILE_ATTRIBUTES (DWORD (-1))
#  endif
#endif

#ifdef PDK_OS_WIN
// on Windows, read() and write() use int and unsigned int
using SignedIOType = int;
using UnsignedIOType = unsigned int;
#else
using SignedIOType = ssize_t;
using UnsignedIOType = size_t;
PDK_STATIC_ASSERT_X(sizeof(SignedIOType) == sizeof(UnsignedIOType),
                    "Unsupported: read/write return a type with different size as the len parameter");
#endif

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::lang::Latin1String;
using pdk::kernel::internal::SystemError;

FileEnginePrivate::FileEnginePrivate() : AbstractFileEnginePrivate()
{
   init();
}

void FileEnginePrivate::init()
{
   m_isSequential = 0;
   m_triedStat = 0;
   m_needLstat = 1;
   m_isLink = 0;
   m_openMode = IoDevice::OpenMode::NotOpen;
   m_fd = -1;
   m_fh = 0;
   m_lastIOCommand = LastIOCommand::IOFlushCommand;
   m_lastFlushFailed = false;
   m_closeFileHandle = false;
#ifdef PDK_OS_WIN
   m_fileAttrib = INVALID_FILE_ATTRIBUTES;
   m_fileHandle = INVALID_HANDLE_VALUE;
   m_mapHandle = NULL;
   m_cachedFd = -1;
#endif
}

FileEngine::FileEngine(const String &file)
   : AbstractFileEngine(*new FileEnginePrivate)
{
   PDK_D(FileEngine);
   implPtr->m_fileEntry = FileSystemEntry(file);
}

FileEngine::FileEngine() : AbstractFileEngine(*new FileEnginePrivate)
{
}

FileEngine::FileEngine(FileEnginePrivate &dd)
   : AbstractFileEngine(dd)
{
}

FileEngine::~FileEngine()
{
   PDK_D(FileEngine);
   if (implPtr->m_closeFileHandle) {
      if (implPtr->m_fh) {
         fclose(implPtr->m_fh);
      } else if (implPtr->m_fd != -1) {
         PDK_CLOSE(implPtr->m_fd);
      }
   }
   implPtr->unmapAll();
}

void FileEngine::setFileName(const String &file)
{
   PDK_D(FileEngine);
   implPtr->init();
   implPtr->m_fileEntry = FileSystemEntry(file);
}

bool FileEngine::open(IoDevice::OpenModes openMode)
{
   PDK_ASSERT_X(openMode & IoDevice::OpenMode::Unbuffered, "FileEngine::open",
                "FileEngine no longer supports buffered mode; upper layer must buffer");
   
   PDK_D(FileEngine);
   if (implPtr->m_fileEntry.isEmpty()) {
      // qWarning("FileEngine::open: No file name specified");
      setError(File::FileError::OpenError, Latin1String("No file name specified"));
      return false;
   }
   
   // Append implies WriteOnly.
   if (openMode & File::OpenMode::Append) {
      openMode |= File::OpenMode::WriteOnly;
   }
   // WriteOnly implies Truncate if neither ReadOnly nor Append are sent.
   if ((openMode & File::OpenMode::WriteOnly) && !(openMode & (File::OpenMode::ReadOnly | File::OpenMode::Append))) {
      openMode |= File::OpenMode::Truncate;
   }
   implPtr->m_openMode = openMode;
   implPtr->m_lastFlushFailed = false;
   implPtr->m_triedStat = 0;
   implPtr->m_fh = 0;
   implPtr->m_fd = -1;
   
   return implPtr->nativeOpen(openMode);
}

bool FileEngine::open(IoDevice::OpenModes openMode, FILE *fh)
{
   return open(openMode, fh, File::FileHandleFlag::DontCloseHandle);
}

bool FileEngine::open(IoDevice::OpenModes openMode, FILE *fh, File::FileHandleFlags handleFlags)
{
   PDK_ASSERT_X(openMode & IoDevice::OpenMode::Unbuffered, "FileEngine::open",
                "FileEngine no longer supports buffered mode; upper layer must buffer");
   
   PDK_D(FileEngine);
   
   // Append implies WriteOnly.
   if (openMode & File::OpenMode::Append) {
      openMode |= File::OpenMode::WriteOnly;
   }
   
   // WriteOnly implies Truncate if neither ReadOnly nor Append are sent.
   if ((openMode & File::OpenMode::WriteOnly) && !(openMode & (File::OpenMode::ReadOnly | File::OpenMode::Append))) {
      openMode |= File::OpenMode::Truncate;
   }
   implPtr->m_openMode = openMode;
   implPtr->m_lastFlushFailed = false;
   implPtr->m_closeFileHandle = (handleFlags & File::FileHandleFlag::AutoCloseHandle);
   implPtr->m_fileEntry.clear();
   implPtr->m_triedStat = 0;
   implPtr->m_fd = -1;
   
   return implPtr->openFh(openMode, fh);
}

bool FileEnginePrivate::openFh(IoDevice::OpenModes openMode, FILE *fh)
{
   PDK_ASSERT_X(openMode & IoDevice::OpenMode::Unbuffered, "FileEngine::open",
                "FileEngine no longer supports buffered mode; upper layer must buffer");
   
   PDK_Q(FileEngine);
   m_fh = fh;
   m_fd = -1;
   
   // Seek to the end when in Append mode.
   if (openMode & IoDevice::OpenMode::Append) {
      int ret;
      do {
         ret = PDK_FSEEK(fh, 0, SEEK_END);
      } while (ret != 0 && errno == EINTR);
      
      if (ret != 0) {
         apiPtr->setError(errno == EMFILE ? File::FileError::ResourceError : File::FileError::OpenError,
                          SystemError::getStdString());
         
         m_openMode = IoDevice::OpenMode::NotOpen;
         m_fh = 0;         
         return false;
      }
   }
   
   return true;
}

bool FileEngine::open(IoDevice::OpenModes openMode, int fd)
{
   return open(openMode, fd, File::FileHandleFlag::DontCloseHandle);
}

bool FileEngine::open(IoDevice::OpenModes openMode, int fd, File::FileHandleFlags handleFlags)
{
   PDK_D(FileEngine);
   
   // Append implies WriteOnly.
   if (openMode & File::OpenMode::Append)
      openMode |= File::OpenMode::WriteOnly;
   
   // WriteOnly implies Truncate if neither ReadOnly nor Append are sent.
   if ((openMode & File::OpenMode::WriteOnly) && !(openMode & (File::OpenMode::ReadOnly | File::OpenMode::Append)))
      openMode |= File::OpenMode::Truncate;
   
   implPtr->m_openMode = openMode;
   implPtr->m_lastFlushFailed = false;
   implPtr->m_closeFileHandle = (handleFlags & File::FileHandleFlag::AutoCloseHandle);
   implPtr->m_fileEntry.clear();
   implPtr->m_fh = 0;
   implPtr->m_fd = -1;
   implPtr->m_triedStat = 0;
   
   return implPtr->openFd(openMode, fd);
}

bool FileEnginePrivate::openFd(IoDevice::OpenModes openMode, int fd)
{
   PDK_Q(FileEngine);
   m_fd = fd;
   m_fh = 0;
   
   // Seek to the end when in Append mode.
   if (openMode & File::OpenMode::Append) {
      int ret;
      do {
         ret = PDK_LSEEK(fd, 0, SEEK_END);
      } while (ret == -1 && errno == EINTR);
      
      if (ret == -1) {
         apiPtr->setError(errno == EMFILE ? File::FileError::ResourceError : File::FileError::OpenError,
                          SystemError::getStdString());
         
         m_openMode = IoDevice::OpenMode::NotOpen;
         m_fd = -1;
         return false;
      }
   }
   
   return true;
}

bool FileEngine::close()
{
   PDK_D(FileEngine);
   implPtr->m_openMode = IoDevice::OpenMode::NotOpen;
   return implPtr->nativeClose();
}

bool FileEnginePrivate::closeFdFh()
{
   PDK_Q(FileEngine);
   if (m_fd == -1 && !m_fh) {
      return false;
   }
   
   // Flush the file if it's buffered, and if the last flush didn't fail.
   bool flushed = !m_fh || (!m_lastFlushFailed && apiPtr->flush());
   bool closed = true;
   m_triedStat = 0;
   
   // Close the file if we created the handle.
   if (m_closeFileHandle) {
      int ret;
      
      if (m_fh) {
         // Close buffered file.
         ret = fclose(m_fh);
      } else {
         // Close unbuffered file.
         ret = PDK_CLOSE(m_fd);
      }
      
      // We must reset these guys regardless; calling close again after a
      // failed close causes crashes on some systems.
      m_fh = 0;
      m_fd = -1;
      closed = (ret == 0);
   }
   
   // Report errors.
   if (!flushed || !closed) {
      if (flushed) {
         // If not flushed, we want the flush error to fall through.
         apiPtr->setError(File::FileError::UnspecifiedError, SystemError::getStdString());
      }
      return false;
   }
   
   return true;
}

bool FileEngine::flush()
{
   PDK_D(FileEngine);
   if ((implPtr->m_openMode & IoDevice::OpenMode::WriteOnly) == 0) {
      // Nothing in the write buffers, so flush succeeds in doing
      // nothing.
      return true;
   }
   return implPtr->nativeFlush();
}

bool FileEngine::syncToDisk()
{
   PDK_D(FileEngine);
   if ((implPtr->m_openMode & IoDevice::OpenMode::WriteOnly) == 0) {
      return true;
   }
   
   return implPtr->nativeSyncToDisk();
}

bool FileEnginePrivate::flushFh()
{
   PDK_Q(FileEngine);
   
   // Never try to flush again if the last flush failed. Otherwise you can
   // get crashes on some systems (AIX).
   if (m_lastFlushFailed) {
      return false;
   }
   int ret = fflush(m_fh);
   m_lastFlushFailed = (ret != 0);
   m_lastIOCommand = FileEnginePrivate::LastIOCommand::IOFlushCommand;
   
   if (ret != 0) {
      apiPtr->setError(errno == ENOSPC ? File::FileError::ResourceError : File::FileError::WriteError,
                       SystemError::getStdString());
      return false;
   }
   return true;
}

pdk::pint64 FileEngine::getSize() const
{
   PDK_D(const FileEngine);
   return implPtr->nativeSize();
}

void FileEnginePrivate::unmapAll()
{
   if (!m_maps.empty()) {
      // Make a copy since unmap() modifies the map.
      std::list<uchar *> keys;
      auto iter = m_maps.begin();
      auto end = m_maps.end();
      while (iter != end) {
         keys.push_back(iter->first);
         ++iter;
      }
      for (auto &iter : keys) {
         unmap(iter);
      }
   }
}

#ifndef PDK_OS_WIN

pdk::pint64 FileEnginePrivate::sizeFdFh() const
{
   PDK_Q(const FileEngine);
   const_cast<FileEngine *>(apiPtr)->flush();
   m_triedStat = 0;
   m_metaData.clearFlags(FileSystemMetaData::MetaDataFlag::SizeAttribute);
   if (!doStat(FileSystemMetaData::MetaDataFlag::SizeAttribute)) {
      return 0;
   }
   return m_metaData.getSize();
}
#endif

pdk::pint64 FileEngine::getPosition() const
{
   PDK_D(const FileEngine);
   return implPtr->getNativePos();
}

pdk::pint64 FileEnginePrivate::getPosFdFh() const
{
   if (m_fh) {
      return pdk::pint64(PDK_FTELL(m_fh));
   }
   
   return PDK_LSEEK(m_fd, 0, SEEK_CUR);
}

bool FileEngine::seek(pdk::pint64 pos)
{
   PDK_D(FileEngine);
   return implPtr->nativeSeek(pos);
}

DateTime FileEngine::fileTime(FileTime time) const
{
   PDK_D(const FileEngine);
   
   if (time == FileTime::AccessTime) {
      // always refresh for the access time
      implPtr->m_metaData.clearFlags(FileSystemMetaData::MetaDataFlag::AccessTime);
   }
   
   if (implPtr->doStat(FileSystemMetaData::MetaDataFlag::Times)) {
      return implPtr->m_metaData.fileTime(time);
   }
   return DateTime();
}

bool FileEnginePrivate::seekFdFh(pdk::pint64 pos)
{
   PDK_Q(FileEngine);
   
   // On Windows' stdlib implementation, the results of calling fread and
   // fwrite are undefined if not called either in sequence, or if preceded
   // with a call to fflush().
   if (m_lastIOCommand != LastIOCommand::IOFlushCommand && !apiPtr->flush())
      return false;
   
   if (pos < 0 || pos != pdk::pint64(PDK_OFF_T(pos))) {
      return false;
   }
   
   if (m_fh) {
      // Buffered stdlib mode.
      int ret;
      do {
         ret = PDK_FSEEK(m_fh, PDK_OFF_T(pos), SEEK_SET);
      } while (ret != 0 && errno == EINTR);
      
      if (ret != 0) {
         apiPtr->setError(File::FileError::ReadError, SystemError::getStdString());
         return false;
      }
   } else {
      // Unbuffered stdio mode.
      if (PDK_LSEEK(m_fd, PDK_OFF_T(pos), SEEK_SET) == -1) {
         // qWarning("File::at: Cannot set file position %lld", pos);
         apiPtr->setError(File::FileError::PositionError, SystemError::getStdString());
         return false;
      }
   }
   return true;
}

int FileEngine::handle() const
{
   PDK_D(const FileEngine);
   return implPtr->getNativeHandle();
}

pdk::pint64 FileEngine::read(char *data, pdk::pint64 maxlen)
{
   PDK_D(FileEngine);
   
   // On Windows' stdlib implementation, the results of calling fread and
   // fwrite are undefined if not called either in sequence, or if preceded
   // with a call to fflush().
   if (implPtr->m_lastIOCommand != FileEnginePrivate::LastIOCommand::IOReadCommand) {
      flush();
      implPtr->m_lastIOCommand = FileEnginePrivate::LastIOCommand::IOReadCommand;
   }
   
   return implPtr->nativeRead(data, maxlen);
}

pdk::pint64 FileEnginePrivate::readFdFh(char *data, pdk::pint64 len)
{
   PDK_Q(FileEngine);
   
   if (len < 0 || len != pdk::pint64(size_t(len))) {
      apiPtr->setError(File::FileError::ReadError, SystemError::getStdString(EINVAL));
      return -1;
   }
   
   pdk::pint64 readBytes = 0;
   bool eof = false;
   
   if (m_fh) {
      // Buffered stdlib mode.
      
      size_t result;
      bool retry = true;
      do {
         result = fread(data + readBytes, 1, size_t(len - readBytes), m_fh);
         eof = feof(m_fh);
         if (retry && eof && result == 0) {
            // On OS X, this is needed, e.g., if a file was written to
            // through another stream since our last read. See test
            // tst_File::appendAndRead
            PDK_FSEEK(m_fh, PDK_FTELL(m_fh), SEEK_SET); // re-sync stream.
            retry = false;
            continue;
         }
         readBytes += result;
      } while (!eof && (result == 0 ? errno == EINTR : readBytes < len));
      
   } else if (m_fd != -1) {
      // Unbuffered stdio mode.
      
      SignedIOType result;
      do {
         // calculate the chunk size
         // on Windows or 32-bit no-largefile Unix, we'll need to read in chunks
         // we limit to the size of the signed type, otherwise we could get a negative number as a result
         pdk::puint64 wantedBytes = pdk::puint64(len) - pdk::puint64(readBytes);
         UnsignedIOType chunkSize = std::numeric_limits<SignedIOType>::max();
         if (chunkSize > wantedBytes) {
            chunkSize = wantedBytes;
         }
         result = PDK_READ(m_fd, data + readBytes, chunkSize);
      } while (result > 0 && (readBytes += result) < len);
      eof = !(result == -1);
   }
   
   if (!eof && readBytes == 0) {
      readBytes = -1;
      apiPtr->setError(File::FileError::ReadError, SystemError::getStdString());
   }
   
   return readBytes;
}

pdk::pint64 FileEngine::readLine(char *data, pdk::pint64 maxlen)
{
   PDK_D(FileEngine);
   
   // On Windows' stdlib implementation, the results of calling fread and
   // fwrite are undefined if not called either in sequence, or if preceded
   // with a call to fflush().
   if (implPtr->m_lastIOCommand != FileEnginePrivate::LastIOCommand::IOReadCommand) {
      flush();
      implPtr->m_lastIOCommand = FileEnginePrivate::LastIOCommand::IOReadCommand;
   }
   return implPtr->nativeReadLine(data, maxlen);
}

pdk::pint64 FileEnginePrivate::readLineFdFh(char *data, pdk::pint64 maxlen)
{
   PDK_Q(FileEngine);
   if (!m_fh) {
      return apiPtr->AbstractFileEngine::readLine(data, maxlen);
   }
   PDK_OFF_T oldPos = 0;
#ifdef PDK_OS_WIN
   bool seq = apiPtr->isSequential();
   if (!seq)
#endif
      oldPos = PDK_FTELL(m_fh);
   
   // IoDevice::readLine() passes maxlen - 1 to File::readLineData()
   // because it has made space for the '\0' at the end of data.  But fgets
   // does the same, so we'd get two '\0' at the end - passing maxlen + 1
   // solves this.
   if (!fgets(data, int(maxlen + 1), m_fh)) {
      if (!feof(m_fh)) {
         apiPtr->setError(File::FileError::ReadError, SystemError::getStdString());
      }  
      return -1;              // error
   }
   
#ifdef PDK_OS_WIN
   if (seq) {
      return pdk::strlen(data);
   }
#endif
   
   pdk::pint64 lineLength = PDK_FTELL(m_fh) - oldPos;
   return lineLength > 0 ? lineLength : pdk::strlen(data);
}

pdk::pint64 FileEngine::write(const char *data, pdk::pint64 len)
{
   PDK_D(FileEngine);
   implPtr->m_metaData.clearFlags(FileSystemMetaData::MetaDataFlag::Times);
   
   // On Windows' stdlib implementation, the results of calling fread and
   // fwrite are undefined if not called either in sequence, or if preceded
   // with a call to fflush().
   if (implPtr->m_lastIOCommand != FileEnginePrivate::LastIOCommand::IOWriteCommand) {
      flush();
      implPtr->m_lastIOCommand = FileEnginePrivate::LastIOCommand::IOWriteCommand;
   }
   
   return implPtr->nativeWrite(data, len);
}

pdk::pint64 FileEnginePrivate::writeFdFh(const char *data, pdk::pint64 len)
{
   PDK_Q(FileEngine);
   
   if (len < 0 || len != pdk::pint64(size_t(len))) {
      apiPtr->setError(File::FileError::WriteError, SystemError::getStdString(EINVAL));
      return -1;
   }
   
   pdk::pint64 writtenBytes = 0;
   
   if (len) { // avoid passing nullptr to fwrite() or PDK_WRITE() (UB)
      
      if (m_fh) {
         // Buffered stdlib mode.
         
         size_t result;
         do {
            result = fwrite(data + writtenBytes, 1, size_t(len - writtenBytes), m_fh);
            writtenBytes += result;
         } while (result == 0 ? errno == EINTR : writtenBytes < len);
         
      } else if (m_fd != -1) {
         // Unbuffered stdio mode.
         
         SignedIOType result;
         do {
            // calculate the chunk size
            // on Windows or 32-bit no-largefile Unix, we'll need to read in chunks
            // we limit to the size of the signed type, otherwise we could get a negative number as a result
            pdk::puint64 wantedBytes = pdk::puint64(len) - pdk::puint64(writtenBytes);
            UnsignedIOType chunkSize = std::numeric_limits<SignedIOType>::max();
            if (chunkSize > wantedBytes)
               chunkSize = wantedBytes;
            result = PDK_WRITE(m_fd, data + writtenBytes, chunkSize);
         } while (result > 0 && (writtenBytes += result) < len);
      }
      
   }
   
   if (len &&  writtenBytes == 0) {
      writtenBytes = -1;
      apiPtr->setError(errno == ENOSPC ? File::FileError::ResourceError : File::FileError::WriteError, 
                       SystemError::getStdString());
   } else {
      // reset the cached size, if any
      m_metaData.clearFlags(FileSystemMetaData::MetaDataFlag::SizeAttribute);
   }
   
   return writtenBytes;
}

#ifndef PDK_NO_FILESYSTEMITERATOR

AbstractFileEngine::Iterator *FileEngine::beginEntryList(Dir::Filters filters, const StringList &filterNames)
{
   return new FileEngineIterator(filters, filterNames);
}

AbstractFileEngine::Iterator *FileEngine::endEntryList()
{
   return 0;
}

#endif // PDK_NO_FILESYSTEMITERATOR

StringList FileEngine::entryList(Dir::Filters filters, const StringList &filterNames) const
{
   return AbstractFileEngine::entryList(filters, filterNames);
}

bool FileEngine::isSequential() const
{
   PDK_D(const FileEngine);
   if (implPtr->m_isSequential == 0) {
      implPtr->m_isSequential = implPtr->getNativeIsSequential() ? 1 : 2;
   }
   return implPtr->m_isSequential == 1;
}

#ifdef PDK_OS_UNIX
bool FileEnginePrivate::isSequentialFdFh() const
{
   if (doStat(FileSystemMetaData::MetaDataFlag::SequentialType)) {
      return m_metaData.isSequential();
   }
   return true;
}
#endif

bool FileEngine::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
   PDK_D(FileEngine);
   if (extension == AtEndExtension && implPtr->m_fh && isSequential()) {
      return feof(implPtr->m_fh);
   }
   if (extension == MapExtension) {
      const MapExtensionOption *options = (const MapExtensionOption*)(option);
      MapExtensionReturn *returnValue = static_cast<MapExtensionReturn*>(output);
      returnValue->address = implPtr->map(options->offset, options->size, options->flags);
      return (returnValue->address != 0);
   }
   if (extension == UnMapExtension) {
      const UnMapExtensionOption *options = (const UnMapExtensionOption*)option;
      return implPtr->unmap(options->address);
   }
   
   return false;
}

bool FileEngine::supportsExtension(Extension extension) const
{
   PDK_D(const FileEngine);
   if (extension == AtEndExtension && implPtr->m_fh && isSequential()) {
      return true;
   }
   if (extension == FastReadLineExtension && implPtr->m_fh){
      return true;
   }  
   if (extension == FastReadLineExtension && implPtr->m_fd != -1 && isSequential()) {
      return true;
   }
   if (extension == UnMapExtension || extension == MapExtension) {
      return true;
   }
   return false;
}

} // internal
} // fs
} // io
} // pdk

#endif // PDK_NO_FSFILEENGINE
