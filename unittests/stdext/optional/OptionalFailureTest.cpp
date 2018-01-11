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
// Created by softboy on 2017/01/09.

#include "gtest/gtest.h"
#include "pdk/stdext/optional/Optional.h"

using pdk::stdext::optional::Optional;

TEST(OptionalFailureTest, testDeepConstantness)
{
   Optional<int> opt;
   const Optional<int> copt;
   // error: cannot assign to return value because function 'operator*' returns a const value
   // *copt = opt;
   PDK_UNUSED(opt);
   PDK_UNUSED(copt);
}

TEST(OptionalFailureTest, testNoImplicitConversion)
{
   Optional<int> opt(1);
   // You can compare against 0 or against another Optional<>,
   // but not against another value
   if (opt == 1)
   {
      SUCCEED();
   } else{
      FAIL();
   }
}

TEST(OptionalFailureTest, testExplicitConstructor)
{
   Optional<int> opt = 1;
   PDK_UNUSED(opt);
}


TEST(OptionalFailureTest, testNoUnsupportedConversion)
{
   Optional<int> opt = 1;
   // Cannot convert from "int" to "std::string"
   // Optional<std::string> opt1(opt);
   PDK_UNUSED(opt);
}

struct A {};
struct B {};

TEST(OptionalFailureTest, testNoUnsupportedConversion1)
{
   Optional<A> opt1;
   Optional<B> opt2;
   // opt2 = opt1 ; // Cannot convert from "A" to "B"
   PDK_UNUSED(opt1);
   PDK_UNUSED(opt2);
}

enum E1 {e1};
enum E2 {e2};

TEST(OptionalFailureTest, testAssignmentOfDifferentEnums)
{
   Optional<E2> o2(e2);
   Optional<E1> o1;
   // o1 = o2; 
   //const Optional<E2> to 'const pdk::stdext::optional::internal::TriviallyCopyableOptionalBase<E1>' is not allowed
   PDK_UNUSED(o1);
   PDK_UNUSED(o2);
}

struct NoInitFromNull{};

TEST(OptionalFailureTest, testConversionFromNull)
{
   // no known conversion from 'nullptr_t' to 'pdk::stdext::None'
   // Optional<NoInitFromNull> opt = nullptr;
}

class MoveOnly
{
public:
   int val;
   MoveOnly(int v) : val(v) {}
   MoveOnly(MoveOnly&& rhs) : val(rhs.val) { rhs.val = 0; }
   void operator=(MoveOnly&& rhs) {val = rhs.val; rhs.val = 0; }
   
   MoveOnly(MoveOnly const&) = delete;
   void operator=(MoveOnly const&) = delete;
};

TEST(OptionalFailureTest, testCopyingOptionalWithNoncopyable_T)
{
   // call to deleted constructor of 'pdk::stdext::optional::internal::OptionalBase<MoveOnly>::ValueType'
   // Optional<MoveOnly> opt1;
   // Optional<MoveOnly> opt2(opt1);
}

struct U
{};

struct T
{
   explicit T(U const&) {}
};

U get_U() { return U(); }

TEST(OptionalFailureTest, testEvalForDiffType)
{
   Optional<T> opt;
   // no known conversion from 'U' to 'const T &' for 1st argument
   // opt.valueOr(U());
   // opt.valueOrEval(get_U);
   PDK_UNUSED(opt);
}

TEST(OptionalFailureTest, testImplicitConversionToBool)
{
   Optional<int> opt;
   // Optional<int>' to 'bool'
   // bool b = opt;
   // ASSERT_FALSE(b);
}

TEST(OptionalFailureTest, testStreamingOutOptional)
{
   Optional<int> opt;
   // Optional<int>' to 'bool'
   // std::cout << opt;
   // std::cout << pdk::stdext::none;
}

TEST(OptionalFailureTest, testRValueRef)
{
   // in instantiation of template class 'pdk::stdext::optional::Optional<int &&>' requested here
   //  Optional<int &&> opt;
}




