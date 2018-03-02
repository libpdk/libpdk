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

#include "pdk/base/io/fs/TemporaryFile.h"
#include "pdk/global/PlatformDefs.h"
#include "pdk/global/Random.h"
#include "pdk/base/io/fs/internal/TemporaryFilePrivate.h"
#include "pdk/base/io/fs/internal/FilePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"

#if !defined(PDK_OS_WIN)
#include "pdk/kernel/internal/CoreUnixPrivate.h"
#include <errno.h>
#endif

#include "pdk/kernel/CoreApplication.h"

namespace pdk {
namespace io {
namespace fs {

using pdk::lang::Character;
using pdk::lang::Latin1Character;
using pdk::lang::Latin1String;
using internal::FileSystemEntry;
using internal::FileSystemEngine;
using internal::TemporaryFileEngine;
using internal::AbstractFileEngine;
using pdk::kernel::internal::SystemError;
using pdk::kernel::CoreApplication;

#if defined(PDK_OS_WIN)
using Char = ushort;

namespace  {
inline Char latin1_char(char ch)
{
   return ushort(uchar(ch));
}
} // anonymous namespace

using NativeFileHandle = HANDLE;
#else // POSIX
using Char = char;
using Latin1Char = char;
using NativeFileHandle = int;
#endif

namespace internal {

TemporaryFileName::TemporaryFileName(const String &templateName)
{
   // Ensure there is a placeholder mask
   String filenameStr = templateName;
   uint phPos = filenameStr.length();
   uint phLength = 0;
   while (phPos != 0) {
      --phPos;
      if (filenameStr[phPos] == Latin1Character('X')) {
         ++phLength;
         continue;
      }
      if (phLength >= 6 || filenameStr[phPos] == Latin1Character('/')) {
         ++phPos;
         break;
      }
      // start over
      phLength = 0;
   }
   if (phLength < 6) {
      filenameStr.append(Latin1String(".XXXXXX"));
   }
   // "Nativify" :-)
   FileSystemEntry::NativePath filename = FileSystemEngine::getAbsoluteName(
            FileSystemEntry(filenameStr, FileSystemEntry::FromInternalPath())).getNativeFilePath();
   // Find mask in native path
   phPos = filename.length();
   phLength = 0;
   while (phPos != 0) {
      --phPos;
      if (filename[phPos] == Latin1Char('X')) {
         ++phLength;
         continue;
      }
      if (phLength >= 6) {
         ++phPos;
         break;
      }
      // start over
      phLength = 0;
   }
   PDK_ASSERT(phLength >= 6);
   m_path = filename;
   m_pos = phPos;
   m_length = phLength;
}

FileSystemEntry::NativePath TemporaryFileName::generateNext()
{
   PDK_ASSERT(m_length != 0);
   PDK_ASSERT(m_pos < static_cast<pdk::sizetype>(m_path.length()));
   PDK_ASSERT(m_length <= m_path.length() - m_pos);
   
   Char *const placeholderStart = (Char *)m_path.getRawData() + m_pos;
   Char *const placeholderEnd = placeholderStart + m_length;
   
   // Replace placeholder with random chars.
   {
      // Since our dictionary is 26+26 characters, it would seem we only need
      // a random number from 0 to 63 to select a character. However, due to
      // the limited range, that would mean 12 (64-52) characters have double
      // the probability of the others: 1 in 32 instead of 1 in 64.
      //
      // To overcome this limitation, we use more bits per character. With 10
      // bits, there are 16 characters with probability 19/1024 and the rest
      // at 20/1024 (i.e, less than .1% difference). This allows us to do 3
      // characters per 32-bit random number, which is also half the typical
      // placeholder length.
      enum { BitsPerCharacter = 10 };
      
      Char *rIter = placeholderEnd;
      while (rIter != placeholderStart) {
         pdk::puint32 rnd = pdk::RandomGenerator::global()->generate();
         auto applyOne = [&]() {
            pdk::puint32 v = rnd & ((1 << BitsPerCharacter) - 1);
            rnd >>= BitsPerCharacter;
            char ch = char((26 + 26) * v / (1 << BitsPerCharacter));
            if (ch < 26) {
               *--rIter = Latin1Char(ch + 'A');
            } else {
               *--rIter = Latin1Char(ch - 26 + 'a');
            }
         };
         applyOne();
         if (rIter == placeholderStart) {
            break;
         }
         applyOne();
         if (rIter == placeholderStart) {
            break;
         }
         applyOne();
      }
   }
   return m_path;
}

#ifndef PDK_NO_TEMPORARYFILE
namespace  {

bool create_file_from_template(NativeFileHandle &file, TemporaryFileName &templ,
                               pdk::puint32 mode, SystemError &error)
{
   const int maxAttempts = 16;
   for (int attempt = 0; attempt < maxAttempts; ++attempt) {
      // Atomically create file and obtain handle
      const FileSystemEntry::NativePath &path = templ.generateNext();
#if defined (PDK_OS_WIN)
      PDK_UNUSED(mode);
      file = CreateFile((const wchar_t *)path.getConstRawData(),
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL, NULL);
      if (file != INVALID_HANDLE_VALUE) {
         return true;
      }
      DWORD err = GetLastError();
      if (err == ERROR_ACCESS_DENIED) {
         WIN32_FILE_ATTRIBUTE_DATA attributes;
         if (!GetFileAttributesEx((const wchar_t *)path.constData(),
                                  GetFileExInfoStandard, &attributes)
             || attributes.dwFileAttributes == INVALID_FILE_ATTRIBUTES) {
            // Potential write error (read-only parent directory, etc.).
            error = SystemError(err, SystemError::ErrorScope::NativeError);
            return false;
         } // else file already exists as a directory.
      } else if (err != ERROR_FILE_EXISTS) {
         error = SystemError(err, SystemError::ErrorScope::NativeError);
         return false;
      }
#else
      file = PDK_OPEN(path.getConstRawData(),
                      PDK_OPEN_CREAT | O_EXCL | PDK_OPEN_RDWR | PDK_OPEN_LARGEFILE,
                      static_cast<mode_t>(mode));
      
      if (file != -1) {
         return true;
      }
      int err = errno;
      if (err != EEXIST) {
         error = SystemError(err, SystemError::ErrorScope::NativeError);
         return false;
      }
#endif
   }
   return false;
}

enum class CreateUnnamedFileStatus
{
   Success = 0,
   NotSupported,
   OtherError
};

CreateUnnamedFileStatus create_unnamed_file(NativeFileHandle &file, TemporaryFileName &tfn, 
                                            pdk::puint32 mode, SystemError *error)
{
#ifdef LINUX_UNNAMED_TMPFILE
   // first, check if we have /proc, otherwise can't make the file exist later
   // (no error message set, as caller will try regular temporary file)
   if (!pdk::kernel::pdk_have_linux_procfs()) {
      return CreateUnnamedFileStatus::NotSupported;
   }
   const char *p = ".";
   int lastSlash = tfn.path.lastIndexOf('/');
   if (lastSlash != -1) {
      tfn.path[lastSlash] = '\0';
      p = tfn.path.data();
   }
   
   file = PDK_OPEN(p, O_TMPFILE | PDK_OPEN_RDWR | PDK_OPEN_LARGEFILE,
                   static_cast<mode_t>(mode));
   if (file != -1) {
      return CreateUnnamedFileStatus::Success;
   }
   if (errno == EOPNOTSUPP || errno == EISDIR) {
      // fs or kernel doesn't support O_TMPFILE, so
      // put the slash back so we may try a regular file
      if (lastSlash != -1) {
         tfn.path[lastSlash] = '/';
      }
      return CreateUnnamedFileStatus::NotSupported;
   }
   // real error
   *error = SystemError(errno, SystemError::ErrorScope::NativeError);
   return CreateUnnamedFileStatus::OtherError;
#else
   PDK_UNUSED(file);
   PDK_UNUSED(tfn);
   PDK_UNUSED(mode);
   PDK_UNUSED(error);
   return CreateUnnamedFileStatus::NotSupported;
#endif
}

} // anonymous namespace

TemporaryFileEngine::~TemporaryFileEngine()
{
   PDK_D(FileEngine);
   implPtr->unmapAll();
   FileEngine::close();
}

bool TemporaryFileEngine::isReallyOpen() const
{
   PDK_D(const FileEngine);
   if (!((0 == implPtr->m_fh) && (-1 == implPtr->m_fd)
      #if defined PDK_OS_WIN
         && (INVALID_HANDLE_VALUE == implPtr->m_fileHandle)
      #endif
         )) {
      return true;
   }
   return false;
}

#endif

void TemporaryFileEngine::setFileName(const String &file)
{
   // Really close the file, so we don't leak
   FileEngine::close();
   FileEngine::setFileName(file);
}

bool TemporaryFileEngine::open(IoDevice::OpenModes openMode)
{
   PDK_D(FileEngine);
   PDK_ASSERT(!isReallyOpen());
   openMode |= IoDevice::OpenMode::ReadWrite;
   if (!m_filePathIsTemplate) {
      return FileEngine::open(openMode);
   }
   TemporaryFileName tfn(m_templateName);
   SystemError error;
#if defined(Q_OS_WIN)
   NativeFileHandle &file = implPtr->m_fileHandle;
#else // POSIX
   NativeFileHandle &file = implPtr->m_fd;
#endif
   
   CreateUnnamedFileStatus st = create_unnamed_file(file, tfn, m_fileMode, &error);
   if (st == CreateUnnamedFileStatus::Success) {
      m_unnamedFile = true;
      implPtr->m_fileEntry.clear();
   } else if (st == CreateUnnamedFileStatus::NotSupported &&
              create_file_from_template(file, tfn, m_fileMode, error)) {
      m_filePathIsTemplate = false;
      m_unnamedFile = false;
      implPtr->m_fileEntry = FileSystemEntry(tfn.m_path, FileSystemEntry::FromNativePath());
   } else {
      setError(File::FileError::OpenError, error.toString());
      return false;
   }
#if !defined(PDK_OS_WIN)
   implPtr->m_closeFileHandle = true;
#endif
   implPtr->m_openMode = openMode;
   implPtr->m_lastFlushFailed = false;
   implPtr->m_triedStat = 0;
   return true;
}

bool TemporaryFileEngine::remove()
{
   PDK_D(FileEngine);
   // Since the TemporaryFileEngine::close() does not really close the file,
   // we must explicitly call FileEngine::close() before we remove it.
   implPtr->unmapAll();
   FileEngine::close();
   if (isUnnamedFile()) {
      return true;
   } 
   if (!m_filePathIsTemplate && FileEngine::remove()) {
      implPtr->m_fileEntry.clear();
      // If a TemporaryFile is constructed using a template file path, the path
      // is generated in TemporaryFileEngine::open() and then filePathIsTemplate
      // is set to false. If remove() and then open() are called on the same
      // TemporaryFile, the path is not regenerated. Here we ensure that if the
      // file path was generated, it will be generated again in the scenario above.
      m_filePathIsTemplate = m_filePathWasTemplate;
      return true;
   }
   return false;
}

bool TemporaryFileEngine::rename(const String &newName)
{
   if (isUnnamedFile()) {
      bool ok = materializeUnnamedFile(newName, DontOverwrite);
      FileEngine::close();
      return ok;
   }
   FileEngine::close();
   return FileEngine::rename(newName);
}

bool TemporaryFileEngine::renameOverwrite(const String &newName)
{
   if (isUnnamedFile()) {
      bool ok = materializeUnnamedFile(newName, Overwrite);
      FileEngine::close();
      return ok;
   }
   FileEngine::close();
   return FileEngine::renameOverwrite(newName);
}

bool TemporaryFileEngine::close()
{
   // Don't close the file, just seek to the front.
   seek(0);
   setError(File::FileError::UnspecifiedError, String());
   return true;
}

String TemporaryFileEngine::getFileName(AbstractFileEngine::FileName file) const
{
   if (isUnnamedFile()) {
      if (file == AbstractFileEngine::FileName::LinkName) {
         // we know our file isn't (won't be) a symlink
         return String();
      }
      // for all other cases, materialize the file
      const_cast<TemporaryFileEngine *>(this)->materializeUnnamedFile(m_templateName, NameIsTemplate);
   }
   return FileEngine::getFileName(file);
}

bool TemporaryFileEngine::materializeUnnamedFile(const String &newName, TemporaryFileEngine::MaterializationMode mode)
{
   PDK_ASSERT(isUnnamedFile());
#ifdef LINUX_UNNAMED_TMPFILE
   PDK_D(FileEngine);
   const ByteArray src = "/proc/self/fd/" + ByteArray::number(implPtr->m_fd);
   auto materializeAt = [=](const FileSystemEntry &dst) {
      return ::linkat(AT_FDCWD, src, AT_FDCWD, dst.getNativeFilePath(), AT_SYMLINK_FOLLOW) == 0;
   };
#else
   auto materializeAt = [](const FileSystemEntry &) { return false; };
#endif
   auto success = [this](const FileSystemEntry &entry) {
      m_filePathIsTemplate = false;
      m_unnamedFile = false;
      getImplPtr()->m_fileEntry = entry;
      return true;
   };
   
   auto materializeAsTemplate = [=](const String &newName) {
      TemporaryFileName tfn(newName);
      static const int maxAttempts = 16;
      for (int attempt = 0; attempt < maxAttempts; ++attempt) {
         tfn.generateNext();
         FileSystemEntry entry(tfn.m_path, FileSystemEntry::FromNativePath());
         if (materializeAt(entry)) {
            return success(entry);
         }
      }
      return false;
   };
   
   if (mode == NameIsTemplate) {
      if (materializeAsTemplate(newName)) {
         return true;
      }
   } else {
      // Use linkat to materialize the file
      FileSystemEntry dst(newName);
      if (materializeAt(dst)) {
         return success(dst);
      }
      if (errno == EEXIST && mode == Overwrite) {
         // retry by first creating a temporary file in the right dir
         if (!materializeAsTemplate(m_templateName)) {
            return false;
         }
         // then rename the materialized file to target (same as renameOverwrite)
         FileEngine::close();
         return FileEngine::renameOverwrite(newName);
      }
   }
   // failed
   setError(File::FileError::RenameError, SystemError(errno, SystemError::ErrorScope::NativeError).toString());
   return false;
}

bool TemporaryFileEngine::isUnnamedFile() const
{
#ifdef LINUX_UNNAMED_TMPFILE
   if (unnamedFile) {
      PDK_ASSERT(getImplPtr()->m_fileEntry.isEmpty());
      PDK_ASSERT(m_filePathIsTemplate);
   }
   return m_unnamedFile;
#else
   return false;
#endif
}

TemporaryFilePrivate::TemporaryFilePrivate()
{
}

TemporaryFilePrivate::TemporaryFilePrivate(const String &templateNameIn)
   : m_templateName(templateNameIn)
{
}

TemporaryFilePrivate::~TemporaryFilePrivate()
{
}

AbstractFileEngine *TemporaryFilePrivate::getEngine() const
{
   if (!m_fileEngine) {
      m_fileEngine = new TemporaryFileEngine(&m_templateName);
      resetFileEngine();
   }
   return m_fileEngine;
}

void TemporaryFilePrivate::resetFileEngine() const
{
   if (!m_fileEngine) {
      return;
   }
   TemporaryFileEngine *tef = static_cast<TemporaryFileEngine *>(m_fileEngine);
   if (m_fileName.isEmpty()) {
      tef->initialize(m_templateName, 0600);
   } else {
      tef->initialize(m_fileName, 0600, false);
   }
}

void TemporaryFilePrivate::materializeUnnamedFile()
{
#ifdef LINUX_UNNAMED_TMPFILE
   if (!m_fileName.isEmpty() || !m_fileEngine) {
      return;
   }
   auto *tef = static_cast<TemporaryFileEngine *>(m_fileEngine);
   m_fileName = tef->m_fileName(AbstractFileEngine::FileName::DefaultName);
#endif
}

String TemporaryFilePrivate::getDefaultTemplateName()
{
   String baseName = CoreApplication::getAppName();
   if (baseName.isEmpty()) {
      baseName = Latin1String("pdk_temp");
   }
   return Dir::getTempPath() + Latin1Char('/') + baseName + Latin1String(".XXXXXX");
}

} // internal

TemporaryFile::TemporaryFile()
   : TemporaryFile(nullptr)
{}

TemporaryFile::TemporaryFile(const String &templateName)
   : TemporaryFile(templateName, nullptr)
{}

TemporaryFile::TemporaryFile(Object *parent)
   : File(*new TemporaryFilePrivate, parent)
{}

TemporaryFile::TemporaryFile(const String &templateName, Object *parent)
   : File(*new TemporaryFilePrivate(templateName), parent)
{}

TemporaryFile::~TemporaryFile()
{
   PDK_D(TemporaryFile);
   close();
   if (!implPtr->m_fileName.isEmpty() && implPtr->m_autoRemove) {
      remove();
   }
}

bool TemporaryFile::getAutoRemove() const
{
   PDK_D(const TemporaryFile);
   return implPtr->m_autoRemove;
}

void TemporaryFile::setAutoRemove(bool flag)
{
   PDK_D(TemporaryFile);
   implPtr->m_autoRemove = flag;
}

String TemporaryFile::getFileName() const
{
   PDK_D(const TemporaryFile);
   auto tef = static_cast<TemporaryFileEngine *>(implPtr->m_fileEngine);
   if (tef && tef->isReallyOpen()) {
      const_cast<TemporaryFilePrivate *>(implPtr)->materializeUnnamedFile();
   }
   if(implPtr->m_fileName.isEmpty()) {
      return String();
   }
   return implPtr->getEngine()->getFileName(AbstractFileEngine::FileName::DefaultName);
}

void TemporaryFile::setFileTemplate(const String &name)
{
   PDK_D(TemporaryFile);
   implPtr->m_templateName = name;
}

bool TemporaryFile::rename(const String &newName)
{
   PDK_D(TemporaryFile);
   auto tef = static_cast<TemporaryFileEngine *>(implPtr->m_fileEngine);
   if (!tef || !tef->isReallyOpen() || !tef->m_filePathWasTemplate) {
      return File::rename(newName);
   }
   unsetError();
   close();
   if (getError() == File::FileError::NoError) {
      if (tef->rename(newName)) {
         unsetError();
         // engine was able to handle the new name so we just reset it
         tef->setFileName(newName);
         implPtr->m_fileName = newName;
         return true;
      }
      implPtr->setError(File::FileError::RenameError, tef->getErrorString());
   }
   return false;
}

TemporaryFile *TemporaryFile::createNativeFile(File &file)
{
   if (AbstractFileEngine *engine = file.getImplPtr()->getEngine()) {
      if(engine->getFileFlags(AbstractFileEngine::FileFlag::FlagsMask) & AbstractFileEngine::FileFlag::LocalDiskFlag)
         return 0; //native already
      //cache
      bool wasOpen = file.isOpen();
      pdk::pint64 oldOff = 0;
      if(wasOpen) {
         oldOff = file.getPosition();
      } else {
         file.open(IoDevice::OpenMode::ReadOnly);
      }
      //dump data
      TemporaryFile *ret = new TemporaryFile;
      ret->open();
      file.seek(0);
      char buffer[1024];
      while(true) {
         pdk::pint64 len = file.read(buffer, 1024);
         if(len < 1) {
            break;
         }
         ret->write(buffer, len);
      }
      ret->seek(0);
      //restore
      if(wasOpen) {
         file.seek(oldOff);
      } else {
         file.close();
      } 
      //done
      return ret;
   }
   return nullptr;
}

bool TemporaryFile::open(OpenModes flags)
{
   PDK_D(TemporaryFile);
   auto tef = static_cast<TemporaryFileEngine *>(implPtr->m_fileEngine);
   if (tef && tef->isReallyOpen()) {
      setOpenMode(flags);
      return true;
   }
   // reset the engine state so it creates a new, unique file name from the template;
   // equivalent to:
   //    delete implPtr->fileEngine;
   //    implPtr->fileEngine = nullptr;
   //    implPtr->getEngine();
   implPtr->resetFileEngine();
   if (File::open(flags)) {
      tef = static_cast<TemporaryFileEngine *>(implPtr->m_fileEngine);
      if (tef->isUnnamedFile()) {
         implPtr->m_fileName.clear();
      } else {
         implPtr->m_fileName = tef->getFileName(AbstractFileEngine::FileName::DefaultName);
      }
      return true;
   }
   return false;
}

} // fs
} // io
} // pdk
