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

#ifndef PDK_M_BASE_IO_FS_TEMPORARY_FILE_H
#define PDK_M_BASE_IO_FS_TEMPORARY_FILE_H

#include "pdk/base/io/IoDevice.h"
#include "pdk/base/io/fs/File.h"

#ifdef open
#error pdk/base/io/fs/TemporaryFile.h must be included before any header file that defines open
#endif

namespace pdk {
namespace io {
namespace fs {

// forward declare class with namespace
namespace internal {
class TemporaryFilePrivate;
class LockFilePrivate;
} // internal

using internal::TemporaryFilePrivate;
using internal::LockFilePrivate;

class PDK_CORE_EXPORT TemporaryFile : public File
{
   PDK_DECLARE_PRIVATE(TemporaryFile);
public:
   TemporaryFile();
   explicit TemporaryFile(const String &templateName);
   explicit TemporaryFile(Object *parent);
   TemporaryFile(const String &templateName, Object *parent);
   ~TemporaryFile();
   
   bool getAutoRemove() const;
   void setAutoRemove(bool b);
   
   // ### Hides open(flags)
   bool open()
   {
      return open(OpenMode::ReadWrite);
   }
   
   String getFileName() const override;
   String getFileTemplate() const;
   void setFileTemplate(const String &name);
   
   // Hides File::rename
   bool rename(const String &newName);
   
   inline static TemporaryFile *createNativeFile(const String &fileName)
   {
      File file(fileName);
      return createNativeFile(file);
   }
   static TemporaryFile *createNativeFile(File &file);
   
protected:
   bool open(OpenModes flags) override;
   
private:
   friend class File;
   friend class LockFilePrivate;
   PDK_DISABLE_COPY(TemporaryFile);
};

} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_TEMPORARY_FILE_H
