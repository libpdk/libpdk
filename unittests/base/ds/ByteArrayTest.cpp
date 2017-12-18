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
   array = "ABC";
   ASSERT_EQ(array.leftJustified(5, '-'), ByteArray("ABC--"));
   ASSERT_EQ(array.leftJustified(4, '-'), ByteArray("ABC-"));
   ASSERT_EQ(array.leftJustified(4), ByteArray("ABC "));
   ASSERT_EQ(array.leftJustified(3), ByteArray("ABC"));
   ASSERT_EQ(array.leftJustified(2), ByteArray("ABC"));
   ASSERT_EQ(array.leftJustified(1), ByteArray("ABC"));
   ASSERT_EQ(array.leftJustified(0), ByteArray("ABC"));
   
   ByteArray n;
   ASSERT_TRUE(!n.leftJustified(3).isNull());
   ASSERT_EQ(array.leftJustified(4, ' ', true), ByteArray("ABC "));
   ASSERT_EQ(array.leftJustified(3, ' ', true), ByteArray("ABC"));
   ASSERT_EQ(array.leftJustified(2, ' ', true), ByteArray("AB"));
   ASSERT_EQ(array.leftJustified(1, ' ', true), ByteArray("A"));
   ASSERT_EQ(array.leftJustified(0, ' ', true), ByteArray(""));
}
