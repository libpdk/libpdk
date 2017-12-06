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
// Created by softboy on 2017/12/05.

#include "gtest/gtest.h"
#include "pdk/kernel/Algorithms.h"
#include <list>
#include <utility>
#include <tuple>
#include <cstdlib>

namespace 
{

static constexpr const uint bitsSetInNibble[] = {
   0, 1, 1, 2, 1, 2, 2, 3,
   1, 2, 2, 3, 2, 3, 3, 4,
};

PDK_STATIC_ASSERT((sizeof bitsSetInNibble / sizeof *bitsSetInNibble) == 16);

constexpr uint bits_set_in_byte(pdk::puint8 data)
{
   return bitsSetInNibble[data & 0xF] + bitsSetInNibble[data >> 4];
}

constexpr uint bits_set_in_short(pdk::puint16 data)
{
   return bits_set_in_byte(data & 0xFF) + bits_set_in_byte(data >> 8);
}

constexpr uint bits_set_in_int(pdk::puint32 data)
{
   return bits_set_in_short(data & 0xFFFF) + bits_set_in_short(data >> 16);
}

constexpr uint bits_set_in_int64(pdk::puint64 data)
{
   return bits_set_in_int(data & 0xFFFFFFFF) + bits_set_in_int(data >> 32);
}

}

template <class T>
class AlgorithmsTest : public testing::Test
{};

using TestTypes = testing::Types
<pdk::puint8, pdk::puint16, pdk::puint32, pdk::puint64>;

TYPED_TEST_CASE(AlgorithmsTest, TestTypes);

TYPED_TEST(AlgorithmsTest, testPopulationCount)
{
   using DataType = std::list<std::tuple<pdk::puint64, uint>>;
   size_t sizeTestType = sizeof(TypeParam);
   DataType data;
   for (uint i = 0; i < UCHAR_MAX; i++) {
      const uchar byte = static_cast<uchar>(i);
      const uint bits = bits_set_in_byte(byte);
      const pdk::puint64 value = static_cast<pdk::puint64>(byte);
      const pdk::puint64 input = value << ((i % sizeTestType) * 8U);
      data.push_back(std::make_tuple(input, bits));
   }
   // @TODO not thread safe
   std::srand(std::time(0));
   // and some random ones:
   if (sizeTestType >= 8) {
      for (size_t i = 0; i < 1000; ++i) {
         const pdk::puint64 input = static_cast<pdk::puint64>(std::rand()) << 32 | static_cast<pdk::puint32>(std::rand());
         data.push_back(std::make_tuple(input, bits_set_in_int64(input)));
      }
   } else if (sizeTestType >= 2) {
      for (size_t i = 0; i < 1000 ; ++i) {
         const pdk::puint32 input = std::rand();
         if (sizeTestType >= 4) {
            data.push_back(std::make_tuple(static_cast<pdk::puint64>(input), bits_set_in_int(input)));
         } else {
            data.push_back(std::make_tuple(static_cast<pdk::puint64>(input & 0xFFFF), bits_set_in_short(input & 0xFFFF)));
         }
      }
   }
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      pdk::puint64 input = std::get<0>(item);
      uint expected = std::get<1>(item);
      const TypeParam value = static_cast<TypeParam>(input);
      ASSERT_EQ(pdk::kernel::population_count(value), expected);
      ++begin;
   }
}

TYPED_TEST(AlgorithmsTest, testCountTrailing)
{
   using DataType = std::list<std::tuple<pdk::puint64, uint>>;
   size_t sizeTestType = sizeof(TypeParam);
   DataType data;
   data.push_back(std::make_tuple(PDK_UINT64_C(0), static_cast<uint>(sizeTestType * 8)));
   for (uint i = 0; i < sizeTestType*8; ++i) {
      const pdk::puint64 input = PDK_UINT64_C(1) << i;
      data.push_back(std::make_tuple(input, i));
   }
   pdk::puint64 typemask;
   if (sizeTestType >= 8) {
      typemask = ~PDK_UINT64_C(0);
   } else {
      typemask = (PDK_UINT64_C(1) << (sizeTestType * 8)) - 1;
   }
   // @TODO not thread safe
   std::srand(std::time(0));
   // and some random ones:
   for (uint i = 0; i < sizeTestType * 8; ++i) {
      for (uint j = 0; j < sizeTestType * 3; ++j) {  // 3 is arbitrary
         const pdk::puint64 r = static_cast<pdk::puint64>(std::rand()) << 32 | static_cast<pdk::puint32>(std::rand());
         const pdk::puint64 b = PDK_UINT64_C(1) << i;
         const pdk::puint64 mask = ((~(b-1)) ^ b) & typemask;
         const pdk::puint64 input = (r & mask) | b;
         data.push_back(std::make_tuple(input, i));
      }
   }
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      pdk::puint64 input = std::get<0>(item);
      uint expected = std::get<1>(item);
      const TypeParam value = static_cast<TypeParam>(input);
      ASSERT_EQ(pdk::kernel::count_trailing_zero_bits(value), expected);
      ++begin;
   }
}
