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
// Created by zzu_softboy on 2017/12/18.

#include "gtest/gtest.h"
#include <list>
#include <utility>
#include <tuple>
#include <vector>
#include <algorithm>

#include "pdk/base/ds/ByteArray.h"

using pdk::ds::ByteArrayData;
using pdk::ds::ByteArrayDataPtr;
using pdk::ds::ByteArray;

namespace
{

struct StaticByteArrays
{
   struct Standard 
   {
      ByteArrayData m_data;
      const char m_string[8];
   }m_standard;
   
   struct NotNullTerminated
   {
      ByteArrayData m_data;
      const char m_string[8];
   }m_notNullTerminated;
   
   struct Shifted {
      ByteArrayData m_data;
      const char m_dummy;  // added to change offset of string
      const char m_string[8];
   } m_shifted;
   
   struct ShiftedNotNullTerminated {
      ByteArrayData m_data;
      const char m_dummy;  // added to change offset of string
      const char m_string[8];
   } m_shiftedNotNullTerminated;
};

const StaticByteArrays statics = {
   {PDK_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(4), "data"},
   {PDK_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(4), "dataBAD"},
   {PDK_STATIC_BYTE_DATA_HEADER_INITIALIZER_WITH_OFFSET(4, sizeof(ByteArrayData) + sizeof(char)), 0, "data"},
   {PDK_STATIC_BYTE_DATA_HEADER_INITIALIZER_WITH_OFFSET(4, sizeof(ByteArrayData) + sizeof(char)), 0, "dataBAD"}
};

const ByteArrayDataPtr staticStandard = {
   const_cast<ByteArrayData *>(&statics.m_standard.m_data)
};

const ByteArrayDataPtr staticNotNullTerminated = {
   const_cast<ByteArrayData *>(&statics.m_notNullTerminated.m_data)
};

const ByteArrayDataPtr staticShifted = {
   const_cast<ByteArrayData *>(&statics.m_shifted.m_data)
};

const ByteArrayDataPtr staticShiftedNotNullTerminated = {
   const_cast<ByteArrayData *>(&statics.m_shiftedNotNullTerminated.m_data)
};

template <typename T>
const T &verify_zero_termination(const T &t)
{
   return t;
}


ByteArray verify_zero_termination(const ByteArray &array)
{
   ByteArray::DataPtr baDataPtr = const_cast<ByteArray &>(array).getDataPtr();
   if (baDataPtr->m_ref.isShared()
       || baDataPtr->m_offset != ByteArray().getDataPtr()->m_offset) {
      return array;
   }
   int baSize = array.size();
   char baTerminator = array.getConstRawData()[baSize];
   
   if ('\0' != baTerminator) {
      
   }
   
   // Skip mutating checks on shared strings
   if (baDataPtr->m_ref.isShared()) {
      return array;
   }
   
   const char *baData = array.getConstRawData();
   const ByteArray baCopy(baData, baSize); // Deep copy
   
   const_cast<char *>(baData)[baSize] = 'x';
   if ('x' != array.getConstRawData()[baSize]) {
      return "*** Failed to replace null-terminator in "
             "result ('" + array + "') ***";
   }
   if (array != baCopy) {
      return  "*** Result ('" + array + "') differs from its copy "
                                        "after null-terminator was replaced ***";
   }
   const_cast<char *>(baData)[baSize] = '\0'; // Restore sanity
   return array;
}

}

TEST(ByteArrayTest, testConstructor)
{
   
}

TEST(ByteArrayTest, testConstByteArray)
{
   const char *ptr = "abc";
   ByteArray carray = ByteArray::fromRawData(ptr, 3);
   ASSERT_EQ(carray.getConstRawData(), ptr);
   carray.squeeze();
   ASSERT_EQ(carray.getConstRawData(), ptr);
   carray.detach();
   ASSERT_EQ(carray.capacaity(), 3);
   ASSERT_EQ(carray.size(), 3);
   ASSERT_NE(carray.getConstRawData(), ptr);
   ASSERT_EQ(carray.getConstRawData()[0], 'a');
   ASSERT_EQ(carray.getConstRawData()[1], 'b');
   ASSERT_EQ(carray.getConstRawData()[2], 'c');
   ASSERT_EQ(carray.getConstRawData()[3], '\0');
}

