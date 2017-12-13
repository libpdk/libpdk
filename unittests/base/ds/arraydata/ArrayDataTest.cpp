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
// Created by zzu_softboy on 2017/12/08.

#include "gtest/gtest.h"
#include <list>
#include <utility>
#include <tuple>
#include <vector>
#include <algorithm>
#include "pdk/base/ds/internal/ArrayData.h"
#include "SimpleVector.h"

using pdk::ds::internal::ArrayData;
using pdk::ds::internal::StaticArrayData;

namespace 
{

template <typename T>
const T &to_const(const T &t)
{
   return t;
}

}

TEST(ArrayDataTest, testRefCounting)
{
   ArrayData array = {{PDK_BASIC_ATOMIC_INITIALIZER(1)}, 0, 0, 0, 0};
   ASSERT_EQ(array.m_ref.m_atomic.load(), 1);
   ASSERT_FALSE(array.m_ref.isStatic());
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   ASSERT_TRUE(array.m_ref.isSharable());
#endif
   ASSERT_TRUE(array.m_ref.ref());
   ASSERT_EQ(array.m_ref.m_atomic.load(), 2);
   ASSERT_TRUE(array.m_ref.deref());
   ASSERT_EQ(array.m_ref.m_atomic.load(), 1);
   
   ASSERT_TRUE(array.m_ref.ref());
   ASSERT_EQ(array.m_ref.m_atomic.load(), 2);
   ASSERT_TRUE(array.m_ref.deref());
   ASSERT_EQ(array.m_ref.m_atomic.load(), 1);
   
   ASSERT_TRUE(!array.m_ref.deref());
   ASSERT_EQ(array.m_ref.m_atomic.load(), 0);
   // Now would be a good time to free/release allocated data
   
   
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   {
      // Reference counting initialized to 0 (non-sharable)
      ArrayData array = {PDK_BASIC_ATOMIC_INITIALIZER(0), 0, 0, 0, 0};
      ASSERT_EQ(array.m_ref.m_atomic.load(), 0);
      ASSERT_FALSE(array.m_ref.isStatic());
      ASSERT_FALSE(array.m_ref.isSharable());
      ASSERT_FALSE(array.m_ref.ref());
      // Reference counting fails, data should be copied
      ASSERT_EQ(array.m_ref.m_atomic.load(), 0);
      
      ASSERT_FALSE(array.m_ref.deref());
      ASSERT_EQ(array.m_ref.m_atomic.load(), 0);
      // Free/release data
   }
#endif
   {
      // Reference counting initialized to -1 (static read-only data)
      ArrayData array = {PDK_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, 0};
      ASSERT_EQ(array.m_ref.m_atomic.load(), -1);
      ASSERT_TRUE(array.m_ref.isStatic());
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
      ASSERT_TRUE(array.m_ref.isSharable());
#endif
      ASSERT_TRUE(array.m_ref.ref());
      ASSERT_EQ(array.m_ref.m_atomic.load(), -1);
      
      ASSERT_TRUE(array.m_ref.deref());
      ASSERT_EQ(array.m_ref.m_atomic.load(), -1);
   }
}

TEST(ArrayDataTest, testSharedNullEmpty)
{
   ArrayData *null = const_cast<ArrayData *>(ArrayData::sm_sharedNull);
   ArrayData *empty = ArrayData::allocate(1, alignof(ArrayData), 0);
   ASSERT_TRUE(null->m_ref.isStatic());
   ASSERT_TRUE(null->m_ref.isShared());
   
   ASSERT_TRUE(empty->m_ref.isStatic());
   ASSERT_TRUE(empty->m_ref.isShared());
   
   ASSERT_EQ(null->m_ref.m_atomic.load(), -1);
   ASSERT_EQ(empty->m_ref.m_atomic.load(), -1);
   
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   ASSERT_TRUE(null->m_ref.isSharable());
   ASSERT_TRUE(empty->m_ref.isSharable());
#endif
   
   ASSERT_TRUE(null->m_ref.ref());
   ASSERT_TRUE(empty->m_ref.ref());
   ASSERT_EQ(null->m_ref.m_atomic.load(), -1);
   ASSERT_EQ(empty->m_ref.m_atomic.load(), -1);
   
   ASSERT_TRUE(null->m_ref.deref());
   ASSERT_TRUE(empty->m_ref.deref());
   ASSERT_EQ(null->m_ref.m_atomic.load(), -1);
   ASSERT_EQ(empty->m_ref.m_atomic.load(), -1);
   
   ASSERT_NE(null, empty);
   
   ASSERT_EQ(null->m_size, 0);
   ASSERT_EQ(null->m_alloc, 0u);
   ASSERT_EQ(null->m_capacityReserved, 0u);
   
   ASSERT_EQ(empty->m_size, 0);
   ASSERT_EQ(empty->m_alloc, 0u);
   ASSERT_EQ(empty->m_capacityReserved, 0u);
}

