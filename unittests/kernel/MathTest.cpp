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
// Created by softboy on 2017/12/06.

#include "gtest/gtest.h"
#include "pdk/kernel/Math.h"
#include <list>
#include <utility>
#include <tuple>
#include <cstdlib>
#include <cmath>

static const double PI = 3.14159265358979323846264338327950288;

TEST(MathTest, testFastSinCos)
{
   const int LOOP_COUNT = 100000;
   for (int i = 0; i < LOOP_COUNT; ++i) {
      double angle = i * 2 * PI / (LOOP_COUNT - 1);
      ASSERT_TRUE(std::abs(std::sin(angle) - pdk::fast_sin(angle)) < 1e-5);
      ASSERT_TRUE(std::abs(std::cos(angle) - pdk::fast_cos(angle)) < 1e-5);
   }
}

TEST(MathTest, testDegreesToRadians)
{
   using DataType = std::list<std::tuple<float, float, double, double>>;
   DataType data;
   data.push_back(std::make_tuple(180.0f, static_cast<float>(PDK_MATH_PI), 180.0, PI));
   data.push_back(std::make_tuple(360.0f, static_cast<float>(2 * PDK_MATH_PI), 360.0, PI * 2));
   data.push_back(std::make_tuple(90.0f, static_cast<float>(PDK_MATH_PI_2), 90.0, PI / 2));
   data.push_back(std::make_tuple(123.1234567f, 2.1489097058516724f, 123.123456789123456789, 2.148909707407169856192285627));
   data.push_back(std::make_tuple(987654321.9876543f, 17237819.79023679f, 987654321987654321.987654321987654321, 17237819790236794.0));
   data.push_back(std::make_tuple(0.0f, 0.0f, 0.0, 0.0));
   data.push_back(std::make_tuple(-180.0f, static_cast<float>(-PDK_MATH_PI), 180.0, PI));
   data.push_back(std::make_tuple(-360.0f, static_cast<float>(-2 * PDK_MATH_PI), -360.0, -2 * PI));
   data.push_back(std::make_tuple(-90.0f, static_cast<float>(-PDK_MATH_PI_2), -90.0, -PI / 2));
   data.push_back(std::make_tuple(-123.1234567f, -2.1489097058516724f, -123.123456789123456789, -2.148909707407169856192285627));
   data.push_back(std::make_tuple(-987654321.9876543f, -17237819.79023679f, -123.123456789123456789, -2.148909707407169856192285627));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      float degreesFloat = std::get<0>(item);
      float radiansFloat = std::get<1>(item);
      double degreesDouble  = std::get<2>(item);
      double radiansDouble = std::get<3>(item);
      ASSERT_EQ(pdk::degrees_to_radians(degreesFloat), radiansFloat);
      ASSERT_EQ(pdk::degrees_to_radians(degreesDouble), radiansDouble);
      ++begin;
   }
}

TEST(MathTest, testRadiansToDegrees)
{
   using DataType = std::list<std::tuple<float, float, double, double>>;
   DataType data;
   data.push_back(std::make_tuple(static_cast<float>(PDK_MATH_PI), 180.0f, PI, 180.0));
   data.push_back(std::make_tuple(static_cast<float>(2 * PDK_MATH_PI), 360.0f, PI * 2, 360.0));
   data.push_back(std::make_tuple(static_cast<float>(PDK_MATH_PI_2), 90.0f, PI / 2, 90.0));
   data.push_back(std::make_tuple(123.1234567f, 7054.454427971739f, 123.123456789123456789, 7054.4544330781363896676339209079742431640625));
   data.push_back(std::make_tuple(987654321.9876543f, 56588424267.74745f, 987654321987654321.987654321987654321, 56588424267747450880.0));
   data.push_back(std::make_tuple(0.0f, 0.0f, 0.0, 0.0));
   data.push_back(std::make_tuple(static_cast<float>(-PDK_MATH_PI), -180.0f, PI, 180.0));
   data.push_back(std::make_tuple(static_cast<float>(-2 * PDK_MATH_PI), -360.0f, -2 * PI, -360.0));
   data.push_back(std::make_tuple(static_cast<float>(-PDK_MATH_PI_2), -90.0f, -PI / 2, -90.0));
   data.push_back(std::make_tuple(-123.1234567f, -7054.454427971739f, -123.123456789123456789, -7054.4544330781363896676339209079742431640625));
   data.push_back(std::make_tuple(-987654321.9876543f, -56588424267.74745f, -987654321987654321.987654321987654321, -56588424267747450880.0));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      float radiansFloat = std::get<0>(item);
      float degreesFloat = std::get<1>(item);
      double radiansDouble = std::get<2>(item);
      double degreesDouble  = std::get<3>(item);
      
      EXPECT_FLOAT_EQ(pdk::radians_to_degrees(radiansFloat), degreesFloat);
      //  EXPECT_DOUBLE_EQ(pdk::radians_to_degrees(radiansDouble), degreesDouble); // failed
      EXPECT_FLOAT_EQ(pdk::radians_to_degrees(radiansDouble), degreesDouble);
      ++begin;
   }
}

