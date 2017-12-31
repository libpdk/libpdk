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
// Created by zzu_softboy on 2017/12/31.

#include "gtest/gtest.h"
#include <list>
#include <utility>
#include <tuple>
#include <algorithm>
#include <string>

#include "pdk/base/ds/BitArray.h"

using pdk::ds::BitArray;

namespace
{

BitArray string_to_bitarray(const std::string &str)
{
   BitArray array;
   array.resize(str.size());
   char trueChar = '1';
   for (uint i = 0; i < str.length(); i++) {
      if (str.at(i) == trueChar) {
         array.setBit(i, true);
      }
   }
   return array;
}

}

TEST(BitArrayTest, testSize)
{
   using DataType = std::list<std::tuple<int, std::string>>;
   DataType data;
   data.push_back(std::make_tuple(1, "1"));
   data.push_back(std::make_tuple(2, "11"));
   data.push_back(std::make_tuple(3, "111"));
   data.push_back(std::make_tuple(9, "111111111"));
   data.push_back(std::make_tuple(10, "1111111111"));
   data.push_back(std::make_tuple(17, "11111111111111111"));
   data.push_back(std::make_tuple(18, "111111111111111111"));
   data.push_back(std::make_tuple(19, "1111111111111111111"));
   data.push_back(std::make_tuple(20, "11111111111111111111"));
   data.push_back(std::make_tuple(21, "111111111111111111111"));
   data.push_back(std::make_tuple(22, "1111111111111111111111"));
   data.push_back(std::make_tuple(23, "11111111111111111111111"));
   data.push_back(std::make_tuple(24, "111111111111111111111111"));
   data.push_back(std::make_tuple(25, "1111111111111111111111111"));
   data.push_back(std::make_tuple(32, "11111111111111111111111111111111"));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      int count = std::get<0>(item);
      std::string expected = std::get<1>(item);
      BitArray array(count);
      array.fill(1);
      std::string actual;
      int length = array.size();
      for (int i = 0; i < length; i++) {
         bool b = array[i];
         if (b) {
            actual += '1';
         } else {
            actual += '0';
         }
      }
      ASSERT_EQ(actual, expected);
      ++begin;
   }
}

TEST(BitArrayTest, testCountBits)
{
   using DataType = std::list<std::tuple<std::string, int, int>>;
   DataType data;
   data.push_back(std::make_tuple(std::string(), 0, 0));
   data.push_back(std::make_tuple(std::string("1"), 1, 1));
   data.push_back(std::make_tuple(std::string("101"), 3, 2));
   data.push_back(std::make_tuple(std::string("101100001"), 9, 4));
   data.push_back(std::make_tuple(std::string("101100001101100001"), 18, 8));
   data.push_back(std::make_tuple(std::string("101100001101100001101100001101100001"), 36, 16));
   data.push_back(std::make_tuple(std::string("00000000000000000000000000000000000"), 35, 0));
   data.push_back(std::make_tuple(std::string("00000000000000000000000000000000000"), 35, 0));
   data.push_back(std::make_tuple(std::string("11111111111111111111111111111111111"), 35, 35));
   data.push_back(std::make_tuple(std::string("11111111111111111111111111111111"), 32, 32));
   data.push_back(std::make_tuple(std::string("11111111111111111111111111111111111111111111111111111111"), 56, 56));
   data.push_back(std::make_tuple(std::string("00000000000000000000000000000000000"), 35, 0));
   data.push_back(std::make_tuple(std::string("00000000000000000000000000000000"), 32, 0));
   data.push_back(std::make_tuple(std::string("00000000000000000000000000000000000000000000000000000000"), 56, 0));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      std::string bitField = std::get<0>(item);
      int numBits = std::get<1>(item);
      int onBits = std::get<2>(item);
      
      BitArray bits(bitField.size());
      for (uint i = 0; i < bitField.size(); ++i) {
         if (bitField.at(i) == '1') {
            bits.setBit(i);
         }
      }
      ASSERT_EQ(bits.size(), numBits);
      ASSERT_EQ(bits.count(true), onBits);
      ASSERT_EQ(bits.count(false), numBits - onBits);
      ++begin;
   }
}

