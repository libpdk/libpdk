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
// Created by softboy on 2017/12/30.

#include "pdk/base/ds/BitArray.h"
#include "pdk/global/Endian.h"
#include "pdk/kernel/Algorithms.h"
#include <cstring>

namespace pdk {
namespace ds {

/*
 * BitArray construction note:
 *
 * We overallocate the byte array by 1 byte. The first user bit is at
 * m_data.getRawData()[1]. On the extra first byte, we store the difference between the
 * number of bits in the byte array (including this byte) and the number of
 * bits in the bit array. Therefore, it's always a number between 8 and 15.
 *
 * This allows for fast calculation of the bit array size:
 *    inline int size() const { return (m_data.size() << 3) - *m_data.constData(); }
 *
 * Note: for an array of zero size, *d.constData() is the ByteArray implicit NUL.
 */

BitArray::BitArray(int size, bool value)
   : m_data(size <= 0 ? 0 : 1 + (size + 7) / 8, pdk::Uninitialized)
{
   PDK_ASSERT_X(size >= 0, "BitArray::BitArray", "Size must be greater than or equal to 0.");
   if (size <= 0) {
      return;
   }
   uchar *meta = reinterpret_cast<uchar *>(m_data.getRawData());
   std::memset(meta + 1, value ? 0xff : 0, m_data.size() -1);
   *meta = m_data.size() * 8 - size;
   if (value && size && size % 8) {
      *(meta + 1 + size / 8) &= (1 << (size % 8)) - 1;
   }
}

int BitArray::count(bool on) const
{
   int numBits = 0;
   const puint8 *bits = reinterpret_cast<const puint8 *>(m_data.getRawData()) + 1;
   // the loops below will try to read from *end
   // it's the ByteArray implicit NUL, so it will not change the bit count
   const puint8 *const end = reinterpret_cast<const puint8 *>(m_data.end());
   while (bits + 7 <= end) {
      puint64 v = pdk::from_unaligned<puint64>(bits);
      bits += 8;
      numBits += static_cast<int>(pdk::population_count(v));
   }
   if (bits + 3 <= end) {
      puint32 v = pdk::from_unaligned<puint32>(bits);
      bits += 4;
      numBits += static_cast<int>(v);
   }
   if (bits + 1 < end) {
      puint16 v = pdk::from_unaligned<puint16>(bits);
      bits += 2;
      numBits += static_cast<int>(pdk::population_count(v));
   }
   if (bits < end) {
      numBits += static_cast<int>(pdk::population_count(bits[0]));
   }
   return on ? numBits : size() - numBits;
}

void BitArray::resize(int size)
{
   if (!size) {
      m_data.resize(0);
   } else {
      int byteSize = m_data.size();
      m_data.resize(1 + (size + 7) / 8);
      uchar *data = reinterpret_cast<uchar *>(m_data.getRawData());
      if (size > (byteSize << 3)) {
         std::memset(data + byteSize, 0, m_data.size() - byteSize);
      } else if (size % 8) {
         *(data + 1 + size / 8) &= (1 << (size % 8)) - 1;
      }
      *data = m_data.size() * 8 - size;
   }
}

BitArray &BitArray::operator &=(const BitArray &other)
{
   resize(std::max(size(), other.size()));
   uchar *data1 = reinterpret_cast<uchar *>(m_data.getRawData()) + 1;
   const uchar *data2 = reinterpret_cast<const uchar *>(other.m_data.getRawData()) + 1;
   int otherDataSize = other.m_data.size() - 1;
   int diff = m_data.size() - 1 - otherDataSize;
   while (otherDataSize-- > 0) {
      *data1++ &= *data2++;
   }
   while (diff-- > 0) {
      *data1++ = 0; 
   }
   return *this;
}

BitArray &BitArray::operator |=(const BitArray &other)
{
   resize(std::max(size(), other.size()));
   uchar *data1 = reinterpret_cast<uchar *>(m_data.getRawData()) + 1;
   const uchar *data2 = reinterpret_cast<const uchar *>(other.m_data.getRawData()) + 1;
   int otherDataSize = other.m_data.size() - 1;
   while (otherDataSize-- > 0) {
      *data1++ |= *data2++;
   }
   return *this;
}

BitArray &BitArray::operator ^=(const BitArray &other)
{
   resize(std::max(size(), other.size()));
   uchar *data1 = reinterpret_cast<uchar *>(m_data.getRawData()) + 1;
   const uchar *data2 = reinterpret_cast<const uchar *>(other.m_data.getRawData()) + 1;
   int otherDataSize = other.m_data.size() - 1;
   while (otherDataSize-- > 0) {
      *data1++ ^= *data2++;
   }
   return *this;
}

BitArray BitArray::operator ~() const
{
   int sz = size();
   BitArray result(sz);
   const uchar *data1 = reinterpret_cast<const uchar *>(m_data.getConstRawData()) + 1;
   uchar *data2 = reinterpret_cast<uchar *>(result.m_data.getRawData()) + 1;
   int dataSize = m_data.size() - 1;
   while (dataSize-- > 0) {
      *data2++ = ~*data1++;
   }
   if (sz && sz % 8) {
      *(data2 - 1) &= (1 << (sz % 8)) - 1;
   }
}

BitArray operator &(const BitArray &lhs, const BitArray &rhs)
{
   BitArray temp = lhs;
   temp &= rhs;
   return temp;
}

BitArray operator |(const BitArray &lhs, const BitArray &rhs)
{
   BitArray temp = lhs;
   temp |= rhs;
   return temp;
}

BitArray operator ^(const BitArray &lhs, const BitArray &rhs)
{
   BitArray temp = lhs;
   temp ^= rhs;
   return temp;
}

} // ds
} // pdk

