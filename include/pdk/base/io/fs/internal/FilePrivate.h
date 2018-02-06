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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_FILE_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_FILE_PRIVATE_H

#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/internal/FileDevicePrivate.h"

namespace pdk {
namespace io {
namespace fs {

// forward declare class
class TemporaryFile;

namespace internal {

class FilePrivate : public FileDevicePrivate
{
   PDK_DECLARE_PUBLIC(File);
   friend class TemporaryFile;
   
protected:
   FilePrivate();
   ~FilePrivate();
   
   bool openExternalFile(int flags, int fd, File::FileHandleFlags handleFlags);
   bool openExternalFile(int flags, FILE *fh, File::FileHandleFlags handleFlags);
   
   AbstractFileEngine *getEngine() const override;
   
   String m_fileName;
};

} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_FILE_PRIVATE_H