TEST(ArrayDataTest, testStaticData)
{
   StaticArrayData<char, 10> charArray = {
      PDK_STATIC_ARRAT_DATA_HEADER_INITIALIZER(char, 10),
      {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j'}
   };
   
   StaticArrayData<int, 10> intArray = {
      PDK_STATIC_ARRAT_DATA_HEADER_INITIALIZER(int, 10),
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
   };
   
   StaticArrayData<double, 10> doubleArray = {
      PDK_STATIC_ARRAT_DATA_HEADER_INITIALIZER(double, 10),
      {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f}
   };
   
   ASSERT_EQ(charArray.m_header.m_size, 10);
   ASSERT_EQ(intArray.m_header.m_size, 10);
   ASSERT_EQ(doubleArray.m_header.m_size, 10);
   
   ASSERT_EQ(charArray.m_header.getData(), reinterpret_cast<void *>(&charArray.m_data));
   ASSERT_EQ(intArray.m_header.getData(), reinterpret_cast<void *>(&intArray.m_data));
   ASSERT_EQ(doubleArray.m_header.getData(), reinterpret_cast<void *>(&doubleArray.m_data));
}

TEST(ArrayDataTest, testSimpleVector)
{
   ArrayData data0 = {
      PDK_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, 0
   };
   
   StaticArrayData<int, 7> data1 = {
      PDK_STATIC_ARRAT_DATA_HEADER_INITIALIZER(int, 7),
      {
         0, 1, 2, 3, 4, 5, 6
      }
   };
   
   int array[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
   SimpleVector<int> v1;
   SimpleVector<int> v2(v1);
   SimpleVector<int> v3(static_cast<TypedArrayData<int> *>(&data0));
   SimpleVector<int> v4(static_cast<TypedArrayData<int> *>(&data1.m_header));
   SimpleVector<int> v5(static_cast<TypedArrayData<int> *>(&data0));
   SimpleVector<int> v6(static_cast<TypedArrayData<int> *>(&data1.m_header));
   SimpleVector<int> v7(10, 5);
   SimpleVector<int> v8(array, array + sizeof(array)/sizeof(*array));
   
   v3 = v1;
   v1.swap(v3);
   v4.clear();
   
   ASSERT_TRUE(v1.isNull());
   ASSERT_TRUE(v2.isNull());
   ASSERT_TRUE(v3.isNull());
   ASSERT_TRUE(v4.isNull());
   ASSERT_FALSE(v5.isNull());
   ASSERT_FALSE(v6.isNull());
   ASSERT_FALSE(v7.isNull());
   ASSERT_FALSE(v8.isNull());
   
   ASSERT_TRUE(v1.isEmpty());
   ASSERT_TRUE(v2.isEmpty());
   ASSERT_TRUE(v3.isEmpty());
   ASSERT_TRUE(v4.isEmpty());
   ASSERT_TRUE(v5.isEmpty());
   ASSERT_FALSE(v6.isEmpty());
   ASSERT_FALSE(v7.isEmpty());
   ASSERT_FALSE(v8.isEmpty());
   
   ASSERT_EQ(v1.size(), static_cast<size_t>(0));
   ASSERT_EQ(v2.size(), static_cast<size_t>(0));
   ASSERT_EQ(v3.size(), static_cast<size_t>(0));
   ASSERT_EQ(v4.size(), static_cast<size_t>(0));
   ASSERT_EQ(v5.size(), static_cast<size_t>(0));
   ASSERT_EQ(v6.size(), static_cast<size_t>(7));
   ASSERT_EQ(v7.size(), static_cast<size_t>(10));
   ASSERT_EQ(v8.size(), static_cast<size_t>(10));
   
   ASSERT_EQ(v1.capacity(), static_cast<size_t>(0));
   ASSERT_EQ(v2.capacity(), static_cast<size_t>(0));
   ASSERT_EQ(v3.capacity(), static_cast<size_t>(0));
   ASSERT_EQ(v4.capacity(), static_cast<size_t>(0));
   ASSERT_EQ(v5.capacity(), static_cast<size_t>(0));
   ASSERT_EQ(v6.capacity(), static_cast<size_t>(0));
   ASSERT_GE(v7.capacity(), static_cast<size_t>(10));
   ASSERT_GE(v8.capacity(), static_cast<size_t>(10));
   
   ASSERT_TRUE(v1.isStatic());
   ASSERT_TRUE(v2.isStatic());
   ASSERT_TRUE(v3.isStatic());
   ASSERT_TRUE(v4.isStatic());
   ASSERT_TRUE(v5.isStatic());
   ASSERT_TRUE(v6.isStatic());
   ASSERT_FALSE(v7.isStatic());
   ASSERT_FALSE(v8.isStatic());
   
   ASSERT_TRUE(v1.isShared());
   ASSERT_TRUE(v2.isShared());
   ASSERT_TRUE(v3.isShared());
   ASSERT_TRUE(v4.isShared());
   ASSERT_TRUE(v5.isShared());
   ASSERT_TRUE(v6.isShared());
   ASSERT_FALSE(v7.isShared());
   ASSERT_FALSE(v8.isShared());
   
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   ASSERT_TRUE(v1.isSharable());
   ASSERT_TRUE(v2.isSharable());
   ASSERT_TRUE(v3.isSharable());
   ASSERT_TRUE(v4.isSharable());
   ASSERT_TRUE(v5.isSharable());
   ASSERT_TRUE(v6.isSharable());
   ASSERT_TRUE(v7.isSharable());
   ASSERT_TRUE(v8.isSharable());
#endif
   
   ASSERT_TRUE(v1.isSharedWith(v2));
   ASSERT_TRUE(v1.isSharedWith(v3));
   ASSERT_TRUE(v1.isSharedWith(v4));
   ASSERT_FALSE(v1.isSharedWith(v5));
   ASSERT_FALSE(v1.isSharedWith(v6));
   
   ASSERT_EQ(v1.constBegin(), v1.constEnd());
   ASSERT_EQ(v4.constBegin(), v4.constEnd());
   ASSERT_EQ((v6.constBegin() + v6.size()), v6.constEnd());
   ASSERT_EQ((v7.constBegin() + v7.size()), v7.constEnd());
   ASSERT_EQ((v8.constBegin() + v8.size()), v8.constEnd());
   
   ASSERT_TRUE(v1 == v2);
   ASSERT_TRUE(v1 == v3);
   ASSERT_TRUE(v1 == v4);
   ASSERT_TRUE(v1 == v5);
   ASSERT_FALSE(v1 == v6);
   
   ASSERT_TRUE(v1 != v6);
   ASSERT_TRUE(v4 != v6);
   ASSERT_TRUE(v5 != v6);
   ASSERT_TRUE(!(v1 != v5));
   
   ASSERT_TRUE(v1 < v6);
   ASSERT_FALSE(v6 < v1);
   ASSERT_TRUE(v6 > v1);
   ASSERT_FALSE(v1 > v6);
   ASSERT_TRUE(v1 <= v6);
   ASSERT_FALSE(v6 <= v1);
   ASSERT_TRUE(v6 >= v1);
   ASSERT_FALSE(v1 >= v6);
   {
      SimpleVector<int> temp(v6);
      ASSERT_EQ(to_const(v6).front(), 0);
      ASSERT_EQ(to_const(v6).back(), 6);
      ASSERT_TRUE(temp.isShared());
      ASSERT_TRUE(temp.isSharedWith(v6));
      
      ASSERT_EQ(temp.front(), 0);
      ASSERT_EQ(temp.back(), 6);
      
      ASSERT_FALSE(temp.isShared());
      const int *const tempBegin = temp.begin();
      
      for (size_t i = 0; i < v6.size(); ++i) {
         ASSERT_EQ(to_const(v6)[i], static_cast<int>(i));
         ASSERT_EQ(to_const(v6).at(i), static_cast<int>(i));
         ASSERT_EQ(&to_const(v6)[i], &to_const(v6).at(i));
         ASSERT_EQ(to_const(v8)[i], to_const(v6)[i]);
         
         ASSERT_EQ(temp[i], static_cast<int>(i));
         ASSERT_EQ(temp.at(i), static_cast<int>(i));
         ASSERT_EQ(&temp[i], &temp.at(i));
         ASSERT_NE(&temp[i], &v6[i]);
      }
      ASSERT_EQ(static_cast<const int *>(temp.begin()), tempBegin);
   }
   
   {
      int count = 0;
      for (int value : v7) {
         ASSERT_EQ(value, 5);
         ++count;
      }
      ASSERT_EQ(count, 10);
   }
   
   {
      int count = 0;
      for (int value : v8) {
         ASSERT_EQ(value, count);
         ++count;
      }
      ASSERT_EQ(count, 10);
   }
   
   v5 = v6;
   ASSERT_TRUE(v5.isSharedWith(v6));
   ASSERT_FALSE(v1.isSharedWith(v5));
   
   v1.swap(v6);
   ASSERT_TRUE(v6.isNull());
   ASSERT_TRUE(v1.isSharedWith(v5));
   
   {
      using std::swap;
      swap(v1, v6);
      ASSERT_TRUE(v5.isSharedWith(v6));
      ASSERT_FALSE(v1.isSharedWith(v5));
   }
   
   v1.prepend(array, array + sizeof(array)/sizeof(array[0]));
   ASSERT_EQ(v1.size(), static_cast<size_t>(10));
   
   v6 = v1;
   ASSERT_TRUE(v6.isSharedWith(v1));
   
   v1.prepend(array, array + sizeof(array) / sizeof(array[0]));
   ASSERT_FALSE(v1.isSharedWith(v6));
   ASSERT_EQ(v1.size(), static_cast<size_t>(20));
   ASSERT_EQ(v6.size(), static_cast<size_t>(10));
   
   for (int i = 0; i < 20; ++i) {
      ASSERT_EQ(v1[i], v6[i % 10]);
   }
   v1.clear();
   v1.append(array, array + sizeof(array)/ sizeof(array[0]));
   ASSERT_EQ(v1.size(), static_cast<size_t>(10));
   ASSERT_TRUE(v1 == v8);
   v6 = v1;
   ASSERT_TRUE(v6.isSharedWith(v1));
   
   v1.append(array, array + sizeof(array)/sizeof(array[0]));
   ASSERT_FALSE(v1.isSharedWith(v6));
   ASSERT_EQ(v1.size(), static_cast<size_t>(20));
   ASSERT_EQ(v6.size(), static_cast<size_t>(10));
   
   for (int i = 0; i < 20; ++i) {
      ASSERT_EQ(v1[i], v6[i % 10]);
   }
   
   v1.insert(0, v6.constBegin(), v6.constEnd());
   ASSERT_EQ(v1.size(), static_cast<size_t>(30));
   
   for (int i = 0; i < 30; ++i) {
      ASSERT_EQ(v1[i], v8[i % 10]);
   }
   v6 = v1;
   ASSERT_TRUE(v6.isSharedWith(v1));
   
   v1.insert(10, v6.constBegin(), v6.constEnd());
   ASSERT_FALSE(v6.isSharedWith(v1));
   ASSERT_EQ(v1.size(), static_cast<size_t>(60));
   ASSERT_EQ(v6.size(), static_cast<size_t>(30));
   for (int i = 0; i < 30; ++i) {
      ASSERT_EQ(v6[i], v8[i % 10]);
   }
   for (int i = 0; i < 60; ++i) {
      ASSERT_EQ(v1[i], v8[i % 10]);
   }
   v1.insert(static_cast<int>(v1.size()), v6.constBegin(), v6.constEnd());
   // v1 is now [ 0..9 x 6, <new data>0..9 x 3</new data> ]
   
   ASSERT_EQ(v1.size(), static_cast<size_t>(90));
   for (int i = 0; i < 90; ++i) {
      ASSERT_EQ(v1[i], v8[i % 10]);
   }
   
   v1.insert(-1, v8.constBegin(), v8.constEnd());
   // v1 is now [ 0..9 x 9, <new data>0..9</new data> ]
   ASSERT_EQ(v1.size(), static_cast<size_t>(100));
   for (int i = 0; i < 100; ++i) {
      ASSERT_EQ(v1[i], v8[i % 10]);
   }
   
   v1.insert(-11, v8.constBegin(), v8.constEnd());
   // v1 is now [ 0..9 x 9, <new data>0..9</new data>, 0..9 ]
   ASSERT_EQ(v1.size(), static_cast<size_t>(110));
   
   v1.insert(-200, v8.constBegin(), v8.constEnd());
   // v1 is now [ <new data>0..9</new data>, 0..9 x 11 ]
   ASSERT_EQ(v1.size(), static_cast<size_t>(120));
   
   for (int i = 0; i < 120; ++i) {
      ASSERT_EQ(v1[i], v8[i % 10]);
   }
   
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   {
      v7.setSharable(true);
      ASSERT_TRUE(v7.isSharable());
      
      SimpleVector<int> copy1(v7);
      ASSERT_TRUE(copy1.isSharedWith(v7));
      
      v7.setSharable(false);
      ASSERT_FALSE(v7.isSharable());
      ASSERT_FALSE(copy1.isSharedWith(v7));
      
      ASSERT_EQ(v7.size(), copy1.size());
      for (size_t i = 0; i < copy1.size(); ++i){
         ASSERT_EQ(v7[i], copy1[i]);
      }
      
      SimpleVector<int> copy2(v7);
      ASSERT_FALSE(copy2.isSharedWith(v7));
      ASSERT_EQ(copy1.size(), copy2.size());
      for (size_t i = 0; i < copy1.size(); ++i){
         ASSERT_EQ(copy1[i], copy2[i]);
      }
      v7.setSharable(true);
      ASSERT_TRUE(v7.isSharable());
      
      SimpleVector<int> copy3(v7);
      ASSERT_TRUE(copy3.isSharedWith(v7));
   }
   
   {
      SimpleVector<int> null;
      SimpleVector<int> empty(0, 5);
      
      ASSERT_TRUE(null.isSharable());
      ASSERT_TRUE(empty.isSharable());
      
      null.setSharable(true);
      empty.setSharable(true);
      
      ASSERT_TRUE(null.isSharable());
      ASSERT_TRUE(empty.isSharable());
      
      ASSERT_TRUE(null.isEmpty());
      ASSERT_TRUE(empty.isEmpty());
      
      null.setSharable(false);
      empty.setSharable(false);
      
      ASSERT_FALSE(null.isSharable());
      ASSERT_FALSE(empty.isSharable());
      
      ASSERT_TRUE(null.isEmpty());
      ASSERT_TRUE(empty.isEmpty());
      
      null.setSharable(true);
      empty.setSharable(true);
      
      ASSERT_TRUE(null.isSharable());
      ASSERT_TRUE(empty.isSharable());
      
      ASSERT_TRUE(null.isEmpty());
      ASSERT_TRUE(empty.isEmpty());
   }
   
#endif
}

TEST(ArrayDataTest, testSimpleVectorReserve)
{
   using DataType = std::list<std::tuple<SimpleVector<int>, size_t, size_t>>;
   DataType data;
   data.push_back(std::make_tuple(SimpleVector<int>(), static_cast<size_t>(0), static_cast<size_t>(0)));
   data.push_back(std::make_tuple(SimpleVector<int>(0, 42), static_cast<size_t>(0), static_cast<size_t>(0)));
   data.push_back(std::make_tuple(SimpleVector<int>(5, 42), static_cast<size_t>(5), static_cast<size_t>(5)));
   
   static const StaticArrayData<int, 15> array = {
      PDK_STATIC_ARRAT_DATA_HEADER_INITIALIZER(int, 15),
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}
   };
   
   ArrayDataPointerRef<int> p = {
      static_cast<TypedArrayData<int> *>(const_cast<ArrayData *>(&array.m_header))
   };
   data.push_back(std::make_tuple(SimpleVector<int>(p), static_cast<size_t>(0), static_cast<size_t>(15)));
   data.push_back(std::make_tuple(SimpleVector<int>::fromRawData(array.m_data, 15), static_cast<size_t>(0), static_cast<size_t>(15)));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      SimpleVector<int> vector = std::get<0>(item);
      size_t capacity = std::get<1>(item);
      size_t size = std::get<2>(item);
      ASSERT_TRUE(!capacity || capacity >= size);
      ASSERT_EQ(vector.capacity(), capacity);
      ASSERT_EQ(vector.size(), size);
      
      const SimpleVector<int> copy(vector);
      
      vector.reserve(0);
      ASSERT_EQ(vector.capacity(), capacity);
      ASSERT_EQ(vector.size(), size);
      
      vector.reserve(10);
      
      if (!capacity) {
         capacity = size;
      }
      
      ASSERT_EQ(vector.capacity(), std::max(static_cast<size_t>(10), capacity));
      ASSERT_EQ(vector.size(), size);
      
      vector.reserve(20);
      ASSERT_EQ(vector.capacity(), static_cast<size_t>(20));
      ASSERT_EQ(vector.size(), size);
      
      vector.reserve(30);
      ASSERT_EQ(vector.capacity(), static_cast<size_t>(30));
      ASSERT_EQ(vector.size(), size);
      
      ASSERT_EQ(vector, copy);
      ++begin;
   }
}


