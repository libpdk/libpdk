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
// Created by zzu_softboy on 2018/04/08.

#ifndef PDKTEST_SHARED_Filesystem_H
#define PDKTEST_SHARED_Filesystem_H

#include "pdk/base/lang/String.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/fs/TemporaryDir.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/global/Logging.h"

#if defined(PDK_OS_WIN)
#include <windows.h>
#include <winioctl.h>
#ifndef IO_REPARSE_TAG_MOUNT_POINT
#define IO_REPARSE_TAG_MOUNT_POINT       (0xA0000003L)
#endif
#define REPARSE_MOUNTPOINT_HEADER_SIZE   8
#ifndef FSCTL_SET_REPARSE_POINT
#define FSCTL_SET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#endif
#endif

namespace pdktest {
namespace shared {

using pdk::io::fs::TemporaryDir;
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::IoDevice;
using pdk::utils::ScopedPointer;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::lang::Latin1Character;
using pdk::kernel::CoreApplication;

class Filesystem
{
   PDK_DISABLE_COPY(Filesystem);
   
public:
   Filesystem() : m_temporaryDir(Filesystem::tempFilePattern())
   {}
   
   String path() const { return m_temporaryDir.getPath(); }
   String absoluteFilePath(const String &fileName) const
   {
      return path() + Latin1Character('/') + fileName;
   }
   
   bool createDirectory(const String &relativeDirName)
   {
      if (m_temporaryDir.isValid()) {
         Dir dir(m_temporaryDir.getPath());
         return dir.mkpath(relativeDirName);
      }
      return false;
   }
   
   bool createFile(const String &relativeFileName)
   {
      ScopedPointer<File> file(openFileForWrite(relativeFileName));
      return !file.isNull();
   }
   
   pdk::pint64 createFileWithContent(const String &relativeFileName)
   {
      ScopedPointer<File> file(openFileForWrite(relativeFileName));
      return file.isNull() ? pdk::pint64(-1) : file->write(relativeFileName.toUtf8());
   }
   
private:
   
   static String tempFilePattern()
   {
      String result = Dir::getTempPath();
      if (!result.endsWith(Latin1Character('/'))) {
         result.append(Latin1Character('/'));
      }
      result += StringLiteral("pdk-test-Filesystem-");
      result += CoreApplication::getAppName();
      result += StringLiteral("-XXXXXX");
      return result;
   }
   
   File *openFileForWrite(const String &fileName) const
   {
      if (m_temporaryDir.isValid()) {
         const String absName = absoluteFilePath(fileName);
         ScopedPointer<File> file(new File(absName));
         if (file->open(IoDevice::OpenMode::WriteOnly)) {
            return file.take();
         }
         warning_stream("Cannot open '%s' for writing: %s", pdk_printable(absName), pdk_printable(file->getErrorString()));
      }
      return 0;
   }
   TemporaryDir m_temporaryDir;
};

} // shared
} // pdktest

#endif // PDKTEST_SHARED_Filesystem_H