TEST(ByteArrayTest, testLeftJustified)
{
   ByteArray array;
   array = "PDK";
   ASSERT_EQ(array.leftJustified(5, '-'), ByteArray("PDK--"));
   ASSERT_EQ(array.leftJustified(4, '-'), ByteArray("PDK-"));
   ASSERT_EQ(array.leftJustified(4), ByteArray("PDK "));
   ASSERT_EQ(array.leftJustified(3), ByteArray("PDK"));
   ASSERT_EQ(array.leftJustified(2), ByteArray("PDK"));
   ASSERT_EQ(array.leftJustified(1), ByteArray("PDK"));
   ASSERT_EQ(array.leftJustified(0), ByteArray("PDK"));
   
   ByteArray n;
   ASSERT_TRUE(!n.leftJustified(3).isNull());
   ASSERT_EQ(array.leftJustified(4, ' ', true), ByteArray("PDK "));
   ASSERT_EQ(array.leftJustified(3, ' ', true), ByteArray("PDK"));
   ASSERT_EQ(array.leftJustified(2, ' ', true), ByteArray("PD"));
   ASSERT_EQ(array.leftJustified(1, ' ', true), ByteArray("P"));
   ASSERT_EQ(array.leftJustified(0, ' ', true), ByteArray(""));
   ASSERT_EQ(array, ByteArray("PDK"));
}

TEST(ByteArrayTest, testRightJustified)
{
   ByteArray array;
   array = "PDK";
   ASSERT_EQ(array.rightJustified(5, '-'), ByteArray("--PDK"));
   ASSERT_EQ(array.rightJustified(4, '-'), ByteArray("-PDK"));
   ASSERT_EQ(array.rightJustified(4), ByteArray(" PDK"));
   ASSERT_EQ(array.rightJustified(3), ByteArray("PDK"));
   ASSERT_EQ(array.rightJustified(2), ByteArray("PDK"));
   ASSERT_EQ(array.rightJustified(1), ByteArray("PDK"));
   ASSERT_EQ(array.rightJustified(0), ByteArray("PDK"));
   
   ByteArray n;
   ASSERT_TRUE(!n.rightJustified(3).isNull());
   ASSERT_EQ(array.rightJustified(4, '-', true), ByteArray("-PDK"));
   ASSERT_EQ(array.rightJustified(4, ' ', true), ByteArray(" PDK"));
   ASSERT_EQ(array.rightJustified(3, ' ', true), ByteArray("PDK"));
   ASSERT_EQ(array.rightJustified(2, ' ', true), ByteArray("PD"));
   ASSERT_EQ(array.rightJustified(1, ' ', true), ByteArray("P"));
   ASSERT_EQ(array.rightJustified(0, ' ', true), ByteArray(""));
   ASSERT_EQ(array, ByteArray("PDK"));
}

namespace
{

void prepare_prepend_data(std::list<ByteArray> &data)
{
   data.push_back(ByteArray(ByteArrayLiteral("data")));
   data.push_back(ByteArray(staticStandard));
   data.push_back(ByteArray(staticShifted));
   data.push_back(ByteArray(staticNotNullTerminated));
   data.push_back(ByteArray(staticShiftedNotNullTerminated));
   data.push_back(ByteArray("data"));
   data.push_back(ByteArray::fromRawData("data", 4));
   data.push_back(ByteArray::fromRawData("dataBAD", 4));
}

}

