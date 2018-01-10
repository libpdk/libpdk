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
   oa = def ;
   check_is_pending_dtor(ARG(T));
   check_is_pending_copy(ARG(T));
   check_uninitialized(oa);
}

TEST(OptionalTest, testWithBuiltinTypes)
{
   test_basics(ARG(double));
}
