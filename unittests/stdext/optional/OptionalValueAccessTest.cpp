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
// Created by softboy on 2017/01/10.

#include "gtest/gtest.h"
#include "pdk/stdext/optional/Optional.h"
#include <list>
#include <utility>
#include <tuple>

using pdk::stdext::optional::Optional;
using pdk::stdext::optional::BadAccess;

struct IntWrapper
{
   int m_i;
   IntWrapper(int i) 
      : m_i(i)
   {}
   
   bool operator==(const IntWrapper &rhs) const
   {
      return m_i == rhs.m_i;
   }
};


template <typename T>
void test_function_value_or_for()
{
   Optional<T> oM0;
   Optional<T> oM1(1);
   const Optional<T> oC0;
   const Optional<T> oC1(2);
   ASSERT_TRUE(oM0.valueOr(5) == 5);
   ASSERT_TRUE(oM1.valueOr(5) == 1);
   ASSERT_TRUE(oC0.valueOr(5) == 5);
   ASSERT_TRUE(oC1.valueOr(5) == 2);
}

template <typename T>
void test_function_value_for()
{
   Optional<T> o0;
   Optional<T> o1(1);
   const Optional<T> oC(2);
   
   try {
      T& v = o1.value();
      ASSERT_TRUE(v == 1);
   }
   catch(...) {
      ASSERT_TRUE(false);
   }
   
   try {
      T const& v = oC.value();
      ASSERT_TRUE(v == 2);
   } catch(...) {
      ASSERT_TRUE(false);
   }
   
   ASSERT_THROW(o0.value(), BadAccess);
}

TEST(OptionalValueAccessTest, testFunctionValue)
{
   test_function_value_for<int>();
   test_function_value_for<double>();
   test_function_value_for<IntWrapper>();
}


struct FatToIntConverter
{
   static int sm_conversions;
   int m_val;
   FatToIntConverter(int val) : m_val(val)
   {}
   
   operator int() const
   {
      sm_conversions += 1;
      return m_val;
   }
};

int FatToIntConverter::sm_conversions = 0;

TEST(OptionalValueAccessTest, testFunctionValueOr)
{
   test_function_value_or_for<int>();
   test_function_value_or_for<double>();
   test_function_value_or_for<IntWrapper>();
   Optional<int> oi(1);
   ASSERT_TRUE(oi.valueOr(FatToIntConverter(2)) == 1);
   ASSERT_TRUE(FatToIntConverter::sm_conversions == 0);
   oi = pdk::stdext::none;
   ASSERT_TRUE(oi.valueOr(FatToIntConverter(2)) == 2);
   ASSERT_TRUE(FatToIntConverter::sm_conversions == 1);
}

struct FunM
{
   int operator()() { return 5; }
};

struct FunC
{
   int operator()() const
   {
      return 6;
   }
};

int funP ()
{
   return 7;
}

int throw_()
{
   throw int();
}

TEST(OptionalValueAccessTest, testFunctionValueOrEval)
{
   Optional<int> o1 = 1;
   Optional<int> oN;
   FunM funM;
   FunC funC;
   
   ASSERT_EQ(o1.valueOrEval(funM), 1);
   ASSERT_EQ(oN.valueOrEval(funM), 5);
   ASSERT_EQ(o1.valueOrEval(FunM()), 1);
   ASSERT_EQ(oN.valueOrEval(FunM()), 5);
   
   ASSERT_EQ(o1.valueOrEval(funC), 1);
   ASSERT_EQ(oN.valueOrEval(funC), 6);
   ASSERT_EQ(o1.valueOrEval(FunC()), 1);
   ASSERT_EQ(oN.valueOrEval(FunC()), 6);
   
   ASSERT_EQ(o1.valueOrEval(funP), 1);
   ASSERT_EQ(oN.valueOrEval(funP), 7);
   Optional<int>::ValueType ret1 = o1.valueOrEval([](){return 8;});
   Optional<int>::ValueType ret2 = oN.valueOrEval([](){return 8;});
   ASSERT_EQ(ret1, 1);
   ASSERT_EQ(ret2, 8);
   try {
      ASSERT_EQ(o1.valueOrEval(throw_), 1);
   } catch(...) {
      ASSERT_TRUE(false);
   }
   ASSERT_THROW(oN.valueOrEval(throw_), int);
}

const Optional<std::string> make_const_opt_val()
{
   return std::string("something");
}

TEST(OptionalValueAccessTest, testConstMove)
{
   std::string s5 = *make_const_opt_val();
   std::string s6 = make_const_opt_val().value();
   PDK_UNUSED(s5);
   PDK_UNUSED(s6);
}


namespace {

struct MoveOnly
{
   explicit MoveOnly(int){}
   MoveOnly(MoveOnly &&){}
   void operator=(MoveOnly &&);
private:
   MoveOnly(MoveOnly const&);
   void operator=(MoveOnly const&);
};

Optional<MoveOnly> make_move_only()
{
   return MoveOnly(1);
}

MoveOnly move_only_default()
{
   return MoveOnly(1);
}

}

TEST(OptionalValueAccessTest, testMoveOnlyGetters)
{
   MoveOnly m1 = *make_move_only();
   MoveOnly m2 = make_move_only().value();
   MoveOnly m3 = make_move_only().valueOr(MoveOnly(1));
   MoveOnly m4 = make_move_only().valueOrEval(move_only_default);
   PDK_UNUSED(m1);
   PDK_UNUSED(m2);
   PDK_UNUSED(m3);
   PDK_UNUSED(m4);
}

