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

using pdk::stdext::optional::Optional;

struct NothrowBoth {
   NothrowBoth(NothrowBoth&&) noexcept(true) {}
   void operator=(NothrowBoth&&) noexcept(true) {}
};
struct NothrowCtor {
   NothrowCtor(NothrowCtor&&) noexcept(true) {}
   void operator=(NothrowCtor&&) noexcept(false) {}
};
struct NothrowAssign {
   NothrowAssign(NothrowAssign&&) noexcept(false) {}
   void operator=(NothrowAssign&&) noexcept(true) {}
};
struct NothrowNone {
   NothrowNone(NothrowNone&&) noexcept(false) {}
   void operator=(NothrowNone&&) noexcept(false) {}
};

#if 0 // these also test type_traits, which are wrong
void test_noexcept_as_defined() // this is a compile-time test
{
   PDK_STATIC_ASSERT(::std::is_nothrow_move_constructible<NothrowBoth>::value);
   PDK_STATIC_ASSERT(::std::is_nothrow_move_assignable<NothrowBoth>::value);
   
   PDK_STATIC_ASSERT(::std::is_nothrow_move_constructible<NothrowCtor>::value);
   PDK_STATIC_ASSERT(!::std::is_nothrow_move_assignable<NothrowCtor>::value);
   
   PDK_STATIC_ASSERT(!::std::is_nothrow_move_constructible<NothrowAssign>::value);
   PDK_STATIC_ASSERT(::std::is_nothrow_move_assignable<NothrowAssign>::value);
   
   PDK_STATIC_ASSERT(!::std::is_nothrow_move_constructible<NothrowNone>::value);
   PDK_STATIC_ASSERT(!::std::is_nothrow_move_assignable<NothrowNone>::value);
}

void test_noexcept_on_optional_with_type_traits() // this is a compile-time test
{
   PDK_STATIC_ASSERT(::std::is_nothrow_move_constructible<Optional<NothrowBoth> >::value);
   PDK_STATIC_ASSERT(::std::is_nothrow_move_assignable<Optional<NothrowBoth> >::value);
   PDK_STATIC_ASSERT(noexcept(Optional<NothrowBoth>()));
   
   PDK_STATIC_ASSERT(::std::is_nothrow_move_constructible<Optional<NothrowCtor> >::value);
   PDK_STATIC_ASSERT(!::std::is_nothrow_move_assignable<Optional<NothrowCtor> >::value);
   PDK_STATIC_ASSERT(noexcept(Optional<NothrowCtor>()));
   
   PDK_STATIC_ASSERT(!::std::is_nothrow_move_constructible<Optional<NothrowAssign> >::value);
   PDK_STATIC_ASSERT(!::std::is_nothrow_move_assignable<Optional<NothrowAssign> >::value);
   PDK_STATIC_ASSERT(noexcept(Optional<NothrowAssign>()));
   
   PDK_STATIC_ASSERT(!::std::is_nothrow_move_constructible<Optional<NothrowNone> >::value);
   PDK_STATIC_ASSERT(!::std::is_nothrow_move_assignable<Optional<NothrowNone> >::value);
   PDK_STATIC_ASSERT(noexcept(Optional<NothrowNone>()));
}
#endif

TEST(OptionalMoveTest, testNoexceptOptionalWithOperator) // compile-time test
{
   typedef Optional<NothrowBoth>   ONx2;
   typedef Optional<NothrowCtor>   ONxC;
   typedef Optional<NothrowAssign> ONxA;
   typedef Optional<NothrowNone>   ONx0;
   ONx2 onx2;
   ONxC onxC;
   ONxA onxA;
   ONx0 onx0;
   
   PDK_STATIC_ASSERT(noexcept(ONx2()));
   PDK_STATIC_ASSERT(noexcept(ONx2(std::move(onx2))));
   PDK_STATIC_ASSERT(noexcept(onx2 = ONx2())); 
   
   PDK_STATIC_ASSERT(noexcept(ONxC()));
   PDK_STATIC_ASSERT(noexcept(ONxC(std::move(onxC))));
   PDK_STATIC_ASSERT(!noexcept(onxC = ONxC()));
   
   PDK_STATIC_ASSERT(noexcept(ONxA()));
   PDK_STATIC_ASSERT(!noexcept(ONxA(std::move(onxA))));
   PDK_STATIC_ASSERT(!noexcept(onxA = ONxA()));
   
   PDK_STATIC_ASSERT(noexcept(ONx0()));
   PDK_STATIC_ASSERT(!noexcept(ONx0(std::move(onx0))));
   PDK_STATIC_ASSERT(!noexcept(onx0 = ONx0()));
}

