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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_SAVE_FILE_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_SAVE_FILE_PRIVATE_H

#ifndef PDK_NO_TEMPORARYFILE

#include "pdk/global/Global.h"
#include "pdk/base/io/fs/internal/FileDevicePrivate.h"
#include "pdk/base/io/fs/SaveFile.h"

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::io::fs::SaveFile;

class SaveFilePrivate : public FileDevicePrivate
{
   PDK_DECLARE_PUBLIC(SaveFile);
   
protected:
   SaveFilePrivate();
   ~SaveFilePrivate();
   
   String m_fileName;
   String m_finalFileName; // fileName with symbolic links resolved
   
   FileDevice::FileError m_writeError;
   
   bool m_useTemporaryFile;
   bool m_directWriteFallback;
};

} // internal
} // fs
} // io
} // pdk

#endif // PDK_NO_TEMPORARYFILE

#endif // PDK_M_BASE_IO_FS_INTERNAL_SAVE_FILE_PRIVATE_H
