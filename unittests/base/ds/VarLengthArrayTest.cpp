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
// Created by zzu_softboy on 2017/12/27.

#include "gtest/gtest.h"
#include <list>
#include <utility>
#include <tuple>
#include <vector>
#include <algorithm>
#include <string>

#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/global/TypeInfo.h"

using pdk::ds::VarLengthArray;
using pdk::ds::ByteArray;

namespace
{

int fooCtor = 0;
int fooDtor = 0;

struct Foo
{
   int *m_ptr;
   Foo()
   {
      m_ptr = new int;
      ++fooCtor;
   }
   
   Foo(const Foo &/*other*/)
   {
      m_ptr = new int;
      ++fooCtor;
   }
   
   ~Foo()
   {
      delete m_ptr;
      ++fooDtor;
   }
};

}

TEST(VarLengthArrayTest, testAppend)
{
   VarLengthArray<std::string> v(0);
   v.append(std::string("a"));
   VarLengthArray<int> v2;
   v2.append(5);
}

TEST(VarLengthArrayTest, removeLast)
{
   {
      VarLengthArray<char, 2> v;
      v.append(0);
      v.append(1);
      ASSERT_EQ(v.size(), 2);
      v.append(2);
      v.append(3);
      ASSERT_EQ(v.size(), 4);
      v.removeLast();
      ASSERT_EQ(v.size(), 3);
      v.removeLast();
      ASSERT_EQ(v.size(), 2);
   }
   {
      VarLengthArray<std::string, 2> v;
      v.append("0");
      v.append("1");
      ASSERT_EQ(v.size(), 2);
      v.append("2");
      v.append("3");
      ASSERT_EQ(v.size(), 4);
      v.removeLast();
      ASSERT_EQ(v.size(), 3);
      v.removeLast();
      ASSERT_EQ(v.size(), 2);
   }
}

TEST(VarLengthArrayTest, testDataAccess)
{
   {
      VarLengthArray<int, 256> array(128);
      ASSERT_TRUE(array.getRawData() == &array[0]);
      array[0] = 0xfee;
      array[10] = 0xff;
      ASSERT_EQ(array[0], 0xfee);
      ASSERT_EQ(array[10], 0xff);
      array.resize(512);
      ASSERT_TRUE(array.getRawData() == &array[0]);
      array[0] = 0xfee;
      array[10] = 0xff;
      ASSERT_EQ(array.at(0), 0xfee);
      ASSERT_EQ(array.at(10), 0xff);
      ASSERT_EQ(array.value(0), 0xfee);
      ASSERT_EQ(array.value(10), 0xff);
      ASSERT_EQ(array.value(1000), 0);
      ASSERT_EQ(array.value(1000, 12), 12);
      ASSERT_EQ(array.size(), 512);
      array.reserve(1024);
      ASSERT_EQ(array.capacity(), 1024);
      ASSERT_EQ(array.size(), 512);
   }
   {
      VarLengthArray<std::string> array(10);
      array[0] = "Hello";
      array[9] = "World";
      ASSERT_STREQ(array.getRawData()->c_str(), "Hello");
      ASSERT_EQ(array[9], "World");
      array.reserve(512);
      ASSERT_STREQ(array.getRawData()->c_str(), "Hello");
      ASSERT_EQ(array[9], "World");
      array.resize(512);
      ASSERT_STREQ(array.getRawData()->c_str(), "Hello");
      ASSERT_EQ(array[9], "World");
   }
   {
      int rawArray[2] = {1, 2};
      VarLengthArray<int> array(10);
      ASSERT_EQ(array.size(), 10);
      array.append(rawArray, 2);
      ASSERT_EQ(array.size(), 12);
      ASSERT_EQ(array[10], 1);
      ASSERT_EQ(array[11], 2);
   }
   {
      std::string strArray[2]{std::string("hello"), std::string("world")};
      VarLengthArray<std::string> array(10);
      ASSERT_EQ(array.size(), 10);
      array.append(strArray, 2);
      ASSERT_EQ(array.size(), 12);
      ASSERT_EQ(array[10], "hello");
      ASSERT_EQ(array[11], "world");
      ASSERT_EQ(array.at(10), "hello");
      ASSERT_EQ(array.at(11), "world");
      ASSERT_EQ(array.value(10), "hello");
      ASSERT_EQ(array.value(11), "world");
      ASSERT_EQ(array.value(1000), std::string());
      ASSERT_EQ(array.value(1000, std::string("null")), std::string("null"));
      ASSERT_EQ(array.value(-12, std::string("neg")), std::string("neg"));
      
      array.append(strArray, 1);
      ASSERT_EQ(array.size(), 13);
      ASSERT_EQ(array[12], std::string("hello"));
      
      array.append(strArray, 0);
      ASSERT_EQ(array.size(), 13);
   }
   {
      // assignment operator and copy constructor
      VarLengthArray<int> array1(10);
      array1[5] = 5;
      
      VarLengthArray<int> array2(10);
      array2[5] = 6;
      array2 = array1;
      ASSERT_EQ(array2[5], 5);
      VarLengthArray<int> array3(array1);
      ASSERT_EQ(array2[5], 5);
   }
}

