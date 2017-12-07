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
// Created by softboy on 2017/12/04.

#ifndef PDK_M_BASE_DS_INTERNAL_ARRAYDATA_H
#define PDK_M_BASE_DS_INTERNAL_ARRAYDATA_H

#include "pdk/utils/RefCount.h"

namespace pdk {
namespace ds {
namespace internal {

using pdk::utils::RefCount;

struct PDK_CORE_EXPORT ArrayData
{
   RefCount m_ref;
   int m_size;
   uint m_alloc: 31;
   uint m_capacityReserved: 1;
   pdk::ptrdiff m_offset;
   static const ArrayData sm_sharedNull[2];
   
   enum AllocationOption 
   {
      CapacityReserved = 0x1,
#if !defined(PDK_NO_UNSHARED_CONTAINERS)
      Unsharable = 0x02,
#endif
      RawData = 0x04,
      Grow = 0x08,
      Default = 0
   };
   
   PDK_DECLARE_FLAGS(AllocationOptions, AllocationOption);
   
   void *getData()
   {
      PDK_ASSERT(m_size == 0 || m_offset < 0 || static_cast<size_t>(m_offset) >= sizeof(ArrayData));
      return reinterpret_cast<char *>(this) + m_offset;
   }
   
   const void *getData() const
   {
      PDK_ASSERT(m_size == 0 || m_offset < 0 || static_cast<size_t>(m_offset) >= sizeof(ArrayData));
      return reinterpret_cast<const char *>(this) + m_offset;
   }
   
   bool isMutable() const
   {
      return m_alloc != 0;
   }
   
   size_t detachCapacity(size_t newSize) const
   {
      if (m_capacityReserved && newSize < m_alloc) {
         return m_alloc; 
      }
      return newSize;
   }
   
   AllocationOptions detachFlags() const
   {
      AllocationOptions result;
      if (m_capacityReserved) {
         result |= CapacityReserved;
      }
      return result;
   }
   
   AllocationOptions cloneFlags() const
   {
      AllocationOptions result;
      if (m_capacityReserved) {
         result |= CapacityReserved;
      }
      return result;
   }
   
   static ArrayData *allocate(size_t objectSize, size_t alignment,
                              size_t capacity, AllocationOptions options = Default) noexcept PDK_REQUIRED_RESULT;
   static void deallocate(ArrayData *data, size_t objectSize, size_t alignment) noexcept;
   static ArrayData *getSharedNull() noexcept
   {
      return const_cast<ArrayData *>(sm_sharedNull);
   }
};

//PDK_DECLARE_OPERATORS_FOR_FLAGS(ArrayData::AllocationOptions)

template <typename T>
struct TypedArrayData : ArrayData
{
   class Iterator
   {
   public:
      using IteratorCategory = std::random_access_iterator_tag;
      using DifferenceType = int;
      using ValueType = T;
      using Pointer = T *;
      using Reference = T &;
      
   public:
      inline Iterator()
         : m_pointer(nullptr)
      {}
      
      inline Iterator(T *next)
         : m_pointer(next)
      {}
      
      inline T &operator*() const
      {
         return *m_pointer;
      }
      
      inline T *operator->() const
      {
         return m_pointer;
      }
      
      inline T &operator[](int index) const
      {
         return *(m_pointer + index);
      }
      
      inline bool operator==(const Iterator &other) const
      {
         return m_pointer == other.m_pointer;
      }
      
      inline bool operator!=(const Iterator &other) const
      {
         return m_pointer != other.m_pointer;
      }
      
      inline bool operator<(const Iterator &other) const
      {
         return m_pointer < other.m_pointer;
      }
      
      inline bool operator<=(const Iterator &other) const
      {
         return m_pointer <= other.m_pointer;
      }
      
      inline bool operator>(const Iterator &other) const
      {
         return m_pointer > other.m_pointer;
      }
      
      inline bool operator>=(const Iterator &other) const
      {
         return m_pointer >= other.m_pointer;
      }
      
      inline Iterator &operator++()
      {
         ++m_pointer;
         return *this;
      }
      
      inline Iterator operator++(int)
      {
         T *orig = this;
         ++m_pointer;
         return orig;
      }
      
