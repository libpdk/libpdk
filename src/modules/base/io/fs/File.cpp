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
// Created by softboy on 2018/02/06.

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/io/fs/internal/FilePrivate.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/TemporaryFile.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/base/io/fs/internal/TemporaryFilePrivate.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/io/internal/IoDevicePrivate.h"
#include "pdk/base/lang/String.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"

namespace pdk {
namespace io {
namespace fs {

using internal::AbstractFileEngine;
using internal::FileSystemEngine;
using internal::FileSystemEntry;
using internal::TemporaryFileName;
using pdk::kernel::internal::SystemError;
using pdk::lang::Latin1String;

namespace internal {

FilePrivate::FilePrivate()
{
}

FilePrivate::~FilePrivate()
{
}

bool FilePrivate::openExternalFile(int flags, int fd, File::FileHandleFlags handleFlags)
{
#ifdef PDK_NO_FSFILEENGINE
   PDK_UNUSED(flags);
   PDK_UNUSED(fd);
   return false;
#else
   delete m_fileEngine;
   m_fileEngine = nullptr;
   FileEngine *fe = new FileEngine;
   m_fileEngine = fe;
   return fe->open(IoDevice::OpenMode(flags), fd, handleFlags);
#endif
}

bool FilePrivate::openExternalFile(int flags, FILE *fh, File::FileHandleFlags handleFlags)
{
#ifdef PDK_NO_FSFILEENGINE
   PDK_UNUSED(flags);
   PDK_UNUSED(fh);
   return false;
#else
   delete m_fileEngine;
   m_fileEngine = nullptr;
   FileEngine *fe = new FileEngine;
   m_fileEngine = fe;
   return fe->open(IoDevice::OpenMode(flags), fh, handleFlags);
#endif
}

AbstractFileEngine *FilePrivate::getEngine() const
{
   if (!m_fileEngine) {
      m_fileEngine = AbstractFileEngine::create(m_fileName);
   }
   return m_fileEngine;
}

} // internal

File::File()
   : FileDevice(*new FilePrivate, 0)
{
}

File::File(Object *parent)
   : FileDevice(*new FilePrivate, parent)
{
}

File::File(const String &name)
   : FileDevice(*new FilePrivate, 0)
{
   PDK_D(File);
   implPtr->m_fileName = name;
}

File::File(const String &name, Object *parent)
   : FileDevice(*new FilePrivate, parent)
{
   PDK_D(File);
   implPtr->m_fileName = name;
}

File::File(FilePrivate &dd, Object *parent)
   : FileDevice(dd, parent)
{
}

File::~File()
{
}

String File::getFileName() const
{
   PDK_D(const File);
   return implPtr->getEngine()->getFileName(AbstractFileEngine::FileName::DefaultName);
}

void File::setFileName(const String &name)
{
   PDK_D(File);
   if (isOpen()) {
      //      qWarning("File::setFileName: File (%s) is already opened",
      //               qPrintable(fileName()));
      close();
   }
   if(implPtr->m_fileEngine) { //get a new file engine later
      delete implPtr->m_fileEngine;
      implPtr->m_fileEngine = 0;
   }
   implPtr->m_fileName = name;
}

bool File::exists() const
{
   PDK_D(const File);
   // 0x1000000 = AbstractFileEngine::Refresh, forcing an update
   return (implPtr->getEngine()->getFileFlags(AbstractFileEngine::FileFlag::FlagsMask
                                           | AbstractFileEngine::FileFlag::Refresh) & AbstractFileEngine::FileFlag::ExistsFlag);
}

bool File::exists(const String &fileName)
{
   return FileInfo::exists(fileName);
}

bool File::remove()
{
   PDK_D(File);
   if (implPtr->m_fileName.isEmpty() &&
       !static_cast<internal::FileEngine *>(implPtr->getEngine())->isUnnamedFile()) {
      // qWarning("File::remove: Empty or null file name");
      return false;
   }
   unsetError();
   close();
   if(getError() == FileError::NoError) {
      if (implPtr->getEngine()->remove()) {
         unsetError();
         return true;
      }
      implPtr->setError(FileError::RemoveError, implPtr->m_fileEngine->getErrorString());
   }
   return false;
}

bool File::remove(const String &fileName)
{
   return File(fileName).remove();
}

bool File::rename(const String &newName)
{
   PDK_D(File);
   // if this is a QTemporaryFile, the virtual fileName() call here may do something
   if (getFileName().isEmpty()) {
      // qWarning("File::rename: Empty or null file name");
      return false;
   }
   if (implPtr->m_fileName == newName) {
      implPtr->setError(FileError::RenameError, tr("Destination file is the same file."));
      return false;
   }
   if (!exists()) {
      implPtr->setError(FileError::RenameError, tr("Source file does not exist."));
      return false;
   }
   
   // If the file exists and it is a case-changing rename ("foo" -> "Foo"),
   // compare Ids to make sure it really is a different file.
   // Note: this does not take file engines into account.
   bool changingCase = false;
   ByteArray targetId = FileSystemEngine::getId(FileSystemEntry(newName));
   if (!targetId.isNull()) {
      ByteArray fileId = implPtr->m_fileEngine ?
               implPtr->m_fileEngine->getId() :
               FileSystemEngine::getId(FileSystemEntry(implPtr->m_fileName));
      changingCase = (fileId == targetId && implPtr->m_fileName.compare(newName, pdk::CaseSensitivity::Sensitive) == 0);
      if (!changingCase) {
         implPtr->setError(FileError::RenameError, tr("Destination file exists"));
         return false;
      }
      
#ifdef PDK_OS_LINUX
      // rename() on Linux simply does nothing when renaming "foo" to "Foo" on a case-insensitive
      // FS, such as FAT32. Move the file away and rename in 2 steps to work around.
      TemporaryFileName tfn(implPtr->m_fileName);
      FileSystemEntry src(implPtr->m_fileName);
      SystemError error;
      for (int attempt = 0; attempt < 16; ++attempt) {
         FileSystemEntry tmp(tfn.generateNext(), FileSystemEntry::FromNativePath());
         // rename to temporary name
         if (!FileSystemEngine::renameFile(src, tmp, error)) {
            continue;
         }
         // rename to final name
         if (FileSystemEngine::renameFile(tmp, FileSystemEntry(newName), error)) {
            implPtr->m_fileEngine->setFileName(newName);
            implPtr->m_fileName = newName;
            return true;
         }
         
         // We need to restore the original file.
         SystemError error2;
         if (FileSystemEngine::renameFile(tmp, src, error2)) {
            break;      // report the original error, below
         }
         // report both errors
         implPtr->setError(FileError::RenameError,
                           tr("Error while renaming: %1").arg(error.toString())
                           + Latin1Character('\n')
                           + tr("Unable to restore from %1: %2").
                           arg(Dir::toNativeSeparators(tmp.getFilePath()), error2.toString()));
         return false;
      }
      implPtr->setError(FileError::RenameError,
                        tr("Error while renaming: %1").arg(error.toString()));
      return false;
#endif // PDK_OS_LINUX
   }
   unsetError();
   close();
   if(getError() == FileError::NoError) {
      if (changingCase ? implPtr->getEngine()->renameOverwrite(newName) : implPtr->getEngine()->rename(newName)) {
         unsetError();
         // engine was able to handle the new name so we just reset it
         implPtr->m_fileEngine->setFileName(newName);
         implPtr->m_fileName = newName;
         return true;
      }
      
      if (isSequential()) {
         implPtr->setError(FileError::RenameError, tr("Will not rename sequential file using block copy"));
         return false;
      }
      
      File out(newName);
      if (open(IoDevice::OpenMode::ReadOnly)) {
         if (out.open(IoDevice::OpenMode::WriteOnly | IoDevice::OpenMode::Truncate)) {
            bool error = false;
            char block[4096];
            pdk::pint64 bytes;
            while ((bytes = read(block, sizeof(block))) > 0) {
               if (bytes != out.write(block, bytes)) {
                  implPtr->setError(FileError::RenameError, out.getErrorString());
                  error = true;
                  break;
               }
            }
            if (bytes == -1) {
               implPtr->setError(FileError::RenameError, getErrorString());
               error = true;
            }
            if(!error) {
               if (!remove()) {
                  implPtr->setError(FileError::RenameError, tr("Cannot remove source file"));
                  error = true;
               }
            }
            if (error) {
               out.remove();
            } else {
               implPtr->m_fileEngine->setFileName(newName);
               setPermissions(permissions());
               unsetError();
               setFileName(newName);
            }
            close();
            return !error;
         }
         close();
      }
      implPtr->setError(FileError::RenameError, out.isOpen() ? getErrorString() : out.getErrorString());
   }
   return false;
}

bool File::rename(const String &oldName, const String &newName)
{
   return File(oldName).rename(newName);
}

bool File::link(const String &linkName)
{
   PDK_D(File);
   if (getFileName().isEmpty()) {
      // qWarning("File::link: Empty or null file name");
      return false;
   }
   FileInfo fi(linkName);
   if (implPtr->getEngine()->link(fi.getAbsoluteFilePath())) {
      unsetError();
      return true;
   }
   implPtr->setError(FileError::RenameError, implPtr->m_fileEngine->getErrorString());
   return false;
}

bool File::link(const String &fileName, const String &linkName)
{
   return File(fileName).link(linkName);
}

bool File::copy(const String &newName)
{
   PDK_D(File);
   if (getFileName().isEmpty()) {
      // qWarning("File::copy: Empty or null file name");
      return false;
   }
   if (File::exists(newName)) {
      // ### Race condition. If a file is moved in after this, it /will/ be
      // overwritten. On Unix, the proper solution is to use hardlinks:
      // return ::link(old, new) && ::remove(old); See also rename().
      implPtr->setError(FileError::CopyError, tr("Destination file exists"));
      return false;
   }
   unsetError();
   close();
   if(getError() == FileError::NoError) {
      if (implPtr->getEngine()->copy(newName)) {
         unsetError();
         return true;
      } else {
         bool error = false;
         if(!open(File::OpenMode::ReadOnly)) {
            error = true;
            implPtr->setError(FileError::CopyError, tr("Cannot open %1 for input").arg(implPtr->m_fileName));
         } else {
            String fileTemplate = Latin1String("%1/qt_temp.XXXXXX");
#ifdef PDK_NO_TEMPORARYFILE
            File out(fileTemplate.arg(FileInfo(newName).getPath()));
            if (!out.open(IoDevice::ReadWrite)) {
               error = true;
            }
#else
            TemporaryFile out(fileTemplate.arg(FileInfo(newName).getPath()));
            if (!out.open()) {
               out.setFileTemplate(fileTemplate.arg(Dir::getTempPath()));
               if (!out.open()) {
                  error = true;
               }
            }
#endif
            if (error) {
               out.close();
               close();
               implPtr->setError(FileError::CopyError, tr("Cannot open for output"));
            } else {
               if (!implPtr->getEngine()->cloneTo(out.getImplPtr()->getEngine())) {
                  char block[4096];
                  pdk::pint64 totalRead = 0;
                  while (!atEnd()) {
                     pdk::pint64 in = read(block, sizeof(block));
                     if (in <= 0) {
                        break;
                     }  
                     totalRead += in;
                     if (in != out.write(block, in)) {
                        close();
                        implPtr->setError(FileError::CopyError, tr("Failure to write block"));
                        error = true;
                        break;
                     }
                  }
                  
                  if (totalRead != getSize()) {
                     // Unable to read from the source. The error string is
                     // already set from read().
                     error = true;
                  }
               }
               if (!error && !out.rename(newName)) {
                  error = true;
                  close();
                  implPtr->setError(FileError::CopyError, tr("Cannot create %1 for output").arg(newName));
               }
#ifdef PDK_NO_TEMPORARYFILE
               if (error) {
                  out.remove();
               }  
#else
               if (!error) {
                  out.setAutoRemove(false);
               } 
#endif
            }
         }
         if(!error) {
            File::setPermissions(newName, permissions());
            close();
            unsetError();
            return true;
         }
      }
   }
   return false;
}

bool File::copy(const String &fileName, const String &newName)
{
   return File(fileName).copy(newName);
}

bool File::open(OpenModes mode)
{
   PDK_D(File);
   if (isOpen()) {
      // qWarning("File::open: File (%s) already open", qPrintable(fileName()));
      return false;
   }
   if (mode & OpenMode::Append) {
      mode |= OpenMode::WriteOnly;
   }
   unsetError();
   if ((mode & (pdk::as_integer<OpenMode>(OpenMode::ReadOnly) | 
                pdk::as_integer<OpenMode>(OpenMode::WriteOnly))) == 0) {
      // qWarning("QIODevice::open: File access not specified");
      return false;
   }
   
   // IoDevice provides the buffering, so there's no need to request it from the file engine.
   if (implPtr->getEngine()->open(mode | IoDevice::OpenMode::Unbuffered)) {
      IoDevice::open(mode);
      if (mode & OpenMode::Append) {
         seek(getSize());
      }
      return true;
   }
   FileError err = implPtr->m_fileEngine->getError();
   if(err == FileError::UnspecifiedError) {
      err = FileError::OpenError;
   }
   
   implPtr->setError(err, implPtr->m_fileEngine->getErrorString());
   return false;
}

bool File::open(FILE *fh, OpenModes mode, FileHandleFlags handleFlags)
{
   PDK_D(File);
   if (isOpen()) {
      // qWarning("File::open: File (%s) already open", qPrintable(fileName()));
      return false;
   }
   if (mode & OpenMode::Append) {
      mode |= OpenMode::WriteOnly;
   }
   
   unsetError();
   if ((mode & (pdk::as_integer<OpenMode>(OpenMode::ReadOnly) | 
                pdk::as_integer<OpenMode>(OpenMode::WriteOnly))) == 0) {
      // qWarning("File::open: File access not specified");
      return false;
   }
   
   // IoDevice provides the buffering, so request unbuffered file engines
   if (implPtr->openExternalFile(mode | OpenMode::Unbuffered, fh, handleFlags)) {
      IoDevice::open(mode);
      if (!(mode & OpenMode::Append) && !isSequential()) {
         pdk::pint64 pos = (pdk::pint64)PDK_FTELL(fh);
         if (pos != -1) {
            // Skip redundant checks in FileDevice::seek().
            IoDevice::seek(pos);
         }
      }
      return true;
   }
   return false;
}

bool File::open(int fd, OpenModes mode, FileHandleFlags handleFlags)
{
   PDK_D(File);
   if (isOpen()) {
      // qWarning("File::open: File (%s) already open", qPrintable(fileName()));
      return false;
   }
   if (mode & OpenMode::Append) {
      mode |= OpenMode::WriteOnly;
   }
   unsetError();
   if ((mode & (pdk::as_integer<OpenMode>(OpenMode::ReadOnly) | 
                pdk::as_integer<OpenMode>(OpenMode::WriteOnly))) == 0) {
      // qWarning("File::open: File access not specified");
      return false;
   }
   
   // IoDevice provides the buffering, so request unbuffered file engines
   if (implPtr->openExternalFile(mode | OpenMode::Unbuffered, fd, handleFlags)) {
      IoDevice::open(mode);
      if (!(mode & OpenMode::Append) && !isSequential()) {
         pdk::pint64 pos = (pdk::pint64)PDK_LSEEK(fd, PDK_OFF_T(0), SEEK_CUR);
         if (pos != -1) {
            // Skip redundant checks in FileDevice::seek().
            IoDevice::seek(pos);
         }
      }
      return true;
   }
   return false;
}

bool File::resize(pdk::pint64 sz)
{
   return FileDevice::resize(sz); // for now
}

bool File::resize(const String &fileName, pdk::pint64 sz)
{
   return File(fileName).resize(sz);
}

File::Permissions File::permissions() const
{
   return FileDevice::permissions(); // for now
}

File::Permissions File::permissions(const String &fileName)
{
   return File(fileName).permissions();
}

bool File::setPermissions(Permissions permissions)
{
   return FileDevice::setPermissions(permissions); // for now
}

bool File::setPermissions(const String &fileName, Permissions permissions)
{
   return File(fileName).setPermissions(permissions);
}

pdk::pint64 File::getSize() const
{
   return FileDevice::getSize(); // for now
}

} // fs
} // io
} // pdk