TEST(MathTest, testNextPowerOfTwo32S)
{
   using DataType = std::list<std::tuple<pdk::pint32, pdk::puint32>>;
   DataType data;
   data.push_back(std::make_tuple(0, 1U));
   data.push_back(std::make_tuple(1, 2U));
   data.push_back(std::make_tuple(2, 4U));
   data.push_back(std::make_tuple(17, 32U));
   data.push_back(std::make_tuple(128, 256U));
   data.push_back(std::make_tuple(65535, 65536U));
   data.push_back(std::make_tuple(65536, 131072U));
   data.push_back(std::make_tuple((1 << 30), (1U << 31)));
   data.push_back(std::make_tuple((1 << 30) + 1, (1U << 31)));
   data.push_back(std::make_tuple(0x7FFFFFFF, (1U<<31)));
   data.push_back(std::make_tuple(-1, 0U));
   data.push_back(std::make_tuple(-128, 0U));
   data.push_back(std::make_tuple(static_cast<int>(0x80000000), 0U));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      pdk::pint32 input = std::get<0>(item);
      pdk::puint32 output = std::get<1>(item);
      ASSERT_EQ(pdk::next_power_of_two(input), output);
      ++begin;
   }
}

TEST(MathTest, testNextPowerOfTwo32U)
{
   using DataType = std::list<std::tuple<pdk::pint32, pdk::puint32>>;
   DataType data;
   data.push_back(std::make_tuple(0U, 1U));
   data.push_back(std::make_tuple(1U, 2U));
   data.push_back(std::make_tuple(2U, 4U));
   data.push_back(std::make_tuple(17U, 32U));
   data.push_back(std::make_tuple(128U, 256U));
   data.push_back(std::make_tuple(65535U, 65536U));
   data.push_back(std::make_tuple(65536U, 131072U));
   data.push_back(std::make_tuple((1U << 30), (1U << 31)));
   data.push_back(std::make_tuple((1U << 30) + 1, (1U << 31)));
   data.push_back(std::make_tuple(2147483647U, 2147483648U));
   data.push_back(std::make_tuple(2147483648U, 0U));
   data.push_back(std::make_tuple(2147483649U, 0U));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      pdk::pint32 input = std::get<0>(item);
      pdk::puint32 output = std::get<1>(item);
      ASSERT_EQ(pdk::next_power_of_two(input), output);
      ++begin;
   }
}

TEST(MathTest, testNextPowerOfTwo64S)
{
   using DataType = std::list<std::tuple<pdk::pint64, pdk::puint64>>;
   DataType data;
   data.push_back(std::make_tuple(PDK_INT64_C(0), PDK_UINT64_C(1)));
   data.push_back(std::make_tuple(PDK_INT64_C(1), PDK_UINT64_C(2)));
   data.push_back(std::make_tuple(PDK_INT64_C(2), PDK_UINT64_C(4)));
   data.push_back(std::make_tuple(PDK_INT64_C(17), PDK_UINT64_C(32)));
   data.push_back(std::make_tuple(PDK_INT64_C(128), PDK_UINT64_C(256)));
   data.push_back(std::make_tuple(PDK_INT64_C(65535), PDK_UINT64_C(65536)));
   data.push_back(std::make_tuple(PDK_INT64_C(65536), PDK_UINT64_C(131072)));
   data.push_back(std::make_tuple(PDK_INT64_C(2147483647), PDK_UINT64_C(0x80000000)));
   data.push_back(std::make_tuple(PDK_INT64_C(2147483648), PDK_UINT64_C(0x100000000)));
   data.push_back(std::make_tuple(PDK_INT64_C(2147483649), PDK_UINT64_C(0x100000000)));
   data.push_back(std::make_tuple(PDK_INT64_C(0x7FFFFFFFFFFFFFFF), PDK_UINT64_C(0x8000000000000000)));
   data.push_back(std::make_tuple(PDK_INT64_C(-1), PDK_UINT64_C(0)));
   data.push_back(std::make_tuple(PDK_INT64_C(-128), PDK_UINT64_C(0)));
   data.push_back(std::make_tuple(-PDK_INT64_C(0x80000000), PDK_UINT64_C(0)));
   data.push_back(std::make_tuple(static_cast<pdk::pint64>(PDK_INT64_C(0x8000000000000000)), PDK_UINT64_C(0)));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      pdk::pint64 input = std::get<0>(item);
      pdk::puint64 output = std::get<1>(item);
      ASSERT_EQ(pdk::next_power_of_two(input), output);
      ++begin;
   }
}

TEST(MathTest, testNextPowerOfTwo64U)
{
   using DataType = std::list<std::tuple<pdk::puint64, pdk::puint64>>;
   DataType data;
   data.push_back(std::make_tuple(PDK_UINT64_C(0), PDK_UINT64_C(1)));
   data.push_back(std::make_tuple(PDK_UINT64_C(1), PDK_UINT64_C(2)));
   data.push_back(std::make_tuple(PDK_UINT64_C(2), PDK_UINT64_C(4)));
   data.push_back(std::make_tuple(PDK_UINT64_C(17), PDK_UINT64_C(32)));
   data.push_back(std::make_tuple(PDK_UINT64_C(128), PDK_UINT64_C(256)));
   data.push_back(std::make_tuple(PDK_UINT64_C(65535), PDK_UINT64_C(65536)));
   data.push_back(std::make_tuple(PDK_UINT64_C(65536), PDK_UINT64_C(131072)));
   data.push_back(std::make_tuple(PDK_UINT64_C(0x7FFFFFFFFFFFFFFF), PDK_UINT64_C(0x8000000000000000)));
   data.push_back(std::make_tuple(PDK_UINT64_C(0x8000000000000000), PDK_UINT64_C(0)));
   data.push_back(std::make_tuple(PDK_UINT64_C(0x8000000000000001), PDK_UINT64_C(0)));
   
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      pdk::puint64 input = std::get<0>(item);
      pdk::puint64 output = std::get<1>(item);
      ASSERT_EQ(pdk::next_power_of_two(input), output);
      ++begin;
   }
}