struct Deallocator
{
   Deallocator(size_t objectSize, size_t alignment)
      : m_objectSize(objectSize),
        m_alignment(alignment)
   {
      
   }
   
   ~Deallocator()
   {
      for(ArrayData *data: m_headers) {
         ArrayData::deallocate(data, m_objectSize, m_alignment);
      }
   }
   
   size_t m_objectSize;
   size_t m_alignment;
   std::vector<ArrayData *> m_headers;
};

TEST(ArrayDataTest, testAllocateData)
{
   using DataType = std::list<std::tuple<size_t, size_t, ArrayData::AllocationOptions, bool, bool, const ArrayData *>>;
   DataType data;
   
   struct
   {
      char const *m_typeName;
      size_t m_objectSize;
      size_t m_alignment;
   } types[] = {
   {"char", sizeof(char), alignof(char)},
   {"short", sizeof(short), alignof(short)},
   {"void *", sizeof(void *), alignof(void *)},
};
   
   ArrayData *sharedEmpty = ArrayData::allocate(0, alignof(ArrayData), 0);
   ASSERT_TRUE(sharedEmpty);
   
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   ArrayData *unsharableEmpty = ArrayData::allocate(0, alignof(ArrayData), 0, ArrayData::Unsharable);
   ASSERT_TRUE(unsharableEmpty);
#endif
   
   struct
   {
      char const *m_description;
      ArrayData::AllocationOptions m_allocateOptions;
      bool m_isCapacityReserved;
      bool m_isSharable;
      const ArrayData *m_commonEmpty;
   } options[] = {
   {"Default", ArrayData::Default, false, true, sharedEmpty},
   {"Reserved", ArrayData::CapacityReserved, true, true, sharedEmpty},
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   {"Reserved | Unsharable", ArrayData::CapacityReserved | ArrayData::Unsharable, true, false, unsharableEmpty},
   {"Unsharable", ArrayData::Unsharable,  false, false, unsharableEmpty},
#endif
   {"Grow", ArrayData::Grow, false, true, sharedEmpty}
};
   
   for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); ++i) {
      for (size_t j = 0; j < sizeof(options)/sizeof(options[0]); ++j) {
         data.push_back(std::make_tuple(types[i].m_objectSize, types[i].m_alignment, options[j].m_allocateOptions,
                                        options[j].m_isCapacityReserved, options[j].m_isSharable, options[j].m_commonEmpty));
      }
   }
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      size_t objectSize = std::get<0>(item);
      size_t alignment = std::get<1>(item);
      ArrayData::AllocationOptions allocateOptions = std::get<2>(item);
      bool isCapacityReserved = std::get<3>(item);
      bool isSharable = std::get<4>(item);
      const ArrayData *commonEmpty = std::get<5>(item);
      size_t minAlignment = std::max(alignment, alignof(ArrayData));
      
      ASSERT_EQ(ArrayData::allocate(objectSize, minAlignment, 0, ArrayData::AllocationOptions(allocateOptions)), commonEmpty);
      
      Deallocator keeper(objectSize, minAlignment);
      keeper.m_headers.reserve(1024);
      
      for (int capacity = 1; capacity <= 1024; capacity <<= 1) {
         ArrayData *data = ArrayData::allocate(objectSize, minAlignment, capacity, ArrayData::AllocationOptions(allocateOptions));
         keeper.m_headers.push_back(data);
         ASSERT_EQ(data->m_size, 0);
         if (allocateOptions & ArrayData::Grow) {
            ASSERT_TRUE(data->m_alloc > static_cast<uint>(capacity));
         } else {
            ASSERT_EQ(data->m_alloc, static_cast<uint>(capacity));
         }
         ASSERT_EQ(data->m_capacityReserved, static_cast<uint>(isCapacityReserved));
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
         ASSERT_EQ(data->m_ref.isSharable(), isSharable);
#endif
         ::memset(data->getData(), 'A', objectSize * capacity);
      }
      ++begin;
   }
}