      inline Iterator &operator--()
      {
         --m_pointer;
         return *this;
      }
      
      inline Iterator operator--(int)
      {
         T *orig = this;
         --m_pointer;
         return orig;
      }
      
      inline Iterator operator+(int value) const
      {
         return Iterator(m_pointer + value);
      }
      
      inline Iterator operator-(int value) const
      {
         return Iterator(m_pointer - value);
      }
      
      inline Iterator &operator+=(int value)
      {
         m_pointer += value;
         return *this;
      }
      
      inline int operator-(const Iterator &value) const
      {
         return m_pointer - value.m_pointer;
      }
      
      inline Iterator &operator-=(int value)
      {
         m_pointer -= value;
         return *this;
      }
      
      inline operator T*() const
      {
         return m_pointer;
      }
   public:
      T *m_pointer;
   };
   
   friend class Iterator;
   
   class ConstIterator
   {
   public:
      using IteratorCategory = std::random_access_iterator_tag;
      using DifferenceType = int;
      using ValueType = T;
      using Pointer = const T *;
      using Reference = const T &;
      
   public:
      inline ConstIterator()
         : m_pointer(nullptr)
      {}
      
      inline ConstIterator(const T *next)
         : m_pointer(next)
      {}
      
      inline explicit ConstIterator(const ConstIterator &other)
         : m_pointer(other.m_pointer)
      {}
      
      inline const T &operator*() const
      {
         return *m_pointer;
      }
      
      inline const T *operator->() const
      {
         return m_pointer;
      }
      
      inline const T &operator[](int index) const
      {
         return *(m_pointer + index);
      }
      
      inline bool operator==(const ConstIterator &other) const
      {
         return m_pointer == other.m_pointer;
      }
      
      inline bool operator!=(const ConstIterator &other) const
      {
         return m_pointer != other.m_pointer;
      }
      
      inline bool operator<(const ConstIterator &other) const
      {
         return m_pointer < other.m_pointer;
      }
      
      inline bool operator<=(const ConstIterator &other) const
      {
         return m_pointer <= other.m_pointer;
      }
      
      inline bool operator>(const ConstIterator &other) const
      {
         return m_pointer > other.m_pointer;
      }
      
      inline bool operator>=(const ConstIterator &other) const
      {
         return m_pointer >= other.m_pointer;
      }
      
      inline ConstIterator &operator++()
      {
         ++m_pointer;
         return *this;
      }
      
      inline ConstIterator &operator++(int)
      {
         T *orig = m_pointer;
         ++m_pointer;
         return orig;
      }
      
      inline ConstIterator &operator--()
      {
         --m_pointer;
         return *this;
      }
      
      inline ConstIterator &operator--(int)
      {
         T *orig = m_pointer;
         --m_pointer;
         return orig;
      }
      
      inline ConstIterator &operator+=(int value)
      {
         m_pointer += value;
         return *this;
      }
      
      inline ConstIterator &operator-=(int value)
      {
         m_pointer -= value;
         return *this;
      }
      
      inline ConstIterator operator-(int value) const
      {
         return ConstIterator(m_pointer - value);
      }
      
      inline ConstIterator operator+(int value) const
      {
         return ConstIterator(m_pointer + value);
      }
      
      inline int operator-(const ConstIterator &value) const
      {
         return m_pointer - value.m_pointer;
      }
      
      inline operator const T *() const
      {
         return m_pointer;
      }
   public:
      const T *m_pointer;
   };
   
   friend class ConstIterator;
   
   T *getData()
   {
      return static_cast<T *>(ArrayData::getData());
   }
   
   const T *getData() const
   {
      return static_cast<const T *>(ArrayData::getData());
   }
   
   Iterator begin(Iterator = Iterator())
   {
      return getData();
   }
   
   Iterator end(Iterator = Iterator())
   {
      return getData() + m_size;
   }
   
   ConstIterator begin(ConstIterator = ConstIterator()) const
   {
      return getData();
   }
   
   ConstIterator end(ConstIterator = ConstIterator()) const
   {
      return getData() + m_size;
   }
   
   ConstIterator constBegin() const
   {
      return getData();
   }
   
   ConstIterator constEnd() const
   {
      return getData() + m_size;
   }
   