TEST(ByteArrayTest, testPrepend)
{
   ByteArray array("foo");
   ASSERT_EQ(array.prepend(static_cast<char *>(0)), ByteArray("foo"));
   ASSERT_EQ(array.prepend(ByteArray()), ByteArray("foo"));
   ASSERT_EQ(array.prepend("a"), ByteArray("afoo"));
   ASSERT_EQ(array.prepend("b"), ByteArray("bafoo"));
   ASSERT_EQ(array.prepend('c'), ByteArray("cbafoo"));
   ASSERT_EQ(array.prepend(-1, 'x'), ByteArray("cbafoo"));
   ASSERT_EQ(array.prepend(3, 'x'), ByteArray("xxxcbafoo"));
   ASSERT_EQ(array.prepend("\0 ", 2), ByteArray::fromRawData("\0 xxxcbafoo", 11));
}

TEST(ByteArrayTest, testPrependExtend)
{
   std::list<ByteArray> data;
   prepare_prepend_data(data);
   std::list<ByteArray>::iterator begin = data.begin();
   std::list<ByteArray>::iterator end = data.end();
   while (begin != end) {
      ByteArray array = *begin;
      ASSERT_EQ(ByteArray().prepend(array), ByteArray("data"));
      ASSERT_EQ(ByteArray("").prepend(array), ByteArray("data"));
      
      ASSERT_EQ(array.prepend(static_cast<char *>(0)), ByteArray("data"));
      ASSERT_EQ(array.prepend(ByteArray()), ByteArray("data"));
      ASSERT_EQ(array.prepend("a"), ByteArray("adata"));
      ASSERT_EQ(array.prepend(ByteArray("b")), ByteArray("badata"));
      ASSERT_EQ(array.prepend('c'), ByteArray("cbadata"));
      ASSERT_EQ(array.prepend(-1, 'x'), ByteArray("cbadata"));
      ASSERT_EQ(array.prepend(3, 'x'), ByteArray("xxxcbadata"));
      ASSERT_EQ(array.prepend("\0 ", 2), ByteArray::fromRawData("\0 xxxcbadata", 12));
      ASSERT_EQ(array.size(), 12);
      ++begin;
   }
   
}

TEST(ByteArrayTest, testAppend)
{
   ByteArray array("foo");
   ASSERT_EQ(array.append(static_cast<char *>(0)), ByteArray("foo"));
   ASSERT_EQ(array.append(ByteArray()), ByteArray("foo"));
   ASSERT_EQ(array.append("a"), ByteArray("fooa"));
   ASSERT_EQ(array.append("b"), ByteArray("fooab"));
   ASSERT_EQ(array.append('c'), ByteArray("fooabc"));
   ASSERT_EQ(array.append(-1, 'x'), ByteArray("fooabc"));
   ASSERT_EQ(array.append(3, 'x'), ByteArray("fooabcxxx"));
   ASSERT_EQ(array.append("\0"), ByteArray("fooabcxxx"));
   ASSERT_EQ(array.append("\0", 1), ByteArray::fromRawData("fooabcxxx", 10));
   ASSERT_EQ(array.size(), 10);
}

TEST(ByteArrayTest, testAppendExtended)
{
   std::list<ByteArray> data;
   prepare_prepend_data(data);
   std::list<ByteArray>::iterator begin = data.begin();
   std::list<ByteArray>::iterator end = data.end();
   while (begin != end) {
      ByteArray array = *begin;
      ASSERT_EQ(ByteArray().append(array), ByteArray("data"));
      ASSERT_EQ(ByteArray("").append(array), ByteArray("data"));
      
      ASSERT_EQ(array.append(static_cast<char *>(0)), ByteArray("data"));
      ASSERT_EQ(array.append(ByteArray()), ByteArray("data"));
      ASSERT_EQ(array.append("a"), ByteArray("dataa"));
      ASSERT_EQ(array.append(ByteArray("b")), ByteArray("dataab"));
      ASSERT_EQ(array.append('c'), ByteArray("dataabc"));
      ASSERT_EQ(array.append(-1, 'x'), ByteArray("dataabc"));
      ASSERT_EQ(array.append(3, 'x'), ByteArray("dataabcxxx"));
      ASSERT_EQ(array.append("\0"), ByteArray("dataabcxxx"));
      ASSERT_EQ(array.append("\0", 1), ByteArray::fromRawData("dataabcxxx\0 ", 11));
      ASSERT_EQ(array.size(), 11);
      ++begin;
   }
}