namespace  {
enum State
{
   sDefaultConstructed,
   sValueCopyConstructed,
   sValueMoveConstructed,
   sCopyConstructed,
   sMoveConstructed,
   sMoveAssigned,
   sCopyAssigned,
   sValueCopyAssigned,
   sValueMoveAssigned,
   sMovedFrom,
   sIntConstructed
};

struct OracleVal
{
   State s;
   int i;
   OracleVal(int i = 0) : s(sIntConstructed), i(i) {}
};


struct Oracle
{
   State s;
   OracleVal val;
   
   Oracle() : s(sDefaultConstructed) {}
   Oracle(const OracleVal& v) : s(sValueCopyConstructed), val(v) {}
   Oracle(OracleVal&& v) : s(sValueMoveConstructed), val(std::move(v)) {v.s = sMovedFrom;}
   Oracle(const Oracle& o) : s(sCopyConstructed), val(o.val) {}
   Oracle(Oracle&& o) : s(sMoveConstructed), val(std::move(o.val)) {o.s = sMovedFrom;}
   
   Oracle& operator=(const OracleVal& v) { s = sValueCopyAssigned; val = v; return *this; }
   Oracle& operator=(OracleVal&& v) { s = sValueMoveAssigned; val = std::move(v); v.s = sMovedFrom; return *this; }
   Oracle& operator=(const Oracle& o) { s = sCopyAssigned; val = o.val; return *this; }
   Oracle& operator=(Oracle&& o) { s = sMoveAssigned; val = std::move(o.val); o.s = sMovedFrom; return *this; }
};

bool operator==( Oracle const& a, Oracle const& b ) { return a.val.i == b.val.i; }
bool operator!=( Oracle const& a, Oracle const& b ) { return a.val.i != b.val.i; }
}

TEST(OptionalMoveTest, testMoveCtorFromU)
{
   Optional<Oracle> o1 ((OracleVal()));
   ASSERT_TRUE(o1);
   ASSERT_TRUE(o1->s == sValueMoveConstructed || o1->s == sMoveConstructed);
   
   OracleVal v1;
   Optional<Oracle> o2 (v1);
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2->s == sValueCopyConstructed || o2->s == sCopyConstructed || o2->s == sMoveConstructed );
   ASSERT_TRUE(v1.s == sIntConstructed);
   
   Optional<Oracle> o3 (std::move(v1));
   ASSERT_TRUE(o3);
   ASSERT_TRUE(o3->s == sValueMoveConstructed || o3->s == sMoveConstructed);
   ASSERT_TRUE(v1.s == sMovedFrom);
}

TEST(OptionalMoveTest, testMoveCtorFormT)
{
   Optional<Oracle> o1 ((Oracle()));
   ASSERT_TRUE(o1);
   ASSERT_TRUE(o1->s == sMoveConstructed);
   
   Oracle v1;
   Optional<Oracle> o2 (v1);
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2->s == sCopyConstructed);
   ASSERT_TRUE(v1.s == sDefaultConstructed);
   
   Optional<Oracle> o3 (std::move(v1));
   ASSERT_TRUE(o3);
   ASSERT_TRUE(o3->s == sMoveConstructed);
   ASSERT_TRUE(v1.s == sMovedFrom);
}