TEST(BitArrayTest, testCountBits2)
{
   BitArray array;
   for (int i = 0; i < 4017; ++i) {
      array.resize(i);
      array.fill(true);
      ASSERT_EQ(array.count(true), i);
      ASSERT_EQ(array.count(false), 0);
      array.fill(false);
      ASSERT_EQ(array.count(true), 0);
      ASSERT_EQ(array.count(false), i);
   }
}

TEST(BitArrayTest, testIsEmpty)
{
   BitArray array1;
   ASSERT_TRUE(array1.isEmpty());
   ASSERT_TRUE(array1.isNull());
   ASSERT_EQ(array1.size(), 0);
   
   BitArray array2(0, true);
   ASSERT_TRUE(array2.isEmpty());
   ASSERT_FALSE(array2.isNull());
   ASSERT_EQ(array2.size(), 0);
   
   BitArray array3(1, true);
   ASSERT_FALSE(array3.isEmpty());
   ASSERT_FALSE(array3.isNull());
   ASSERT_EQ(array3.size(), 1);
   
   array1.resize(0);
   ASSERT_TRUE(array1.isEmpty());
   ASSERT_FALSE(array1.isNull());
   ASSERT_EQ(array1.size(), 0);
   
   array2.resize(0);
   ASSERT_TRUE(array2.isEmpty());
   ASSERT_FALSE(array2.isNull());
   ASSERT_EQ(array2.size(), 0);
   
   array1.resize(1);
   ASSERT_FALSE(array1.isEmpty());
   ASSERT_FALSE(array1.isNull());
   ASSERT_EQ(array1.size(), 1);
   
   array1.resize(2);
   ASSERT_FALSE(array1.isEmpty());
   ASSERT_FALSE(array1.isNull());
   ASSERT_EQ(array1.size(), 2);
}

TEST(BitArrayTest, testSwap)
{
   BitArray array1 = string_to_bitarray("1");
   BitArray array2 = string_to_bitarray("10");
   array1.swap(array2);
   ASSERT_EQ(array1, string_to_bitarray("10"));
   ASSERT_EQ(array2, string_to_bitarray("1"));
}

#include <iostream>
TEST(BitArrayTest, testFill)
{
   int N = 64;
   int M = 17;
   BitArray array(N, false);
   int i;
   int j;
   for (i = 0; i < N - M; ++i) {
      array.fill(true, i, i + M);
      for (j = 0; j < N; ++j) {
         if (j >= i && j < i + M) {
            ASSERT_TRUE(array.at(j));
         } else {
            ASSERT_FALSE(array.at(j));
         }
      }
      array.fill(false, i, i + M);
   }
   for (i = 0; i < N; ++i) {
      array.fill(i % 2 == 0, i, i + 1);
   }
   for (i = 0; i < N; ++i) {
      ASSERT_TRUE(array.at(i) == (i % 2 == 0));
   }
}

TEST(BitArrayTest, testToggleBit)
{
   using DataType = std::list<std::tuple<int, BitArray, BitArray>>;
   DataType data;
   data.push_back(std::make_tuple(0, string_to_bitarray("11111111"), string_to_bitarray("01111111")));
   data.push_back(std::make_tuple(1, string_to_bitarray("11111111"), string_to_bitarray("10111111")));
   data.push_back(std::make_tuple(10, string_to_bitarray("11111111111"), string_to_bitarray("11111111110")));
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      int index = std::get<0>(item);
      BitArray input = std::get<1>(item);
      BitArray res = std::get<2>(item);
      input.toggleBit(index);
      ASSERT_EQ(input, res);
      ++begin;
   }
}