TEST(ByteArrayTest, testMid)
{
   ByteArray data("zzu_softboy");
   ASSERT_EQ(data.mid(0, 3), ByteArray("zzu"));
   ASSERT_EQ(data.mid(0, -1), ByteArray("zzu_softboy"));
   ASSERT_EQ(data.mid(1, -1), ByteArray("zu_softboy"));
   ASSERT_EQ(data.mid(-10, 1), ByteArray());
   ASSERT_EQ(data.mid(-10, -1), ByteArray("zzu_softboy"));
   ASSERT_EQ(data.mid(-11, 22), ByteArray("zzu_softboy"));
   ASSERT_EQ(data.mid(-11, 23), ByteArray("zzu_softboy"));
   ASSERT_EQ(data.mid(-5, 6), ByteArray("z"));
   ASSERT_EQ(data.mid(0, 11), ByteArray("zzu_softboy"));
}

TEST(ByteArrayTest, testStartsWith)
{
   using DataType = std::list<std::tuple<ByteArray, ByteArray, bool>>;
   DataType data;
   data.push_back(std::make_tuple(ByteArray(), ByteArray(), true));
   data.push_back(std::make_tuple(ByteArray(), ByteArray(""), true));
   data.push_back(std::make_tuple(ByteArray(), ByteArray("hello"), false));
   data.push_back(std::make_tuple(ByteArray(""), ByteArray(), true));
   data.push_back(std::make_tuple(ByteArray(""), ByteArray(""), true));
   data.push_back(std::make_tuple(ByteArray(""), ByteArray("h"), false));
   data.push_back(std::make_tuple(ByteArray("hello"), ByteArray("h"), true));
   data.push_back(std::make_tuple(ByteArray("hello"), ByteArray("hello"), true));
   data.push_back(std::make_tuple(ByteArray("hello"), ByteArray(""), true));
   data.push_back(std::make_tuple(ByteArray("hello"), ByteArray("hellohello"), false));
   data.push_back(std::make_tuple(ByteArray("hello"), ByteArray(""), true));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      ByteArray left = std::get<0>(item);
      ByteArray right = std::get<1>(item);
      bool result = std::get<2>(item);
      ASSERT_TRUE(left.startsWith(right) == result);
      if (right.isNull()) {
         ASSERT_TRUE(left.startsWith(static_cast<char *>(0)) == result);
      } else {
         ASSERT_TRUE(left.startsWith(right.getRawData()) == result);
      }
      ++begin;
   }
}

TEST(ByteArrayTest, testStartsWithChar)
{
   ASSERT_TRUE(ByteArray("hello").startsWith('h'));
   ASSERT_FALSE(ByteArray("hello").startsWith('\0'));
   ASSERT_FALSE(ByteArray("hello").startsWith('o'));
   ASSERT_TRUE(ByteArray("h").startsWith('h'));
   ASSERT_FALSE(ByteArray("h").startsWith('\0'));
   ASSERT_FALSE(ByteArray("h").startsWith('o'));
   ASSERT_FALSE(ByteArray("hello").startsWith('l'));
   ASSERT_FALSE(ByteArray("").startsWith('h'));
   ASSERT_FALSE(ByteArray("").startsWith('\0'));
   ASSERT_FALSE(ByteArray().startsWith('o'));
   ASSERT_FALSE(ByteArray().startsWith('\0'));
}

