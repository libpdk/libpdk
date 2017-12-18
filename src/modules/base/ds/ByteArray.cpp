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

} // ds
} // pdk
