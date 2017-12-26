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
// Created by softboy on 2017/12/15.

#include "pdk/base/ds/ByteArray.h"
#include "pdk/utils/MemoryHelper.h"
#include "pdk/base/lang/Character.h"
#include "pdk/base/ds/internal/ByteArrayMatcher.h"
#include "pdk/kernel/internal/StringAlgorithms.h"

#include <limits.h>

#define PDK_BA_IS_RAW_DATA(data)\
   ((data)->m_offset != sizeof(pdk::ds::ByteArrayData))

namespace pdk {
namespace ds {

namespace
{
inline bool is_upper(char c)
{
   return c >= 'A' && c <= 'Z';
}

inline char to_lower(char c)
{
   if (c >= 'A' && c <= 'Z') {
      return c - 'A' + 'a';
   }
   return c;
}

}

ByteArray::ByteArray(const char *data, int size)
{
   if (!data) {
      m_data = Data::getSharedNull();
   } else {
      if (size < 0) {
         size = static_cast<int>(std::strlen(data));
      }
      if (!size) {
         m_data = Data::allocate(0);
      } else {
         m_data = Data::allocate(static_cast<uint>(size) + 1u);
         PDK_CHECK_ALLOC_PTR(m_data);
         m_data->m_size = size;
         std::memcpy(m_data->getData(), data, size);
         m_data->getData()[size] = '\0';
      }
   }
}

ByteArray::ByteArray(int size, char data)
{
   if (size <= 0) {
      m_data = Data::allocate(0);
   } else {
      m_data = Data::allocate(static_cast<uint>(size) + 1u);
      PDK_CHECK_ALLOC_PTR(m_data);
      m_data->m_size = size;
      std::memset(m_data->getData(), data, size);
      m_data->getData()[size] = '\0';
   }
}

ByteArray::ByteArray(int size, Initialization)
{
   m_data = Data::allocate(static_cast<uint>(size) + 1u);
   PDK_CHECK_ALLOC_PTR(m_data);
   m_data->m_size = size;
   m_data->getData()[size] = '\0';
}

ByteArray ByteArray::fromRawData(const char *data, int size)
{
   Data *ptr;
   if (!data) {
      ptr = Data::getSharedNull();
   } else if (!size) {
      ptr = Data::allocate(0);
   } else {
      ptr = Data::fromRawData(data, size);
      PDK_CHECK_ALLOC_PTR(ptr);
   }
   ByteArrayDataPtr dataPtr{ ptr };
   return ByteArray(dataPtr);
}

ByteArray ByteArray::fromBase64(const ByteArray &base64, Base64Options options)
{
   unsigned int buf = 0;
   int nbits = 0;
   ByteArray temp((base64.size() * 3) / 4, pdk::Initialization::Uninitialized);
   
   int offset = 0;
   for (int i = 0; i < base64.size(); ++i) {
      int ch = base64.at(i);
      int d;
      
      if (ch >= 'A' && ch <= 'Z') {
         d = ch - 'A';
      } else if (ch >= 'a' && ch <= 'z') {
         d = ch - 'a' + 26;
      } else if (ch >= '0' && ch <= '9') {
         d = ch - '0' + 52;
      } else if (ch == '+' && (options & Base64Option::Base64UrlEncoding) == 0) {
         d = 62; 
      } else if (ch == '-' && (options & Base64Option::Base64UrlEncoding) != 0) {
         d = 62;
      } else if (ch == '/' && (options & Base64Option::Base64UrlEncoding) == 0) {
         d = 63;
      } else if (ch == '_' && (options & Base64Option::Base64UrlEncoding) != 0) {
         d = 63;
      } else {
         d = -1;
      }
      if (d != -1) {
         buf = (buf << 6) | d;
         nbits += 6;
         if (nbits >= 8) {
            nbits -= 8;
            temp[offset++] = buf >> nbits;
            buf &= (1 << nbits) - 1;
         }
      }
   }
   temp.truncate(offset);
   return temp;
}

ByteArray ByteArray::fromBase64(const ByteArray &base64)
{
   return fromBase64(base64, Base64Option::Base64Encoding);  
}

ByteArray ByteArray::fromHex(const ByteArray &hexEncoded)
{
   ByteArray result((hexEncoded.size() + 1) / 2, pdk::Initialization::Uninitialized);
   uchar *resultDataPtr = reinterpret_cast<uchar *>(result.getRawData()) + result.size();
   bool oddDigit = true;
   for (int i = hexEncoded.size() - 1; i >= 0; --i) {
      uchar ch = static_cast<uchar>(hexEncoded.at(i));
      int temp = pdk::from_hex(ch);
      if (temp == -1) {
         continue;
      }
      if (oddDigit) {
         --resultDataPtr;
         *resultDataPtr = temp;
         oddDigit = false;
      } else {
         *resultDataPtr |= temp << 4;
         oddDigit = true;
      }
   }
   result.remove(0, resultDataPtr - reinterpret_cast<const uchar *>(result.getConstRawData()));
   return result;
}

void ByteArray::reallocData(uint alloc, Data::AllocationOptions options)
{
   if (m_data->m_ref.isShared() || PDK_BA_IS_RAW_DATA(m_data)) {
      Data *ptr = Data::allocate(alloc, options);
      PDK_CHECK_ALLOC_PTR(ptr);
      ptr->m_size = std::min(static_cast<int>(alloc) - 1, m_data->m_size);
      std::memcpy(ptr->getData(), m_data->getData(), ptr->m_size);
      ptr->getData()[ptr->m_size] = '\0';
      if (!m_data->m_ref.deref()) {
         Data::deallocate(m_data);
      }
      m_data = ptr;
   } else {
      size_t blockSize;
      if (options & Data::Grow) {
         pdk::utils::CalculateGrowingBlockSizeResult allocResult = 
               pdk::utils::calculate_growing_block_size(alloc, 
                                                        sizeof(pdk::lang::Character),
                                                        sizeof(Data));
         blockSize = allocResult.m_size;
         alloc = static_cast<uint>(allocResult.m_elementCount);
      } else {
         blockSize = pdk::utils::calculate_block_size(alloc, 
                                                      sizeof(pdk::lang::Character),
                                                      sizeof(Data));
      }
      Data *ptr = static_cast<Data *>(std::realloc(m_data, blockSize));
      PDK_CHECK_ALLOC_PTR(ptr);
      ptr->m_alloc = alloc;
      ptr->m_capacityReserved = (options & Data::CapacityReserved) ? 1 : 0;
      m_data = ptr;
   }
}

void ByteArray::expand(int i)
{
   resize(std::max(i + 1, m_data->m_size));
}

ByteArray ByteArray::nullTerminated() const
{
   if (!PDK_BA_IS_RAW_DATA(m_data)) {
      return *this;
   }
   ByteArray copy(*this);
   copy.detach();
   return copy;
}

namespace
{

template <typename T>
PDK_NEVER_INLINE
ByteArray to_case_template(T &input, const uchar *table)
{
   // find the first bad character in input
   const char *origBegin = input.constBegin();
   const char *firstBad = origBegin;
   const char *end = input.constEnd();
   for ( ; firstBad != end ; ++firstBad) {
      uchar ch = static_cast<uchar>(*firstBad);
      uchar converted = table[ch];
      if (ch != converted) {
         break;
      }
   }
   if (firstBad == end) {
      return std::move(input);
   }
   // transform the rest
   ByteArray s = std::move(input);    // will copy if T is const ByteArray
   char *b = s.begin();            // will detach if necessary
   char *p = b + (firstBad - origBegin);
   end = b + s.size();
   for ( ; p != end; ++p) {
      *p = char(static_cast<uchar>(table[static_cast<uchar>(*p)]));
   }
   return s;
}

#define REHASH(a) \
   if (needleLengthMinusOne < sizeof(uint) * CHAR_BIT) \
   hashHayStack -= (a) << needleLengthMinusOne; \
   hashHayStack <<= 1

int last_indexof_helper(const char *haystack, int haystackLength, const char *needle, 
                        int needleLength, int from)
{
   int delta = haystackLength - needleLength;
   if (from < 0) {
      from = delta;
   }
   if (from < 0 || from > haystackLength) {
      return -1;
   }
   if (from > delta) {
      from = delta;
   }
   const char *end = haystack;
   haystack += from;
   const uint needleLengthMinusOne = needleLength - 1;
   const char *n = needle + needleLengthMinusOne;
   const char *h = haystack + needleLengthMinusOne;
   uint hashNeedle = 0;
   uint hashHayStack = 0;
   int idx;
   for (idx = 0; idx < needleLength; ++idx) {
      hashNeedle = ((hashNeedle << 1) + *(n - idx));
      hashHayStack = ((hashHayStack << 1) + *(h - idx));
   }
   hashHayStack -= *haystack;
   while (haystack >= end) {
      hashHayStack += *haystack;
      if (hashHayStack == hashNeedle && std::memcmp(needle, haystack, needleLength) == 0) {
         return haystack - end;
      }
      --haystack;
      REHASH(*(haystack + needleLength));
   }
   return -1;
}

inline ByteArray &bytearray_insert(ByteArray *array, int pos, const char *arr, int length)
{
   PDK_ASSERT(pos >= 0);
   if (pos < 0 || length <= 0 || arr == nullptr) {
      return *array;
   }
   int oldSize = array->size();
   array->resize(std::max(pos, oldSize) + length);
   char *dest = array->getRawData();
   if (pos > oldSize) {
      std::memset(dest + oldSize, 0x20, pos - oldSize);
   } else {
      std::memmove(dest + pos + length, dest + pos, oldSize - pos);
   }
   std::memcpy(dest + pos, arr, length);
   return *array;
}

}

ByteArray ByteArray::toLowerHelper(const ByteArray &a)
{
   return to_case_template(a, latin1Lowercased);
}

ByteArray ByteArray::toLowerHelper(ByteArray &a)
{
   return to_case_template(a, latin1Lowercased);
}

ByteArray ByteArray::toUpperHelper(const ByteArray &a)
{
   return to_case_template(a, latin1Uppercased);
}

ByteArray ByteArray::toUpperHelper(ByteArray &a)
{
   return to_case_template(a, latin1Uppercased);
}

ByteArray ByteArray::trimmedHelper(ByteArray &a)
{
   return pdk::internal::StringAlgorithms<ByteArray>::trimmed(a);
}

ByteArray ByteArray::trimmedHelper(const ByteArray &a)
{
   return pdk::internal::StringAlgorithms<const ByteArray>::trimmed(a);
}

ByteArray ByteArray::simplifiedHelper(ByteArray &a)
{
   return pdk::internal::StringAlgorithms<ByteArray>::simplified(a);
}

ByteArray ByteArray::simplifiedHelper(const ByteArray &a)
{
   return pdk::internal::StringAlgorithms<const ByteArray>::simplified(a);
}

ByteArray &ByteArray::operator=(const ByteArray &other) noexcept
{
   other.m_data->m_ref.ref();
   if (!m_data->m_ref.deref()) {
      Data::deallocate(m_data);
   }
   m_data = other.m_data;
   return *this;
}

ByteArray &ByteArray::operator =(const char *str)
{
   Data *ptr;
   if (!str) {
      ptr = Data::getSharedNull();
   } else if (!*str) {
      ptr = Data::allocate(0);
   } else {
      const int length = static_cast<int>(std::strlen(str));
      const uint fullLength = length + 1;
      if (m_data->m_ref.isShared() || fullLength > m_data->m_alloc
          || (length < m_data->m_size && fullLength < static_cast<uint>((m_data->m_alloc >> 1)))) {
         reallocData(fullLength, m_data->detachFlags());
      }
      ptr = m_data;
      std::memcpy(ptr->getData(), str, fullLength);
      ptr->m_size = length;
   }
   ptr->m_ref.ref();
   if (!m_data->m_ref.deref()) {
      Data::deallocate(m_data);
   }
   m_data = ptr;
   return *this;
}

void ByteArray::resize(int size)
{
   if (size < 0) {
      size = 0;
   }
   if (PDK_BA_IS_RAW_DATA(m_data) && !m_data->m_ref.isShared() && size < m_data->m_size) {
      m_data->m_size = size;
      return;
   }
   if (0 == size && !m_data->m_capacityReserved) {
      Data *ptr = Data::allocate(0);
      if (!m_data->m_ref.deref()) {
         Data::deallocate(m_data);
      }
      m_data = ptr;
   } else if (m_data->m_size == 0 &&m_data->m_ref.isStatic()) {
      //
      // Optimize the idiom:
      //    ByteArray a;
      //    a.resize(sz);
      //    ...
      //
      Data *ptr = Data::allocate(static_cast<uint>(size) + 1u);
      PDK_CHECK_ALLOC_PTR(ptr);
      ptr->m_size = size;
      ptr->getData()[size] = '\0';
      m_data = ptr;
   } else {
      if (m_data->m_ref.isShared() || static_cast<uint>(size) + 1u > m_data->m_alloc
          || (!m_data->m_capacityReserved && size < m_data->m_size
              && static_cast<uint>(size) + 1u < static_cast<uint>((m_data->m_alloc >> 1))))
      {
         reallocData(static_cast<uint>(size) + 1u, m_data->detachFlags() | Data::Grow);
      }
      if (m_data->m_alloc) {
         m_data->m_size = size;
         m_data->getData()[size] = '\0';
      }
   }
}

ByteArray &ByteArray::fill(char c, int size)
{
   resize(size < 0 ? m_data->m_size : size);
   if (m_data->m_size) {
      std::memset(m_data->getData(), c, m_data->m_size);
   }
   return *this;
}

void ByteArray::clear()
{
   if (!m_data->m_ref.deref()) {
      Data::deallocate(m_data);
   }
   m_data = Data::getSharedNull();
}

namespace internal {

int find_byte_array(const char *haystack, int haystackLength, int from,
                    const char *needle, int needleLength);

}

int ByteArray::indexOf(const ByteArray &needle, int from) const
{
   const int searchedLength = needle.m_data->m_size;
   if (searchedLength == 0) {
      return from;
   }
   if (searchedLength == 1) {
      return indexOf(*needle.m_data->getData(), from);
   }
   const int selfLength = m_data->m_size;
   if (from > selfLength || searchedLength + from > selfLength) {
      return -1;
   }
   return internal::find_byte_array(m_data->getData(), m_data->m_size, from, 
                                    needle.m_data->getData(), searchedLength);
}

int ByteArray::indexOf(const char *needle, int from) const
{
   const int searchedLength = pdk::strlen(needle);
   if (searchedLength == 1) {
      return indexOf(*needle, from);
   }
   const int selfLength = m_data->m_size;
   if (from > selfLength || searchedLength + from > selfLength) {
      return -1;
   }
   if (searchedLength == 0) {
      return from;
   }
   return internal::find_byte_array(m_data->getData(), m_data->m_size, from, 
                                    needle, searchedLength);
}

int ByteArray::indexOf(char needle, int from) const
{
   if (from < 0) {
      from = std::max(from + m_data->m_size, 0);
   }
   const char *dataPtr = m_data->getData();
   if (from < m_data->m_size) {
      const char *iter = dataPtr + from - 1;
      const char *end = dataPtr + m_data->m_size;
      while (++iter != end) {
         if (*iter == needle) {
            return iter - dataPtr;
         }
      }
   }
   return -1;
}

int ByteArray::lastIndexOf(const ByteArray &needle, int from) const
{
   const int needleLength = needle.m_data->m_size;
   if (needleLength == 1) {
      return lastIndexOf(*needle.m_data->getData(), from);
   }
   return last_indexof_helper(m_data->getData(), m_data->m_size, 
                              needle.m_data->getData(), needleLength, from);
}

int ByteArray::lastIndexOf(const char *needle, int from) const
{
   const int needleLength = pdk::strlen(needle);
   if (needleLength == 1) {
      return lastIndexOf(*needle, from);
   }
   return last_indexof_helper(m_data->getData(), m_data->m_size, 
                              needle, needleLength, from);
}

int ByteArray::lastIndexOf(char needle, int from) const
{
   if (from < 0) {
      from += m_data->m_size;
   } else if (from > m_data->m_size) {
      from = m_data->m_size - 1;
   }
   if (from >= 0) {
      const char *end = m_data->getData();
      const char *start = m_data->getData() + from + 1;
      while (start-- != end) {
         if (*start == needle) {
            return start - end;
         }
      }
   }
   return -1;
}

int ByteArray::count(const ByteArray &array) const
{
   int num = 0;
   int pos = -1;
   if (m_data->m_size > 500 && array.m_data->m_size > 5) {
      internal::ByteArrayMatcher matcher(array);
      while ((pos = matcher.findIndex(*this, pos + 1)) != -1) {
         ++num;
      }
   } else {
      while ((pos = indexOf(array, pos + 1)) != -1) {
         ++num;
      }
   }
   return num;
}

int ByteArray::count(const char *array) const
{
   return count(ByteArray::fromRawData(array, pdk::strlen(array)));
}

int ByteArray::count(char c) const
{
   int num = 0;
   const char *i = m_data->getData() + m_data->m_size;
   const char *b = m_data->getData();
   while (i != b) {
      if (*--i == c) {
         ++num;
      }
   }
   return num;
}

ByteArray ByteArray::left(int length) const
{
   if (length >= m_data->m_size) {
      return *this;
   }
   if (length < 0) {
      length = 0;
   }
   return ByteArray(m_data->getData(), length);
}

ByteArray ByteArray::right(int length) const
{
   if (length >= m_data->m_size) {
      return *this;
   }
   if (length < 0) {
      length = 0;
   }
   return ByteArray(m_data->getData() + m_data->m_size - length, length);
}

ByteArray ByteArray::mid(int index, int length) const
{
   using namespace pdk::ds::internal;
   ContainerImplHelper::CutResult result = ContainerImplHelper::mid(size(), &index, &length);
   if (result == ContainerImplHelper::CutResult::Null) {
      return ByteArray();
   } else if (result == ContainerImplHelper::CutResult::Empty) {
      ByteArrayDataPtr empty = { Data::allocate(0) };
      return ByteArray(empty);
   } else if (result == ContainerImplHelper::CutResult::Full) {
      return *this;
   } else {
      return ByteArray(m_data->getData() + index, length);
   }
}

bool ByteArray::startsWith(const ByteArray &array) const
{
   if (m_data == array.m_data || array.m_data->m_size == 0) {
      return true;
   }
   if (m_data->m_size < array.m_data->m_size) {
      return false;
   }
   return std::memcmp(m_data->getData(), array.m_data->getData(), array.m_data->m_size) == 0;
}

bool ByteArray::startsWith(const char *str) const
{
   if (!str || !*str) {
      return true;
   }
   const int length = static_cast<int>(std::strlen(str));
   if (m_data->m_size < length) {
      return false;
   }
   return pdk::strncmp(m_data->getData(), str, length) == 0;
}

bool ByteArray::startsWith(char c) const
{
   if (m_data->m_size == 0) {
      return false;
   }
   return m_data->getData()[0] == c;
}

bool ByteArray::endsWith(const ByteArray &array) const
{
   if (m_data == array.m_data || array.m_data->m_size == 0) {
      return true;
   }
   if (m_data->m_size < array.m_data->m_size) {
      return false;
   }
   return std::memcmp(m_data->getData() + m_data->m_size - array.m_data->m_size,
                      array.m_data->getData(),
                      array.m_data->m_size) == 0;
}

bool ByteArray::endsWith(const char *str) const
{
   if (!str || !*str) {
      return true;
   }
   const int length = static_cast<int>(std::strlen(str));
   if (m_data->m_size < length) {
      return false;
   }
   return pdk::strncmp(m_data->getData() + m_data->m_size - length, str, length) == 0;
}

bool ByteArray::endsWith(const char c) const
{
   if (m_data->m_size == 0) {
      return false;
   }
   return m_data->getData()[m_data->m_size - 1] == c;
}

ByteArray ByteArray::leftJustified(int width, char fill, bool truncate) const
{
   ByteArray result;
   int length = m_data->m_size;
   int padLength = width - length;
   if (padLength > 0) {
      result.resize(length + padLength);
      if (length) {
         std::memcpy(result.getRawData(), m_data->getData(), length);
      }
      std::memset(result.m_data->getData() + length, fill, padLength);
   } else {
      if (truncate) {
         result = left(width);
      } else {
         result = *this;
      }
   }
   return result;
}

ByteArray ByteArray::rightJustified(int width, char fill, bool truncate) const
{
   ByteArray result;
   int length = m_data->m_size;
   int padLength = width - length;
   if (padLength > 0) {
      result.resize(length + padLength);
      if (length) {
         std::memcpy(result.m_data->getData() + padLength, getRawData(), length);
      }
      std::memset(result.m_data->getData(), fill, padLength);
   } else {
      if (truncate) {
         result = left(width);
      } else {
         result = *this;
      }
   }
   return result;
}

ByteArray &ByteArray::prepend(const ByteArray &array)
{
   if (m_data->m_size == 0 && m_data->m_ref.isStatic() && !PDK_BA_IS_RAW_DATA(array.m_data)) {
      *this = array;
   } else if (array.m_data->m_size != 0) {
      ByteArray temp = *this;
      *this = array;
      append(temp);
   }
   return *this;
}

ByteArray &ByteArray::prepend(const char *str)
{
   return prepend(str, pdk::strlen(str));
}

ByteArray &ByteArray::prepend(const char *str, int length)
{
   if (str) {
      if (m_data->m_ref.isShared() || 
          static_cast<uint>(m_data->m_size + length) + 1u > m_data->m_alloc) {
         reallocData(static_cast<uint>(m_data->m_size + length) + 1u, m_data->detachFlags() | Data::Grow);
      }
      std::memmove(m_data->getData() + length, m_data->getData(), m_data->m_size);
      std::memcpy(m_data->getData(), str, length);
      m_data->m_size += length;
      m_data->getData()[m_data->m_size] = '\0';
   }
   return *this;
}

ByteArray &ByteArray::prepend(char c)
{
   if (m_data->m_ref.isShared() || 
       static_cast<uint>(m_data->m_size) + 2u > m_data->m_alloc) {
      reallocData(static_cast<uint>(m_data->m_size) + 2u, m_data->detachFlags() | Data::Grow);
   }
   std::memmove(m_data->getData() + 1, m_data->getData(), m_data->m_size);
   m_data->getData()[0] = c;
   ++m_data->m_size;
   m_data->getData()[m_data->m_size] = '\0';
   return *this;
}

ByteArray &ByteArray::append(const ByteArray &array)
{
   if (m_data->m_size == 0 && m_data->m_ref.isStatic() && !PDK_BA_IS_RAW_DATA(array.m_data)) {
      *this = array;
   } else if (array.m_data->m_size != 0) {
      if (m_data->m_ref.isShared() || 
          static_cast<uint>(m_data->m_size + array.m_data->m_size) + 1u > m_data->m_alloc) {
         reallocData(static_cast<uint>(m_data->m_size + array.m_data->m_size) + 1u, m_data->detachFlags() | Data::Grow);
      }
      std::memcpy(m_data->getData() + m_data->m_size, array.m_data->getData(), array.m_data->m_size);
      m_data->m_size += array.m_data->m_size;
      m_data->getData()[m_data->m_size] = '\0';
   }
   return *this;
}

ByteArray &ByteArray::append(const char *str)
{
   if (str) {
      const int length = static_cast<int>(std::strlen(str));
      if (m_data->m_ref.isShared() || 
          static_cast<uint>(m_data->m_size + length) + 1u > m_data->m_alloc) {
         reallocData(static_cast<uint>(m_data->m_size + length) + 1u, m_data->detachFlags() | Data::Grow);
      }
      std::memcpy(m_data->getData() + m_data->m_size, str, length + 1);
      m_data->m_size += length;
   }
   return *this;
}

ByteArray &ByteArray::append(const char *str, int length)
{
   if (length < 0) {
      length = pdk::strlen(str);
   }
   if (str && length) {
      if (m_data->m_ref.isShared() || 
          static_cast<uint>(m_data->m_size + length) + 1u > m_data->m_alloc) {
         reallocData(static_cast<uint>(m_data->m_size + length) + 1u, m_data->detachFlags() | Data::Grow);
      }
      std::memcpy(m_data->getData() + m_data->m_size, str, length);
      m_data->m_size += length;
      m_data->getData()[m_data->m_size] = '\0';
   }
   return *this;
}

ByteArray &ByteArray::append(char c)
{
   if (m_data->m_ref.isShared() || static_cast<uint>(m_data->m_size) + 2u > m_data->m_alloc) {
      reallocData(static_cast<uint>(m_data->m_size) + 2u, m_data->detachFlags() | Data::Grow);
   }
   m_data->getData()[m_data->m_size++] = c;
   m_data->getData()[m_data->m_size] = '\0';
   return *this;
}

ByteArray &ByteArray::insert(int pos, const ByteArray &array)
{
   ByteArray copy(array);
   return bytearray_insert(this, pos, copy.m_data->getData(), copy.m_data->m_size);
}

ByteArray &ByteArray::insert(int pos, const char *str)
{
   return bytearray_insert(this, pos, str, pdk::strlen(str));
}

ByteArray &ByteArray::insert(int pos, char c)
{
   return bytearray_insert(this, pos, &c, 1);
}

ByteArray &ByteArray::insert(int pos, const char *str, int length)
{
   return bytearray_insert(this, pos, str, length);
}

ByteArray &ByteArray::insert(int pos, int count, char c)
{
   if (pos < 0 || count <= 0) {
      return *this;
   }
   int oldSize = size();
   resize(std::max(pos, oldSize) + count);
   char *dest = m_data->getData();
   if (pos > oldSize) {
      std::memset(dest + oldSize, 0x20, pos - oldSize);
   } else if (pos < oldSize){
      std::memmove(dest + pos + count, dest + pos, oldSize - pos);
   }
   std::memset(dest + pos, c, count);
   return *this;
}

ByteArray &ByteArray::remove(int index, int length)
{
   if (length <= 0 || static_cast<uint>(index) >= static_cast<uint>(m_data->m_size)) {
      return *this;
   }
   detach();
   if (length >= m_data->m_size - index) {
      resize(index);
   } else {
      std::memmove(m_data->getData() + index, m_data->getData() + index + length, 
                   m_data->m_size - index - length);
      resize(m_data->m_size - length);
   }
   return *this;
}

ByteArray &ByteArray::replace(int index, int length, const ByteArray &after)
{
   if (length == after.m_data->m_size && (index + length) <= m_data->m_size) {
      detach();
      std::memmove(m_data->getData() + index, after.m_data->getData(), length * sizeof(char));
      return *this;
   } else {
      ByteArray copy(after);
      // @TODO optimize me
      remove(index, length);
      return insert(index, copy);
   }
}

ByteArray &ByteArray::replace(int index, int length, const char *after)
{
   return replace(index, length, after, pdk::strlen(after));
}

ByteArray &ByteArray::replace(int index, int length, const char *after, int alength)
{
   if (length == alength && (index + length <= m_data->m_size)) {
      detach();
      std::memcpy(m_data->getData() + index, after, length * sizeof(char));
      return *this;
   } else {
      remove(index, length);
      return bytearray_insert(this, index, after, alength);
   }
}

ByteArray &ByteArray::replace(const ByteArray &before, const ByteArray &after)
{
   if (isNull() || before.m_data == after.m_data) {
      return *this;
   }
   ByteArray replacement = after;
   if (after.m_data == m_data) {
      replacement.detach();
   }
   return replace(before.getConstRawData(), before.size(), 
                  replacement.getConstRawData(), replacement.size());
}

ByteArray &ByteArray::replace(const char *before, const ByteArray &after)
{
   ByteArray replacement = after;
   if (after.m_data == m_data) {
      replacement.detach();      
   }
   return replace(before, pdk::strlen(before), 
                  replacement.getConstRawData(), replacement.size());
}

ByteArray &ByteArray::replace(const char *before, int blength, const char *after, int alength)
{
   if (isNull() || (before == after && blength == alength)) {
      return *this;
   }
   const char *a = after;
   const char *b = before;
   char *selfDataPtr = m_data->getData();
   int selfLength = m_data->m_size;
   if (after >= selfDataPtr && after < selfDataPtr + selfLength) {
      char *copy = static_cast<char *>(std::malloc(alength));
      PDK_CHECK_ALLOC_PTR(copy);
      std::memcpy(copy, after, alength);
      a = copy;
   }
   if (before >= selfDataPtr && before < selfDataPtr + selfLength) {
      char *copy = static_cast<char *>(std::malloc(blength));
      PDK_CHECK_ALLOC_PTR(copy);
      std::memcpy(copy, before, blength);
      b = copy;
   }
   
   internal::ByteArrayMatcher matcher(before, blength);
   int index = 0;
   
   if (blength == alength) {
      if (blength) {
         while ((index = matcher.findIndex(*this, index)) != -1) {
            std::memcpy(selfDataPtr + index, after, alength);
            index += blength;
         }
      }
   } else if (alength < blength){
      uint to = 0;
      uint movestart = 0;
      uint num = 0;
      while ((index = matcher.findIndex(*this, index)) != -1) {
         if (num) {
            int msize = index - movestart;
            if (msize > 0) {
               std::memmove(selfDataPtr + to, selfDataPtr + movestart, msize);
               to += msize;
            }
         } else {
            to = index;
         }
         if (alength) {
            std::memcpy(selfDataPtr + to, after, alength);
         }
         index += blength;
         movestart = index;
         ++num;
      }
      if (num) {
         int msize = selfLength - movestart;
         if (msize > 0) {
            std::memmove(selfDataPtr + to, selfDataPtr + movestart, msize);
         }
         resize(selfLength - num * (blength - alength));
      }
   } else {
      // the most complex case. We don't want to lose performance by doing repeated
      // copies and reallocs of the string.
      while (index != -1) {
         uint indices[4096];
         uint pos = 0;
         while(pos < 4095) {
            index = matcher.findIndex(*this, index);
            if (index == -1) {
               break;
            }
            indices[pos++] = index;
            index += blength;
            // avoid infinite loop
            if (!blength) {
               index++;
            }
         }
         if (!pos) {
            break;
         }
         
         // we have a table of replacement positions, use them for fast replacing
         int adjust = pos * (alength - blength);
         // index has to be adjusted in case we get back into the loop above.
         if (index != -1) {
            index += adjust;
         }
         int newlen = selfLength + adjust;
         int moveend = selfLength;
         if (newlen > selfLength) {
            resize(newlen);
            selfLength = newlen;
         }
         selfDataPtr = this->m_data->getData();
         
         while(pos) {
            pos--;
            int movestart = indices[pos] + blength;
            int insertstart = indices[pos] + pos * (alength - blength);
            int moveto = insertstart + alength;
            std::memmove(selfDataPtr + moveto, selfDataPtr + movestart, (moveend - movestart));
            if (alength) {
               std::memcpy(selfDataPtr + insertstart, after, alength);
            }
            moveend = movestart - blength;
         }
      }
   }
   if (a != after) {
      std::free(const_cast<char *>(a));
   }
   if (b != before) {
      std::free(const_cast<char *>(b));
   }
   return *this;
}

ByteArray &ByteArray::replace(char before, const ByteArray &after)
{
   char b[2] = { before, '\0'};
   ByteArray cb = fromRawData(b, 1);
   return replace(cb, after);
}

ByteArray &ByteArray::replace(char before, char after)
{
   if (m_data->m_size) {
      char *i = getRawData();
      char *e = i + m_data->m_size;
      for (; i != e; ++i) {
         if (*i == before) {
            *i = after;
         }
      }
   }
   return *this;
}

ByteArray ByteArray::toBase64(Base64Options options) const
{
   const char alphabetBase64[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
                                 "ghijklmn" "opqrstuv" "wxyz0123" "456789+/";
   const char alphabetBase64url[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
                                    "ghijklmn" "opqrstuv" "wxyz0123" "456789-_";
   const char *const alphabet = options & Base64Option::Base64UrlEncoding
         ? alphabetBase64url
         : alphabetBase64;
   const char padchar = '=';
   int padLength = 0;
   ByteArray temp((m_data->m_size + 2) / 3 * 4, pdk::Initialization::Uninitialized);
   
   int i = 0;
   char *out = temp.getRawData();
   const char *baseDataPtr = m_data->getData();
   while (i < m_data->m_size) {
      // encode 3 bytes at a time
      int chunk = 0;
      chunk |= int(uchar(baseDataPtr[i++])) << 16;
      if (i == m_data->m_size) {
         padLength = 2;
      } else {
         chunk |= int(uchar(baseDataPtr[i++])) << 8;
         if (i == m_data->m_size) {
            padLength = 1;
         } else {
            chunk |= int(uchar(baseDataPtr[i++]));
         }
      }
      
      int j = (chunk & 0x00fc0000) >> 18;
      int k = (chunk & 0x0003f000) >> 12;
      int l = (chunk & 0x00000fc0) >> 6;
      int m = (chunk & 0x0000003f);
      *out++ = alphabet[j];
      *out++ = alphabet[k];
      
      if (padLength > 1) {
         if ((options & Base64Option::OmitTrailingEquals) == 0)
            *out++ = padchar;
      } else {
         *out++ = alphabet[l];
      }
      if (padLength > 0) {
         if ((options & Base64Option::OmitTrailingEquals) == 0) {
            *out++ = padchar;
         }
      } else {
         *out++ = alphabet[m];
      }
   }
   PDK_ASSERT((options & Base64Option::OmitTrailingEquals) || (out == temp.size() + temp.getRawData()));
   if (options & Base64Option::OmitTrailingEquals) {
      temp.truncate(out - temp.getRawData());
   }
   return temp;
}

ByteArray ByteArray::toBase64() const
{
   return toBase64(Base64Option::Base64Encoding);
}

ByteArray ByteArray::toHex() const
{
   ByteArray hex(m_data->m_size * 2, pdk::Initialization::Uninitialized);
   char *hexData = hex.getRawData();
   const uchar *data = reinterpret_cast<const uchar *>(m_data->getData());
   for (int i = 0; i < m_data->m_size; ++i) {
      hexData[i*2] = pdk::to_hex_lower(data[i] >> 4);
      hexData[i*2+1] = pdk::to_hex_lower(data[i] & 0xf);
   }
   return hex;
}

std::list<ByteArray> ByteArray::split(char sep) const
{
   std::list<ByteArray> list;
   int start = 0;
   int end;
   while ((end = indexOf(sep, start)) != -1) {
      list.push_back(mid(start, end - start));
      start = end + 1;
   }
   list.push_back(mid(start));
   return list;
}

ByteArray ByteArray::repeated(int times) const
{
   if (m_data->m_size == 0) {
      return *this;
   }
   if (times <= 1) {
      if (times == 1) {
         return *this;
      }
      return ByteArray();
   }
   int selfLength = m_data->m_size;
   const int resultSize = times * selfLength;
   ByteArray result;
   result.reserve(resultSize);
   if (result.m_data->m_alloc != static_cast<uint>(resultSize) + 1u) {
      return ByteArray(); // not enough memory
   }
   char *resultDataPtr = result.m_data->getData();
   char *selfDataPtr = m_data->getData();
   std::memcpy(resultDataPtr, selfDataPtr, selfLength);
   int currentSize = selfLength;
   char *writer = resultDataPtr + currentSize;
   const int halfResultSize = resultSize >> 1;
   while (currentSize <= halfResultSize) {
      std::memcpy(writer, resultDataPtr, currentSize);
      writer += currentSize;
      currentSize <<= 1;
   }
   std::memcpy(writer, resultDataPtr, resultSize - currentSize);
   resultDataPtr[resultSize] = '\0';
   result.m_data->m_size = resultSize;
   return result;
}

bool ByteArray::isNull() const
{
   return m_data == Data::getSharedNull();
}

void ByteArray::truncate(int pos)
{
   if (pos < m_data->m_size) {
      resize(pos);
   }
}

void ByteArray::chop(int n)
{
   if (n > 0) {
      resize(m_data->m_size - n);
   }
}

} // ds
} // pdk
