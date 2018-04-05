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
// Created by softboy on 2018/03/05.

#include "pdk/base/io/Buffer.h"
#include "pdk/base/io/internal/IoDevicePrivate.h"
#include "pdk/base/io/Debug.h"
#include "pdk/kernel/CallableInvoker.h"

namespace pdk {
namespace io {

using pdk::kernel::CallableInvoker;

namespace internal {

using pdk::io::Buffer;

class BufferPrivate : public IoDevicePrivate
{
   PDK_DECLARE_PUBLIC(Buffer);
   
public:
   BufferPrivate()
      : m_buf(nullptr), 
        m_writtenSinceLastEmit(0),
        m_signalConnectionCount(0),
        m_signalsEmitted(false)
   {}
   
   ~BufferPrivate()
   {}
   
   ByteArray *m_buf;
   ByteArray m_defaultBuf;
   
   virtual pdk::pint64 peek(char *data, pdk::pint64 maxSize) override;
   virtual ByteArray peek(pdk::pint64 maxSize) override;
   
private:
   void emitSignals();
   
   pdk::pint64 m_writtenSinceLastEmit;
   int m_signalConnectionCount;
   bool m_signalsEmitted;
};

void BufferPrivate::emitSignals()
{
   PDK_Q(Buffer);
   apiPtr->emitBytesWrittenSignal(m_writtenSinceLastEmit);
   m_writtenSinceLastEmit = 0;
   apiPtr->emitReadyReadSignal();
   m_signalsEmitted = false;
}

pdk::pint64 BufferPrivate::peek(char *data, pdk::pint64 maxSize)
{
   pdk::pint64 readBytes = std::min(maxSize, static_cast<pdk::pint64>(m_buf->size()) - m_pos);
   std::memcpy(data, m_buf->getConstRawData() + m_pos, readBytes);
   return readBytes;
}

ByteArray BufferPrivate::peek(pdk::pint64 maxSize)
{
   pdk::pint64 readBytes = std::min(maxSize, static_cast<pdk::pint64>(m_buf->size()) - m_pos);
   if (m_pos == 0 && maxSize >= m_buf->size()) {
      return *m_buf;
   }
   return ByteArray(m_buf->getConstRawData() + m_pos, readBytes);
}

} // internal

using internal::BufferPrivate;

Buffer::Buffer(Object *parent)
   : IoDevice(*new BufferPrivate, parent)
{
   PDK_D(Buffer);
   implPtr->m_buf = &implPtr->m_defaultBuf;
}

Buffer::Buffer(ByteArray *byteArray, Object *parent)
   : IoDevice(*new BufferPrivate, parent)
{
   PDK_D(Buffer);
   implPtr->m_buf = byteArray ? byteArray : &implPtr->m_defaultBuf;
   implPtr->m_defaultBuf.clear();
}

Buffer::~Buffer()
{
}

void Buffer::setBuffer(ByteArray *byteArray)
{
   PDK_D(Buffer);
   if (isOpen()) {
      warning_stream("Buffer::setBuffer: Buffer is open");
      return;
   }
   if (byteArray) {
      implPtr->m_buf = byteArray;
   } else {
      implPtr->m_buf = &implPtr->m_defaultBuf;
   }
   implPtr->m_defaultBuf.clear();
}

ByteArray &Buffer::getBuffer()
{
   PDK_D(Buffer);
   return *implPtr->m_buf;
}

const ByteArray &Buffer::getBuffer() const
{
   PDK_D(const Buffer);
   return *implPtr->m_buf;
}

const ByteArray &Buffer::getData() const
{
   PDK_D(const Buffer);
   return *implPtr->m_buf;
}

void Buffer::setData(const ByteArray &data)
{
   PDK_D(Buffer);
   if (isOpen()) {
      warning_stream("Buffer::setData: Buffer is open");
      return;
   }
   *implPtr->m_buf = data;
}

bool Buffer::open(OpenModes flags)
{
   PDK_D(Buffer);
   
   if ((flags & (OpenMode::Append | OpenMode::Truncate)) != 0) {
      flags |= OpenMode::WriteOnly;
   }
   
   if ((flags & (OpenMode::ReadOnly | OpenMode::WriteOnly)) == 0) {
      warning_stream("Buffer::open: Buffer access not specified");
      return false;
   }
   
   if ((flags & OpenMode::Truncate).getUnderData() == pdk::as_integer<OpenMode>(OpenMode::Truncate)) {
      implPtr->m_buf->resize(0);
   }
   
   return IoDevice::open(flags | IoDevice::OpenMode::Unbuffered);
}

void Buffer::close()
{
   IoDevice::close();
}

pdk::pint64 Buffer::getPosition() const
{
   return IoDevice::getPosition();
}

pdk::pint64 Buffer::getSize() const
{
   PDK_D(const Buffer);
   return pdk::pint64(implPtr->m_buf->size());
}

bool Buffer::seek(pdk::pint64 pos)
{
   PDK_D(Buffer);
   if (pos > implPtr->m_buf->size() && isWritable()) {
      if (seek(implPtr->m_buf->size())) {
         const pdk::pint64 gapSize = pos - implPtr->m_buf->size();
         if (write(ByteArray(gapSize, 0)) != gapSize) {
            warning_stream("Buffer::seek: Unable to fill gap");
            return false;
         }
      } else {
         return false;
      }
   } else if (pos > implPtr->m_buf->size() || pos < 0) {
      warning_stream("Buffer::seek: Invalid pos: %d", int(pos));
      return false;
   }
   return IoDevice::seek(pos);
}

bool Buffer::atEnd() const
{
   return IoDevice::atEnd();
}

bool Buffer::canReadLine() const
{
   PDK_D(const Buffer);
   if (!isOpen()) {
      return false;
   }
   return implPtr->m_buf->indexOf('\n', int(getPosition())) != -1 || IoDevice::canReadLine();
}

pdk::pint64 Buffer::readData(char *data, pdk::pint64 len)
{
   PDK_D(Buffer);
   if ((len = std::min(len, pdk::pint64(implPtr->m_buf->size()) - getPosition())) <= 0) {
      return pdk::pint64(0);
   }
   std::memcpy(data, implPtr->m_buf->getConstRawData() + getPosition(), len);
   return len;
}

pdk::pint64 Buffer::writeData(const char *data, pdk::pint64 len)
{
   PDK_D(Buffer);
   int extraBytes = getPosition() + len - implPtr->m_buf->size();
   if (extraBytes > 0) { // overflow
      int newSize = implPtr->m_buf->size() + extraBytes;
      implPtr->m_buf->resize(newSize);
      if (implPtr->m_buf->size() != newSize) { // could not resize
         warning_stream("Buffer::writeData: Memory allocation error");
         return -1;
      }
   }
   
   std::memcpy(implPtr->m_buf->getRawData() + getPosition(), data, int(len));
   
   implPtr->m_writtenSinceLastEmit += len;
   // @TODO need workaround that signal been block
   if (implPtr->m_signalConnectionCount && !implPtr->m_signalsEmitted) {
      implPtr->m_signalsEmitted = true;
      CallableInvoker::invokeAsync(this, &BufferPrivate::emitSignals);
   }
   return len;
}

void Buffer::connectNotify(pdk::puint32 signal)
{
   SignalType signalEnum = SignalType(signal);
   if (signalEnum == SignalType::ReadyRead || signalEnum == SignalType::BytesWritten) {
      getImplPtr()->m_signalConnectionCount++;
   }
}

void Buffer::disconnectNotify(pdk::puint32 signal)
{
   SignalType signalEnum = SignalType(signal);
   if (signalEnum == SignalType::ReadyRead || signalEnum == SignalType::BytesWritten) {
      getImplPtr()->m_signalConnectionCount--;
   }
}

} // io
} // pdk