class Unaligned
{
   char dummy[8]; 
};

TEST(ArrayDataTest, testAlignment)
{
   for (size_t i = 1; i < 10; ++i) {
      size_t alignment = static_cast<size_t>(1u) << i;
      
      size_t minAlignment = std::max(alignment, alignof(ArrayData));
      Deallocator keeper(sizeof(Unaligned), minAlignment);
      keeper.m_headers.reserve(100);
      for (int j = 0; j < 100; j++) {
         ArrayData *data = ArrayData::allocate(sizeof(Unaligned),
                                               minAlignment, 8, ArrayData::Default);
         keeper.m_headers.push_back(data);
         ASSERT_TRUE(data);
         ASSERT_EQ(data->m_size, 0);
         ASSERT_TRUE(data->m_alloc >= static_cast<uint>(8));
         // These conditions should hold as long as header and array are
         // allocated together
         ASSERT_TRUE(data->m_offset >= static_cast<pdk::ptrdiff>(sizeof(ArrayData)));
         ASSERT_TRUE(data->m_offset <= static_cast<pdk::ptrdiff>(sizeof(ArrayData) + minAlignment - alignof(ArrayData)));
         // Data is aligned
         ASSERT_EQ(static_cast<pdk::uintptr>(reinterpret_cast<pdk::uintptr>(data->getData()) % alignment), static_cast<pdk::uintptr>(0u));
         // Check that the allocated array can be used. Best tested with a
         // memory checker, such as valgrind, running.
         std::memset(data->getData(), 'A', sizeof(Unaligned) * 8);
      }
   }
}

