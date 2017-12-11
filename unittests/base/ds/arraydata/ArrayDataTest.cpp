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
   ASSERT_EQ(null->m_alloc, 0);
   ASSERT_EQ(null->m_capacityReserved, 0u);
   
   ASSERT_EQ(empty->m_size, 0);
   ASSERT_EQ(empty->m_alloc, 0);
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
}
