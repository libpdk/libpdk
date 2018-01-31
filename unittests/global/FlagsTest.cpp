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
// Created by softboy on 2017/11/14.

#include "gtest/gtest.h"
#include "pdk/global/Flags.h"
#include "pdk/global/TypeTraits.h"
#include <list>
#include <utility>

namespace 
{

enum Option
{
   NoOptions = 0x0,
   ShowTabs = 0x1,
   ShowAll = 0x2,
   SqueezeBlank = 0x4,
   CloseBtn = 0x8
};

PDK_DECLARE_FLAGS(Options, Option);
PDK_DECLARE_OPERATORS_FOR_FLAGS(Options)

template <unsigned int N, typename T>
bool verify_const_expr(T n)
{
   return n == N;
}

}

TEST(FlagsTest, testClassicEnum)
{
   Options opts = Option::ShowAll | Option::ShowTabs;
   ASSERT_EQ(static_cast<Options::UnderType>(opts), 3);
   ASSERT_TRUE(opts.testFlag(Option::ShowAll));
   ASSERT_TRUE(opts.testFlag(Option::ShowTabs));
   opts &= ~Option::ShowTabs;
   ASSERT_EQ(static_cast<Options::UnderType>(opts), 2);
   opts = 0;
   ASSERT_FALSE(opts.testFlag(Option::ShowAll));
   ASSERT_FALSE(opts.testFlag(Option::ShowTabs));
}

TEST(FlagTest, testConstExpr)
{
   Options opts = Option::ShowTabs | Option::ShowAll;
   switch (opts) {
   case Option::ShowTabs:
      ASSERT_TRUE(false);
      break;
   case Option::ShowAll:
      ASSERT_TRUE(false);
      break;
   case static_cast<Options::UnderType>(Option::ShowTabs | Option::ShowAll):
      ASSERT_TRUE(true);
      break;
   default:
      ASSERT_TRUE(false);
      break;
   }
   ASSERT_TRUE(verify_const_expr<Option::ShowTabs | Option::ShowAll>
               (3));
   ASSERT_TRUE(verify_const_expr<(Option::ShowTabs | Option::ShowAll) &
               Option::ShowTabs>(Option::ShowTabs));
}

TEST(FlagsTest, testSignedness)
{
//   PDK_STATIC_ASSERT(pdk::internal::IsUnsignedClassicEnum<Option>::value ==
//                     pdk::internal::IsUnsignedClassicEnum<Options::UnderType>::value);
//   PDK_STATIC_ASSERT(pdk::internal::IsSignedClassicEnum<Option>::value ==
//                     pdk::internal::IsSignedClassicEnum<Options::UnderType>::value);
}

namespace
{
enum class MyClassTypeEnum
{
   Opt1 = 1,
   Opt2,
   Opt3,
   Opt4
};

PDK_DECLARE_FLAGS(MyClassTypeEnums, MyClassTypeEnum);
PDK_DECLARE_OPERATORS_FOR_FLAGS(MyClassTypeEnums)

enum class MyNoOpClassTypeEnum
{
   Opt1 = 1,
         Opt2,
         Opt3,
         Opt4
};

PDK_DECLARE_FLAGS(MyNoOpClassTypeEnums, MyNoOpClassTypeEnum);

}

TEST(FlagsTest, testClassEnum)
{
   MyClassTypeEnum opt1 = MyClassTypeEnum::Opt1;
   MyClassTypeEnum opt2 = MyClassTypeEnum::Opt2;
   MyClassTypeEnums flag1(opt1);
   ASSERT_EQ(static_cast<Options::UnderType>(flag1), 1);
   MyClassTypeEnums flag2(opt2);
   ASSERT_EQ(static_cast<Options::UnderType>(flag2), 2);
   MyClassTypeEnums flag3;
   ASSERT_EQ(static_cast<Options::UnderType>(flag3), 0);
   MyClassTypeEnums flag4(opt1 | opt2);
   ASSERT_EQ(static_cast<Options::UnderType>(flag4), 3);
   ASSERT_TRUE(flag4.testFlag(MyClassTypeEnum::Opt1));
   ASSERT_FALSE(flag4.testFlag(MyClassTypeEnum::Opt4));
   ASSERT_FALSE(flag3);
   ASSERT_EQ(static_cast<Options::UnderType>(flag4 & 1), 1);
   
   ASSERT_EQ(static_cast<Options::UnderType>(flag4 & int(1)), 1);
   ASSERT_EQ(static_cast<Options::UnderType>(flag4 & uint(1)), 1);
   ASSERT_EQ(static_cast<Options::UnderType>(flag4 & MyClassTypeEnum::Opt1), 1);
   
   MyClassTypeEnums aux;
   aux = flag4;
   aux &= int(1);
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 1);
   
   aux = flag4;
   aux &= uint(1);
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 1);
   
   aux = flag4;
   aux &= MyClassTypeEnum::Opt2;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 2);
   
   aux = flag4;
   aux &= MyClassTypeEnum::Opt2;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 2);
   
   aux = flag4 ^ flag4;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 0);
   
   aux = flag4 ^ flag1;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 2);
   
   aux = flag4 ^ flag3;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 3);
   
   aux = flag4;
   aux ^= MyClassTypeEnum::Opt1;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 2);
   
   aux = flag4;
   aux ^= MyClassTypeEnum::Opt2;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 1);
   
   aux = flag1 | flag2;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 3);
   
   aux = MyClassTypeEnum::Opt1 | MyClassTypeEnum::Opt2;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 3);
   
   aux = flag1;
   aux |= flag2;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 3);
   
   aux = MyClassTypeEnum::Opt1;
   aux |= MyClassTypeEnum::Opt2;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), 3);
   
   aux = ~flag1;
   ASSERT_EQ(static_cast<Options::UnderType>(aux), -2);
}

TEST(FlagsTest, testInitializerLists)
{
   Options opts = {
      Option::ShowAll,
      Option::ShowTabs
   };
   ASSERT_TRUE(opts.testFlag(Option::ShowAll));
   ASSERT_TRUE(opts.testFlag(Option::ShowTabs));
   MyClassTypeEnums flags = {
      MyClassTypeEnum::Opt1,
      MyClassTypeEnum::Opt2
   };
   ASSERT_TRUE(flags.testFlag(MyClassTypeEnum::Opt1));
   ASSERT_TRUE(flags.testFlag(MyClassTypeEnum::Opt2));
}

TEST(FlagsTest, testSetFlags)
{
   MyClassTypeEnums flags;
   flags.setFlag(MyClassTypeEnum::Opt1, true);
   ASSERT_TRUE(flags.testFlag(MyClassTypeEnum::Opt1));
   flags.setFlag(MyClassTypeEnum::Opt1, false);
   ASSERT_FALSE(flags.testFlag(MyClassTypeEnum::Opt1));
}