TEST(VarLengthArrayTest, testAppendCausingRealloc)
{
   VarLengthArray<float, 1> array(1);
   for (int i = 0; i < 30; i++) {
      array.append(i);
   }
   
}

TEST(VarLengthArrayTest, testResize)
{
   //MOVABLE
   {
      VarLengthArray<ByteArray> array(1);
      ASSERT_EQ(array.size(), 1);
      array[0].append('A');
      array.resize(2);
   }
   // POD
   {
      VarLengthArray<int,1> array(1);
      ASSERT_EQ(array.size(), 1);
      array[0] = 1;
      array.resize(2);
      ASSERT_EQ(array.size(), 2);
      array[1] = 2;
      ASSERT_EQ(array[1], 2);
      ASSERT_EQ(array.size(), 2);
   }
   // COMPLEX
   {
      VarLengthArray<VarLengthArray<std::string, 15>, 1> array(1);
      ASSERT_EQ(array.size(), 1);
      array[0].resize(10);
      array.resize(2);
      ASSERT_EQ(array[1].size(), 0);
      ASSERT_EQ(array[0].size(), 10);
      array[1].resize(20);
      ASSERT_EQ(array[1].size(), 20);
      ASSERT_EQ(array.size(), 2);
   }
}

struct MyBase
{
   MyBase()
      : m_data(this),
        m_isCopy(false)
   {
      ++sm_liveCount;
   }
   
   MyBase(const MyBase &)
      : m_data(this),
        m_isCopy(true)
   {
      ++sm_copyCount;
      ++sm_liveCount;
   }
   
   MyBase &operator =(const MyBase &)
   {
      if (!m_isCopy) {
         m_isCopy = true;
         ++sm_copyCount;
      } else {
         ++sm_errorCount;
      }
      return *this;
   }
   
   ~MyBase()
   {
      if (m_isCopy) {
         if (!sm_copyCount) {
            ++sm_errorCount;
         } else {
            --sm_copyCount;
         }
      }
      if (!sm_liveCount) {
         ++sm_errorCount;
      } else {
         --sm_liveCount;
      }
   }
   
   bool hasMoved() const
   {
      return this != m_data;
   }
   
public:
   static int sm_errorCount;
   static int sm_liveCount;
   static int sm_copyCount;
   
protected:
   MyBase const * const m_data;
   bool m_isCopy;
};

int MyBase::sm_copyCount = 0;
int MyBase::sm_errorCount = 0;
int MyBase::sm_liveCount = 0;

struct MyPrimitive : MyBase
{
   MyPrimitive()
   {
      ++sm_errorCount;
   }
   
   ~MyPrimitive()
   {
      ++sm_errorCount;
   }
   
   MyPrimitive(const MyPrimitive &other)
      : MyBase(other)
   {
      ++sm_errorCount;
   }
};

