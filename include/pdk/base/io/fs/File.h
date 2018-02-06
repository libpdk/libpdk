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

#ifndef PDK_M_BASE_IO_FS_FILE_H
#define PDK_M_BASE_IO_FS_FILE_H

#include "pdk/base/io/fs/FileDevice.h"
#include "pdk/base/lang/String.h"
#include <cstdio>

#ifdef open
#error pdk/base/io/fs/File.h must be included before any header file that defines open
#endif

namespace pdk {
namespace io {
namespace fs {

// forward declare class with namespace
namespace internal {
class TemporaryFile;
class FilePrivate;
} // internal

using pdk::lang::String;
using internal::FilePrivate;

class PDK_CORE_EXPORT File : public FileDevice
{
   PDK_DECLARE_PRIVATE(File);
   
public:
   File();
   File(const String &name);
   explicit File(Object *parent);
   File(const String &name, Object *parent);
   ~File();
   
   String fileName() const override;
   void setFileName(const String &name);
   
#if defined(PDK_OS_DARWIN)
   // Mac always expects filenames in UTF-8... and decomposed...
   static inline ByteArray encodeName(const String &fileName)
   {
      return fileName.normalized(String::NormalizationForm::Form_D).toUtf8();
   }
   static String decodeName(const ByteArray &localFileName)
   {
      // note: duplicated in qglobal.cpp (qEnvironmentVariable)
      return String::fromUtf8(localFileName).normalized(String::NormalizationForm::Form_C);
   }
#else
   static inline ByteArray encodeName(const String &fileName)
   {
      return fileName.toLocal8Bit();
   }
   static String decodeName(const ByteArray &localFileName)
   {
      return String::fromLocal8Bit(localFileName);
   }
#endif
   inline static String decodeName(const char *localFileName)
   {
      return decodeName(ByteArray(localFileName));
   }
   
   bool exists() const;
   static bool exists(const String &fileName);
   
   String readLink() const;
   static String readLink(const String &fileName);
   inline String symLinkTarget() const
   {
      return readLink();
   }
   
   inline static String symLinkTarget(const String &fileName) 
   {
      return readLink(fileName);
   }
   
   bool remove();
   static bool remove(const String &fileName);
   
   bool rename(const String &newName);
   static bool rename(const String &oldName, const String &newName);
   
   bool link(const String &newName);
   static bool link(const String &oldname, const String &newName);
   
   bool copy(const String &newName);
   static bool copy(const String &fileName, const String &newName);
   
   bool open(OpenModes flags) override;
   bool open(FILE *f, OpenMode ioFlags, FileHandleFlags handleFlags = FileHandleFlag::DontCloseHandle);
   bool open(int fd, OpenMode ioFlags, FileHandleFlags handleFlags = FileHandleFlag::DontCloseHandle);
   
   pdk::pint64 getSize() const override;
   
   bool resize(pdk::pint64 sz) override;
   static bool resize(const String &filename, pdk::pint64 sz);
   
   Permissions permissions() const override;
   static Permissions permissions(const String &filename);
   bool setPermissions(Permissions permissionSpec) override;
   static bool setPermissions(const String &filename, Permissions permissionSpec);
   
protected:
   File(FilePrivate &dd, Object *parent = nullptr);
   
private:
   friend class TemporaryFile;
   friend class FilePrivate;
   PDK_DISABLE_COPY(File);
};

} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_FILE_H