TEST(ArrayDataTest, testTypedData)
{
   StaticArrayData<int, 10> data = {
      PDK_STATIC_ARRAT_DATA_HEADER_INITIALIZER(int, 10),
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
   };
   ASSERT_EQ(data.m_header.m_size, 10);
   {
      TypedArrayData<int> *array = static_cast<TypedArrayData<int> *>(&data.m_header);
      ASSERT_EQ(array->getData(), data.m_data);
      int j = 0;
      for (TypedArrayData<int>::Iterator iter = array->begin(); iter != array->end(); ++iter, ++j)
      {
         ASSERT_EQ(static_cast<const int *>(iter), data.m_data + j);
      }
      ASSERT_EQ(j, 10);
   }
   
   {
      const TypedArrayData<int> *array = static_cast<const TypedArrayData<int> *>(&data.m_header);
      ASSERT_EQ(array->getData(), data.m_data);
      int j = 0;
      for (TypedArrayData<int>::ConstIterator iter = array->begin(); iter != array->end(); ++iter, ++j)
      {
         ASSERT_EQ(static_cast<const int *>(iter), data.m_data + j);
      }
      ASSERT_EQ(j, 10);
   }
   
   {
      TypedArrayData<int> *null = TypedArrayData<int>::getSharedNull();
      TypedArrayData<int> *empty = TypedArrayData<int>::allocate(0);
      ASSERT_TRUE(null != empty);
      ASSERT_EQ(null->m_size, 0);
      ASSERT_EQ(empty->m_size, 0);
      ASSERT_EQ(null->begin(), null->end());
      ASSERT_EQ(empty->begin(), empty->end());
   }
   
   {
      Deallocator keeper(sizeof(char), alignof(TypedArrayData<char>::AlignmentDummy));
      ArrayData *array = TypedArrayData<char>::allocate(10);
      keeper.m_headers.push_back(array);
      ASSERT_TRUE(array);
      ASSERT_EQ(array->m_size, 0);
      ASSERT_EQ(array->m_alloc, 10u);
      
      // Check that the allocated array can be used. Best tested with a
      // memory checker, such as valgrind, running.
      std::memset(array->getData(), 0, 10 * sizeof(char));
      keeper.m_headers.clear();
      TypedArrayData<char>::deallocate(array);
   }
   {
      Deallocator keeper(sizeof(short), alignof(TypedArrayData<short>::AlignmentDummy));
      ArrayData *array = TypedArrayData<short>::allocate(10);
      keeper.m_headers.push_back(array);
      ASSERT_TRUE(array);
      ASSERT_EQ(array->m_size, 0);
      ASSERT_EQ(array->m_alloc, 10u);
      
      // Check that the allocated array can be used. Best tested with a
      // memory checker, such as valgrind, running.
      std::memset(array->getData(), 0, 10 * sizeof(short));
      keeper.m_headers.clear();
      TypedArrayData<short>::deallocate(array);
   }
   
   {
      Deallocator keeper(sizeof(double), alignof(TypedArrayData<double>::AlignmentDummy));
      ArrayData *array = TypedArrayData<double>::allocate(10);
      keeper.m_headers.push_back(array);
      ASSERT_TRUE(array);
      ASSERT_EQ(array->m_size, 0);
      ASSERT_EQ(array->m_alloc, 10u);
      
      // Check that the allocated array can be used. Best tested with a
      // memory checker, such as valgrind, running.
      std::memset(array->getData(), 0, 10 * sizeof(double));
      keeper.m_headers.clear();
      TypedArrayData<double>::deallocate(array);
   }
}

