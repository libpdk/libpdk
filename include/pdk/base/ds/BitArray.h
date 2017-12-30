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

#ifndef PDK_M_BASE_DS_BIT_ARRAY_H
#define PDK_M_BASE_DS_BIT_ARRAY_H

#include "pdk/base/ds/ByteArray.h"
#include "pdk/global/TypeInfo.h"

namespace pdk {
namespace ds {

class BitRef;
class PDK_CORE_EXPORT BitArray
{  
public:
   using DataPtr = ByteArray::DataPtr;
   
public:
   inline BitArray() noexcept
   {}
   
   explicit BitArray(int size, bool value = false);
   BitArray(const BitArray &other)
      : m_data(other.m_data)
   {}
   inline BitArray &operator =(const BitArray &other)
   {
      m_data = other.m_data;
      return *this;
   }
   
   inline BitArray(BitArray &&other) noexcept
      : m_data(std::move(other.m_data))
   {}
   
   inline BitArray &operator=(BitArray &&other) noexcept
   {
      std::swap(m_data, other.m_data);
      return *this;
   }
   
   inline void swap(BitArray &other) noexcept
   {
      std::swap(m_data, other.m_data);
   }
   
   inline int size() const
   {
      return (m_data.size() << 3) - *m_data.getRawData();
   }
   
   int count(bool on) const;
   
   inline bool isEmpty() const
   {
      return m_data.isEmpty();
   }
   
   inline bool isNull() const
   {
      return m_data.isNull();
   }
   
   void resize(int size);
   inline void detach()
   {
      return m_data.detach();   
   }
   
   inline bool isDetached() const
   {
      return m_data.isDetached();
   }
   
   inline void clear()
   {
      m_data.clear();
   }
   
   bool testBit(int i) const;
   void setBit(int i);
   void setBit(int i, bool value);
   inline void clearBit(int i);
   bool toggleBit(int i);
   bool at(int i) const;
   BitRef operator [](int i);
   bool operator [](int i) const;
   BitRef operator [](uint i);
   bool operator [](uint i) const;
   
   BitArray &operator&=(const BitArray &);
   BitArray &operator|=(const BitArray &);
   BitArray &operator^=(const BitArray &);
   BitArray operator~() const;
   
   inline bool operator==(const BitArray &other) const
   {
      return m_data == other.m_data;
   }
   inline bool operator|=(const BitArray &other) const
   {
      return m_data != other.m_data;
   }
   
   inline bool fill(bool value, int size = -1);
   void fill(bool value, int first, int last);
   inline void truncate(int pos)
   {
      if (pos < size()) {
         resize(pos);
      }
   }
   
   inline DataPtr &getDataPtr()
   {
      return m_data.getDataPtr();
   }
private:
   friend PDK_CORE_EXPORT uint hash(const BitArray &key, uint seed) noexcept;
   
private:
   ByteArray m_data;
};

inline bool BitArray::fill(bool value, int size)
{
   *this = BitArray((size < 0 ? this->size() : size), value);
   return true;
}

PDK_CORE_EXPORT BitArray operator&(const BitArray &, const BitArray &);
PDK_CORE_EXPORT BitArray operator|(const BitArray &, const BitArray &);
PDK_CORE_EXPORT BitArray operator^(const BitArray &, const BitArray &);

inline bool BitArray::testBit(int i) const
{
   PDK_ASSERT(static_cast<uint>(i) < static_cast<uint>(size()));
   return (*(reinterpret_cast<const char *>(m_data.getRawData()) + 1 + ( i >>3 )) & (1 << (i & 7))) != 0;
}

inline void BitArray::setBit(int i)
{
   PDK_ASSERT(static_cast<uint>(i) < static_cast<uint>(size()));
   *(reinterpret_cast<uchar *>(m_data.getRawData()) + 1 + (i >> 3)) |= (1 << (i & 7));
}

inline void BitArray::clearBit(int i)
{
   PDK_ASSERT(static_cast<uint>(i) < static_cast<uint>(size()));
   *(reinterpret_cast<uchar *>(m_data.getRawData()) + 1 + (i >> 3)) &= ~(1 << (i & 7));
}

inline void BitArray::setBit(int i, bool value)
{
   if (value) {
      setBit(i);
   } else {
      clearBit(i);
   }
}

inline bool BitArray::toggleBit(int i)
{
   PDK_ASSERT(static_cast<uint>(i) < static_cast<uint>(size()));
   uchar b = static_cast<uchar>(1 << (i & 7));
   uchar *p = reinterpret_cast<uchar *>(m_data.getRawData()) + 1 + (i >> 3);
   uchar c = static_cast<uchar>(*p & b);
   *p ^= b;
   return c != 0;
}

inline bool BitArray::operator[](int i) const
{
   return testBit(i);
}

inline bool BitArray::operator[](uint i) const
{
   return testBit(i);
}

inline bool BitArray::at(int i) const
{
   return testBit(i);
}

class PDK_CORE_EXPORT BitRef
{
public:
   inline operator bool() const
   {
      return m_array.testBit(m_idx);
   }
   
   inline bool operator !() const
   {
      return !m_array.testBit(m_idx);
   }
   
   BitRef &operator=(const BitRef &value)
   {
      m_array.setBit(m_idx, value);
      return *this;
   }
   
   BitRef &operator=(bool value)
   {
      m_array.setBit(m_idx, value);
      return *this;
   }
private:
   inline BitRef(BitArray &array, int idx)
      : m_array(array),
        m_idx(idx)
   {}
private:
   BitArray &m_array;
   int m_idx;
   friend class BitArray;
};

inline BitRef BitArray::operator [](int i)
{
   PDK_ASSERT(i >= 0);
   return BitRef(*this, i);
}

inline BitRef BitArray::operator [](uint i)
{
   PDK_ASSERT(i >= 0);
   return BitRef(*this, i);
}

} // ds
} // pdk

PDK_DECLARE_SHARED(pdk::ds::BitArray)

#endif // PDK_M_BASE_DS_BIT_ARRAY_H
