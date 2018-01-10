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
#include "Common.cpp"
#include <list>
#include <utility>
#include <tuple>

void test_implicit_construction (Optional<double> opt, double v, double z )
{
   check_value(opt, v, z);
}

void test_implicit_construction (Optional<X> opt, const X &v, const X &z )
{
   check_value(opt, v, z);
}
template <typename T>
void test_basics(const T *)
{
   T z(0);
   T a(1);
   // Default construction.
   // 'def' state is Uninitialized.
   // T::T() is not called (and it is not even defined)
   Optional<T> def;
   check_uninitialized(def);
   
   // Implicit construction
   // The first parameter is implicitely converted to Optional<T>(a);
   test_implicit_construction(a, a, z);
   
   // Direct initialization.
   // 'oa' state is Initialized with 'a'
   // T::T(T const& x ) is used.
   set_pending_copy(ARG(T));
   Optional<T> oa(a);
   check_is_not_pending_copy(ARG(T));
   check_initialized(oa);
   check_value(oa, a, z);
   T b(2);
   Optional<T> ob;
   // Value-Assignment upon Uninitialized optional.
   // T::T(T const& x ) is used.
   set_pending_copy(ARG(T));
   ob = a;
   check_is_not_pending_copy(ARG(T));
   check_value(oa, a, z);
   
   // Value-Assignment upon Initialized optional.
   // T::operator=(T const& x ) is used
   set_pending_assign(ARG(T));
   set_pending_copy(ARG(T));
   set_pending_dtor(ARG(T));
   ob = b;
   check_is_not_pending_assign(ARG(T));
   check_is_pending_copy(ARG(T));
   check_is_pending_dtor(ARG(T));
   check_initialized(ob);
   check_value(ob, b, z);
   
   // Assignment initialization.
   // T::T (T const& x ) is used to copy new value.
   set_pending_copy(ARG(T));
   Optional<T> const oa2 (oa);
   check_is_not_pending_copy(ARG(T));
   check_initialized_const(oa2);
   check_value_const(oa2, a, z);
   
   // Assignment
   // T::operator= (T const& x ) is used to copy new value.
   set_pending_assign(ARG(T));
   oa = ob;
   check_is_not_pending_assign(ARG(T));
   check_initialized(oa);
   check_value(oa, b, z);
   
   // Uninitializing Assignment upon Initialized Optional
   // T::~T() is used to destroy previous value in oa.
   set_pending_dtor(ARG(T));
   set_pending_copy(ARG(T));
   oa = def;
   check_is_not_pending_dtor(ARG(T));
   check_is_pending_copy(ARG(T));
   check_uninitialized(oa);
   
   // Uninitializing Assignment upon Uninitialized Optional
   // (Dtor is not called this time)
   set_pending_dtor(ARG(T));
   set_pending_copy(ARG(T));
   oa = def;
   check_is_pending_dtor(ARG(T));
   check_is_pending_copy(ARG(T));
   check_uninitialized(oa);
}

template<class T>
void test_conditional_ctor_and_get_valur_or(T const*)
{   
   T a(321);
   
   T z(123);
   
   Optional<T> const cdef0(false,a);
   Optional<T> def0(false,a);
   Optional<T> def1 = pdk::stdext::make_optional(false, a); //  T is not within boost so ADL won't find make_optional unqualified
   check_uninitialized(def0);
   check_uninitialized(def1);
   
   Optional<T> const co0(true,a);
   
   Optional<T> o0(true,a);
   Optional<T> o1 = pdk::stdext::make_optional(true, a); //  T is not within boost so ADL won't find make_optional unqualified
   
   check_initialized(o0);
   check_initialized(o1);
   check_value(o0,a,z);
   check_value(o1,a,z);
   
   T b = def0.getValueOr(z);
   ASSERT_TRUE(b == z);
   
   b = o0.getValueOr(z);
   ASSERT_TRUE(b == a);
   
   T const& crz = z;
   T&        rz = z;
   
   T const& crzz = def0.getValueOr(crz);
   ASSERT_TRUE( crzz == crz);
   
   T& rzz = def0.getValueOr(rz);
   ASSERT_TRUE( rzz == rz);

   T const& crb = o0.getValueOr(crz);
   ASSERT_TRUE( crb == a);
   
   T& rb = o0.getValueOr(rz);
   ASSERT_TRUE( rb == b);

   T& ra = a;
   
   Optional<T&> defref(false, ra);
   ASSERT_TRUE(!defref);
   
   Optional<T&> ref(true, ra);
   ASSERT_TRUE(!!ref);
   
   a = T(432);
   
   ASSERT_TRUE(*ref == a);
   
   T& r1 = defref.getValueOr(z);
   ASSERT_TRUE(r1 == z);
   
   T& r2 = ref.getValueOr(z);
   ASSERT_TRUE(r2 == a);
}

TEST(OptionalTest, testWithBuiltinTypes)
{
   test_basics(ARG(double));
   test_conditional_ctor_and_get_valur_or(ARG(double));
}
