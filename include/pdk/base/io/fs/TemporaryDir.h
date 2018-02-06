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

#ifndef PDK_M_BASE_IO_FS_TEMPORARY_DIR_H
#define PDK_M_BASE_IO_FS_TEMPORARY_DIR_H

#include "pdk/base/io/fs/Dir.h"
#include "pdk/utils/ScopedPointer.h"

#ifndef PDK_NO_TEMPORARYFILE

namespace pdk {
namespace io {
namespace fs {

// forward declare class with namespace
namespace internal {
class TemporaryDirPrivate;
} // internal

using internal::TemporaryDirPrivate;
class PDK_CORE_EXPORT TemporaryDir
{
public:
   TemporaryDir();
   explicit TemporaryDir(const String &templateName);
   ~TemporaryDir();
   
   bool isValid() const;
   String getErrorString() const;
   
   bool autoRemove() const;
   void setAutoRemove(bool b);
   bool remove();
   
   String getPath() const;
   String getFilePath(const String &fileName) const;
   
private:
   pdk::utils::ScopedPointer<TemporaryDirPrivate> m_implPtr;
   
   PDK_DISABLE_COPY(TemporaryDir);
};



} // fs
} // io
} // pdk

#endif // PDK_NO_TEMPORARYFILE

#endif // PDK_M_BASE_IO_FS_TEMPORARY_DIR_H
