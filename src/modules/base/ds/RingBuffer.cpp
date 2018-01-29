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

#include "pdk/base/ds/internal/RingBufferPrivate.h"
#include "pdk/base/ds/internal/ByteArrayPrivate.h"

namespace pdk {
namespace ds {
namespace internal {

namespace {
ByteArray &retrive_at(std::list<ByteArray> &array, size_t pos)
{
   PDK_ASSERT(pos >= 0 && pos < array.size());
   auto iter = array.begin();
   while (pos != 0) {
      ++iter;  
      --pos;
   }
   return *iter;
}

const ByteArray &retrive_at(const std::list<ByteArray> &array, size_t pos)
{
   return const_cast<const ByteArray &>(retrive_at(const_cast<std::list<ByteArray> &>(array), pos));
}

}

const char *RingBuffer::readPointerAtPosition(pint64 pos, pint64 &length) const
{
   if (pos >= 0) {
      pos += m_head;
      for (size_t i = 0; i < m_buffers.size(); ++i) {
         length = (i == static_cast<size_t>(m_tailBuffer) ? m_tail : retrive_at(m_buffers, i).size());
         if (length > pos) {
            length -= pos;
            return retrive_at(m_buffers, i).getConstRawData() + pos;
         }
         pos -= length;
      }
   }
   length = 0;
   return 0;
}

void RingBuffer::free(pint64 bytes)
{
   PDK_ASSERT(bytes <= m_bufferSize);
   while (bytes > 0) {
      const pdk::pint64 blockSize = m_buffers.front().size() - m_head;
      if (m_tailBuffer == 0 || blockSize > bytes) {
         // keep a single block around if it does not exceed
         // the basic block size, to avoid repeated allocations
         // between uses of the buffer
         if (m_bufferSize <= bytes) {
            if (m_buffers.front().size() <= m_basicBlockSize) {
               m_bufferSize = 0;
               m_head = 0;
               m_tail = 0;
            } else {
               clear();// try to minify/squeeze us
            }
         } else {
            PDK_ASSERT(bytes < MAX_BYTE_ARRAY_SIZE);
            m_head += static_cast<int>(bytes);
            m_bufferSize -= bytes;
         }
         return;
      }
      m_bufferSize -= blockSize;
      bytes -= blockSize;
      m_buffers.pop_front();
      --m_tailBuffer;
      m_head = 0;
   }
}

char *RingBuffer::reserve(pint64 bytes)
{
   if (bytes <= 0 | bytes >= MAX_BYTE_ARRAY_SIZE) {
      return 0;
   }
   if (m_bufferSize == 0) {
      if (m_buffers.empty()) {
         m_buffers.push_back(ByteArray(std::max(m_basicBlockSize, static_cast<int>(bytes)), pdk::Uninitialized));
      } else {
         m_buffers.front().resize(std::max(m_basicBlockSize, static_cast<int>(bytes)));
      }
   } else {
      const pdk::pint64 newSize = bytes + m_tail;
      // if need a new buffer
      if (0 == m_basicBlockSize || (newSize > m_buffers.back().capacity() &&
                                    (m_tail >= m_basicBlockSize || newSize >= MAX_BYTE_ARRAY_SIZE))) {
         // shrink this buffer to its current size
         m_buffers.back().resize(m_tail);
         // create a new ByteArray
         m_buffers.push_back(ByteArray(std::max(m_basicBlockSize, static_cast<int>(bytes)), pdk::Uninitialized));
         ++m_tailBuffer;
         m_tail = 0;
      } else if(newSize > m_buffers.back().size()){
         m_buffers.back().resize(std::max(m_basicBlockSize, static_cast<int>(newSize)));
      }
   }
   char *writePtr = m_buffers.back().getRawData() + m_tail;
   m_bufferSize += bytes;
   PDK_ASSERT(bytes < MAX_BYTE_ARRAY_SIZE);
   m_tail += static_cast<int>(bytes);
   return writePtr;
}

char *RingBuffer::reserveFront(pint64 bytes)
{
   if (bytes <= 0 | bytes >= MAX_BYTE_ARRAY_SIZE) {
      return 0;
   }
   if (m_head < bytes || m_basicBlockSize == 0) {
      if (m_head > 0) {
         m_buffers.front().remove(0, m_head);
         if (m_tailBuffer) {
            m_tail -= m_head;
         }
      }
      m_head = std::max(m_basicBlockSize, static_cast<int>(bytes));
      if (m_basicBlockSize == 0) {
         if (m_buffers.empty()) {
            m_buffers.push_front(ByteArray(m_head, pdk::Uninitialized));
         } else {
            m_buffers.front().resize(m_head);
         }
         m_tail = m_head;
      } else {
         m_buffers.push_front(ByteArray(m_head, pdk::Uninitialized));
         ++m_tailBuffer;
      }
   }
   m_head -= static_cast<int>(bytes);
   m_bufferSize += bytes;
   return m_buffers.front().getRawData() + m_head;
}

void RingBuffer::chop(pint64 bytes)
{
   PDK_ASSERT(bytes <= m_bufferSize);
   while (bytes > 0) {
      if (m_tailBuffer == 0 || m_tail > bytes) {
         // keep a single block around if it does not exceed
         // the basic block size, to avoid repeated allocations
         // between uses of the buffer
         if (m_bufferSize <= bytes) {
            if (m_buffers.front().size() <= m_basicBlockSize) {
               m_bufferSize = 0;
               m_head = 0;
               m_tail = 0;
            } else {
               clear(); // try to minify/squeeze us
            }
         } else {
            PDK_ASSERT(bytes < MAX_BYTE_ARRAY_SIZE);
            m_tail -= static_cast<int>(bytes);
            m_bufferSize -= bytes;
         }
         return;
      }
      m_bufferSize -= m_tail;
      bytes -= m_tail;
      m_buffers.pop_back();
      --m_tailBuffer;
      m_tail = m_buffers.back().size();
   }
}

void RingBuffer::clear()
{
   if (m_buffers.empty()) {
      return;
   }
   auto iter = m_buffers.begin();
   ++iter;
   m_buffers.erase(iter, m_buffers.end());
   m_buffers.front().clear();
   m_head = 0;
   m_tail = 0;
   m_tailBuffer = 0;
   m_bufferSize = 0;
}

pdk::pint64 RingBuffer::indexOf(char c, pint64 maxLength, pint64 pos) const
{
   if (maxLength <= 0 || pos < 0) {
      return -1;
   }
   pdk::pint64 index = -(pos + m_head);
   for (size_t i = 0; i < m_buffers.size(); ++i) {
      const pdk::pint64 nextBlockIndex = std::min(index + (i == static_cast<size_t>(m_tailBuffer) ? m_tail : retrive_at(m_buffers, i).size()), maxLength);
      if (nextBlockIndex > 0) {
         const char *ptr = retrive_at(m_buffers, i).getConstRawData();
         if (index < 0) {
            ptr -= index;
            index = 0;
         }
         const char *findPtr = reinterpret_cast<const char *>(std::memchr(ptr, c,
                                                                          nextBlockIndex - index));
         if (findPtr) {
            return static_cast<pdk::pint64>(findPtr - ptr) + index + pos;
         }
         if (nextBlockIndex == maxLength) {
            return -1;
         }
      }
      index = nextBlockIndex;
   }
   return -1;
}

pdk::pint64 RingBuffer::read(char *data, pint64 maxLength)
{
   const pdk::pint64 bytesToRead = std::min(size(), maxLength);
   pdk::pint64 readSoFar = 0;
   while (readSoFar < bytesToRead) {
      const pdk::pint64 bytesToReadFromThisBlock = std::min(bytesToRead - readSoFar,
                                                            nextDataBlockSize());
      if (data) {
         std::memcpy(data + readSoFar, readPointer(), bytesToReadFromThisBlock);
      }
      
      readSoFar += bytesToReadFromThisBlock;
      free(bytesToReadFromThisBlock);
   }
   return readSoFar;
}

ByteArray RingBuffer::read()
{
   if (m_bufferSize == 0) {
      return ByteArray();
   }
   ByteArray qba(m_buffers.front());
   m_buffers.pop_front();
   qba.reserve(0); // avoid that resizing needlessly reallocates
   if (m_tailBuffer == 0) {
      qba.resize(m_tail);
      m_tail = 0;
   } else {
      --m_tailBuffer;
   }
   qba.remove(0, m_head); // does nothing if head is 0
   m_head = 0;
   m_bufferSize -= qba.size();
   return qba;
}

pdk::pint64 RingBuffer::peek(char *data, pdk::pint64 maxLength, pdk::pint64 pos) const
{
   pdk::pint64 readSoFar = 0;
   
   if (pos >= 0) {
      pos += m_head;
      for (size_t i = 0; readSoFar < maxLength && i < m_buffers.size(); ++i) {
         pdk::pint64 blockLength = (i == static_cast<size_t>(m_tailBuffer) ? m_tail : retrive_at(m_buffers, i).size());
         if (pos < blockLength) {
            blockLength = std::min(blockLength - pos, maxLength - readSoFar);
            std::memcpy(data + readSoFar, retrive_at(m_buffers, i).getConstRawData() + pos, blockLength);
            readSoFar += blockLength;
            pos = 0;
         } else {
            pos -= blockLength;
         }
      }
   }
   return readSoFar;
}

void RingBuffer::append(const char *data, pdk::pint64 size)
{
   char *writePointer = reserve(size);
   if (size == 1) {
      *writePointer = *data;
   } else if (size) {
      std::memcpy(writePointer, data, size);
   }
   
}

void RingBuffer::append(const ByteArray &qba)
{
   if (m_tail == 0) {
      if (m_buffers.empty()) {
         m_buffers.push_back(qba);
      } else {
         m_buffers.back() = qba;
      }
   } else {
      m_buffers.back().resize(m_tail);
      m_buffers.push_back(qba);
      ++m_tailBuffer;
   }
   m_tail = qba.size();
   m_bufferSize += m_tail;
}

pdk::pint64 RingBuffer::readLine(char *data, pdk::pint64 maxLength)
{
   if (!data || --maxLength <= 0) {
      return -1;
   }
   pdk::pint64 i = indexOf('\n', maxLength);
   i = read(data, i >= 0 ? (i + 1) : maxLength);
   // Terminate it.
   data[i] = '\0';
   return i;
}

} // internal
} // ds
} // pdk
