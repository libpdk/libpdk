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
#include "pdk/base/io/fs/FileDevice.h"
#include "pdk/base/io/fs/internal/FileDevicePrivate.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"

#ifndef PDK_FILE_WRITEBUFFER_SIZE
#define PDK_FILE_WRITEBUFFER_SIZE 16384
#endif

namespace pdk {
namespace io {
namespace fs {

using pdk::io::fs::internal::AbstractFileEngine;

namespace internal {

using pdk::error_string;

FileDevicePrivate::FileDevicePrivate()
   : m_fileEngine(nullptr),
     m_cachedSize(0),
     m_error(File::FileError::NoError), 
     m_lastWasWrite(false)
{
   m_writeBufferChunkSize = PDK_FILE_WRITEBUFFER_SIZE;
}

FileDevicePrivate::~FileDevicePrivate()
{
   delete m_fileEngine;
   m_fileEngine = nullptr;
}

AbstractFileEngine *FileDevicePrivate::getEngine() const
{
   if (!m_fileEngine) {
      m_fileEngine = new FileEngine;
   }
   return m_fileEngine;
}

void FileDevicePrivate::setError(FileDevice::FileError err)
{
   m_error = err;
   m_errorString.clear();
}

void FileDevicePrivate::setError(FileDevice::FileError err, const String &errStr)
{
   m_error = err;
   m_errorString = errStr;
}

void FileDevicePrivate::setError(FileDevice::FileError err, int errNum)
{
   m_error = err;
   m_errorString = pdk::error_string(errNum);
}

} // internal


FileDevice::FileDevice()
   : IoDevice(*new FileDevicePrivate, nullptr)
{
}

FileDevice::FileDevice(Object *parent)
   : IoDevice(*new FileDevicePrivate, parent)
{
}

FileDevice::FileDevice(FileDevicePrivate &dd, Object *parent)
   : IoDevice(dd, parent)
{
}

FileDevice::~FileDevice()
{
   close();
}

bool FileDevice::isSequential() const
{
   PDK_D(const FileDevice);
   return implPtr->m_fileEngine && implPtr->m_fileEngine->isSequential();
}

int FileDevice::getHandle() const
{
   PDK_D(const FileDevice);
   if (!isOpen() || !implPtr->m_fileEngine) {
      return -1;
   }
   return implPtr->m_fileEngine->getHandle();
}

String FileDevice::getFileName() const
{
   return String();
}

bool FileDevice::flush()
{
   PDK_D(FileDevice);
   if (!implPtr->m_fileEngine) {
      // warning_stream("FileDevice::flush: No file engine. Is IODevice open?");
      return false;
   }
   if (!implPtr->m_writeBuffer.isEmpty()) {
      pdk::pint64 size = implPtr->m_writeBuffer.nextDataBlockSize();
      pdk::pint64 written = implPtr->m_fileEngine->write(implPtr->m_writeBuffer.readPointer(), size);
      if (written > 0) {
         implPtr->m_writeBuffer.free(written);
      }
      if (written != size) {
         FileDevice::FileError err = implPtr->m_fileEngine->getError();
         if (err == FileDevice::FileError::UnspecifiedError) {
            err = FileDevice::FileError::WriteError;
         }
         implPtr->setError(err, implPtr->m_fileEngine->getErrorString());
         return false;
      }
   }
   
   if (!implPtr->m_fileEngine->flush()) {
      FileDevice::FileError err = implPtr->m_fileEngine->getError();
      if (err == FileDevice::FileError::UnspecifiedError) {
         err = FileDevice::FileError::WriteError;
      }
      implPtr->setError(err, implPtr->m_fileEngine->getErrorString());
      return false;
   }
   return true;
}

void FileDevice::close()
{
   PDK_D(FileDevice);
   if (!isOpen()) {
      return;
   }
   bool flushed = flush();
   IoDevice::close();
   
   // reset write buffer
   implPtr->m_lastWasWrite = false;
   implPtr->m_writeBuffer.clear();
   // reset cached size
   implPtr->m_cachedSize = 0;
   // keep earlier error from flush
   if (implPtr->m_fileEngine->close() && flushed) {
      unsetError();
   } else if (flushed) {
      implPtr->setError(implPtr->m_fileEngine->getError(), implPtr->m_fileEngine->getErrorString());
   }
}

pdk::pint64 FileDevice::getPosition() const
{
   return IoDevice::getPosition();
}

bool FileDevice::atEnd() const
{
   PDK_D(const FileDevice);
   
   // If there's buffered data left, we're not at the end.
   if (!implPtr->isBufferEmpty()) {
      return false;
   }
   if (!isOpen()) {
      return true;
   }
   
   if (!implPtr->ensureFlushed()) {
      return false;
   }
   
   // If the file engine knows best, say what it says.
   if (implPtr->m_fileEngine->supportsExtension(AbstractFileEngine::AtEndExtension)) {
      // Check if the file engine supports AtEndExtension, and if it does,
      // check if the file engine claims to be at the end.
      return implPtr->m_fileEngine->atEnd();
   }
   
   // if it looks like we are at the end, or if size is not cached,
   // fall through to bytesAvailable() to make sure.
   if (getPosition() < implPtr->m_cachedSize) {
      return false;
   }
   // Fall back to checking how much is available (will stat files).
   return bytesAvailable() == 0;
}

bool FileDevice::seek(pdk::pint64 off)
{
   PDK_D(FileDevice);
   if (!isOpen()) {
      // warning_stream("FileDevice::seek: IODevice is not open");
      return false;
   }
   if (!implPtr->ensureFlushed()) {
      return false;
   }
   if (!implPtr->m_fileEngine->seek(off) || !IoDevice::seek(off)) {
      FileDevice::FileError err = implPtr->m_fileEngine->getError();
      if (err == FileDevice::FileError::UnspecifiedError) {
         err = FileDevice::FileError::PositionError;
      }
      implPtr->setError(err, implPtr->m_fileEngine->getErrorString());
      return false;
   }
   unsetError();
   return true;
}

pdk::pint64 FileDevice::readLineData(char *data, pdk::pint64 maxlen)
{
   PDK_D(FileDevice);
   if (!implPtr->ensureFlushed()) {
      return -1;
   }
   
   pdk::pint64 read;
   if (implPtr->m_fileEngine->supportsExtension(AbstractFileEngine::FastReadLineExtension)) {
      read = implPtr->m_fileEngine->readLine(data, maxlen);
   } else {
      // Fall back to IoDevice's readLine implementation if the engine
      // cannot do it faster.
      read = IoDevice::readLineData(data, maxlen);
   }
   if (read < maxlen) {
      // failed to read all requested, may be at the end of file, stop caching size so that it's rechecked
      implPtr->m_cachedSize = 0;
   }
   return read;
}

pdk::pint64 FileDevice::readData(char *data, pdk::pint64 len)
{
   PDK_D(FileDevice);
   if (!len) {
      return 0;
   }
   unsetError();
   if (!implPtr->ensureFlushed()) {
      return -1;
   } 
   
   const pdk::pint64 read = implPtr->m_fileEngine->read(data, len);
   if (read < 0) {
      FileDevice::FileError err = implPtr->m_fileEngine->getError();
      if (err == FileDevice::FileError::UnspecifiedError) {
         err = FileDevice::FileError::ReadError;
      }
      implPtr->setError(err, implPtr->m_fileEngine->getErrorString());
   }
   
   if (read < len) {
      // failed to read all requested, may be at the end of file, stop caching size so that it's rechecked
      implPtr->m_cachedSize = 0;
   }
   
   return read;
}

bool FileDevicePrivate::putCharHelper(char c)
{
   
   // Cutoff for code that doesn't only touch the buffer.
   pdk::pint64 writeBufferSize = m_writeBuffer.size();
   if ((m_openMode & IoDevice::OpenMode::Unbuffered) || writeBufferSize + 1 >= m_writeBufferChunkSize
    #ifdef PDK_OS_WIN
       || ((openMode & IoDevice::OpenMode::Text) && c == '\n'
           && writeBufferSize + 2 >= m_writeBufferChunkSize)
    #endif
       ) {
      return IoDevicePrivate::putCharHelper(c);
   }
   
   if (!(m_openMode & IoDevice::OpenMode::WriteOnly)) {
      if (m_openMode == IoDevice::OpenMode::NotOpen) {
         warning_stream("IoDevice::putChar: Closed device");
      } else {
         warning_stream("IoDevice::putChar: ReadOnly device");
      }
      return false;
   }
   
   // Make sure the device is positioned correctly.
   const bool sequential = isSequential();
   if (m_pos != m_devicePos && !sequential && !getApiPtr()->seek(m_pos)) {
      return false;
   }
   m_lastWasWrite = true;
   int len = 1;
#ifdef PDK_OS_WIN
   if ((m_openMode & IoDevice::OpenMode::Text) && c == '\n') {
      ++len;
      *m_writeBuffer.reserve(1) = '\r';
   }
#endif
   
   // Write to buffer.
   *m_writeBuffer.reserve(1) = c;
   
   if (!sequential) {
      m_pos += len;
      m_devicePos += len;
      if (!m_buffer.isEmpty()) {
         m_buffer.skip(len);
      }
   }
   
   return true;
}

pdk::pint64 FileDevice::writeData(const char *data, pdk::pint64 len)
{
   PDK_D(FileDevice);
   unsetError();
   implPtr->m_lastWasWrite = true;
   bool buffered = !(implPtr->m_openMode & OpenMode::Unbuffered);
   
   // Flush buffered data if this read will overflow.
   if (buffered && (implPtr->m_writeBuffer.size() + len) > implPtr->m_writeBufferChunkSize) {
      if (!flush()) {
         return -1;
      }
   }
   
   // Write directly to the engine if the block size is larger than
   // the write buffer size.
   if (!buffered || len > implPtr->m_writeBufferChunkSize) {
      const pdk::pint64 ret = implPtr->m_fileEngine->write(data, len);
      if (ret < 0) {
         FileDevice::FileError err = implPtr->m_fileEngine->getError();
         if (err == FileDevice::FileError::UnspecifiedError) {
            err = FileDevice::FileError::WriteError;
         }
         implPtr->setError(err, implPtr->m_fileEngine->getErrorString());
      }
      return ret;
   }
   
   // Write to the buffer.
   implPtr->m_writeBuffer.append(data, len);
   return len;
}

FileDevice::FileError FileDevice::getError() const
{
   PDK_D(const FileDevice);
   return implPtr->m_error;
}

void FileDevice::unsetError()
{
   PDK_D(FileDevice);
   implPtr->setError(FileDevice::FileError::NoError);
}

pdk::pint64 FileDevice::getSize() const
{
   PDK_D(const FileDevice);
   if (!implPtr->ensureFlushed()) {
      return 0;
   }
   implPtr->m_cachedSize = implPtr->getEngine()->getSize();
   return implPtr->m_cachedSize;
}

bool FileDevice::resize(pdk::pint64 sz)
{
   PDK_D(FileDevice);
   if (!implPtr->ensureFlushed()) {
      return false;
   }
   implPtr->getEngine();
   if (isOpen() && implPtr->m_fileEngine->getPosition() > sz) {
      seek(sz);
   }
   
   if (implPtr->m_fileEngine->setSize(sz)) {
      unsetError();
      implPtr->m_cachedSize = sz;
      return true;
   }
   implPtr->m_cachedSize = 0;
   implPtr->setError(File::FileError::ResizeError, implPtr->m_fileEngine->getErrorString());
   return false;
}

File::Permissions FileDevice::permissions() const
{
   PDK_D(const FileDevice);
   AbstractFileEngine::FileFlags perms = implPtr->getEngine()->getFileFlags(AbstractFileEngine::FileFlag::PermsMask) & 
         AbstractFileEngine::FileFlag::PermsMask;
   return File::Permissions((int)perms); //ewww
}

bool FileDevice::setPermissions(Permissions permissions)
{
   PDK_D(FileDevice);
   if (implPtr->getEngine()->setPermissions(permissions)) {
      unsetError();
      return true;
   }
   implPtr->setError(File::FileError::PermissionsError, implPtr->m_fileEngine->getErrorString());
   return false;
}

uchar *FileDevice::map(pdk::pint64 offset, pdk::pint64 size, MemoryMapFlags flags)
{
   PDK_D(FileDevice);
   if (implPtr->getEngine()
       && implPtr->m_fileEngine->supportsExtension(AbstractFileEngine::MapExtension)) {
      unsetError();
      uchar *address = implPtr->m_fileEngine->map(offset, size, flags);
      if (address == 0) {
         implPtr->setError(implPtr->m_fileEngine->getError(), implPtr->m_fileEngine->getErrorString());
      }
      return address;
   }
   return 0;
}

bool FileDevice::unmap(uchar *address)
{
   PDK_D(FileDevice);
   if (implPtr->getEngine()
       && implPtr->m_fileEngine->supportsExtension(AbstractFileEngine::UnMapExtension)) {
      unsetError();
      bool success = implPtr->m_fileEngine->unmap(address);
      if (!success) {
         implPtr->setError(implPtr->m_fileEngine->getError(), implPtr->m_fileEngine->getErrorString());
      }  
      return success;
   }
   implPtr->setError(FileError::PermissionsError, tr("No file engine available or engine does not support UnMapExtension"));
   return false;
}

static inline AbstractFileEngine::FileTime FileDeviceTimeToAbstractFileEngineTime(FileDevice::FileTime time)
{
   PDK_STATIC_ASSERT(int(FileDevice::FileTime::FileAccessTime) == int(AbstractFileEngine::FileTime::AccessTime));
   PDK_STATIC_ASSERT(int(FileDevice::FileTime::FileBirthTime) == int(AbstractFileEngine::FileTime::BirthTime));
   PDK_STATIC_ASSERT(int(FileDevice::FileTime::FileMetadataChangeTime) == int(AbstractFileEngine::FileTime::MetadataChangeTime));
   PDK_STATIC_ASSERT(int(FileDevice::FileTime::FileModificationTime) == int(AbstractFileEngine::FileTime::ModificationTime));
   return AbstractFileEngine::FileTime(time);
}

DateTime FileDevice::fileTime(FileDevice::FileTime time) const
{
   PDK_D(const FileDevice);
   if (implPtr->getEngine()) {
      return implPtr->getEngine()->getFileTime(FileDeviceTimeToAbstractFileEngineTime(time));
   }
   return DateTime();
}

bool FileDevice::setFileTime(const DateTime &newDate, FileDevice::FileTime fileTime)
{
   PDK_D(FileDevice);
   if (!implPtr->getEngine()) {
      implPtr->setError(FileDevice::FileError::UnspecifiedError, tr("No file engine available"));
      return false;
   }
   if (!implPtr->m_fileEngine->setFileTime(newDate, FileDeviceTimeToAbstractFileEngineTime(fileTime))) {
      implPtr->setError(implPtr->m_fileEngine->getError(), implPtr->m_fileEngine->getErrorString());
      return false;
   }
   
   unsetError();
   return true;
}

} // fs
} // io
} // pdk