struct CountedObject
{
   CountedObject()
      : m_id(sm_liveCount++),
        m_flags(DefaultConstructed)
   {}
   
   CountedObject(const CountedObject &other)
      : m_id(other.m_id),
        m_flags(other.m_flags == DefaultConstructed
                ? static_cast<ObjectFlags>(CopyConstructed | DefaultConstructed)
                : CopyConstructed)
   {
      ++sm_liveCount;
   }
   
   CountedObject &operator =(const CountedObject &other) 
   {
      m_flags = static_cast<ObjectFlags>(other.m_flags | CopyAssigned);
      m_id = other.m_id;
      return *this;
   }
   
   struct LeakChecker
   {
      LeakChecker()
         : m_previousLiveCount(sm_liveCount)
      {
      }
      
      ~LeakChecker()
      {
         doCheck();
      }
      
      void doCheck()
      {
         ASSERT_EQ(sm_liveCount, m_previousLiveCount);
      }
      
   private:
      const size_t m_previousLiveCount;
   };
   
   ~CountedObject()
   {
      --sm_liveCount;
   }
   
   enum ObjectFlags 
   {
      DefaultConstructed  = 1,
      CopyConstructed     = 2,
      CopyAssigned        = 4
   };
   
   size_t m_id; // not unique
   ObjectFlags m_flags;
   static size_t sm_liveCount;
};

