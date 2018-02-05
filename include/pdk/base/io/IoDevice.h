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
// Created by softboy on 2018/01/28.

#ifndef PDK_M_BASE_IO_IODEVICE_H
#define PDK_M_BASE_IO_IODEVICE_H

#include "pdk/global/Global.h"
#include "pdk/kernel/Object.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/utils/ScopedPointer.h"
#include <string>

#ifdef open
#error pdk/base/io/IoDevice.h must be included before any header file that defines open
#endif

namespace pdk {

namespace ds {
class ByteArray;
} // ds

namespace io {
namespace internal {
class IoDevicePrivate;
} // internal

using pdk::kernel::Object;
using pdk::ds::ByteArray;
using internal::IoDevicePrivate;

class PDK_CORE_EXPORT IoDevice : public Object
{
public:
   enum OpenMode
   {
      NotOpen = 0x0000,
      ReadOnly = 0x0001,
      WriteOnly = 0x0002,
      ReadWrite = ReadOnly | WriteOnly,
      Append = 0x0004,
      Truncate = 0x0008,
      Text = 0x0010,
      Unbuffered = 0x0020
   };
   PDK_DECLARE_FLAGS(OpenModes, OpenMode);
   IoDevice();
   explicit IoDevice(Object *parent);
   virtual ~IoDevice();
   OpenModes getOpenMode() const;
   void setTextModeEnable(bool enabled);
   bool isTextModeEnabled() const;
   
   bool isOpen() const;
   bool isReadable() const;
   bool isWritable() const;
   virtual bool isSequential() const;
   
   int getReadChannelCount() const;
   int getWriteChannelCount() const;
   int getCurrentReadChannel() const;
   void setCurrentReadChannel(int channel);
   int getCurrentWriteChannel() const;
   void setCurrentWriteChannel(int channel);
   
   virtual bool open(OpenModes mode);
   virtual void close();
   
   virtual pdk::pint64 getPosition() const;
   virtual pdk::pint64 getSize() const;
   virtual bool seek(pdk::pint64 pos);
   virtual bool atEnd() const;
   virtual bool reset();
   
   virtual pdk::pint64 bytesAvailable() const;
   virtual pdk::pint64 bytesToWrite() const;
   
   pdk::pint64 read(char *data, pdk::pint64 maxLength);
   ByteArray read(pdk::pint64 maxLength);
   ByteArray readAll();
   pdk::pint64 readLine(char *data, pdk::pint64 maxLength);
   ByteArray readLine(pdk::pint64 maxLength = 0);
   virtual bool canReadLine() const;
   
   void startTransaction();
   void commitTransaction();
   void rollbackTransaction();
   bool isTransactionStarted() const;
   
   pdk::pint64 write(const char *data, pdk::pint64 length);
   pdk::pint64 write(const char *data);
   inline pdk::pint64 write(const ByteArray &data)
   {
      return write(data.getConstRawData(), data.size());
   }
   
   pdk::pint64 peek(char *data, pdk::pint64 maxLength);
   ByteArray peek(pdk::pint64 maxLength);
   pdk::pint64 skip(pdk::pint64 maxSize);
   
   virtual bool waitForReadyRead(int msecs);
   virtual bool waitForBytesWritten(int msecs);
   
   void ungetChar(char c);
   bool putChar(char c);
   bool getChar(char *c);
   
   std::string getErrorString() const;
   // SIGNALS:
   // void readyRead();
   // void channelReadyRead(int channel);
   // void bytesWritten(pdk::pint64 bytes);
   // void channelBytesWritten(int channel, pdk::pint64 bytes);
   // void aboutToClose();
   // void readChannelFinished();
protected:
   IoDevice(IoDevicePrivate &dd, Object *parent = nullptr);
   virtual pdk::pint64 readData(char *data, pdk::pint64 maxLength) = 0;
   virtual pdk::pint64 readLineData(char *data, pdk::pint64 maxLength);
   virtual pdk::pint64 writeData(const char *data, pdk::pint64 length) = 0;
   void setOpenMode(OpenModes openMode);
   void setErrorString(const std::string &errorString);
   pdk::utils::ScopedPointer<IoDevicePrivate> m_implPtr;
   
private:
   
   PDK_DECLARE_PRIVATE(IoDevice);
   PDK_DISABLE_COPY(IoDevice);
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(IoDevice::OpenModes)

} // io
} // pdk

#endif // PDK_M_BASE_IO_IODEVICE_H