TEST(ByteArrayTest, testEndsWith)
{
   using DataType = std::list<std::tuple<ByteArray, ByteArray, bool>>;
   DataType data;
   data.push_back(std::make_tuple(ByteArray(), ByteArray(), true));
   data.push_back(std::make_tuple(ByteArray(), ByteArray(""), true));
   data.push_back(std::make_tuple(ByteArray(), ByteArray("hello"), false));
   data.push_back(std::make_tuple(ByteArray(""), ByteArray(), true));
   data.push_back(std::make_tuple(ByteArray(""), ByteArray(""), true));
   data.push_back(std::make_tuple(ByteArray(""), ByteArray("h"), false));
   data.push_back(std::make_tuple(ByteArray("hello"), ByteArray("o"), true));
   data.push_back(std::make_tuple(ByteArray("hello"), ByteArray("hello"), true));
   data.push_back(std::make_tuple(ByteArray("hello"), ByteArray(""), true));
   data.push_back(std::make_tuple(ByteArray("hello"), ByteArray("hellohello"), false));
   data.push_back(std::make_tuple(ByteArray("hello"), ByteArray(""), true));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      ByteArray left = std::get<0>(item);
      ByteArray right = std::get<1>(item);
      bool result = std::get<2>(item);
      ASSERT_TRUE(left.endsWith(right) == result);
      if (right.isNull()) {
         ASSERT_TRUE(left.endsWith(static_cast<char *>(0)) == result);
      } else {
         ASSERT_TRUE(left.endsWith(right.getRawData()) == result);
      }
      ++begin;
   }
}

TEST(ByteArrayTest, testEndsWithChar)
{
   ASSERT_TRUE(ByteArray("hello").endsWith('o'));
   ASSERT_FALSE(ByteArray("hello").endsWith('\0'));
   ASSERT_FALSE(ByteArray("hello").endsWith('h'));
   ASSERT_TRUE(ByteArray("h").endsWith('h'));
   ASSERT_FALSE(ByteArray("h").endsWith('\0'));
   ASSERT_FALSE(ByteArray("h").endsWith('o'));
   ASSERT_FALSE(ByteArray("hello").endsWith('l'));
   ASSERT_FALSE(ByteArray("").endsWith('h'));
   ASSERT_FALSE(ByteArray("").endsWith('\0'));
   ASSERT_FALSE(ByteArray("").endsWith('a'));
   ASSERT_FALSE(ByteArray().endsWith('a'));
   ASSERT_FALSE(ByteArray().endsWith('\0'));
}

TEST(ByteArrayTest, testIterators)
{
   {
      ByteArray array;
      ByteArray::iterator begin = array.begin();
      ByteArray::iterator end = array.end();
      ASSERT_EQ(begin, end);
   }
   {
      ByteArray array("a");
      ByteArray::iterator begin = array.begin();
      ByteArray::iterator end = array.end();
      ASSERT_NE(begin, end);
      ++begin;
      ASSERT_EQ(begin, end);
   }
   {
      std::list<char> expected{'a', 'b', 'c'};
      std::list<char> actual;
      ByteArray array("abc");
      ByteArray::iterator begin = array.begin();
      ByteArray::iterator end = array.end();
      while (begin != end) {
         actual.push_back(*begin);
         ++begin;
      }
      ASSERT_EQ(actual, expected);
   }
   {
      std::list<char> expected{'a', 'b', 'c'};
      std::list<char> actual;
      const ByteArray array("abc");
      ByteArray::const_iterator begin = array.begin();
      ByteArray::const_iterator end = array.end();
      while (begin != end) {
         actual.push_back(*begin);
         ++begin;
      }
      ASSERT_EQ(actual, expected);
   }
   {
      std::list<char> expected{'a', 'b', 'c'};
      std::list<char> actual;
      const ByteArray array("abc");
      ByteArray::const_iterator begin = array.cbegin();
      ByteArray::const_iterator end = array.cend();
      while (begin != end) {
         actual.push_back(*begin);
         ++begin;
      }
      ASSERT_EQ(actual, expected);
   }
}

