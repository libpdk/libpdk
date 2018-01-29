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

#ifndef PDK_M_BASE_IO_INTERNAL_IODEVICE_H
#define PDK_M_BASE_IO_INTERNAL_IODEVICE_H

#include "pdk/base/io/IoDevice.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/kernel/Object.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/base/ds/internal/RingBufferPrivate.h"

#include <string>
#include <vector>

namespace pdk {
namespace io {
namespace internal {

#ifndef PDK_IO_DEVICE_BUFFER_SIZE
#define PDK_IO_DEVICE_BUFFER_SIZE 16384
#endif

using pdk::kernel::internal::ObjectPrivate;
using pdk::ds::internal::RingBuffer;

PDK_CORE_EXPORT int substract_from_timeout(int timeout, int elapsed);

class PDK_CORE_EXPORT IoDevicePrivate : public ObjectPrivate
{
   PDK_DECLARE_PUBLIC(IoDevice);
public:
   IoDevicePrivate();
   virtual ~IoDevicePrivate();
   enum class AccessMode
   {
      Unset,
      Sequential,
      RandomAccess
   };
   class RingBufferRef
   {
      RingBuffer *m_buf;
      inline RingBufferRef() : m_buf(nullptr)
      {}
      friend class IoDevicePrivate;
   public:
      // wrap functions from QRingBuffer
      inline void setChunkSize(int size)
      {
         PDK_ASSERT(m_buf);
         m_buf->setChunkSize(size);
      }
      
      inline int chunkSize() const
      {
         PDK_ASSERT(m_buf);
         return m_buf->getChunkSize();
      }
      
      inline pdk::pint64 nextDataBlockSize() const
      {
         return (m_buf ? m_buf->nextDataBlockSize() : PDK_INT64_C(0));
      }
      
      inline const char *readPointer() const
      {
         return (m_buf ? m_buf->readPointer() : nullptr);
      }
      
      inline const char *readPointerAtPosition(pdk::pint64 pos, pdk::pint64 &length) const
      {
         PDK_ASSERT(m_buf);
         return m_buf->readPointerAtPosition(pos, length);
      }
      
      inline void free(pdk::pint64 bytes)
      {
         PDK_ASSERT(m_buf);
         m_buf->free(bytes);
      }
      
      inline char *reserve(pdk::pint64 bytes)
      {
         PDK_ASSERT(m_buf);
         return m_buf->reserve(bytes);
      }
      
      inline char *reserveFront(pdk::pint64 bytes)
      {
         PDK_ASSERT(m_buf);
         return m_buf->reserveFront(bytes);
      }
      
      inline void truncate(pdk::pint64 pos)
      {
         PDK_ASSERT(m_buf);
         m_buf->truncate(pos);
      }
      
      inline void chop(pdk::pint64 bytes)
      {
         PDK_ASSERT(m_buf);
         m_buf->chop(bytes);
      }
      
      inline bool isEmpty() const
      {
         return !m_buf || m_buf->isEmpty();
      }
      
      inline int getChar()
      {
         return (m_buf ? m_buf->getChar() : -1);
      }
      
      inline void putChar(char c)
      {
         PDK_ASSERT(m_buf);
         m_buf->putChar(c);
      }
      
      inline void ungetChar(char c)
      {
         PDK_ASSERT(m_buf);
         m_buf->ungetChar(c);
      }
      
      inline pdk::pint64 size() const
      {
         return (m_buf ? m_buf->size() : PDK_INT64_C(0));
      }
      
      inline void clear()
      {
         if (m_buf) {
            m_buf->clear(); 
         }
      }
      
      inline pdk::pint64 indexOf(char c) const
      {
         return (m_buf ? m_buf->indexOf(c, m_buf->size()) : PDK_INT64_C(-1));
      }
      
      inline pdk::pint64 indexOf(char c, pdk::pint64 maxLength, pdk::pint64 pos = 0) const
      {
         return (m_buf ? m_buf->indexOf(c, maxLength, pos) : PDK_INT64_C(-1));
      }
      
      inline pdk::pint64 read(char *data, pdk::pint64 maxLength)
      {
         return (m_buf ? m_buf->read(data, maxLength) : PDK_INT64_C(0));
      }
      
      inline ByteArray read()
      {
         return (m_buf ? m_buf->read() : ByteArray());
      }
      
      inline pdk::pint64 peek(char *data, pdk::pint64 maxLength, pdk::pint64 pos = 0) const
      {
         return (m_buf ? m_buf->peek(data, maxLength, pos) : PDK_INT64_C(0));
      }
      
      inline void append(const char *data, pdk::pint64 size)
      {
         PDK_ASSERT(m_buf);
         m_buf->append(data, size);
      }
      
      inline void append(const ByteArray &qba)
      { 
         PDK_ASSERT(m_buf);
         m_buf->append(qba);
      }
      
      inline pdk::pint64 skip(pdk::pint64 length)
      {
         return (m_buf ? m_buf->skip(length) : PDK_INT64_C(0));
      }
      
      inline pdk::pint64 readLine(char *data, pdk::pint64 maxLength)
      {
         return (m_buf ? m_buf->readLine(data, maxLength) : PDK_INT64_C(-1));
      }
      
      inline bool canReadLine() const
      {
         return m_buf && m_buf->canReadLine();
      }
   };
   
   virtual bool putCharHelper(char c);
   
   inline bool isSequential() const
   {
      if (m_accessMode == AccessMode::Unset) {
         m_accessMode = getApiPtr()->isSequential() 
               ? AccessMode::Sequential
               : AccessMode::RandomAccess;
      }
      return m_accessMode == AccessMode::Sequential;
   }
   
   inline bool isBufferEmpty() const
   {
      return m_buffer.isEmpty() || (m_transactionStarted && isSequential()
                                    && m_transactionPos == m_buffer.size());
   }
   
   bool allWriteBuffersEmpty() const;
   void seekBuffer(pdk::pint64 newPos);
   inline void setCurrentReadChannel(int channel)
   {
      m_buffer.m_buf = (static_cast<size_t>(channel) < m_readBuffers.size() ? &m_readBuffers[channel] : nullptr);
      m_currentReadChannel = channel;
   }
   
   inline void setCurrentWriteChannel(int channel)
   {
      m_writeBuffer.m_buf = (static_cast<size_t>(channel) < m_writeBuffers.size() ? &m_writeBuffers[channel] : nullptr);
      m_currentWriteChannel = channel;
   }
   
   void setReadChannelCount(int count);
   void setWriteChannelCount(int count);
   
   virtual pdk::pint64 peek(char *data, pdk::pint64 maxSize);
   virtual ByteArray peek(pdk::pint64 maxSize);
public:
   IoDevice::OpenModes m_openMode;
   std::string m_errorString;
   std::vector<RingBuffer> m_readBuffers;
   std::vector<RingBuffer> m_writeBuffers;
   RingBufferRef m_buffer;
   RingBufferRef m_writeBuffer;
   pdk::pint64 m_pos;
   pdk::pint64 m_devicePos;
   int m_readChannelCount;
   int m_writeChannelCount;
   int m_currentReadChannel;
   int m_currentWriteChannel;
   int m_readBufferChunkSize;
   int m_writeBufferChunkSize;
   pdk::pint64 m_transactionPos;
   bool m_transactionStarted;
   bool m_baseReadLineDataCalled;
   mutable AccessMode m_accessMode;
};

} // internal
} // io
} // pdk


#endif // PDK_M_BASE_IO_INTERNAL_IODEVICE_H