TEST(OptionalMoveTest, testMoveCtorFromOptionalT)
{
   Optional<Oracle> o1;
   Optional<Oracle> o2(std::move(o1));
   
   ASSERT_TRUE(!o1);
   ASSERT_TRUE(!o2);
   
   Optional<Oracle> o3((Oracle()));
   Optional<Oracle> o4(std::move(o3));
   ASSERT_TRUE(o3);
   ASSERT_TRUE(o4);
   ASSERT_TRUE(o3->s == sMovedFrom);
   ASSERT_TRUE(o4->s == sMoveConstructed);
   
   Optional<Oracle> o5((Optional<Oracle>()));
   ASSERT_TRUE(!o5);
   
   Optional<Oracle> o6((Optional<Oracle>(Oracle())));
   ASSERT_TRUE(o6);
   ASSERT_TRUE(o6->s == sMoveConstructed);
   
   Optional<Oracle> o7(o6); // does copy ctor from non-const lvalue compile?
   PDK_UNUSED(o7);
}

TEST(OptionalMoveTest, testMoveAssignFromU)
{
   Optional<Oracle> o1 = pdk::stdext::none; // test if additional ctors didn't break it
   o1 = pdk::stdext::none;                 // test if additional assignments didn't break it
   o1 = OracleVal();
   ASSERT_TRUE(o1);
   
   ASSERT_TRUE(o1->s == sValueMoveConstructed);  
   
   o1 = OracleVal();
   ASSERT_TRUE(o1);
   ASSERT_TRUE(o1->s == sMoveAssigned); 
   
   OracleVal v1;
   Optional<Oracle> o2;
   o2 = v1;
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2->s == sValueCopyConstructed);
   ASSERT_TRUE(v1.s == sIntConstructed);
   o2 = v1;
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2->s == sCopyAssigned || o2->s == sMoveAssigned);
   ASSERT_TRUE(v1.s == sIntConstructed);
   
   Optional<Oracle> o3;
   o3 = std::move(v1);
   ASSERT_TRUE(o3);
   ASSERT_TRUE(o3->s == sValueMoveConstructed);
   ASSERT_TRUE(v1.s == sMovedFrom);
}

TEST(OptionalMoveTest, testMoveAssignFromT)
{
   Optional<Oracle> o1;
   o1 = Oracle();
   ASSERT_TRUE(o1);
   ASSERT_TRUE(o1->s == sMoveConstructed);  
   
   o1 = Oracle();
   ASSERT_TRUE(o1);
   ASSERT_TRUE(o1->s == sMoveAssigned); 
   
   Oracle v1;
   Optional<Oracle> o2;
   o2 = v1;
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2->s == sCopyConstructed);
   ASSERT_TRUE(v1.s == sDefaultConstructed);
   o2 = v1;
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2->s == sCopyAssigned);
   ASSERT_TRUE(v1.s == sDefaultConstructed);
   
   Optional<Oracle> o3;
   o3 = std::move(v1);
   ASSERT_TRUE(o3);
   ASSERT_TRUE(o3->s == sMoveConstructed);
   ASSERT_TRUE(v1.s == sMovedFrom);
}

TEST(OptionalMoveTest, testMoveAssignFromOptionalT)
{
   Optional<Oracle> o1;
   Optional<Oracle> o2;
   o1 = Optional<Oracle>();
   ASSERT_TRUE(!o1);
   Optional<Oracle> o3((Oracle()));
   o1 = o3;
   ASSERT_TRUE(o3);
   ASSERT_TRUE(o3->s == sMoveConstructed);
   ASSERT_TRUE(o1);
   ASSERT_TRUE(o1->s == sCopyConstructed);
   
   o2 = std::move(o3);
   ASSERT_TRUE(o3);
   ASSERT_TRUE(o3->s == sMovedFrom);
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2->s == sMoveConstructed);
   
   o2 = Optional<Oracle>((Oracle()));
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2->s == sMoveAssigned);
}