TEST(ByteArrayTest, testReverseIterators)
{
   ByteArray data = "abcd";
   ByteArray reverseData = data;
   std::reverse(reverseData.begin(), reverseData.end());
   const ByteArray &constReverseData = reverseData;
   ASSERT_TRUE(std::equal(data.begin(), data.end(), reverseData.rbegin()));
   ASSERT_TRUE(std::equal(data.begin(), data.end(), reverseData.crbegin()));
   ASSERT_TRUE(std::equal(data.begin(), data.end(), constReverseData.rbegin()));
   ASSERT_TRUE(std::equal(reverseData.rbegin(), reverseData.rend(), data.begin()));
   ASSERT_TRUE(std::equal(reverseData.crbegin(), reverseData.crend(), data.begin()));
   ASSERT_TRUE(std::equal(constReverseData.rbegin(), constReverseData.rend(), data.begin()));
}

TEST(ByteArrayTest, testInsert)
{
   ByteArray array("Meal");
   ASSERT_EQ(array.insert(1, ByteArray("ontr")), ByteArray("Montreal"));
   ASSERT_EQ(array.insert(array.size(), ByteArray("foo")), ByteArray("Montrealfoo"));
   
   array = "13";
   ASSERT_EQ(array.insert(1, ByteArray("2")), ByteArray("123"));
   
   array = "ac";
   ASSERT_EQ(array.insert(1, 'b'), ByteArray("abc"));
   
   array = "ac";
   ASSERT_EQ(array.insert(-1, 3, 'b'), ByteArray("ac"));
   ASSERT_EQ(array.insert(1, 3, 'x'), ByteArray("axxxc"));
   ASSERT_EQ(array.insert(6, 3, 'x'), ByteArray("axxxc xxx"));
   ASSERT_EQ(array.size(), 9);
   
   array = "ikl";
   ASSERT_EQ(array.insert(1, 'j'), ByteArray("ijkl"));
   ASSERT_EQ(array.size(), 4);
   
   array = "ab";
   ASSERT_EQ(array.insert(1, "\0X\0", 3), ByteArray::fromRawData("a\0X\0b", 5));
   ASSERT_EQ(array.size(), 5);
}

TEST(ByteArrayTest, testInsertExtended)
{
   std::list<ByteArray> data;
   prepare_prepend_data(data);
   std::list<ByteArray>::iterator begin = data.begin();
   std::list<ByteArray>::iterator end = data.end();
   while (begin != end) {
      ByteArray array = *begin;
      ASSERT_EQ(array.insert(1, "i"), ByteArray("diata"));
      ASSERT_EQ(array.insert(1, 3, 'x'), ByteArray("dxxxiata"));
      ASSERT_EQ(array.size(), 8);
      ++begin;
   }
   
}

TEST(ByteArrayTest, testToUpperAndLowercase)
{
   using DataType = std::list<std::tuple<ByteArray, ByteArray, ByteArray>>;
   DataType data;
   data.push_back(std::make_tuple(ByteArray(), ByteArray(), ByteArray()));
   data.push_back(std::make_tuple(ByteArrayLiteral("Hello World"), 
                                  ByteArrayLiteral("HELLO WORLD"), 
                                  ByteArrayLiteral("hello world")));
   data.push_back(std::make_tuple(ByteArrayLiteral("Hello World, this is a STRING"),
                                  ByteArrayLiteral("HELLO WORLD, THIS IS A STRING"), 
                                  ByteArrayLiteral("hello world, this is a string")));
   data.push_back(std::make_tuple(ByteArrayLiteral("R\311sum\351"),
                                  ByteArrayLiteral("R\311SUM\311"), 
                                  ByteArrayLiteral("r\351sum\351")));
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      ByteArray input = std::get<0>(item);
      ByteArray uppercase = std::get<1>(item);
      ByteArray lowercase = std::get<2>(item);
      ASSERT_EQ(lowercase.toLower(), lowercase);
      ++begin;
   }
}
