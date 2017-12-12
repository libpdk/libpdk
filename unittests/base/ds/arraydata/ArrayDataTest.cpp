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
   
   
}
