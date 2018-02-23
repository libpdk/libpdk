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

#ifndef PDK_M_BASE_IO_FS_SAVE_FILE_H
#define PDK_M_BASE_IO_FS_SAVE_FILE_H

#include "pdk/global/Global.h"

#ifndef PDK_NO_TEMPORARYFILE

#include "pdk/base/io/fs/FileDevice.h"
#include "pdk/base/lang/String.h"

#ifdef open
#error pdk/base/io/fs/savefile.h must be included before any header file that defines open
#endif

namespace pdk {
namespace io {
namespace fs {

// forward declare class with namespace
namespace internal
{
class AbstractFileEngine;
class SaveFilePrivate;
} // internal

using internal::SaveFilePrivate;
using internal::AbstractFileEngine;

class PDK_CORE_EXPORT SaveFile : public FileDevice
{
   PDK_DECLARE_PRIVATE(SaveFile);
public:
   
   explicit SaveFile(const String &name);
   explicit SaveFile(Object *parent = nullptr);
   explicit SaveFile(const String &name, Object *parent);
   ~SaveFile();
   
   String getFileName() const override;
   void setFileName(const String &name);
   
   bool open(OpenModes flags) override;
   bool commit();
   
   void cancelWriting();
   
   void setDirectWriteFallback(bool enabled);
   bool getDirectWriteFallback() const;
protected:
   pdk::pint64 writeData(const char *data, pdk::pint64 len) override;
   
private:
   void close() override;
#if !PDK_CONFIG(translation)
   static String tr(const char *string)
   {
      return String::fromLatin1(string);
   }
#endif
private:
   PDK_DISABLE_COPY(SaveFile);
};

} // io
} // fs
} // pdk

#endif // PDK_NO_TEMPORARYFILE

#endif // PDK_M_BASE_IO_FS_SAVE_FILE_H
