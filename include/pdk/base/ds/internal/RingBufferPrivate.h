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
// Created by softboy on 2018/01/29.

#ifndef PDK_M_BASE_DS_INTERNAL_RING_BUFFER_PRIVATE_H
#define PDK_M_BASE_DS_INTERNAL_RING_BUFFER_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/ds/ByteArray.h"
#include <list>

#ifndef PDK_RING_BUFFER_CHUNK_SIZE
#define PDK_RING_BUFFER_CHUNK_SIZE 4096
#endif

namespace pdk {
namespace ds {
namespace internal {

class RingBuffer
{
 public:
   explicit inline RingBuffer(int growth = PDK_RING_BUFFER_CHUNK_SIZE)
      : m_head(0),
        m_tail(0),
        m_tailBuffer(0),
        m_basicBlockSize(growth),
        m_bufferSize(0)
   {}
   
   inline void setChunkSize(int size)
   {
      m_basicBlockSize = size;   
   }
   
   inline int getChunkSize() const
   {
      return m_basicBlockSize;
   }
   
   inline pdk::pint64 nextDataBlockSize() const
   {
      return (m_tailBuffer == 0 ? m_tail : m_buffers.front().size()) - m_head;
   }
   
   inline const char *readPointer() const
   {
      return m_bufferSize == 0 ? nullptr : (m_buffers.front().getConstRawData() + m_head);
   }
   
   PDK_CORE_EXPORT const char *readPointerAtPosition(pdk::pint64 pos, pdk::pint64 &length) const;
   PDK_CORE_EXPORT void free(pdk::pint64 bytes);
   PDK_CORE_EXPORT char *reserve(pdk::pint64 bytes);
   PDK_CORE_EXPORT char *reserveFront(pdk::pint64 bytes);
   
   inline void truncate(pdk::pint64 pos)
   {
      if (pos < size()) {
         chop(size() - pos);
      }
   }
   
   PDK_CORE_EXPORT void chop(pdk::pint64 bytes);
   inline bool isEmpty() const
   {
      return m_bufferSize == 0;
   }
   
   inline int getChar()
   {
      if (isEmpty()) {
         return -1;
      }
      char c = *readPointer();
      free(1);
      return static_cast<int>(static_cast<uchar>(c));
   }
   
   inline void putChar(char c)
   {
      char *ptr = reserve(1);
      *ptr = c;
   }
   
   void ungetChar(char c)
   {
      if (m_head > 0) {
         --m_head;
         m_buffers.front()[m_head] = c;
         ++m_bufferSize;
      } else {
         char *ptr = reserveFront(1);
         *ptr = c;
      }
   }
   
   inline pdk::pint64 size() const
   {
      return m_bufferSize;
   }
   
   PDK_CORE_EXPORT void clear();
   inline pdk::pint64 indexOf(char c) const
   {
      return indexOf(c, size());
   }
   
   PDK_CORE_EXPORT pdk::pint64 indexOf(char c, pdk::pint64 maxLength, pdk::pint64 pos = 0) const;
   PDK_CORE_EXPORT pdk::pint64 read(char *data, pdk::pint64 maxLength);
   PDK_CORE_EXPORT ByteArray read();
   PDK_CORE_EXPORT pdk::pint64 peek(char *data, pdk::pint64 maxLength, pdk::pint64 pos = 0) const;
   PDK_CORE_EXPORT void append(const char *data, pdk::pint64 size);
   PDK_CORE_EXPORT void append(const ByteArray &byteArray);
   
   inline pdk::pint64 skip(pdk::pint64 length)
   {
      pdk::pint64 bytesToSkip = std::min(length, m_bufferSize);
      free(bytesToSkip);
      return bytesToSkip;
   }
   
   PDK_CORE_EXPORT pdk::pint64 readLine(char *data, pdk::pint64 maxLength);
   inline bool canReadLine() const
   {
      return indexOf('\n') >= 0;
   }
private:
   std::list<ByteArray> m_buffers;
   int m_head;
   int m_tail;
   int m_tailBuffer;
   int m_basicBlockSize;
   pdk::pint64 m_bufferSize;
};

} // internal
} // ds
} // pdk

#endif // PDK_M_BASE_DS_INTERNAL_RING_BUFFER_PRIVATE_H