struct MyMovable : MyBase
{
   MyMovable(char input = 'j')
      : m_i(input)
   {}
   
   bool operator ==(const MyMovable &other) const
   {
      return m_i == other.m_i;
   }
   
   char m_i;
};

struct MyComplex : MyBase
{
   MyComplex(char input = 'j')
      : m_i(input)
   {}
   
   bool operator ==(const MyComplex &other) const
   {
      return m_i == other.m_i;
   }
   
   char m_i;
};

PDK_DECLARE_TYPEINFO(MyPrimitive, PDK_PRIMITIVE_TYPE);
PDK_DECLARE_TYPEINFO(MyMovable, PDK_MOVABLE_TYPE);
PDK_DECLARE_TYPEINFO(MyComplex, PDK_COMPLEX_TYPE);

bool reallocTestProceed = true;
template <class T, int PreAlloc>

int count_moved(VarLengthArray<T, PreAlloc> const &c)
{
   int result = 0;
   for (int i = 0; i < c.size(); ++i) {
      if (c[i].hasMoved()) {
         ++result;
      }
   }
   return result;
}

template <typename T>
void realloc_test()
{
   reallocTestProceed = false;
   using Container = VarLengthArray<T, 16>;
   enum {
      isStatic = pdk::TypeInfo<T>::isStatic,
      isComplex = pdk::TypeInfo<T>::isComplex,
      isPrimitive = !isComplex && !isStatic,
      isMovable = !isStatic
   };
   
   Container array;
   ASSERT_EQ(MyBase::sm_liveCount, 0);
   ASSERT_EQ(MyBase::sm_copyCount, 0);
   
   ASSERT_TRUE(array.capacity() >= 16);
   ASSERT_EQ(array.size(), 0);
   
   Container breal(8);
   const Container &b = breal;
   ASSERT_EQ(MyBase::sm_liveCount, isPrimitive ? 0 : 8);
   ASSERT_EQ(MyBase::sm_copyCount, 0);
   
   ASSERT_TRUE(b.capacity() >= 16);
   ASSERT_EQ(b.size(), 8);
   
   // Assignment
   array = b;
   ASSERT_EQ(MyBase::sm_liveCount, isPrimitive ? 0 : 16);
   ASSERT_EQ(MyBase::sm_copyCount, isComplex ? 8 : 0);
   
   ASSERT_TRUE(array.capacity() >= 16);
   ASSERT_EQ(array.size(), 8);
   
   ASSERT_TRUE(b.capacity() >= 16);
   ASSERT_EQ(b.size(), 8);
   
   // append
   array.append(b.getRawData(), b.size());
   ASSERT_EQ(MyBase::sm_liveCount, isPrimitive ? 0 : 24);
   ASSERT_EQ(MyBase::sm_copyCount, isComplex ? 16 : 0);
   
   ASSERT_TRUE(array.capacity() >= 16);
   ASSERT_EQ(array.size(), 16);
   
   ASSERT_TRUE(b.capacity() >= 16);
   ASSERT_EQ(b.size(), 8);
   
   // removeLast
   array.removeLast();
   ASSERT_EQ(MyBase::sm_liveCount, isPrimitive ? 0 : 23);
   ASSERT_EQ(MyBase::sm_copyCount, isComplex ? 15 : 0);
   
   ASSERT_TRUE(array.capacity() >= 16);
   ASSERT_EQ(array.size(), 15);
   
   ASSERT_TRUE(b.capacity() >= 16);
   ASSERT_EQ(b.size(), 8);
   
   // Movable types
   const int capacity = array.capacity();
   if (!isPrimitive) {
      ASSERT_EQ(count_moved(array), 0);
   }
   // Reserve, no re-allocation
   array.reserve(capacity);
   ASSERT_EQ(MyBase::sm_liveCount, isPrimitive ? 0 : 23);
   ASSERT_EQ(MyBase::sm_copyCount, isComplex ? 15 : 0);
   
   ASSERT_EQ(array.capacity(), capacity);
   ASSERT_EQ(array.size(), 15);
   
   ASSERT_TRUE(b.capacity() >= 16);
   ASSERT_EQ(b.size(), 8);
   
   // Reserve, force re-allocation
   array.reserve(capacity * 2);
   if (!isPrimitive) {
      ASSERT_EQ(count_moved(array), isMovable ? 15 : 0 );
   }
   ASSERT_EQ(MyBase::sm_liveCount, isPrimitive ? 0 : 23);
   ASSERT_EQ(MyBase::sm_copyCount, isComplex ? 15 : 0);
   
   ASSERT_TRUE(array.capacity() >= capacity * 2);
   ASSERT_EQ(array.size(), 15);
   
   ASSERT_TRUE(b.capacity() >= 16);
   ASSERT_EQ(b.size(), 8);
   
   // resize, grow
   array.resize(40);
   if (!isPrimitive) {
      ASSERT_EQ(count_moved(array), isMovable ? 15 : 0 );
   }
   ASSERT_EQ(MyBase::sm_liveCount, isPrimitive ? 0 : 48);
   ASSERT_EQ(MyBase::sm_copyCount, isComplex ? 15 : 0);
   
   ASSERT_TRUE(array.capacity() >= array.size());
   ASSERT_EQ(array.size(), 40);
   
   ASSERT_TRUE(b.capacity() >= 16);
   ASSERT_EQ(b.size(), 8);
   
   // Copy constructor, allocate
   {
      Container array1(array);
      if (!isPrimitive) {
         ASSERT_EQ(count_moved(array1), 0);
      }
      ASSERT_EQ(MyBase::sm_liveCount, isPrimitive ? 0 : 88);
      ASSERT_EQ(MyBase::sm_copyCount, isComplex ? 55 : 0);
      
      ASSERT_TRUE(array.capacity() >= array.size());
      ASSERT_EQ(array.size(), 40);
      
      ASSERT_TRUE(b.capacity() >= 16);
      ASSERT_EQ(b.size(), 8);
      
      ASSERT_TRUE(array1.capacity() >= 40);
      ASSERT_EQ(array1.size(), 40);
   }
   
   // resize, shrink
   array.resize(10);
   if (!isPrimitive) {
      ASSERT_EQ(count_moved(array), isMovable ? 10 : 0);
   }
   ASSERT_EQ(MyBase::sm_liveCount, isPrimitive ? 0 : 18);
   ASSERT_EQ(MyBase::sm_copyCount, isComplex ? 10 : 0);
   
   ASSERT_TRUE(array.capacity() >= array.size());
   ASSERT_EQ(array.size(), 10);
   
   ASSERT_TRUE(b.capacity() >= 16);
   ASSERT_EQ(b.size(), 8);
   
   // Copy constructor, don't allocate
   {
      Container array1(array);
      if (!isPrimitive) {
         ASSERT_EQ(count_moved(array1), 0);
      }
      ASSERT_EQ(MyBase::sm_liveCount, isPrimitive ? 0 : 28);
      ASSERT_EQ(MyBase::sm_copyCount, isComplex ? 20 : 0);
      
      ASSERT_TRUE(array.capacity() >= array.size());
      ASSERT_EQ(array.size(), 10);
      
      ASSERT_TRUE(b.capacity() >= 16);
      ASSERT_EQ(b.size(), 8);
      
      ASSERT_TRUE(array1.capacity() >= 16);
      ASSERT_EQ(array1.size(), 10);
   }
   
   array.clear();
   ASSERT_EQ(array.size(), 0);
   
   breal.clear();
   ASSERT_EQ(b.size(), 0);
   ASSERT_EQ(MyBase::sm_errorCount, 0);
   ASSERT_EQ(MyBase::sm_liveCount, 0);
   
   reallocTestProceed = true;
}