TEST(BitArrayTest, testOperatorAndEq)
{
   using DataType = std::list<std::tuple<BitArray, BitArray, BitArray>>;
   DataType data;
   data.push_back(std::make_tuple(string_to_bitarray("11111111"),
                                  string_to_bitarray("00101100"),
                                  string_to_bitarray("00101100")));
   
   data.push_back(std::make_tuple(string_to_bitarray("11011011"),
                                  string_to_bitarray("00101100"),
                                  string_to_bitarray("00001000")));
   
   data.push_back(std::make_tuple(string_to_bitarray("11011011111"),
                                  string_to_bitarray("00101100"),
                                  string_to_bitarray("00001000000")));
   
   data.push_back(std::make_tuple(string_to_bitarray("11011011"),
                                  string_to_bitarray("00101100111"),
                                  string_to_bitarray("00001000000")));
   
   data.push_back(std::make_tuple(string_to_bitarray(""),
                                  string_to_bitarray("00101100111"),
                                  string_to_bitarray("00000000000")));
   
   data.push_back(std::make_tuple(string_to_bitarray("00101100111"),
                                  string_to_bitarray(""),
                                  string_to_bitarray("00000000000")));
   
   data.push_back(std::make_tuple(string_to_bitarray(""),
                                  string_to_bitarray(""),
                                  string_to_bitarray("")));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      BitArray input1 = std::get<0>(item);
      BitArray input2 = std::get<1>(item);
      BitArray result = std::get<2>(item);
      input1 &= input2;
      ASSERT_EQ(input1, result);
      ++begin;
   }
}


TEST(BitArrayTest, testOperatorOrEq)
{
   using DataType = std::list<std::tuple<BitArray, BitArray, BitArray>>;
   DataType data;
   data.push_back(std::make_tuple(string_to_bitarray("11111111"),
                                  string_to_bitarray("00101100"),
                                  string_to_bitarray("11111111")));
   
   data.push_back(std::make_tuple(string_to_bitarray("11011011"),
                                  string_to_bitarray("00101100"),
                                  string_to_bitarray("11111111")));
   
   data.push_back(std::make_tuple(string_to_bitarray("01000010"),
                                  string_to_bitarray("10100001"),
                                  string_to_bitarray("11100011")));
   
   data.push_back(std::make_tuple(string_to_bitarray("11011011"),
                                  string_to_bitarray("00101100000"),
                                  string_to_bitarray("11111111000")));
   
   data.push_back(std::make_tuple(string_to_bitarray("11011011111"),
                                  string_to_bitarray("00101100"),
                                  string_to_bitarray("11111111111")));
   
   data.push_back(std::make_tuple(string_to_bitarray(""),
                                  string_to_bitarray("00101100111"),
                                  string_to_bitarray("00101100111")));
   
   data.push_back(std::make_tuple(string_to_bitarray("00101100111"),
                                  string_to_bitarray(""),
                                  string_to_bitarray("00101100111")));
   
   data.push_back(std::make_tuple(string_to_bitarray(""),
                                  string_to_bitarray(""),
                                  string_to_bitarray("")));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      BitArray input1 = std::get<0>(item);
      BitArray input2 = std::get<1>(item);
      BitArray result = std::get<2>(item);
      input1 |= input2;
      ASSERT_EQ(input1, result);
      ++begin;
   }
}

TEST(BitArrayTest, testOperatorXOrEq)
{
   using DataType = std::list<std::tuple<BitArray, BitArray, BitArray>>;
   DataType data;
   data.push_back(std::make_tuple(string_to_bitarray("11111111"),
                                  string_to_bitarray("00101100"),
                                  string_to_bitarray("11010011")));
   
   data.push_back(std::make_tuple(string_to_bitarray("11011011"),
                                  string_to_bitarray("00101100"),
                                  string_to_bitarray("11110111")));
   
   data.push_back(std::make_tuple(string_to_bitarray("01000010"),
                                  string_to_bitarray("10100001"),
                                  string_to_bitarray("11100011")));
   
   data.push_back(std::make_tuple(string_to_bitarray("01000010"),
                                  string_to_bitarray("10100001101"),
                                  string_to_bitarray("11100011101")));
   
   data.push_back(std::make_tuple(string_to_bitarray("01000010111"),
                                  string_to_bitarray("101000011"),
                                  string_to_bitarray("11100011011")));
   
   data.push_back(std::make_tuple(string_to_bitarray(""),
                                  string_to_bitarray("00101100111"),
                                  string_to_bitarray("00101100111")));
   
   data.push_back(std::make_tuple(string_to_bitarray("00101100111"),
                                  string_to_bitarray(""),
                                  string_to_bitarray("00101100111")));
   
   data.push_back(std::make_tuple(string_to_bitarray(""),
                                  string_to_bitarray(""),
                                  string_to_bitarray("")));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      BitArray input1 = std::get<0>(item);
      BitArray input2 = std::get<1>(item);
      BitArray result = std::get<2>(item);
      input1 ^= input2;
      ASSERT_EQ(input1, result);
      ++begin;
   }
}

