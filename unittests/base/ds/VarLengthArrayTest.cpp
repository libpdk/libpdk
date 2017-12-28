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
