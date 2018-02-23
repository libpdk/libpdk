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

#include "pdk/base/io/fs/TemporaryDir.h"

#ifndef PDK_NO_TEMPORARYFILE

#include "pdk/base/io/fs/DirIterator.h"
#include "pdk/global/PlatformDefs.h"
#include "pdk/base/io/fs/internal/TemporaryFilePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/Debug.h"
#include <utility>
#include "pdk/global/Random.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"

#if !defined(PDK_OS_WIN)
#include <errno.h>
#endif

namespace pdk {
namespace io {
namespace fs {

using pdk::kernel::CoreApplication;
using pdk::kernel::internal::SystemError;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::lang::String;
using pdk::io::fs::Dir;
using pdk::io::fs::File;

namespace  {

String default_template_name()
{
   String baseName = CoreApplication::getAppName();
   if (baseName.isEmpty()) {
      baseName = Latin1String("pdk_temp");
   }
   return Dir::getTempPath() + Latin1Character('/') + baseName + Latin1String("-XXXXXX");
}

} // anonymous namespace

namespace internal {

class TemporaryDirPrivate
{
public:
   TemporaryDirPrivate();
   ~TemporaryDirPrivate();
   void create(const String &templateName);
   
   String m_pathOrError;
   bool m_autoRemove;
   bool m_success;
};

TemporaryDirPrivate::TemporaryDirPrivate()
   : m_autoRemove(true),
     m_success(false)
{}

TemporaryDirPrivate::~TemporaryDirPrivate()
{}

void TemporaryDirPrivate::create(const String &templateName)
{
   TemporaryFileName tfn(templateName);
   for (int i = 0; i < 256; ++i) {
      tfn.generateNext();
      FileSystemEntry fileSystemEntry(tfn.m_path, FileSystemEntry::FromNativePath());
      if (FileSystemEngine::createDirectory(fileSystemEntry, false)) {
         SystemError error;
         FileSystemEngine::setPermissions(fileSystemEntry,
                                          File::Permissions(pdk::as_integer<File::Permission>(File::Permission::ReadOwner) |
                                                            pdk::as_integer<File::Permission>(File::Permission::WriteOwner) |
                                                            pdk::as_integer<File::Permission>(File::Permission::ExeOwner)), error);
         if (error.getError() != 0) {
            if (!FileSystemEngine::removeDirectory(fileSystemEntry, false)) {
               warning_stream() << "Unable to remove unused directory" << templateName;
            }
            continue;
         }
         m_success = true;
         m_pathOrError = fileSystemEntry.getFilePath();
         return;
      }
#  ifdef PDK_OS_WIN
      const int exists = ERROR_ALREADY_EXISTS;
      int code = GetLastError();
#  else
      const int exists = EEXIST;
      int code = errno;
#  endif
      if (code != exists) {
         break;
      }
   }
   m_pathOrError = pdk::pdk_error_string();
   m_success = false;
}

} // internal

using internal::TemporaryDirPrivate;

TemporaryDir::TemporaryDir()
   : m_implPtr(new TemporaryDirPrivate)
{
   m_implPtr->create(default_template_name());
}

TemporaryDir::TemporaryDir(const String &templatePath)
   : m_implPtr(new TemporaryDirPrivate)
{
   if (templatePath.isEmpty()) {
      m_implPtr->create(default_template_name());
   } else {
      m_implPtr->create(templatePath);
   }
}

TemporaryDir::~TemporaryDir()
{
   if (m_implPtr->m_autoRemove) {
      remove();
   }
}

bool TemporaryDir::isValid() const
{
   return m_implPtr->m_success;
}

String TemporaryDir::getErrorString() const
{
   return m_implPtr->m_success ? String() : m_implPtr->m_pathOrError;
}

String TemporaryDir::getPath() const
{
   return m_implPtr->m_success ? m_implPtr->m_pathOrError : String();
}

String TemporaryDir::getFilePath(const String &fileName) const
{
   if (Dir::isAbsolutePath(fileName)) {
      warning_stream("TemporaryDir::filePath: Absolute paths are not allowed: %s", pdk_utf8_printable(fileName));
      return String();
   }
   if (!m_implPtr->m_success) {
      return String();
   }
   
   String ret = m_implPtr->m_pathOrError;
   if (!fileName.isEmpty()) {
      ret += Latin1Character('/');
      ret += fileName;
   }
   return ret;
}

bool TemporaryDir::autoRemove() const
{
   return m_implPtr->m_autoRemove;
}

void TemporaryDir::setAutoRemove(bool flag)
{
   m_implPtr->m_autoRemove = flag;
}

bool TemporaryDir::remove()
{
   if (!m_implPtr->m_success) {
      return false;
   }
   
   PDK_ASSERT(!getPath().isEmpty());
   PDK_ASSERT(getPath() != Latin1String("."));
   
   const bool result = Dir(getPath()).removeRecursively();
   if (!result) {
      warning_stream() << "TemporaryDir: Unable to remove"
                       << Dir::toNativeSeparators(getPath())
                       << "most likely due to the presence of read-only files.";
   }
   return result;
}

} // fs
} // io
} // pdk

#endif
