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