TEST(VarLengthArrayTest, testRealloc)
{
   realloc_test<MyBase>();
   ASSERT_TRUE(reallocTestProceed);
   realloc_test<MyPrimitive>();
   ASSERT_TRUE(reallocTestProceed);
   realloc_test<MyMovable>();
   ASSERT_TRUE(reallocTestProceed);
   realloc_test<MyComplex>();
   ASSERT_TRUE(reallocTestProceed);
}

TEST(VarLengthArrayTest, testReverseIterators)
{
   VarLengthArray<int> v;
   v << 1 << 2 << 3 << 4;
   VarLengthArray<int> vr = v;
   std::reverse(vr.begin(), vr.end());
   const VarLengthArray<int> &cvr = vr;
   ASSERT_TRUE(std::equal(v.begin(), v.end(), vr.rbegin()));
   ASSERT_TRUE(std::equal(v.begin(), v.end(), vr.crbegin()));
   ASSERT_TRUE(std::equal(v.begin(), v.end(), cvr.rbegin()));
   
   ASSERT_TRUE(std::equal(vr.rbegin(), vr.rend(), v.begin()));
   ASSERT_TRUE(std::equal(vr.crbegin(), vr.crend(), v.begin()));
   ASSERT_TRUE(std::equal(cvr.rbegin(), cvr.rend(), v.begin()));
}

