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
#include "pdk/kernel/internal/SystemErrorPrivate.h"
#include "pdk/kernel/CoreApplication.h"

namespace pdk {
namespace io {
namespace fs {

using internal::AbstractFileEngine;
using internal::FileSystemEngine;
using internal::FileSystemEntry;

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

String File::fileName() const
{
   PDK_D(const File);
   return implPtr->getEngine()->fileName(AbstractFileEngine::FileName::DefaultName);
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
   return (implPtr->getEngine()->fileFlags(AbstractFileEngine::FileFlag::FlagsMask
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
   if(error() == FileError::NoError) {
      if (implPtr->getEngine()->remove()) {
         unsetError();
         return true;
      }
      implPtr->setError(FileError::RemoveError, implPtr->m_fileEngine->errorString());
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
   if (fileName().isEmpty()) {
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
   ByteArray targetId = FileSystemEngine::id(FileSystemEntry(newName));
   if (!targetId.isNull()) {
      ByteArray fileId = implPtr->m_fileEngine ?
               implPtr->m_fileEngine->id() :
               FileSystemEngine::id(FileSystemEntry(implPtr->m_fileName));
      changingCase = (fileId == targetId && implPtr->m_fileName.compare(newName, pdk::CaseSensitivity::Sensitive) == 0);
      if (!changingCase) {
         implPtr->setError(FileError::RenameError, tr("Destination file exists"));
         return false;
      }

#ifdef PDK_OS_LINUX
      // rename() on Linux simply does nothing when renaming "foo" to "Foo" on a case-insensitive
      // FS, such as FAT32. Move the file away and rename in 2 steps to work around.
      TemporaryFileName tfn(d->fileName);
      FileSystemEntry src(d->fileName);
      QSystemError error;
      for (int attempt = 0; attempt < 16; ++attempt) {
         FileSystemEntry tmp(tfn.generateNext(), FileSystemEntry::FromNativePath());

         // rename to temporary name
         if (!FileSystemEngine::renameFile(src, tmp, error))
            continue;

         // rename to final name
         if (FileSystemEngine::renameFile(tmp, FileSystemEntry(newName), error)) {
            d->fileEngine->setFileName(newName);
            d->fileName = newName;
            return true;
         }

         // We need to restore the original file.
         QSystemError error2;
         if (FileSystemEngine::renameFile(tmp, src, error2))
            break;      // report the original error, below

         // report both errors
         d->setError(File::RenameError,
                     tr("Error while renaming: %1").arg(error.toString())
                     + QLatin1Char('\n')
                     + tr("Unable to restore from %1: %2").
                     arg(QDir::toNativeSeparators(tmp.filePath()), error2.toString()));
         return false;
      }
      d->setError(File::RenameError,
                  tr("Error while renaming: %1").arg(error.toString()));
      return false;
#endif // PDK_OS_LINUX
   }
   unsetError();
   close();
   if(error() == FileError::NoError) {
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
      if (open(IoDevice::ReadOnly)) {
         if (out.open(IoDevice::WriteOnly | IoDevice::Truncate)) {
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

//bool
//File::rename(const String &oldName, const String &newName)
//{
//   return File(oldName).rename(newName);
//}

//bool
//File::link(const String &linkName)
//{
//   PDK_D(File);
//   if (fileName().isEmpty()) {
//      qWarning("File::link: Empty or null file name");
//      return false;
//   }
//   FileInfo fi(linkName);
//   if (d->engine()->link(fi.absoluteFilePath())) {
//      unsetError();
//      return true;
//   }
//   d->setError(File::RenameError, d->fileEngine->errorString());
//   return false;
//}

//bool
//File::link(const String &fileName, const String &linkName)
//{
//   return File(fileName).link(linkName);
//}

//bool
//File::copy(const String &newName)
//{
//   PDK_D(File);
//   if (fileName().isEmpty()) {
//      qWarning("File::copy: Empty or null file name");
//      return false;
//   }
//   if (File::exists(newName)) {
//      // ### Race condition. If a file is moved in after this, it /will/ be
//      // overwritten. On Unix, the proper solution is to use hardlinks:
//      // return ::link(old, new) && ::remove(old); See also rename().
//      d->setError(File::CopyError, tr("Destination file exists"));
//      return false;
//   }
//   unsetError();
//   close();
//   if(error() == File::NoError) {
//      if (d->engine()->copy(newName)) {
//         unsetError();
//         return true;
//      } else {
//         bool error = false;
//         if(!open(File::ReadOnly)) {
//            error = true;
//            d->setError(File::CopyError, tr("Cannot open %1 for input").arg(d->fileName));
//         } else {
//            String fileTemplate = QLatin1String("%1/qt_temp.XXXXXX");
//#ifdef QT_NO_TEMPORARYFILE
//            File out(fileTemplate.arg(FileInfo(newName).path()));
//            if (!out.open(QIODevice::ReadWrite))
//               error = true;
//#else
//            QTemporaryFile out(fileTemplate.arg(FileInfo(newName).path()));
//            if (!out.open()) {
//               out.setFileTemplate(fileTemplate.arg(QDir::tempPath()));
//               if (!out.open())
//                  error = true;
//            }
//#endif
//            if (error) {
//               out.close();
//               close();
//               d->setError(File::CopyError, tr("Cannot open for output"));
//            } else {
//               if (!d->engine()->cloneTo(out.d_func()->engine())) {
//                  char block[4096];
//                  qint64 totalRead = 0;
//                  while (!atEnd()) {
//                     qint64 in = read(block, sizeof(block));
//                     if (in <= 0)
//                        break;
//                     totalRead += in;
//                     if (in != out.write(block, in)) {
//                        close();
//                        d->setError(File::CopyError, tr("Failure to write block"));
//                        error = true;
//                        break;
//                     }
//                  }

//                  if (totalRead != size()) {
//                     // Unable to read from the source. The error string is
//                     // already set from read().
//                     error = true;
//                  }
//               }
//               if (!error && !out.rename(newName)) {
//                  error = true;
//                  close();
//                  d->setError(File::CopyError, tr("Cannot create %1 for output").arg(newName));
//               }
//#ifdef QT_NO_TEMPORARYFILE
//               if (error)
//                  out.remove();
//#else
//               if (!error)
//                  out.setAutoRemove(false);
//#endif
//            }
//         }
//         if(!error) {
//            File::setPermissions(newName, permissions());
//            close();
//            unsetError();
//            return true;
//         }
//      }
//   }
//   return false;
//}

//bool
//File::copy(const String &fileName, const String &newName)
//{
//   return File(fileName).copy(newName);
//}

//bool File::open(OpenMode mode)
//{
//   PDK_D(File);
//   if (isOpen()) {
//      qWarning("File::open: File (%s) already open", qPrintable(fileName()));
//      return false;
//   }
//   if (mode & Append)
//      mode |= WriteOnly;

//   unsetError();
//   if ((mode & (ReadOnly | WriteOnly)) == 0) {
//      qWarning("QIODevice::open: File access not specified");
//      return false;
//   }

//   // QIODevice provides the buffering, so there's no need to request it from the file engine.
//   if (d->engine()->open(mode | QIODevice::Unbuffered)) {
//      QIODevice::open(mode);
//      if (mode & Append)
//         seek(size());
//      return true;
//   }
//   File::FileError err = d->fileEngine->error();
//   if(err == File::UnspecifiedError)
//      err = File::OpenError;
//   d->setError(err, d->fileEngine->errorString());
//   return false;
//}

//bool File::open(FILE *fh, OpenMode mode, FileHandleFlags handleFlags)
//{
//   PDK_D(File);
//   if (isOpen()) {
//      qWarning("File::open: File (%s) already open", qPrintable(fileName()));
//      return false;
//   }
//   if (mode & Append)
//      mode |= WriteOnly;
//   unsetError();
//   if ((mode & (ReadOnly | WriteOnly)) == 0) {
//      qWarning("File::open: File access not specified");
//      return false;
//   }

//   // QIODevice provides the buffering, so request unbuffered file engines
//   if (d->openExternalFile(mode | Unbuffered, fh, handleFlags)) {
//      QIODevice::open(mode);
//      if (!(mode & Append) && !isSequential()) {
//         qint64 pos = (qint64)QT_FTELL(fh);
//         if (pos != -1) {
//            // Skip redundant checks in FileDevice::seek().
//            QIODevice::seek(pos);
//         }
//      }
//      return true;
//   }
//   return false;
//}

//bool File::open(int fd, OpenMode mode, FileHandleFlags handleFlags)
//{
//   PDK_D(File);
//   if (isOpen()) {
//      qWarning("File::open: File (%s) already open", qPrintable(fileName()));
//      return false;
//   }
//   if (mode & Append)
//      mode |= WriteOnly;
//   unsetError();
//   if ((mode & (ReadOnly | WriteOnly)) == 0) {
//      qWarning("File::open: File access not specified");
//      return false;
//   }

//   // QIODevice provides the buffering, so request unbuffered file engines
//   if (d->openExternalFile(mode | Unbuffered, fd, handleFlags)) {
//      QIODevice::open(mode);
//      if (!(mode & Append) && !isSequential()) {
//         qint64 pos = (qint64)QT_LSEEK(fd, QT_OFF_T(0), SEEK_CUR);
//         if (pos != -1) {
//            // Skip redundant checks in FileDevice::seek().
//            QIODevice::seek(pos);
//         }
//      }
//      return true;
//   }
//   return false;
//}

//bool File::resize(qint64 sz)
//{
//   return FileDevice::resize(sz); // for now
//}

//bool
//File::resize(const String &fileName, qint64 sz)
//{
//   return File(fileName).resize(sz);
//}

//File::Permissions File::permissions() const
//{
//   return FileDevice::permissions(); // for now
//}

//File::Permissions
//File::permissions(const String &fileName)
//{
//   return File(fileName).permissions();
//}

//bool File::setPermissions(Permissions permissions)
//{
//   return FileDevice::setPermissions(permissions); // for now
//}

//bool
//File::setPermissions(const String &fileName, Permissions permissions)
//{
//   return File(fileName).setPermissions(permissions);
//}

//qint64 File::size() const
//{
//   return FileDevice::size(); // for now
//}

} // fs
} // io
} // pdk
