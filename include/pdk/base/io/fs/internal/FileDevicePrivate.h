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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_FILE_DEVICE_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_FILE_DEVICE_PRIVATE_H

#include "pdk/base/io/internal/IoDevicePrivate.h"
#include "pdk/base/io/fs/FileDevice.h"

namespace pdk {

// forward declare class with namespace
namespace lang {
class String;
} // lang

namespace io {
namespace fs {

namespace internal {

//forward declare class
class AbstractFileEngine;
class FileEngine;
using pdk::io::internal::IoDevicePrivate;
using pdk::lang::String;
using pdk::io::fs::FileDevice;

class FileDevicePrivate : public IoDevicePrivate
{
   PDK_DECLARE_PUBLIC(FileDevice);
protected:
   FileDevicePrivate();
   ~FileDevicePrivate();
   
   virtual AbstractFileEngine *getEngine() const;
   
   inline bool ensureFlushed() const;
   
   bool putCharHelper(char c) override;
   
   void setError(FileDevice::FileError err);
   void setError(FileDevice::FileError err, const String &errorString);
   void setError(FileDevice::FileError err, int errNum);
   
   mutable AbstractFileEngine *m_fileEngine;
   mutable pdk::pint64 m_cachedSize;
   
   FileDevice::FileHandleFlags m_handleFlags;
   FileDevice::FileError m_error;
   
   bool m_lastWasWrite;
};

inline bool FileDevicePrivate::ensureFlushed() const
{
   // This function ensures that the write buffer has been flushed (const
   // because certain const functions need to call it.
   if (m_lastWasWrite) {
      const_cast<FileDevicePrivate *>(this)->m_lastWasWrite = false;
      if (!const_cast<FileDevice *>(getApiPtr())->flush()) {
         return false;
      }
   }
   return true;
}

} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_FILE_DEVICE_PRIVATE_H