namespace {
class MoveOnly
{
public:
   int val;
   MoveOnly(int v) : val(v) {}
   MoveOnly(MoveOnly&& rhs) : val(rhs.val) { rhs.val = 0; }
   void operator=(MoveOnly&& rhs) {val = rhs.val; rhs.val = 0; }
   
private:
   MoveOnly(MoveOnly const&);
   void operator=(MoveOnly const&);
   
   friend class MoveOnlyB;
};

class MoveOnlyB
{
public:
   int val;
   MoveOnlyB(int v) : val(v) {}
   MoveOnlyB(MoveOnlyB&& rhs) : val(rhs.val) { rhs.val = 0; }
   void operator=(MoveOnlyB&& rhs) {val = rhs.val; rhs.val = 0; }
   MoveOnlyB(MoveOnly&& rhs) : val(rhs.val) { rhs.val = 0; }
   void operator=(MoveOnly&& rhs) {val = rhs.val; rhs.val = 0; }
   
private:
   MoveOnlyB(MoveOnlyB const&);
   void operator=(MoveOnlyB const&);
   MoveOnlyB(MoveOnly const&);
   void operator=(MoveOnly const&);
};
}

TEST(OptionalMoveTest, testWithMoveOnly)
{
   Optional<MoveOnly> o1;
   Optional<MoveOnly> o2((MoveOnly(1)));
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2->val == 1);
   Optional<MoveOnly> o3 (std::move(o1));
   ASSERT_TRUE(!o3);
   Optional<MoveOnly> o4 (std::move(o2));
   ASSERT_TRUE(o4);
   ASSERT_TRUE(o4->val == 1);
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2->val == 0);
   
   o3 = std::move(o4);
   ASSERT_TRUE(o3);
   ASSERT_TRUE(o3->val == 1);
   ASSERT_TRUE(o4);
   ASSERT_TRUE(o4->val == 0);
}


TEST(OptionalMoveTest, testMoveAssignFromOptionalU)
{
   Optional<MoveOnly> a((MoveOnly(2)));
   Optional<MoveOnlyB> b1;
   b1 = std::move(a);
   
   ASSERT_TRUE(b1);
   ASSERT_TRUE(b1->val == 2);
   ASSERT_TRUE(a);
   ASSERT_TRUE(a->val == 0);
   
   b1 = MoveOnly(4);
   
   ASSERT_TRUE(b1);
   ASSERT_TRUE(b1->val == 4);
}

TEST(OptionalMoveTest, testMoveCtorFromOptionalU)
{
   Optional<MoveOnly> a((MoveOnly(2)));
   Optional<MoveOnlyB> b1(std::move(a));
   
   ASSERT_TRUE(b1);
   ASSERT_TRUE(b1->val == 2);
   ASSERT_TRUE(a);
   ASSERT_TRUE(a->val == 0);
   
   Optional<MoveOnlyB> b2((Optional<MoveOnly>((MoveOnly(4)))));
   
   ASSERT_TRUE(b2);
   ASSERT_TRUE(b2->val == 4);
}

TEST(OptionalMoveTest, testSwap)
{
   Optional<MoveOnly> a((MoveOnly(2)));
   Optional<MoveOnly> b((MoveOnly(3)));
   swap(a, b);
   
   ASSERT_TRUE(a->val == 3);
   ASSERT_TRUE(b->val == 2);
}

TEST(OptionalMoveTest, testOptionalRefToMovables)
{
   MoveOnly m(3);
   Optional<MoveOnly&> orm = m;
   orm->val = 2;
   ASSERT_TRUE(m.val == 2);
   
   Optional<MoveOnly&> orm2 = orm;
   orm2->val = 1;
   ASSERT_TRUE(m.val == 1);
   ASSERT_TRUE(orm->val == 1);
   
   Optional<MoveOnly&> orm3 = std::move(orm);
   orm3->val = 4;
   ASSERT_TRUE(m.val == 4);
   ASSERT_TRUE(orm->val == 4);
   ASSERT_TRUE(orm2->val == 4);
}
