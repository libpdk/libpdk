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
#include "pdk/global/Numeric.h"
#include <list>
#include <utility>

namespace
{

void init_overflow_data(std::list<int> &data)
{
   data.push_back(8);
   data.push_back(16);
   data.push_back(32);
   data.push_back(64);
   data.push_back(48); // it's either 32- or 64-bit, so on average it's 48 :-)
}

template <typename Int>
static void add_overflow_template()
{
#if defined(PDK_CC_MSVC) && PDK_CC_MSVC < 2000
   SUCCEED() << "this test generates an Internal Compiler Error compiling in release mode";
#endif
   const Int max = std::numeric_limits<Int>::max();
   Int result;
   
   // basic values
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(0), static_cast<Int>(0), &result), false);
   ASSERT_EQ(result, static_cast<Int>(0));
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(1), static_cast<Int>(0), &result), false);
   ASSERT_EQ(result, static_cast<Int>(1));
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(0), static_cast<Int>(1), &result), false);
   ASSERT_EQ(result, static_cast<Int>(1));
   
   // half-way through max
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(max/2), static_cast<Int>(max/2), &result), false);
   ASSERT_EQ(result, static_cast<Int>(max/2 * 2));
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(max/2 - 1), static_cast<Int>(max/2 + 1), &result), false);
   ASSERT_EQ(result, static_cast<Int>(max/2 * 2));
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(max/2 + 1), static_cast<Int>(max/2), &result), false);
   ASSERT_EQ(result, static_cast<Int>(max));
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(max/2), static_cast<Int>(max/2 + 1), &result), false);
   ASSERT_EQ(result, static_cast<Int>(max));
   
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(max / 4 * 3), static_cast<Int>(max/4), &result), false);
   ASSERT_EQ(result, static_cast<Int>(max / 4 * 4));
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(max), static_cast<Int>(0), &result), false);
   ASSERT_EQ(result, static_cast<Int>(max));
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(0), static_cast<Int>(max), &result), false);
   ASSERT_EQ(result, static_cast<Int>(max));
   
   if (max > std::numeric_limits<uint>::max()) {
      ASSERT_EQ(pdk::add_overflow(static_cast<Int>(std::numeric_limits<uint>::max()), 
                                  static_cast<Int>(std::numeric_limits<uint>::max()), &result), false);
      ASSERT_EQ(result, static_cast<Int>(2 * static_cast<Int>(std::numeric_limits<uint>::max())));
   }
   ASSERT_EQ(pdk::add_overflow(max, static_cast<Int>(1), &result), true);
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(1), max, &result), true);
   ASSERT_EQ(pdk::add_overflow(static_cast<Int>(max/2 + 1), static_cast<Int>(max/2 + 1), &result), true);
}

} 

TEST(NumericTest, testAddOverflow)
{
   std::list<int> data;
   init_overflow_data(data);
   std::list<int>::iterator begin = data.begin();
   std::list<int>::iterator end = data.end();
   while (begin != end) {
      int size = *begin;
      if (8 == size) {
         add_overflow_template<pdk::puint8>();
      } else if (16 == size) {
         add_overflow_template<pdk::puint16>();
      } else if (32 == size) {
         add_overflow_template<pdk::puint32>();
      } else if (48 == size) {
         add_overflow_template<ulong>();
      } else if (64 == size) {
         add_overflow_template<pdk::puint64>();
      }
      ++begin;
   }
}
