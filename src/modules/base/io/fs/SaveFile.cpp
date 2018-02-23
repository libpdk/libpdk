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

#ifndef PDK_NO_TEMPORARYFILE

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/io/fs/SaveFile.h"
#include "pdk/base/io/fs/internal/SaveFilePrivate.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/TemporaryFile.h"
#include "pdk/base/io/fs/internal/TemporaryFilePrivate.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/internal/IoDevicePrivate.h"

#ifdef PDK_OS_UNIX
#include <errno.h>
#endif

namespace pdk {
namespace io {
namespace fs {

using pdk::io::fs::internal::AbstractFileEngine;
using pdk::lang::Latin1Character;
using pdk::io::fs::Dir;
using internal::TemporaryFileEngine;

namespace internal {

SaveFilePrivate::SaveFilePrivate()
   : m_writeError(FileDevice::FileError::NoError),
     m_useTemporaryFile(true),
     m_directWriteFallback(false)
{}

SaveFilePrivate::~SaveFilePrivate()
{}

} // internal

using internal::SaveFilePrivate;

SaveFile::SaveFile(const String &name)
   : FileDevice(*new SaveFilePrivate, nullptr)
{
   PDK_D(SaveFile);
   implPtr->m_fileName = name;
}

SaveFile::SaveFile(Object *parent)
   : FileDevice(*new SaveFilePrivate, parent)
{}

SaveFile::SaveFile(const String &name, Object *parent)
   : FileDevice(*new SaveFilePrivate, parent)
{
   PDK_D(SaveFile);
   implPtr->m_fileName = name;
}

SaveFile::~SaveFile()
{
   PDK_D(SaveFile);
   FileDevice::close();
   if (implPtr->m_fileEngine) {
      implPtr->m_fileEngine->remove();
      delete implPtr->m_fileEngine;
      implPtr->m_fileEngine = nullptr;
   }
}


String SaveFile::getFileName() const
{
   return getImplPtr()->m_fileName;
}

bool SaveFile::open(OpenModes mode)
{
   PDK_D(SaveFile);
   if (isOpen()) {
      warning_stream("SaveFile::open: File (%s) already open", pdk_printable(getFileName()));
      return false;
   }
   unsetError();
   if ((mode & (pdk::as_integer<OpenMode>(OpenMode::ReadOnly) | pdk::as_integer<OpenMode>(OpenMode::WriteOnly))) == 0) {
      warning_stream("SaveFile::open: Open mode not specified");
      return false;
   }
   // In the future we could implement ReadWrite by copying from the existing file to the temp file...
   if ((mode & OpenMode::ReadOnly) || (mode & OpenMode::Append)) {
      warning_stream("SaveFile::open: Unsupported open mode 0x%x", int(mode));
      return false;
   }
   
   // check if existing file is writable
   FileInfo existingFile(implPtr->m_fileName);
   if (existingFile.exists() && !existingFile.isWritable()) {
      implPtr->setError(FileDevice::FileError::WriteError, SaveFile::tr("Existing file %1 is not writable").arg(implPtr->m_fileName));
      implPtr->m_writeError = FileDevice::FileError::WriteError;
      return false;
   }
   
   if (existingFile.isDir()) {
      implPtr->setError(FileDevice::FileError::WriteError, SaveFile::tr("Filename refers to a directory"));
      implPtr->m_writeError = FileDevice::FileError::WriteError;
      return false;
   }
   
   // Resolve symlinks. Don't use FileInfo::canonicalFilePath so it still give the expected
   // target even if the file does not exist
   implPtr->m_finalFileName = implPtr->m_fileName;
   if (existingFile.isSymLink()) {
      int maxDepth = 128;
      while (--maxDepth && existingFile.isSymLink()) {
         existingFile.setFile(existingFile.getSymLinkTarget());
      }
      if (maxDepth > 0) {
         implPtr->m_finalFileName = existingFile.getFilePath();
      }
   }
   
   auto openDirectly = [&]() {
      implPtr->m_fileEngine = AbstractFileEngine::create(implPtr->m_finalFileName);
      if (implPtr->m_fileEngine->open(mode | IoDevice::OpenMode::Unbuffered)) {
         implPtr->m_useTemporaryFile = false;
         FileDevice::open(mode);
         return true;
      }
      return false;
   };
   
#ifdef PDK_OS_WIN
   // check if it is an Alternate Data Stream
   if (implPtr->m_finalFileName == implPtr->m_fileName && implPtr->m_fileName.indexOf(Latin1Character(':'), 2) > 1) {
      // yes, we can't rename onto it...
      if (implPtr->m_directWriteFallback) {
         if (openDirectly()) {
            return true;
         }
         implPtr->setError(implPtr->m_fileEngine->getError(), implPtr->m_fileEngine->getErrorString());
         delete implPtr->m_fileEngine;
         implPtr->m_fileEngine = nullptr;
      } else {
         String msg =
               SaveFile::tr("SaveFile cannot open '%1' without direct write fallback "
                            "enabled: path contains an Alternate Data Stream specifier")
               .arg(Dir::toNativeSeparators(implPtr->m_fileName));
         implPtr->setError(FileDevice::FileError::OpenError, msg);
      }
      return false;
   }
#endif
   
   implPtr->m_fileEngine = new TemporaryFileEngine(&implPtr->m_finalFileName);
   // if the target file exists, we'll copy its permissions below,
   // but until then, let's ensure the temporary file is not accessible
   // to a third party
   int perm = (existingFile.exists() ? 0600 : 0666);
   static_cast<TemporaryFileEngine *>(implPtr->m_fileEngine)->initialize(implPtr->m_finalFileName, perm);
   // Same as in QFile: QIODevice provides the buffering, so there's no need to request it from the file engine.
   if (!implPtr->m_fileEngine->open(mode | IoDevice::OpenMode::Unbuffered)) {
      FileDevice::FileError err = implPtr->m_fileEngine->getError();
#ifdef PDK_OS_UNIX
      if (implPtr->m_directWriteFallback && err == FileDevice::FileError::OpenError && errno == EACCES) {
         delete implPtr->m_fileEngine;
         if (openDirectly()) {
            return true;
         }
         err = implPtr->m_fileEngine->getError();
      }
#endif
      if (err == FileDevice::FileError::UnspecifiedError) {
         err = FileDevice::FileError::OpenError;
      }
      implPtr->setError(err, implPtr->m_fileEngine->getErrorString());
      delete implPtr->m_fileEngine;
      implPtr->m_fileEngine = nullptr;
      return false;
   }
   
   implPtr->m_useTemporaryFile = true;
   FileDevice::open(mode);
   if (existingFile.exists()) {
      setPermissions(existingFile.permissions());
   }
   return true;
}

void SaveFile::close()
{
   fatal_stream("SaveFile::close called");
}

bool SaveFile::commit()
{
   PDK_D(SaveFile);
   if (!implPtr->m_fileEngine) {
      return false;
   }
   
   if (!isOpen()) {
      warning_stream("SaveFile::commit: File (%s) is not open", pdk_printable(getFileName()));
      return false;
   }
   FileDevice::close(); // calls flush()
   
   // Sync to disk if possible. Ignore errors (e.g. not supported).
   implPtr->m_fileEngine->syncToDisk();
   
   if (implPtr->m_useTemporaryFile) {
      if (implPtr->m_writeError != FileDevice::FileError::NoError) {
         implPtr->m_fileEngine->remove();
         implPtr->m_writeError = FileDevice::FileError::NoError;
         delete implPtr->m_fileEngine;
         implPtr->m_fileEngine = nullptr;
         return false;
      }
      // atomically replace old file with new file
      // Can't use QFile::rename for that, must use the file engine directly
      PDK_ASSERT(implPtr->m_fileEngine);
      if (!implPtr->m_fileEngine->renameOverwrite(implPtr->m_finalFileName)) {
         implPtr->setError(implPtr->m_fileEngine->getError(), implPtr->m_fileEngine->getErrorString());
         implPtr->m_fileEngine->remove();
         delete implPtr->m_fileEngine;
         implPtr->m_fileEngine = nullptr;
         return false;
      }
   }
   delete implPtr->m_fileEngine;
   implPtr->m_fileEngine = nullptr;
   return true;
}

void SaveFile::cancelWriting()
{
   PDK_D(SaveFile);
   if (!isOpen()) {
      return;
   }
   implPtr->setError(FileDevice::FileError::WriteError, SaveFile::tr("Writing canceled by application"));
   implPtr->m_writeError = FileDevice::FileError::WriteError;
}

pdk::pint64 SaveFile::writeData(const char *data, pdk::pint64 len)
{
   PDK_D(SaveFile);
   if (implPtr->m_writeError != FileDevice::FileError::NoError) {
      return -1;
   }
   const pdk::pint64 ret = FileDevice::writeData(data, len);
   if (implPtr->m_error != FileDevice::FileError::NoError) {
      implPtr->m_writeError = implPtr->m_error;
   }
   return ret;
}

void SaveFile::setDirectWriteFallback(bool enabled)
{
   PDK_D(SaveFile);
   implPtr->m_directWriteFallback = enabled;
}

bool SaveFile::getDirectWriteFallback() const
{
   PDK_D(const SaveFile);
   return implPtr->m_directWriteFallback;
}

} // fs
} // io
} // pdk

#endif