   class AlignmentDummy
   {
      ArrayData m_header;
      T m_data;
   };
   
   static TypedArrayData *allocate(size_t capacity,
                                   AllocationOptions options = Default) PDK_REQUIRED_RESULT
   {
      PDK_STATIC_ASSERT(sizeof(TypedArrayData) == sizeof(ArrayData));
      return static_cast<TypedArrayData *>(ArrayData::allocate(
                                                   sizeof(T), alignof(AlignmentDummy), capacity, options));
   }
   
   static void deallocate(ArrayData *data)
   {
      PDK_STATIC_ASSERT(sizeof(TypedArrayData) == sizeof(ArrayData));
      ArrayData::deallocate(data, sizeof(T), alignof(AlignmentDummy));
   }
   
   static TypedArrayData *fromRawData(const T *data, size_t size,
                                      AllocationOptions options = Default)
   {
      PDK_STATIC_ASSERT(sizeof(TypedArrayData) == sizeof(ArrayData));
      TypedArrayData *result = allocate(0, options | RawData);
      if (result) {
         PDK_ASSERT(!result->m_ref.isShared());
         result->m_offset = reinterpret_cast<const char *>(data) 
               - reinterpret_cast<const char *>(result);
         result->m_size = size;
      }
      return result;
   }
   
   static TypedArrayData *getSharedNull() noexcept
   {
      PDK_STATIC_ASSERT(sizeof(TypedArrayData) == sizeof(ArrayData));
      return static_cast<TypedArrayData *>(ArrayData::getSharedNull());
   }
   
   static TypedArrayData *getSharedEmpty()
   {
      PDK_STATIC_ASSERT(sizeof(TypedArrayData) == sizeof(ArrayData));
      return allocate(0);
   }
   
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   static TypedArrayData *getUnsharableEmpty()
   {
      PDK_STATIC_ASSERT(sizeof(TypedArrayData) == sizeof(ArrayData));
      return allocate(0, Unsharable);
   }
#endif
};

template <typename T, size_t N>
struct StaticArrayData
{
   ArrayData m_header;
   T m_data[N];
};

template <typename T>
struct ArrayDataPointerRef
{
   TypedArrayData<T> *m_ptr;
};

#define PDK_STATIC_ARRAY_HEADER_INITIALIZER_WITH_OFFSET(size, offset) \
{PDK_REFCOUNT_INITIALIZE_STATIC, size, 0, 0, offset} \
   /**/

#define PDK_STATIC_ARRAT_DATA_HEADER_INITIALIZER(type, size) \
   PDK_STATIC_ARRAY_HEADER_INITIALIZER_WITH_OFFSET(size, \
   ((sizeof(pdk::ds::internal::ArrayData) + (alignof(type) - 1)) & ~(alignof(type) - 1))) \
   /**/


#define PDK_ARRAY_LITERAL(Type, ...)\
   ([]() -> pdk::ds::internal::ArrayDataPointerRef<Type> {\
      struct StaticWrapper\
      {\
         static pdk::ds::internal::ArrayDataPointerRef<Type> get()\
         {\
            PDK_ARRAY_LITERAL_IMPL(Type, __VA_ARGS__)\
            return ref;\
         }\
      };\
   })                                                     

#define PDK_ARRAY_LITERAL_IMPL(Type, ...)\
   union {Type type_must_be_POD;} dummy;PDK_UNUSED(dummy);\
   Type data[] = {__VA_ARGS__}; PDK_UNUSED(data);\
   enum {Size = sizeof(data)/sizeof(data[0])};\
   static const pdk::ds::internal::StaticArrayData<Type, Size> literal = {\
      PDK_STATIC_ARRAY_DATA_HEADER_INITIALIZER(Type, Size), {__VAR_ARGS__}\
   };\
   pdk::ds::internal::ArrayDataPointerRef<Type> ref = \
   {\
     static_cast<pdk::ds::internal::TypedArrayData<Type> *>(\
      const_cast<pdk::ds::internal::ArrayData *>(&literal.m_header)\
      )\
   }

} // internal
} // ds
} // pdk

#endif // PDK_M_BASE_DS_INTERNAL_ARRAYDATA_H