TEST(VarLengthArrayTest, testCount)
{
   {
      const VarLengthArray<int> list;
      ASSERT_EQ(list.length(), 0);
      ASSERT_EQ(list.count(), 0);
      ASSERT_EQ(list.size(), 0);
   }
   {
      VarLengthArray<int> list;
      list.append(0);
      ASSERT_EQ(list.length(), 1);
      ASSERT_EQ(list.count(), 1);
      ASSERT_EQ(list.size(), 1);
   }
   {
      VarLengthArray<int> list;
      list.append(0);
      list.append(1);
      ASSERT_EQ(list.length(), 2);
      ASSERT_EQ(list.count(), 2);
      ASSERT_EQ(list.size(), 2);
   }
   {
      VarLengthArray<int> list;
      list.append(0);
      list.append(1);
      list.append(2);
      ASSERT_EQ(list.length(), 3);
      ASSERT_EQ(list.count(), 3);
      ASSERT_EQ(list.size(), 3);
   }
   {
      VarLengthArray<int> list;
      list.append(0);
      list.append(1);
      list.append(2);
      ASSERT_EQ(list.length(), 3);
      ASSERT_EQ(list.count(), 3);
      ASSERT_EQ(list.size(), 3);
      list.removeLast();
      ASSERT_EQ(list.length(), 2);
      ASSERT_EQ(list.count(), 2);
      ASSERT_EQ(list.size(), 2);
      list.removeLast();
      ASSERT_EQ(list.length(), 1);
      ASSERT_EQ(list.count(), 1);
      ASSERT_EQ(list.size(), 1);
   }
}

TEST(VarLengthArrayTest, testFirst)
{
   VarLengthArray<int> list;
   list.append(27);
   ASSERT_EQ(list.first(), 27);
   list.append(4);
   ASSERT_EQ(list.first(), 27);
   list.append(1987);
   ASSERT_EQ(list.first(), 27);
   ASSERT_EQ(list.size(), 3);
   // remove some, make sure it stays sane
   list.removeLast();
   ASSERT_EQ(list.first(), 27);
   ASSERT_EQ(list.size(), 2);
   list.removeLast();
   ASSERT_EQ(list.first(), 27);
   ASSERT_EQ(list.size(), 1);
}

TEST(VarLengthArrayTest, testLast)
{
   VarLengthArray<int> list;
   list.append(27);
   ASSERT_EQ(list.last(), 27);
   list.append(4);
   ASSERT_EQ(list.last(), 4);
   list.append(1987);
   ASSERT_EQ(list.last(), 1987);
   ASSERT_EQ(list.size(), 3);
   // remove some, make sure it stays sane
   list.removeLast();
   ASSERT_EQ(list.last(), 4);
   ASSERT_EQ(list.size(), 2);
   list.removeLast();
   ASSERT_EQ(list.last(), 27);
   ASSERT_EQ(list.size(), 1);
}
