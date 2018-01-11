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
// Created by softboy on 2017/01/11.

#include "gtest/gtest.h"
#include "pdk/stdext/optional/Optional.h"
#include <type_traits>

using pdk::stdext::optional::Optional;
using pdk::stdext::make_optional;


template <typename Expected, typename Deduced>
void verify_type(Deduced)
{
   ASSERT_TRUE((std::is_same<Expected, Deduced>::value));
}

namespace {

struct MoveOnly
{
   int value;
   explicit MoveOnly(int i) : value(i) {}
   MoveOnly(MoveOnly && r) : value(r.value) { r.value = 0; }
   MoveOnly& operator=(MoveOnly && r) { value = r.value; r.value = 0; return *this; }
   
private:
   MoveOnly(MoveOnly const&);
   void operator=(MoveOnly const&);
};

MoveOnly make_move_only(int i)
{
   return MoveOnly(i);
}

}

TEST(MakeOptionalTest, testMakeOptionalForMoveOnlyType)
{
   verify_type<Optional<MoveOnly>>(make_optional(make_move_only(2)));
   verify_type<Optional<MoveOnly>>(make_optional(true, make_move_only(2)));
   
   Optional<MoveOnly> o1 = make_optional(make_move_only(1));
   ASSERT_TRUE(o1);
   ASSERT_EQ (1, o1->value);
   
   Optional<MoveOnly> o2 = make_optional(true, make_move_only(2));
   ASSERT_TRUE(o2);
   ASSERT_EQ (2, o2->value);
   
   Optional<MoveOnly> oN = make_optional(false, make_move_only(2));
   ASSERT_TRUE(!oN);
}

TEST(MakeOptionalTest, testNestedMakeOptional)
{
   verify_type<Optional<Optional<int>>>(make_optional(make_optional(1)));
   verify_type<Optional<Optional<int>>>(make_optional(true, make_optional(true, 2)));
   
   Optional<Optional<int>> oo1 = make_optional(make_optional(1));
   ASSERT_TRUE(oo1);
   ASSERT_TRUE(*oo1);
   ASSERT_EQ(1, **oo1);
   
   Optional<Optional<int>> oo2 = make_optional(true, make_optional(true, 2));
   ASSERT_TRUE(oo2);
   ASSERT_TRUE(*oo2);
   ASSERT_EQ (2, **oo2);
   
   Optional<Optional<int>> oo3 = make_optional(true, make_optional(false, 3));
   ASSERT_TRUE(oo3);
   ASSERT_TRUE(!*oo3);
   
   Optional<Optional<int>> oo4 = make_optional(false, make_optional(true, 4));
   ASSERT_TRUE(!oo4);
}