TEST(BitArrayTest, testOperatorNg)
{
   using DataType = std::list<std::tuple<BitArray, BitArray>>;
   DataType data;
   data.push_back(std::make_tuple(string_to_bitarray("11111111"),
                                  string_to_bitarray("00000000")));
   
   data.push_back(std::make_tuple(string_to_bitarray("11011011"),
                                  string_to_bitarray("00100100")));
   
   data.push_back(std::make_tuple(string_to_bitarray("00000000"),
                                  string_to_bitarray("11111111")));
   
   data.push_back(std::make_tuple(string_to_bitarray(""),
                                  string_to_bitarray("")));
   
   data.push_back(std::make_tuple(string_to_bitarray("1"),
                                  string_to_bitarray("0")));
   
   data.push_back(std::make_tuple(string_to_bitarray("0"),
                                  string_to_bitarray("1")));
   
   data.push_back(std::make_tuple(string_to_bitarray("01"),
                                  string_to_bitarray("10")));
   
   data.push_back(std::make_tuple(string_to_bitarray("1110101"),
                                  string_to_bitarray("0001010")));
   
   data.push_back(std::make_tuple(string_to_bitarray("01110101"),
                                  string_to_bitarray("10001010")));
   
   data.push_back(std::make_tuple(string_to_bitarray("011101010"),
                                  string_to_bitarray("100010101")));
   
   data.push_back(std::make_tuple(string_to_bitarray("0111010101111010"),
                                  string_to_bitarray("1000101010000101")));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      BitArray input = std::get<0>(item);
      BitArray result = std::get<1>(item);
      input = ~input;
      ASSERT_EQ(input, result);
      ++begin;
   }
}

TEST(BitArrayTest, testInvertOnNull)
{
   BitArray array;
   ASSERT_EQ(array = ~array, BitArray());
}

TEST(BitArrayTest, testOperatorNotEq)
{
   using DataType = std::list<std::tuple<BitArray, BitArray, bool>>;
   DataType data;
   data.push_back(std::make_tuple(string_to_bitarray("11111111"),
                                  string_to_bitarray("00101100"),
                                  true));
   
   data.push_back(std::make_tuple(string_to_bitarray("11011011"),
                                  string_to_bitarray("11011011"),
                                  false));
   
   data.push_back(std::make_tuple(string_to_bitarray(""),
                                  string_to_bitarray("00101100111"),
                                  true));
   
   data.push_back(std::make_tuple(string_to_bitarray(""),
                                  string_to_bitarray(""),
                                  false));
   
   data.push_back(std::make_tuple(string_to_bitarray("00101100"),
                                  string_to_bitarray("11111111"),
                                  true));
   
   data.push_back(std::make_tuple(string_to_bitarray("00101100111"),
                                  string_to_bitarray(""),
                                  true));
   
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      BitArray input1 = std::get<0>(item);
      BitArray input2 = std::get<1>(item);
      bool res =  std::get<2>(item);
      bool b = input1 != input2;
      ASSERT_EQ(b, res);
      ++begin;
   }
}

TEST(BitArrayTest, testOperatorResize)
{
   BitArray array = string_to_bitarray("11");
   array.resize(10);
   ASSERT_EQ(array.size(), 10);
   ASSERT_EQ(array, string_to_bitarray("1100000000"));
   array.setBit(9);
   array.resize(9);
   ASSERT_EQ(array, string_to_bitarray("110000000"));
   array.resize(10);
   ASSERT_EQ(array, string_to_bitarray("1100000000"));
   array.resize(9);
   BitArray arrayB = string_to_bitarray("1111111111");
   arrayB &= array;
   ASSERT_EQ(arrayB, string_to_bitarray("1100000000"));
}
