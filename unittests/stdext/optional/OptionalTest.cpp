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

void test_default_implicit_construction(double, Optional<double> opt)
{
   ASSERT_TRUE(!opt);
}

void test_default_implicit_construction (const X&, Optional<X> opt)
{
   ASSERT_TRUE(!opt);
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
   Optional<T> oa2(a);
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

template<class T>
void test_uninitialized_access( T const* )
{
   //   Optional<T> def ;
   
   //   bool passed = false ;
   //   try
   //   {
   //      // This should throw because 'def' is uninitialized
   //      T const& n = def.get();
   //      PDK_UNUSED(n);
   //      passed = true ;
   //   }
   //   catch (...) {}
   //   ASSERT_TRUE(!passed);
   
   //   passed = false ;
   //   try
   //   {
   //      // This should throw because 'def' is uninitialized
   //      T const& n = *def;
   //      PDK_UNUSED(n);
   //      passed = true ;
   //   }
   //   catch (...) {}
   //   ASSERT_TRUE(!passed);
   
   //   passed = false;
   //   try
   //   {
   //      T v(5) ;
   //      PDK_UNUSED(v);
   //      // This should throw because 'def' is uninitialized
   //      *def = v ;
   //      passed = true ;
   //   }
   //   catch (...) {}
   //   ASSERT_TRUE(!passed);
   
   //   passed = false ;
   //   try
   //   {
   //      // This should throw because 'def' is uninitialized
   //      T v = *(def.operator->()) ;
   //      PDK_UNUSED(v);
   //      passed = true ;
   //   }
   //   catch (...) {}
   //   ASSERT_TRUE(!passed);
}

template<class T>
void test_no_throwing_swap(T const*)
{  
   T z(0);
   T a(14);
   T b(15);
   
   Optional<T> def0 ;
   Optional<T> def1 ;
   Optional<T> opt0(a) ;
   Optional<T> opt1(b) ;
   
   int count = get_instance_count(ARG(T)) ;
   
   swap(def0, def1);
   check_uninitialized(def0);
   check_uninitialized(def1);
   
   swap(def0, opt0);
   check_uninitialized(opt0);
   check_initialized(def0);
   check_value(def0, a, z);
   
   // restore def0 and opt0
   swap(def0,opt0);
   
   swap(opt0,opt1);
   check_instance_count(count, ARG(T));
   check_initialized(opt0);
   check_initialized(opt1);
   check_value(opt0, b, z);
   check_value(opt1, a, z);
}

template<class T>
void test_relops( T const* )
{
   T v0(0);
   T v1(1);
   T v2(1);
   
   Optional<T> def0 ;
   Optional<T> def1 ;
   Optional<T> opt0(v0);
   Optional<T> opt1(v1);
   Optional<T> opt2(v2);
   
   // Check identity
   ASSERT_TRUE(def0 == def0);
   ASSERT_TRUE(opt0 == opt0);
   ASSERT_TRUE(!(def0 != def0));
   ASSERT_TRUE(!(opt0 != opt0));
   
   // Check when both are uininitalized.
   ASSERT_TRUE(  def0 == def1); // both uninitialized compare equal
   ASSERT_TRUE(!(def0 <  def1)); // uninitialized is never less    than uninitialized
   ASSERT_TRUE(!(def0 >  def1)); // uninitialized is never greater than uninitialized
   ASSERT_TRUE(!(def0 != def1));
   ASSERT_TRUE(  def0 <= def1);
   ASSERT_TRUE(  def0 >= def1);
   
   // Check when only lhs is uninitialized.
   ASSERT_TRUE(  def0 != opt0); // uninitialized is never equal to initialized
   ASSERT_TRUE(!(def0 == opt0));
   ASSERT_TRUE(  def0 <  opt0); // uninitialized is always less than initialized
   ASSERT_TRUE(!(def0 >  opt0));
   ASSERT_TRUE(  def0 <= opt0);
   ASSERT_TRUE(!(def0 >= opt0));
   
   // Check when only rhs is uninitialized.
   ASSERT_TRUE(  opt0 != def0); // initialized is never equal to uninitialized
   ASSERT_TRUE(!(opt0 == def0));
   ASSERT_TRUE(!(opt0 <  def0)); // initialized is never less than uninitialized
   ASSERT_TRUE(  opt0 >  def0);
   ASSERT_TRUE(!(opt0 <= def0));
   ASSERT_TRUE(  opt0 >= opt0);
   
   // If both are initialized, values are compared
   ASSERT_TRUE(opt0 != opt1);
   ASSERT_TRUE(opt1 == opt2);
   ASSERT_TRUE(opt0 <  opt1);
   ASSERT_TRUE(opt1 >  opt0);
   ASSERT_TRUE(opt1 <= opt2);
   ASSERT_TRUE(opt1 >= opt0);
   
   // Compare against a value directly
   ASSERT_TRUE(opt0 == v0);
   ASSERT_TRUE(opt0 != v1);
   ASSERT_TRUE(opt1 == v2);
   ASSERT_TRUE(opt0 <  v1);
   ASSERT_TRUE(opt1 >  v0);
   ASSERT_TRUE(opt1 <= v2);
   ASSERT_TRUE(opt1 >= v0);
   ASSERT_TRUE(v0 != opt1);
   ASSERT_TRUE(v1 == opt2);
   ASSERT_TRUE(v0 <  opt1);
   ASSERT_TRUE(v1 >  opt0);
   ASSERT_TRUE(v1 <= opt2);
   ASSERT_TRUE(v1 >= opt0);
   ASSERT_TRUE(  def0 != v0);
   ASSERT_TRUE(!(def0 == v0));
   ASSERT_TRUE(  def0 <  v0);
   ASSERT_TRUE(!(def0 >  v0));
   ASSERT_TRUE(  def0 <= v0);
   ASSERT_TRUE(!(def0 >= v0));
   ASSERT_TRUE(  v0 != def0);
   ASSERT_TRUE(!(v0 == def0));
   ASSERT_TRUE(!(v0 <  def0));
   ASSERT_TRUE(  v0 >  def0);
   ASSERT_TRUE(!(v0 <= def0));
   ASSERT_TRUE(  v0 >= opt0);
}

template<class T>
void test_none( T const* )
{
   TRACE( std::endl << BOOST_CURRENT_FUNCTION  );
   
   using pdk::stdext::none;
   
   Optional<T> def0;
   Optional<T> def1(none);
   Optional<T> non_def(T(1234));
   
   ASSERT_TRUE(def0 == none);
   ASSERT_TRUE(non_def != none);
   ASSERT_TRUE(!def1);
   ASSERT_TRUE(!(non_def < none));
   ASSERT_TRUE(non_def > none);
   ASSERT_TRUE(!(non_def <= none));
   ASSERT_TRUE(non_def >= none);
   
   non_def = none;
   ASSERT_TRUE(!non_def);
   
   test_default_implicit_construction(T(1), none);
}

struct VBase : virtual X
{
   VBase(int v) : X(v) {}
   // MSVC 8.0 doesn't generate this correctly...
   VBase(const VBase& other) : X(static_cast<const X&>(other)) {}
};

template<class T>
void test_direct_value_manip( T const* )
{
   T x(3);
   
   Optional<T> const c_opt0(x);
   Optional<T> opt0(x);
   
   ASSERT_TRUE(c_opt0.get().V() == x.V());
   ASSERT_TRUE(opt0.get().V() == x.V());
   
   ASSERT_TRUE(c_opt0->V() == x.V());
   ASSERT_TRUE(opt0->V() == x.V());
   
   ASSERT_TRUE((*c_opt0).V() == x.V());
   ASSERT_TRUE((*  opt0).V() == x.V());
   
   T y(4);
   opt0 = y ;
   ASSERT_TRUE(opt0.get().V() == y.V());
}

template<class T>
void test_throwing_direct_init( T const* )
{
   
   T a(6);
   int count = get_instance_count( ARG(T));
   set_throw_on_copy( ARG(T));
   bool passed = false ;
   try
   {
      // This should:
      //   Attempt to copy construct 'a' and throw.
      // 'opt' won't be constructed.
      set_pending_copy(ARG(T)) ;
      Optional<T> opt(a) ;
      passed = true ;
   }
   catch(... ){}
   
   ASSERT_TRUE(!passed);
   check_is_not_pending_copy(ARG(T));
   check_instance_count(count, ARG(T));
   reset_throw_on_copy(ARG(T));
}

template<class T>
void test_throwing_copy_initialization( T const* )
{
   T z(0);
   T a(10);
   Optional<T> opt (a);
   int count = get_instance_count(ARG(T));
   set_throw_on_copy(ARG(T));
   bool passed = false ;
   try
   {
      // This should:
      //   Attempt to copy construct 'opt' and throw.
      //   opt1 won't be constructed.
      set_pending_copy(ARG(T));
      Optional<T> opt1 = opt ;
      passed = true ;
   }
   catch(... ) {}
   ASSERT_TRUE(!passed);
   check_is_not_pending_copy(ARG(T));
   check_instance_count(count, ARG(T));
   // Nothing should have happened to the source optional.
   check_initialized(opt);
   check_value(opt, a, z);
   reset_throw_on_copy(ARG(T));
}

template<class T>
void test_throwing_assign_to_uninitialized( T const* )
{
   T z(0);
   T a(11);
   Optional<T> opt0 ;
   Optional<T> opt1(a) ;
   int count = get_instance_count(ARG(T));
   set_throw_on_copy(ARG(T));
   bool passed = false ;
   try
   {
      // This should:
      //   Attempt to copy construct 'opt1.value()' into opt0 and throw.
      //   opt0 should be left uninitialized.
      set_pending_copy(ARG(T));
      opt0 = opt1 ;
      passed = true ;
   }
   catch(... ) {}
   
   ASSERT_TRUE(!passed);
   check_is_not_pending_copy(ARG(T));
   check_instance_count(count, ARG(T));
   check_uninitialized(opt0);
   reset_throw_on_copy(ARG(T));
}

template<class T>
void test_throwing_assign_to_initialized( T const* )
{
   T z(0);
   T a(12);
   T b(13);
   T x(-1);
   Optional<T> opt0(a);
   Optional<T> opt1(b);
   int count = get_instance_count(ARG(T));
   set_throw_on_assign( ARG(T));
   bool passed = false;
   try
   {
      // This should:
      //   Attempt to copy construct 'opt1.value()' into opt0 and throw.
      //   opt0 is kept initialized but its value not neccesarily fully assigned
      //   (in this test, incompletely assigned is flaged with the value -1 being set)
      set_pending_assign(ARG(T));
      opt0 = opt1;
      passed = true;
   }
   catch(... ) {}
   
   ASSERT_TRUE(!passed);
   // opt0 was left uninitialized
   check_is_not_pending_assign(ARG(T));
   check_instance_count(count, ARG(T));
   check_initialized(opt0);
   check_value(opt0, x, z);
   reset_throw_on_assign( ARG(T));
}

template<class T>
void test_throwing_swap( T const* )
{
   T a(16);
   T b(17);
   T x(-1);
   
   Optional<T> opt0(a) ;
   Optional<T> opt1(b) ;
   
   set_throw_on_assign(ARG(T));
   
   //
   // Case 1: Both Initialized.
   //
   bool passed = false;
   try
   {
      // This should attempt to swap optionals and fail at swap(X&,X&).
      swap(opt0, opt1);
      passed = true ;
   }
   catch(... ) {}
   
   ASSERT_TRUE(!passed);
   
   // optional's swap doesn't affect the initialized states of the arguments. Therefore,
   // the following must hold:
   check_initialized(opt0);
   check_initialized(opt1);
   check_value(opt0, x, a);
   check_value(opt1, b, x);
   //
   // Case 2: Only one Initialized.
   //
   reset_throw_on_assign(ARG(T));
   
   set_throw_on_copy(ARG(T));
   
   passed = false;
   opt0.~Optional();
   try
   {
      // This should attempt to swap optionals and fail at opt0.reset(*opt1)
      // Both opt0 and op1 are left unchanged (unswaped)
      swap(opt0, opt1);
      passed = true ;
   }
   catch(... ) {}
   ASSERT_TRUE(!passed);
   check_uninitialized(opt0);
   check_initialized(opt1);
   check_value(opt1, b, x);
   reset_throw_on_copy(ARG(T));
}

TEST(OptionalTest, testWithBuiltinTypes)
{
   test_basics(ARG(double));
   test_conditional_ctor_and_get_valur_or(ARG(double));
   test_uninitialized_access(ARG(double));
   test_no_throwing_swap(ARG(double));
   test_relops(ARG(double));
   test_none(ARG(double));
}


TEST(OptionalTest, testWithClassTypes)
{
   test_basics(ARG(X));
   test_basics(ARG(VBase));
   test_conditional_ctor_and_get_valur_or(ARG(X));
   test_direct_value_manip(ARG(X));
   test_uninitialized_access(ARG(X));
   test_throwing_direct_init(ARG(X));
   test_throwing_copy_initialization(ARG(X));
   test_throwing_assign_to_uninitialized(ARG(X));
   test_throwing_assign_to_initialized(ARG(X));
   test_throwing_swap(ARG(X));
   test_relops(ARG(X));
   test_none(ARG(X));
   ASSERT_TRUE(X::m_count == 0);
}

int eat(bool)
{
   return 1;
}

int eat(char)
{
   return 1;
}

int eat(int)
{
   return 1;
}

int eat(const void *)
{
   return 1;
}

template<class T>
int eat(T)
{
   return 0;
}

template<class T>
void test_no_implicit_conversions_impl(const T &)
{
   Optional<T> def;
   ASSERT_TRUE(eat(def) == 0);
}

TEST(OptionalTest, testNoImplicitConversions)
{
   bool b = false;
   char c = 0;
   int i = 0;
   void const* p = 0;
   test_no_implicit_conversions_impl(b);
   test_no_implicit_conversions_impl(c);
   test_no_implicit_conversions_impl(i);
   test_no_implicit_conversions_impl(p);
}

class CustomAddressOfClass  
{
   int n;
   
public:
   CustomAddressOfClass() : n(0) {}
   CustomAddressOfClass(CustomAddressOfClass const& that) : n(that.n) {}
   explicit CustomAddressOfClass(int m) : n(m) {}
   int* operator& () { return &n; }
   bool operator== (CustomAddressOfClass const& that) const { return n == that.n; }
};

TEST(OptionalTest, testCustomAddressofOperator)
{
   Optional<CustomAddressOfClass> o1(CustomAddressOfClass(10));
   ASSERT_TRUE(!!o1);
   ASSERT_TRUE(o1.get() == CustomAddressOfClass(10));
   o1 = CustomAddressOfClass(20);
   ASSERT_TRUE(!!o1);
   ASSERT_TRUE(o1.get() == CustomAddressOfClass(20));
   
   o1 = pdk::stdext::none;
   ASSERT_TRUE(!o1);
}

