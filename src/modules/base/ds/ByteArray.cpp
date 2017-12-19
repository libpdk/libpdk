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

int ByteArray::indexOf(const ByteArray &array, int from) const
{
//   const int searchedLength = array.m_data->m_size;
//   if (searchedLength == 0) {
//      return from;
//   }
//   if (searchedLength == 1) {
//      return indexOf(*array.m_data->getData(), from);
//   }
//   const int selfLength = m_data->m_size;
//   if (from > selfLength || searchedLength + from > selfLength) {
//      return -1;
//   }
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
   if (m_data == array.m_data || array.m_data->m_size) {
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

namespace
{

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
