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

#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/ds/internal/ByteArrayPrivate.h"
#include "pdk/base/io/internal/IoDevicePrivate.h"
#include "pdk/kernel/StringUtils.h"

#include <algorithm>

#ifdef PDK_IODEVICE_DEBUG
#  include <ctype.h>
#endif

namespace pdk {
namespace io {

using pdk::ds::internal::MAX_BYTE_ARRAY_SIZE;
using internal::IoDevicePrivate;

#ifdef PDK_IODEVICE_DEBUG
void debug_binary_string(const ByteArray &input)
{
   ByteArray tmp;
   int startOffset = 0;
   for (int i = 0; i < input.size(); ++i) {
      tmp += input[i];
      
      if ((i % 16) == 15 || i == (input.size() - 1)) {
         printf("\n%15d:", startOffset);
         startOffset += tmp.size();
         
         for (int j = 0; j < tmp.size(); ++j)
            printf(" %02x", int(uchar(tmp[j])));
         for (int j = tmp.size(); j < 16 + 1; ++j)
            printf("   ");
         for (int j = 0; j < tmp.size(); ++j)
            printf("%c", isprint(int(uchar(tmp[j]))) ? tmp[j] : '.');
         tmp.clear();
      }
   }
   printf("\n\n");
}

void debug_binary_string(const char *data, pdk::pint64 maxlen)
{
   debug_binary_string(ByteArray(data, maxlen));
}
#endif

#define PDK_VOID

namespace {

void check_warn_message(const IoDevice *device, const char *function, const char *what)
{
   
}

}

#define CHECK_MAXLEN(function, returnType) \
   do { \
   if (maxLength < 0) { \
   check_warn_message(this, #function, "Called with maxLength < 0"); \
   return returnType; \
} \
} while (0)

#define CHECK_MAXBYTEARRAYSIZE(function) \
   do { \
   if (maxLength >= MAX_BYTE_ARRAY_SIZE) { \
   check_warn_message(this, #function, "maxLength argument exceeds QByteArray size limit"); \
   maxLength = MAX_BYTE_ARRAY_SIZE - 1; \
} \
} while (0)

#define CHECK_WRITABLE(function, returnType) \
   do { \
   if ((implPtr->m_openMode & OpenMode::WriteOnly) == 0) { \
   if (implPtr->m_openMode == OpenMode::NotOpen) { \
   check_warn_message(this, #function, "device not open"); \
   return returnType; \
} \
   check_warn_message(this, #function, "ReadOnly device"); \
   return returnType; \
} \
} while (0)

#define CHECK_READABLE(function, returnType) \
   do { \
   if ((implPtr->m_openMode & OpenMode::ReadOnly) == 0) { \
   if (implPtr->m_openMode == OpenMode::NotOpen) { \
   check_warn_message(this, #function, "device not open"); \
   return returnType; \
} \
   check_warn_message(this, #function, "WriteOnly device"); \
   return returnType; \
} \
} while (0)


namespace internal {

IoDevicePrivate::IoDevicePrivate()
   : m_openMode(IoDevice::OpenMode::NotOpen),
     m_pos(0),
     m_devicePos(0),
     m_readChannelCount(0),
     m_writeChannelCount(0),
     m_currentReadChannel(0),
     m_currentWriteChannel(0),
     m_readBufferChunkSize(PDK_IO_DEVICE_BUFFER_SIZE),
     m_writeBufferChunkSize(0),
     m_transactionPos(0),
     m_transactionStarted(false),
     m_baseReadLineDataCalled(false),
     m_accessMode(AccessMode::Unset)
{}

IoDevicePrivate::~IoDevicePrivate()
{}

void IoDevicePrivate::setReadChannelCount(int count)
{
   if (static_cast<size_t>(count) > m_readBuffers.size()) {
      m_readBuffers.insert(m_readBuffers.end(), count - m_readBuffers.size(),
                           RingBuffer(m_readBufferChunkSize));
   } else {
      m_readBuffers.resize(count);
   }
   m_readChannelCount = count;
   setCurrentReadChannel(m_currentReadChannel);
}

void IoDevicePrivate::setWriteChannelCount(int count)
{
   if (static_cast<size_t>(count) > m_writeBuffers.size()) {
      // If writeBufferChunkSize is zero (default value), we don't use
      // IoDevice's write buffers.
      if (m_writeBufferChunkSize != 0) {
         m_writeBuffers.insert(m_writeBuffers.end(), count - m_writeBuffers.size(),
                               RingBuffer(m_writeBufferChunkSize));
      } 
   } else {
      m_writeBuffers.resize(count);
   }
   m_writeChannelCount = count;
   setCurrentWriteChannel(m_currentWriteChannel);
}

bool IoDevicePrivate::allWriteBuffersEmpty() const
{
   for (const RingBuffer &ringBuffer : m_writeBuffers) {
      if (!ringBuffer.isEmpty()) {
         return false;
      }
   }
   return true;
}

void IoDevicePrivate::seekBuffer(pint64 newPos)
{
   const pdk::pint64 offset = newPos - m_pos;
   m_pos = newPos;
   if (offset < 0 || offset >= m_buffer.size()) {
      // When seeking backwards, an operation that is only allowed for
      // random-access devices, the buffer is cleared. The next read
      // operation will then refill the buffer.
      m_buffer.clear();
   } else {
      m_buffer.free(offset);
   }
}

bool IoDevicePrivate::putCharHelper(char c)
{
   return getApiPtr()->write(&c, 1) == 1;
}

pdk::pint64 IoDevicePrivate::read(char *data, pdk::pint64 maxLength, bool peeking)
{
   PDK_Q(IoDevice);
   
   const bool buffered = (m_openMode & IoDevice::OpenMode::Unbuffered) == 0;
   const bool sequential = isSequential();
   const bool keepDataInBuffer = sequential
         ? peeking || m_transactionStarted
         : peeking && buffered;
   const pdk::pint64 savedPos = m_pos;
   pdk::pint64 readSoFar = 0;
   bool madeBufferReadsOnly = true;
   bool deviceAtEof = false;
   char *readPtr = data;
   pdk::pint64 bufferPos = (sequential && m_transactionStarted) ? m_transactionPos : PDK_INT64_C(0);
   while(true) {
      // Try reading from the buffer.
      pdk::pint64 bufferReadChunkSize = keepDataInBuffer
            ? m_buffer.peek(data, maxLength, bufferPos)
            : m_buffer.read(data, maxLength);
      if (bufferReadChunkSize > 0) {
         bufferPos += bufferReadChunkSize;
         if (!sequential) {
            m_pos += bufferReadChunkSize;
         }
#if defined PDK_IODEVICE_DEBUG
         printf("%p \treading %lld bytes from buffer into position %lld\n", q,
                bufferReadChunkSize, readSoFar);
#endif
         readSoFar += bufferReadChunkSize;
         data += bufferReadChunkSize;
         maxLength -= bufferReadChunkSize;
      }
      
      if (maxLength > 0 && !deviceAtEof) {
         pdk::pint64 readFromDevice = 0;
         // Make sure the device is positioned correctly.
         if (sequential || m_pos == m_devicePos || apiPtr->seek(m_pos)) {
            madeBufferReadsOnly = false; // fix readData attempt
            if ((!buffered || maxLength >= m_readBufferChunkSize) && !keepDataInBuffer) {
               // Read big chunk directly to output buffer
               readFromDevice = apiPtr->readData(data, maxLength);
               deviceAtEof = (readFromDevice != maxLength);
#if defined PDK_IODEVICE_DEBUG
               printf("%p \treading %lld bytes from device (total %lld)\n", q,
                      readFromDevice, readSoFar);
#endif
               if (readFromDevice > 0) {
                  readSoFar += readFromDevice;
                  data += readFromDevice;
                  maxLength -= readFromDevice;
                  if (!sequential) {
                     m_pos += readFromDevice;
                     m_devicePos += readFromDevice;
                  }
               }
            } else {
               // Do not read more than maxLength on unbuffered devices
               const pdk::pint64 bytesToBuffer = (buffered || m_readBufferChunkSize < maxLength)
                     ? pdk::pint64(m_readBufferChunkSize)
                     : maxLength;
               // Try to fill pdk::io::IoDevice buffer by single read
               readFromDevice = apiPtr->readData(m_buffer.reserve(bytesToBuffer), bytesToBuffer);
               deviceAtEof = (readFromDevice != bytesToBuffer);
               m_buffer.chop(bytesToBuffer - std::max(PDK_INT64_C(0), readFromDevice));
               if (readFromDevice > 0) {
                  if (!sequential) {
                     m_devicePos += readFromDevice;
                  }
#if defined PDK_IODEVICE_DEBUG
                  printf("%p \treading %lld from device into buffer\n", q,
                         readFromDevice);
#endif
                  continue;
               }
            }
         } else {
            readFromDevice = -1;
         }
         
         if (readFromDevice < 0 && readSoFar == 0) {
            // error and we haven't read anything: return immediately
            return static_cast<pdk::pint64>(-1);
         }
      }
      
      if ((m_openMode & IoDevice::OpenMode::Text) && readPtr < data) {
         const char *endPtr = data;
         
         // optimization to avoid initial self-assignment
         while (*readPtr != '\r') {
            if (++readPtr == endPtr) {
               break;
            }
         }
         char *writePtr = readPtr;
         while (readPtr < endPtr) {
            char ch = *readPtr++;
            if (ch != '\r') {
               *writePtr++ = ch;
            } else {
               --readSoFar;
               --data;
               ++maxLength;
            }
         }
         
         // Make sure we get more data if there is room for more. This
         // is very important for when someone seeks to the start of a
         // '\r\n' and reads one character - they should get the '\n'.
         readPtr = data;
         continue;
      }
      
      break;
   }
   
   // Restore positions after reading
   if (keepDataInBuffer) {
      if (peeking) {
         m_pos = savedPos; // does nothing on sequential devices
      } else {
         m_transactionPos = bufferPos;
      }
   } else if (peeking) {
      seekBuffer(savedPos); // unbuffered random-access device
   }
   
   if (madeBufferReadsOnly && isBufferEmpty()) {
      apiPtr->readData(data, 0);
   }
   return readSoFar;
}


pdk::pint64 IoDevicePrivate::peek(char *data, pdk::pint64 maxLength)
{
   return read(data, maxLength, true);
}

ByteArray IoDevicePrivate::peek(pdk::pint64 maxLength)
{
   ByteArray result(maxLength, pdk::Uninitialized);
   const pdk::pint64 readBytes = read(result.getRawData(), maxLength, true);
   if (readBytes < maxLength) {
      if (readBytes <= 0) {
         result.clear();
      } else {
         result.resize(readBytes);
      }
   }
   return result;
}

pdk::pint64 IoDevicePrivate::skipByReading(pdk::pint64 maxLength)
{
   pdk::pint64 readSoFar = 0;
   do {
      char dummy[4096];
      const pdk::pint64 readBytes = std::min<pdk::pint64>(maxLength, sizeof(dummy));
      const pdk::pint64 readResult = read(dummy, readBytes);
      
      // Do not try again, if we got less data.
      if (readResult != readBytes) {
         if (readSoFar == 0) {
            return readResult;
         }
         if (readResult == -1) {
            return readSoFar;
         }
         return readSoFar + readResult;
      }
      
      readSoFar += readResult;
      maxLength -= readResult;
   } while (maxLength > 0);
   
   return readSoFar;
}

pdk::pint64 IoDevicePrivate::skip(pdk::pint64 maxLength)
{
   // Base implementation discards the data by reading into the dummy buffer.
   // It's slow, but this works for all types of devices. Subclasses can
   // reimplement this function to improve on that.
   return skipByReading(maxLength);
}

} // internal

IoDevice::IoDevice()
   : Object(*new IoDevicePrivate)
{
   // TODO debug info
}

IoDevice::IoDevice(kernel::Object *parent)
   : Object(*new IoDevicePrivate, parent)
{
   // TODO debug info
}

IoDevice::IoDevice(IoDevicePrivate &dd, Object *parent)
   : Object(dd, parent)
{
}

IoDevice::~IoDevice()
{
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::~QIODevice()\n", this);
#endif
}

bool IoDevice::isSequential() const
{
   return false;
}

IoDevice::OpenModes IoDevice::getOpenMode() const
{
   return getImplPtr()->m_openMode;
}

void IoDevice::setOpenMode(OpenModes openMode)
{
   PDK_D(IoDevice);
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::IoDevice::setOpenMode(0x%x)\n", this, int(openMode));
#endif
   implPtr->m_openMode = openMode;
   implPtr->m_accessMode = IoDevicePrivate::AccessMode::Unset;
   implPtr->setReadChannelCount(isReadable() ? std::max(implPtr->m_readChannelCount, 1) : 0);
   implPtr->setWriteChannelCount(isWritable() ? std::max(implPtr->m_writeChannelCount, 1) : 0);
}

void IoDevice::setTextModeEnabled(bool enabled)
{
   PDK_D(IoDevice);
   if (!isOpen()) {
      check_warn_message(this, "setTextModeEnabled", "The device is not open");
      return;
   }
   if (enabled) {
      implPtr->m_openMode |= OpenMode::Text;
   } else {
      implPtr->m_openMode &= ~pdk::as_integer<OpenMode>(OpenMode::Text);
   }
}

bool IoDevice::isTextModeEnabled() const
{
   return getImplPtr()->m_openMode & OpenMode::Text;
}

bool IoDevice::isOpen() const
{
   return getImplPtr()->m_openMode.getUnderData() != pdk::as_integer<OpenMode>(OpenMode::NotOpen);
}

bool IoDevice::isReadable() const
{
   return (getOpenMode() & OpenMode::ReadOnly) != 0;
}

bool IoDevice::isWritable() const
{
   return (getOpenMode() & OpenMode::WriteOnly) != 0;
}

int IoDevice::getReadChannelCount() const
{
   return getImplPtr()->m_readChannelCount;
}

int IoDevice::getWriteChannelCount() const
{
   return getImplPtr()->m_writeChannelCount;
}

int IoDevice::getCurrentReadChannel() const
{
   return getImplPtr()->m_currentReadChannel;
}

void IoDevice::setCurrentReadChannel(int channel)
{
   PDK_D(IoDevice);
   if (implPtr->m_transactionStarted) {
      check_warn_message(this, "setReadChannel", "Failed due to read transaction being in progress");
      return;
   }
   // TODO add debug info
   implPtr->setCurrentReadChannel(channel);
}

int IoDevice::getCurrentWriteChannel() const
{
   return getImplPtr()->m_currentWriteChannel;
}

void IoDevice::setCurrentWriteChannel(int channel)
{
   PDK_D(IoDevice);
   // TODO add debug info
   implPtr->setCurrentWriteChannel(channel);
}

bool IoDevice::open(OpenModes mode)
{
   PDK_D(IoDevice);
   implPtr->m_openMode = mode;
   implPtr->m_pos = (mode & OpenMode::Append) ? getSize() : static_cast<pdk::pint64>(0);
   implPtr->m_accessMode = IoDevicePrivate::AccessMode::Unset;
   implPtr->m_readBuffers.clear();
   implPtr->m_writeBuffers.clear();
   implPtr->setReadChannelCount(isReadable() ? 1: 0);
   implPtr->setWriteChannelCount(isWritable() ? 1: 0);
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::IoDevice::open(0x%x)\n", this, static_cast<pdk::puint32>(mode));
#endif
   return true;
}

void IoDevice::close()
{
   PDK_D(IoDevice);
   if (implPtr->m_openMode == OpenMode::NotOpen) {
      return;
   }
#if defined PDK_ODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::close()\n", this);
#endif
   // TODO emit signal
   // emit aboutToClose();
   implPtr->m_openMode = OpenMode::NotOpen;
   implPtr->m_errorString.clear();
   implPtr->m_pos = 0;
   implPtr->m_transactionStarted = false;
   implPtr->m_transactionPos = 0;
   implPtr->setReadChannelCount(0);
   // Do not clear write buffers to allow delayed close in sockets
   implPtr->setWriteChannelCount(0);
}

pdk::pint64 IoDevice::getPosition() const
{
   PDK_D(const IoDevice);
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::getPosition() == %lld\n", this, implPtr->m_pos);
#endif
   return implPtr->m_pos;
}

pdk::pint64 IoDevice::getSize() const
{
   return getImplPtr()->isSequential() ? bytesAvailable() : static_cast<pdk::pint64>(0);
}

bool IoDevice::seek(pint64 pos)
{
   PDK_D(IoDevice);
   if (implPtr->isSequential()) {
      check_warn_message(this, "seek", "Cannot call seek on a sequential device");
      return false;
   }
   if (implPtr->m_openMode == OpenMode::NotOpen) {
      check_warn_message(this, "seek", "The device is not open");
      return false;
   }
   if (pos < 0) {
      // warning_stream("pdk::io::IoDevice::seek: Invalid pos: %lld", pos);
      return false;
   }
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::seek(%lld), before: implPtr->m_pos = %lld, implPtr->m_buffer.size() = %lld\n",
          this, pos, implPtr->m_pos, implPtr->m_buffer.size());
#endif
   implPtr->m_devicePos = pos;
   implPtr->seekBuffer(pos);
#if defined PDK_IODEVICE_DEBUG
   printf("%p \tafter: implPtr->m_pos == %lld, implPtr->m_buffer.size() == %lld\n", this, implPtr->m_pos,
          implPtr->m_buffer.size());
#endif
   return true;
}

bool IoDevice::atEnd() const
{
   PDK_D(const IoDevice);
   const bool result = (implPtr->m_openMode == OpenMode::NotOpen || (implPtr->isBufferEmpty()
                                                                     && bytesAvailable() == 0));
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::atEnd() returns %s, implPtr->m_openMode == %d, implPtr->m_pos == %lld\n", this,
          result ? "true" : "false", int(implPtr->m_openMode), implPtr->m_pos);
#endif
   return result;
}

bool IoDevice::reset()
{
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::reset()\n", this);
#endif
   return seek(0);
}

pdk::pint64 IoDevice::bytesAvailable() const
{
   PDK_D(const IoDevice);
   if (!implPtr->isSequential()) {
      return std::max(getSize() - implPtr->m_pos, static_cast<pdk::pint64>(0));
   }
   return implPtr->m_buffer.size() - implPtr->m_transactionPos;
}

pdk::pint64 IoDevice::bytesToWrite() const
{
   return getImplPtr()->m_writeBuffer.size();
}

pdk::pint64 IoDevice::read(char *data, pint64 maxLength)
{
   PDK_D(IoDevice);
   
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::read(%p, %lld), m_implPtr->m_pos = %lld, m_implPtr->m_buffer.size() = %lld\n",
          this, data, maxLength, m_implPtr->m_pos, m_implPtr->m_buffer.size());
#endif
   const bool sequential = m_implPtr->isSequential();
   
   // Short-cut for getChar(), unless we need to keep the data in the buffer.
   if (maxLength == 1 && !(sequential && m_implPtr->m_transactionStarted)) {
      int chint;
      while ((chint = m_implPtr->m_buffer.getChar()) != -1) {
         if (!sequential) {
            ++m_implPtr->m_pos;
         }
         char c = char(uchar(chint));
         if (c == '\r' && (m_implPtr->m_openMode & OpenMode::Text)) {
            continue;
         }
         
         *data = c;
#if defined PDK_IODEVICE_DEBUG
         printf("%p \tread 0x%hhx (%c) returning 1 (shortcut)\n", this,
                int(c), isprint(c) ? c : '?');
#endif
         if (m_implPtr->m_buffer.isEmpty()) {
            readData(data, 0);
         }
         return static_cast<pdk::pint64>(1);
      }
   }
   
   CHECK_MAXLEN(read, static_cast<pdk::pint64>(-1));
   CHECK_READABLE(read, static_cast<pdk::pint64>(-1));
   
   const pdk::pint64 readBytes = m_implPtr->read(data, maxLength);
   
#if defined PDK_IODEVICE_DEBUG
   printf("%p \treturning %lld, d->pos == %lld, d->buffer.size() == %lld\n", this,
          readBytes, d->pos, d->buffer.size());
   if (readBytes > 0)
      debugBinaryString(data - readBytes, readBytes);
#endif
   
   return readBytes;
}

ByteArray IoDevice::read(pint64 maxLength)
{
   PDK_D(IoDevice);
   ByteArray result;
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::read(%lld), implPtr->m_pos = %lld, implPtr->m_buffer.size() = %lld\n",
          this, maxLength, implPtr->m_pos, implPtr->m_buffer.size());
#endif
   // Try to prevent the data from being copied, if we have a chunk
   // with the same size in the read buffer.
   if (maxLength == implPtr->m_buffer.nextDataBlockSize() && !implPtr->m_transactionStarted
       && (implPtr->m_openMode & (OpenMode::ReadOnly | OpenMode::Text)) == OpenMode::ReadOnly) {
      result = implPtr->m_buffer.read();
      if (!implPtr->isSequential()) {
         implPtr->m_pos += maxLength;
      }
      if (implPtr->m_buffer.isEmpty()) {
         readData(nullptr, 0);
      }
      return result;
   }
   CHECK_MAXLEN(read, result);
   CHECK_MAXBYTEARRAYSIZE(read);
   result.resize(static_cast<int>(maxLength));
   pdk::pint64 readBytes = read(result.getRawData(), result.size());
   if (readBytes <= 0) {
      result.clear();
   } else {
      result.resize(static_cast<int>(readBytes));
   }
   return result;
}

ByteArray IoDevice::readAll()
{
   PDK_D(IoDevice);
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::readAll(), implPtr->m_pos = %lld, implPtr->m_buffer.size() = %lld\n",
          this, implPtr->m_pos, implPtr->m_buffer.size());
#endif
   ByteArray result;
   pdk::pint64 readBytes = (implPtr->isSequential() ? PDK_INT64_C(0) : getSize());
   if (readBytes == 0) {
      // Size is unknown, read incrementally.
      pdk::pint64 readChunkSize = std::max(static_cast<pdk::pint64>(implPtr->m_readBufferChunkSize),
                                           implPtr->isSequential() ? (implPtr->m_buffer.size() - implPtr->m_transactionPos)
                                                                   : implPtr->m_buffer.size());
      pdk::pint64 readResult;
      do {
         if (readBytes + readChunkSize >= MAX_BYTE_ARRAY_SIZE) {
            // If resize would fail, don't read more, return what we have.
            break;
         }
         result.resize(readBytes + readChunkSize);
         readResult = read(result.getRawData() + readBytes, readChunkSize);
         if (readResult > 0 || readBytes == 0) {
            readBytes += readResult;
            readChunkSize = implPtr->m_readBufferChunkSize;
         }
      } while (readResult > 0);
   } else {
      // Read it all in one go.
      // If resize fails, don't read anything.
      readBytes -= implPtr->m_pos;
      if (readBytes >= MAX_BYTE_ARRAY_SIZE) {
         return ByteArray();
      }
      result.resize(readBytes);
      readBytes = read(result.getRawData(), readBytes);
   }
   if (readBytes <= 0) {
      result.clear();
   } else {  
      result.resize(static_cast<int>(readBytes));
   }
   return result;
}

pdk::pint64 IoDevice::readLine(char *data, pint64 maxLength)
{
   PDK_D(IoDevice);
   if (maxLength < 2) {
      check_warn_message(this, "readLine", "Called with maxLength < 2");
      return static_cast<pdk::pint64>(-1);
   }
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::readLine(%p, %lld), implPtr->m_pos = %lld, implPtr->m_buffer.size() = %lld\n",
          this, data, maxLength, implPtr->m_pos, implPtr->m_buffer.size());
#endif
   // Leave room for a '\0'
   --maxLength;
   const bool sequential = implPtr->isSequential();
   const bool keepDataInBuffer = sequential && implPtr->m_transactionStarted;
   pdk::pint64 readSoFar = 0;
   if (keepDataInBuffer) {
      if (implPtr->m_transactionPos < implPtr->m_buffer.size()) {
         // Peek line from the specified position
         const pdk::pint64 i = implPtr->m_buffer.indexOf('\n', maxLength, implPtr->m_transactionPos);
         readSoFar = implPtr->m_buffer.peek(data, i >= 0 ? (i - implPtr->m_transactionPos + 1) : maxLength,
                                            implPtr->m_transactionPos);
         implPtr->m_transactionPos += readSoFar;
         if (implPtr->m_transactionPos == implPtr->m_buffer.size()) {
            readData(data, 0);
         }
      }
   } else if (!implPtr->m_buffer.isEmpty()) {
      // QRingBuffer::readLine() terminates the line with '\0'
      readSoFar = implPtr->m_buffer.readLine(data, maxLength + 1);
      if (implPtr->m_buffer.isEmpty()) {
         readData(data,0);
      }
      if (!sequential) {
         implPtr->m_pos += readSoFar;
      }  
   }
   if (readSoFar) {
#if defined PDK_IODEVICE_DEBUG
      printf("%p \tread from buffer: %lld bytes, last character read: %hhx\n", this,
             readSoFar, data[readSoFar - 1]);
      debug_binary_string(data, static_cast<int>(readSoFar));
#endif
      if (data[readSoFar - 1] == '\n') {
         if (implPtr->m_openMode & OpenMode::Text) {
            // QRingBuffer::readLine() isn't Text aware.
            if (readSoFar > 1 && data[readSoFar - 2] == '\r') {
               --readSoFar;
               data[readSoFar - 1] = '\n';
            }
         }
         data[readSoFar] = '\0';
         return readSoFar;
      }
   }
   if (implPtr->m_pos != implPtr->m_devicePos && !sequential && !seek(implPtr->m_pos)) {
      return static_cast<pdk::pint64>(-1);
   }
   implPtr->m_baseReadLineDataCalled = false;
   // Force base implementation for transaction on sequential device
   // as it stores the data in internal buffer automatically.
   pdk::pint64 readBytes = keepDataInBuffer
         ? IoDevice::readLineData(data + readSoFar, maxLength - readSoFar)
         : readLineData(data + readSoFar, maxLength - readSoFar);
#if defined PDK_IODEVICE_DEBUG
   printf("%p \tread from readLineData: %lld bytes, readSoFar = %lld bytes\n", this,
          readBytes, readSoFar);
   if (readBytes > 0) {
      debug_binary_string(data, static_cast<int>(readSoFar + readBytes));
   }
#endif
   if (readBytes < 0) {
      data[readSoFar] = '\0';
      return readSoFar ? readSoFar : -1;
   }
   readSoFar += readBytes;
   if (!implPtr->m_baseReadLineDataCalled && !sequential) {
      implPtr->m_pos += readBytes;
      // If the base implementation was not called, then we must
      // assume the device position is invalid and force a seek.
      implPtr->m_devicePos = static_cast<pdk::pint64>(-1);
   }
   data[readSoFar] = '\0';
   
   if (implPtr->m_openMode & OpenMode::Text) {
      if (readSoFar > 1 && data[readSoFar - 1] == '\n' && data[readSoFar - 2] == '\r') {
         data[readSoFar - 2] = '\n';
         data[readSoFar - 1] = '\0';
         --readSoFar;
      }
   }
   
#if defined PDK_IODEVICE_DEBUG
   printf("%p \treturning %lld, implPtr->m_pos = %lld, implPtr->m_buffer.size() = %lld, getSize() = %lld\n",
          this, readSoFar, implPtr->m_pos, implPtr->m_buffer.size(), size());
   debug_binary_string(data, static_cast<int>(readSoFar));
#endif
   return readSoFar;
}

ByteArray IoDevice::readLine(pint64 maxLength)
{
   PDK_D(IoDevice);
   ByteArray result;
   
   CHECK_MAXLEN(readLine, result);
   CHECK_MAXBYTEARRAYSIZE(readLine);
   
#if defined PDK_IODEVICE_DEBUG
   printf("%p QIODevice::readLine(%lld), implPtr->m_pos = %lld, implPtr->m_buffer.size() = %lld\n",
          this, maxLength, implPtr->m_pos, implPtr->m_buffer.size());
#endif
   
   result.resize(static_cast<int>(maxLength));
   pdk::pint64 readBytes = 0;
   if (!result.size()) {
      // If resize fails or maxLength == 0, read incrementally
      if (maxLength == 0) {
         maxLength = MAX_BYTE_ARRAY_SIZE - 1;
      }
      // The first iteration needs to leave an extra byte for the terminating null
      result.resize(1);
      pdk::pint64 readResult;
      do {
         result.resize(int(std::min(maxLength, static_cast<pdk::pint64>(result.size() + implPtr->m_readBufferChunkSize))));
         readResult = readLine(result.getRawData() + readBytes, result.size() - readBytes);
         if (readResult > 0 || readBytes == 0) {
            readBytes += readResult;
         }
      } while (readResult == implPtr->m_readBufferChunkSize
               && result[int(readBytes - 1)] != '\n');
   } else {
      readBytes = readLine(result.getRawData(), result.size());
   }
   if (readBytes <= 0) {
      result.clear();
   } else {
      result.resize(readBytes);
   }
   return result;
}

pdk::pint64 IoDevice::readLineData(char *data, pint64 maxLength)
{
   PDK_D(IoDevice);
   pdk::pint64 readSoFar = 0;
   char c;
   int lastReadReturn = 0;
   implPtr->m_baseReadLineDataCalled = true;
   
   while (readSoFar < maxLength && (lastReadReturn = read(&c, 1)) == 1) {
      *data++ = c;
      ++readSoFar;
      if (c == '\n') {
         break;
      }
   }
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::readLineData(%p, %lld), implPtr->m_pos = %lld, implPtr->m_buffer.size() = %lld, "
          "returns %lld\n", this, data, maxLength, implPtr->m_pos, implPtr->m_buffer.size(), readSoFar);
#endif
   if (lastReadReturn != 1 && readSoFar == 0) {
      return isSequential() ? lastReadReturn : -1;
   }
   return readSoFar;
}

bool IoDevice::canReadLine() const
{
   PDK_D(const IoDevice);
   return implPtr->m_buffer.indexOf('\n', implPtr->m_buffer.size(),
                                    implPtr->isSequential() ? implPtr->m_transactionPos : PDK_INT64_C(0)) >= 0;
}

void IoDevice::startTransaction()
{
   PDK_D(IoDevice);
   if (implPtr->m_transactionStarted) {
      check_warn_message(this, "startTransaction", "Called while transaction already in progress");
      return;
   }
   implPtr->m_transactionPos = implPtr->m_pos;
   implPtr->m_transactionStarted = true;
}

void IoDevice::commitTransaction()
{
   PDK_D(IoDevice);
   if (!implPtr->m_transactionStarted) {
      check_warn_message(this, "commitTransaction", "Called while no transaction in progress");
      return;
   }
   if (implPtr->isSequential()) {
      implPtr->m_buffer.free(implPtr->m_transactionPos);
   }
   implPtr->m_transactionStarted = false;
   implPtr->m_transactionPos = 0;
}

void IoDevice::rollbackTransaction()
{
   PDK_D(IoDevice);
   if (!implPtr->m_transactionStarted) {
      check_warn_message(this, "rollbackTransaction", "Called while no transaction in progress");
      return;
   }
   if (!implPtr->isSequential()) {
      implPtr->seekBuffer(implPtr->m_transactionPos);
   }
   implPtr->m_transactionStarted = false;
   implPtr->m_transactionPos = 0;
}

bool IoDevice::isTransactionStarted() const
{
   return getImplPtr()->m_transactionStarted;
}

pdk::pint64 IoDevice::write(const char *data, pdk::pint64 maxLength)
{
   PDK_D(IoDevice);
   CHECK_WRITABLE(write, static_cast<pdk::pint64>(-1));
   CHECK_MAXLEN(write, static_cast<pdk::pint64>(-1));
   
   const bool sequential = implPtr->isSequential();
   // Make sure the device is positioned correctly.
   if (implPtr->m_pos != implPtr->m_devicePos && !sequential && !seek(implPtr->m_pos))
      return pdk::pint64(-1);
   
#ifdef PDK_OS_WIN
   if (implPtr->m_openMode & Text) {
      const char *endOfData = data + maxLength;
      const char *startOfBlock = data;
      
      pdk::pint64 writtenSoFar = 0;
      const pdk::pint64 savedPos = implPtr->m_pos;
      
      while(true) {
         const char *endOfBlock = startOfBlock;
         while (endOfBlock < endOfData && *endOfBlock != '\n') {
            ++endOfBlock;
         }
         pdk::pint64 blockSize = endOfBlock - startOfBlock;
         if (blockSize > 0) {
            pdk::pint64 ret = writeData(startOfBlock, blockSize);
            if (ret <= 0) {
               if (writtenSoFar && !sequential) {
                  implPtr->m_buffer.skip(implPtr->m_pos - savedPos);
               }
               return writtenSoFar ? writtenSoFar : ret;
            }
            if (!sequential) {
               implPtr->m_pos += ret;
               implPtr->m_devicePos += ret;
            }
            writtenSoFar += ret;
         }
         
         if (endOfBlock == endOfData) {
            break;
         }
         pdk::pint64 ret = writeData("\r\n", 2);
         if (ret <= 0) {
            if (writtenSoFar && !sequential) {
               implPtr->m_buffer.skip(implPtr->m_pos - savedPos);
            }
            return writtenSoFar ? writtenSoFar : ret;
         }
         if (!sequential) {
            implPtr->m_pos += ret;
            implPtr->m_devicePos += ret;
         }
         ++writtenSoFar;
         startOfBlock = endOfBlock + 1;
      }
      
      if (writtenSoFar && !sequential) {
         implPtr->m_buffer.skip(implPtr->m_pos - savedPos);
      } 
      return writtenSoFar;
   }
#endif
   
   pdk::pint64 written = writeData(data, maxLength);
   if (!sequential && written > 0) {
      implPtr->m_pos += written;
      implPtr->m_devicePos += written;
      implPtr->m_buffer.skip(written);
   }
   return written;
}

pdk::pint64 IoDevice::write(const char *data)
{
   return write(data, pdk::strlen(data));
}

void IoDevice::ungetChar(char c)
{
   PDK_D(IoDevice);
   CHECK_READABLE(read, PDK_VOID);
   
   if (implPtr->m_transactionStarted) {
      check_warn_message(this, "ungetChar", "Called while transaction is in progress");
      return;
   }
   
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::ungetChar(0x%hhx '%c')\n", this, c, isprint(c) ? c : '?');
#endif
   
   implPtr->m_buffer.ungetChar(c);
   if (!implPtr->isSequential()) {
      --implPtr->m_pos;
   }
}

bool IoDevice::putChar(char c)
{
   return getImplPtr()->putCharHelper(c);
}

bool IoDevice::getChar(char *c)
{
   // readability checked in read()
   char ch;
   return (1 == read(c ? c : &ch, 1));
}

pdk::pint64 IoDevice::peek(char *data, pdk::pint64 maxLength)
{
   PDK_D(IoDevice);
   CHECK_MAXLEN(peek, static_cast<pdk::pint64>(-1));
   CHECK_READABLE(peek, static_cast<pdk::pint64>(-1));
   return implPtr->peek(data, maxLength);
}

ByteArray IoDevice::peek(pdk::pint64 maxLength)
{
   PDK_D(IoDevice);
   CHECK_MAXLEN(peek, ByteArray());
   CHECK_MAXBYTEARRAYSIZE(peek);
   CHECK_READABLE(peek, ByteArray());
   return implPtr->peek(maxLength);
}

pdk::pint64 IoDevice::skip(pdk::pint64 maxLength)
{
   PDK_D(IoDevice);
   CHECK_MAXLEN(skip, static_cast<pdk::pint64>(-1));
   CHECK_READABLE(skip, static_cast<pdk::pint64>(-1));
   
   const bool sequential = implPtr->isSequential();
   
#if defined PDK_IODEVICE_DEBUG
   printf("%p pdk::io::IoDevice::skip(%lld), implPtr->m_pos = %lld, implPtr->m_buffer.size() = %lld\n",
          this, maxLength, implPtr->m_pos, implPtr->m_buffer.size());
#endif
   
   if ((sequential && implPtr->m_transactionStarted) || (implPtr->m_openMode & OpenMode::Text) != 0) {
      return implPtr->skipByReading(maxLength);
   }
   
   // First, skip over any data in the internal buffer.
   pdk::pint64 skippedSoFar = 0;
   if (!implPtr->m_buffer.isEmpty()) {
      skippedSoFar = implPtr->m_buffer.skip(maxLength);
#if defined PDK_IODEVICE_DEBUG
      printf("%p \tskipping %lld bytes in buffer\n", this, skippedSoFar);
#endif
      if (!sequential) {
         implPtr->m_pos += skippedSoFar;
      }
      
      if (implPtr->m_buffer.isEmpty()) {
         readData(nullptr, 0);
      }
      if (skippedSoFar == maxLength) {
         return skippedSoFar;
      }
      
      maxLength -= skippedSoFar;
   }
   
   // Try to seek on random-access device. At this point,
   // the internal read buffer is empty.
   if (!sequential) {
      const pdk::pint64 bytesToSkip = std::min(getSize() - implPtr->m_pos, maxLength);
      
      // If the size is unknown or file position is at the end,
      // fall back to reading below.
      if (bytesToSkip > 0) {
         if (!seek(implPtr->m_pos + bytesToSkip)) {
            return skippedSoFar ? skippedSoFar : PDK_INT64_C(-1);
         }
         if (bytesToSkip == maxLength) {
            return skippedSoFar + bytesToSkip;
         }
         skippedSoFar += bytesToSkip;
         maxLength -= bytesToSkip;
      }
   }
   
   const pdk::pint64 skipResult = implPtr->skip(maxLength);
   if (skippedSoFar == 0) {
      return skipResult;
   }
   
   if (skipResult == -1) {
      return skippedSoFar;
   }
   
   return skippedSoFar + skipResult;
}

bool IoDevice::waitForBytesWritten(int msecs)
{
   PDK_UNUSED(msecs);
   return false;
}

void IoDevice::setErrorString(const String &str)
{
   getImplPtr()->m_errorString = str;
}

String IoDevice::getErrorString() const
{
   PDK_D(const IoDevice);
   if (implPtr->m_errorString.isEmpty()) {
      return tr("Unknown error");
   }
   return implPtr->m_errorString;
}


} // io
} // pdk