size_t CountedObject::sm_liveCount = 0;

TEST(ArrayDataTest, testArrayOperations)
{
   CountedObject::LeakChecker leakChecker; 
   PDK_UNUSED(leakChecker);
   const int intArray[5] = {80, 101, 100, 114, 111};
   const std::string stringArray[5] = {
      std::string("just"),
      std::string("for"),
      std::string("testing"),
      std::string("a"),
      std::string("vector")
   };
   const CountedObject objArray[5];
   ASSERT_TRUE(!pdk::TypeInfo<int>::isComplex && !pdk::TypeInfo<int>::isStatic);
   ASSERT_TRUE(pdk::TypeInfo<CountedObject>::isComplex && pdk::TypeInfo<CountedObject>::isStatic);
   ASSERT_EQ(CountedObject::sm_liveCount, 5u);
   
   for (size_t i = 0; i < 5; ++i) {
      ASSERT_EQ(objArray[i].m_id, static_cast<size_t>(i));
   }
   
   SimpleVector<int> vi(intArray, intArray + 5);
   SimpleVector<std::string> vs(stringArray, stringArray + 5);
   SimpleVector<CountedObject> vo(objArray, objArray + 5);
   
   ASSERT_EQ(CountedObject::sm_liveCount, 10u);
   
   for (int i = 0; i < 5; ++i) {
      ASSERT_EQ(vi[i], intArray[i]);
      ASSERT_EQ(vo[i].m_id, objArray[i].m_id);
      ASSERT_EQ(static_cast<int>(vo[i].m_flags), CountedObject::CopyConstructed
                | CountedObject::DefaultConstructed);
   }
   
   vi.clear();
   vs.clear();
   vo.clear();
   
   ASSERT_EQ(CountedObject::sm_liveCount, static_cast<size_t>(5));
   
   int referenceInt = 7;
   CountedObject referenceObject;
   vi = SimpleVector<int>(5, referenceInt);
   vo = SimpleVector<CountedObject>(5, referenceObject);
   ASSERT_EQ(vi.size(), 5u);
   ASSERT_EQ(vo.size(), 5u);
   
   ASSERT_EQ(CountedObject::sm_liveCount, static_cast<size_t>(11));
   for (int i = 0; i < 5; ++i) {
      ASSERT_EQ(vi[i], referenceInt);
      ASSERT_EQ(vo[i].m_id, referenceObject.m_id);
      ASSERT_EQ(static_cast<int>(vo[i].m_flags), CountedObject::CopyConstructed
                | CountedObject::DefaultConstructed);
   }
   
   vi.reserve(30);
   vo.reserve(30);
   
   ASSERT_EQ(vi.size(), 5u);
   ASSERT_EQ(vo.size(), 5u);
   
   ASSERT_TRUE(vi.capacity() >= 30u);
   ASSERT_TRUE(vo.capacity() >= 30u);
   
   vi.insert(0, intArray, intArray + 5);
   vo.insert(0, objArray, objArray + 5);
   
   ASSERT_EQ(vi.size(), 10u);
   ASSERT_EQ(vo.size(), 10u);
   
   vi.insert(0, intArray, intArray + 5);
   vo.insert(0, objArray, objArray + 5);
   
   ASSERT_EQ(vi.size(), 15u);
   ASSERT_EQ(vo.size(), 15u);
   
   // Displace less elements than array is extended by
   vi.insert(5, vi.constBegin(), vi.constEnd());
   vo.insert(5, vo.constBegin(), vo.constEnd());
   
   ASSERT_EQ(vi.size(), 30u);
   ASSERT_EQ(vo.size(), 30u);
   
   ASSERT_EQ(CountedObject::sm_liveCount, static_cast<size_t>(36));
   
   for (int i = 0; i < 5; ++i) {
      ASSERT_EQ(vi[i], intArray[i % 5]);
      ASSERT_EQ(vo[i].m_id, vo[i % 5].m_id);
      ASSERT_EQ(static_cast<int>(vo[i].m_flags), CountedObject::DefaultConstructed
                | CountedObject::CopyAssigned);
   }
   
   for (int i = 5; i < 15; ++i) {
      ASSERT_EQ(vi[i], intArray[i % 5]);
      ASSERT_EQ(vo[i].m_id, vo[i % 5].m_id);
      ASSERT_EQ(static_cast<int>(vo[i].m_flags), CountedObject::CopyConstructed
                | CountedObject::CopyAssigned);
   }
   
   for (int i = 15; i < 20; ++i) {
      ASSERT_EQ(vi[i], referenceInt);
      ASSERT_EQ(vo[i].m_id, referenceObject.m_id);
      ASSERT_EQ(static_cast<int>(vo[i].m_flags), CountedObject::CopyAssigned
                | CountedObject::CopyConstructed);
   }
   
   for (int i = 20; i < 25; ++i) {
      ASSERT_EQ(vi[i], intArray[i % 5]);
      ASSERT_EQ(vo[i].m_id, vo[i % 5].m_id);
      ASSERT_EQ(vo[i].m_flags & CountedObject::CopyAssigned, static_cast<int>(CountedObject::CopyAssigned));
   }
   
   for (int i = 25; i < 30; ++i) {
      ASSERT_EQ(vi[i], referenceInt);
      ASSERT_EQ(vo[i].m_id, referenceObject.m_id);
      ASSERT_EQ(static_cast<int>(vo[i].m_flags), CountedObject::CopyAssigned
                | CountedObject::CopyConstructed);
   }
}
