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

#ifndef PDK_M_BASE_IO_FS_FILE_DEVICE_H
#define PDK_M_BASE_IO_FS_FILE_DEVICE_H

#include "pdk/base/io/IoDevice.h"
#include "pdk/base/lang/String.h"

namespace pdk {

// forward declare class with namespace
namespace time {
class DateTime;
} // time

namespace io {
namespace fs {

// forward declare class with namespace
namespace internal {
class FileDevicePrivate;
} // internal

using pdk::time::DateTime;
using internal::FileDevicePrivate;
using pdk::lang::String;

class PDK_CORE_EXPORT FileDevice : public IoDevice
{
   PDK_DECLARE_PRIVATE(FileDevice);
public:
   enum class FileError : uint
   {
      NoError = 0,
      ReadError = 1,
      WriteError = 2,
      FatalError = 3,
      ResourceError = 4,
      OpenError = 5,
      AbortError = 6,
      TimeOutError = 7,
      UnspecifiedError = 8,
      RemoveError = 9,
      RenameError = 10,
      PositionError = 11,
      ResizeError = 12,
      PermissionsError = 13,
      CopyError = 14
   };
   
   enum class FileTime
   {
      FileAccessTime,
      FileBirthTime,
      FileMetadataChangeTime,
      FileModificationTime
   };
   
   enum class Permission : uint
   {
      ReadOwner = 0x4000, WriteOwner = 0x2000, ExeOwner = 0x1000,
      ReadUser  = 0x0400, WriteUser  = 0x0200, ExeUser  = 0x0100,
      ReadGroup = 0x0040, WriteGroup = 0x0020, ExeGroup = 0x0010,
      ReadOther = 0x0004, WriteOther = 0x0002, ExeOther = 0x0001
   };
   
   PDK_DECLARE_FLAGS(Permissions, Permission);
   
   enum class FileHandleFlag : uint
   {
      AutoCloseHandle = 0x0001,
      DontCloseHandle = 0
   };
   PDK_DECLARE_FLAGS(FileHandleFlags, FileHandleFlag);
   
   ~FileDevice();
   
   FileError getError() const;
   void unsetError();
   
   virtual void close() override;
   
   bool isSequential() const override;
   
   int getHandle() const;
   virtual String getFileName() const;
   
   pdk::pint64 getPosition() const override;
   bool seek(pdk::pint64 offset) override;
   bool atEnd() const override;
   bool flush();
   
   pdk::pint64 getSize() const override;
   
   virtual bool resize(pdk::pint64 sz);
   virtual Permissions permissions() const;
   virtual bool setPermissions(Permissions permissionSpec);
   
   enum class MemoryMapFlag
   {
      NoOptions = 0,
      MapPrivateOption = 0x0001
   };
   
   PDK_DECLARE_FLAGS(MemoryMapFlags, MemoryMapFlag);
   
   uchar *map(pdk::pint64 offset, pdk::pint64 size, MemoryMapFlags flags = MemoryMapFlag::NoOptions);
   bool unmap(uchar *address);
   
   DateTime fileTime(FileDevice::FileTime time) const;
   bool setFileTime(const DateTime &newDate, FileDevice::FileTime fileTime);
   
protected:
   FileDevice();
   explicit FileDevice(Object *parent);
   FileDevice(FileDevicePrivate &dd, Object *parent = nullptr);
   
   pdk::pint64 readData(char *data, pdk::pint64 maxlen) override;
   pdk::pint64 writeData(const char *data, pdk::pint64 len) override;
   pdk::pint64 readLineData(char *data, pdk::pint64 maxlen) override;
   
private:
   PDK_DISABLE_COPY(FileDevice);
};

} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_FILE_DEVICE_H
