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
#include "pdk/stdext/utility/DisableIf.h"

using pdk::stdext::optional::Optional;

namespace  {

struct ScopeGuard // no copy/move ctor/assign
{
   int val_;
   explicit ScopeGuard(int v) : val_(v) {}
   int& val() { return val_; }
   const int& val() const { return val_; }
   
private:
   ScopeGuard(ScopeGuard const&);
   void operator=(ScopeGuard const&);
};


struct Abstract
{
   virtual int& val() = 0;
   virtual const int& val() const = 0;
   virtual ~Abstract() {}
   Abstract(){}
   
private:
   Abstract(Abstract const&);
   void operator=(Abstract const&);
};

struct Impl : Abstract
{
   int val_;
   Impl(int v) : val_(v) {}
   int& val() { return val_; }
   const int& val() const { return val_; }
};

template <typename T>
struct concrete_type_of
{
   typedef T type;
};

template <>
struct concrete_type_of<Abstract>
{
   typedef Impl type;
};

template <>
struct concrete_type_of<const Abstract>
{
   typedef const Impl type;
};

template <typename T>
struct has_arrow
{
   static const bool value = true;
};

template <>
struct has_arrow<int>
{
   static const bool value = false;
};

template <>
struct has_arrow<Optional<int> >
{
   static const bool value = false;
};

int& val(int& i) { return i; }
int& val(Abstract& a) { return a.val(); }
int& val(Impl& a) { return a.val(); }
int& val(ScopeGuard& g) { return g.val(); }
template <typename T> int& val(T& o) { return *o; }

const int& val(const int& i) { return i; }
const int& val(const Abstract& a) { return a.val(); }
const int& val(const Impl& a) { return a.val(); }
const int& val(const ScopeGuard& g) { return g.val(); }
template <typename T> const int& val(const T& o) { return *o; }

bool operator==(const Abstract& l, const Abstract& r) { return l.val() == r.val(); }
bool operator==(const ScopeGuard& l, const ScopeGuard& r) { return l.val() == r.val(); }

bool operator<(const Abstract& l, const Abstract& r) { return l.val() < r.val(); }
bool operator<(const ScopeGuard& l, const ScopeGuard& r) { return l.val() < r.val(); }
}

template <typename T>
void test_copy_assignment_for_const()
{
   const typename concrete_type_of<T>::type v(2);
   Optional<const T&> o;
   o = Optional<const T&>(v);
   ASSERT_TRUE(o);
   ASSERT_TRUE(o != pdk::stdext::none);
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_TRUE(val(*o) == val(v));
   ASSERT_TRUE(val(*o) == 2);
}

template <typename T>
void test_copy_assignment_for_noconst_const()
{
   typename concrete_type_of<T>::type v(2);
   Optional<const T&> o;
   o = Optional<const T&>(v);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(o != pdk::stdext::none);
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_TRUE(val(*o) == val(v));
   ASSERT_TRUE(val(*o) == 2);
   
   val(v) = 9;
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 9);
   ASSERT_EQ(val(v), 9);
}

template <typename T>
void test_copy_assignment_for()
{
   typename concrete_type_of<T>::type v(2);
   Optional<T &> o;
   o = Optional<T &>(v);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(o != pdk::stdext::none);
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_TRUE(val(*o) == val(v));
   ASSERT_TRUE(val(*o) == 2);
   
   val(v) = 9;
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 9);
   ASSERT_EQ(val(v), 9);
   
   val(*o) = 7;
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 7);
   ASSERT_EQ(val(v), 7);
}

template <typename T>
void test_rebinding_assignment_semantics_const()
{
   const typename concrete_type_of<T>::type v(2), w(7);
   Optional<const T&> o(v);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 2);
   
   o = Optional<const T&>(w);
   ASSERT_EQ(val(v), 2);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(std::addressof(*o) != std::addressof(v));
   ASSERT_NE(val(*o), val(v));
   ASSERT_NE(val(*o), 2);
   
   ASSERT_TRUE(std::addressof(*o) == std::addressof(w));
   ASSERT_EQ(val(*o), val(w));
   ASSERT_EQ(val(*o), 7);
}

template <typename T>
void test_rebinding_assignment_semantics_noconst_const()
{
   typename concrete_type_of<T>::type v(2), w(7);
   Optional<const T&> o(v);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 2);
   
   o = Optional<const T&>(w);
   ASSERT_EQ(val(v), 2);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(std::addressof(*o) != std::addressof(v));
   ASSERT_NE(val(*o), val(v));
   ASSERT_NE(val(*o), 2);
   
   ASSERT_TRUE(std::addressof(*o) == std::addressof(w));
   ASSERT_EQ(val(*o), val(w));
   ASSERT_EQ(val(*o), 7);
}

template <typename T>
void test_rebinding_assignment_semantics()
{
   typename concrete_type_of<T>::type v(2), w(7);
   Optional<T &> o(v);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 2);
   
   o = Optional<T &>(w);
   ASSERT_EQ(val(v), 2);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(std::addressof(*o) != std::addressof(v));
   ASSERT_NE(val(*o), val(v));
   ASSERT_NE(val(*o), 2);
   
   ASSERT_TRUE(std::addressof(*o) == std::addressof(w));
   ASSERT_EQ(val(*o), val(w));
   ASSERT_EQ(val(*o), 7);
   
   val(*o) = 8;
   ASSERT_TRUE(std::addressof(*o) == std::addressof(w));
   ASSERT_EQ(val(*o), val(w));
   ASSERT_EQ(val(*o), 8);
   ASSERT_EQ(val(w), 8);
   ASSERT_EQ(val(v), 2);
}

template <typename T, typename U>
void test_converting_assignment()
{
   typename concrete_type_of<T>::type v1(1), v2(2), v3(3);
   Optional<U&> oA(v1), oB(pdk::stdext::none);
   
   oA = v2;
   ASSERT_TRUE(oA);
   ASSERT_TRUE(std::addressof(*oA) == std::addressof(v2));
   
   oB = v3;
   ASSERT_TRUE(oB);
   ASSERT_TRUE(std::addressof(*oB) == std::addressof(v3));
}

template <typename T>
void test_optional_ref_assignment()
{
   test_copy_assignment_for<T>();
   test_rebinding_assignment_semantics<T>();
   
   test_copy_assignment_for_const<T>();
   test_copy_assignment_for_noconst_const<T>();
   test_rebinding_assignment_semantics_const<T>();
   test_rebinding_assignment_semantics_noconst_const<T>();
}

template <typename T>
void test_all_const_cases()
{
   test_converting_assignment<T, T>();
   test_converting_assignment<const T, const T>();
   test_converting_assignment<T, const T>();
}

template <typename T>
void test_converting_ctor()
{
   typename concrete_type_of<T>::type v1(1), v2(2);
   
   {
      Optional<T&> o1 = v1, o1_ = v1, o2 = v2;
      
      ASSERT_TRUE(o1);
      ASSERT_TRUE(std::addressof(*o1) == std::addressof(v1));
      ASSERT_TRUE(o1_);
      ASSERT_TRUE(std::addressof(*o1_) == std::addressof(v1));
      ASSERT_TRUE(std::addressof(*o1_) == std::addressof(*o1));
      
      ASSERT_TRUE(o2);
      ASSERT_TRUE(std::addressof(*o2) == std::addressof(v2));
      ASSERT_TRUE(std::addressof(*o2) != std::addressof(*o1));
   }
   {
      const Optional<T&> o1 = v1, o1_ = v1, o2 = v2;
      
      ASSERT_TRUE(o1);
      ASSERT_TRUE(std::addressof(*o1) == std::addressof(v1));
      ASSERT_TRUE(o1_);
      ASSERT_TRUE(std::addressof(*o1_) == std::addressof(v1));
      ASSERT_TRUE(std::addressof(*o1_) == std::addressof(*o1));
      
      ASSERT_TRUE(o2);
      ASSERT_TRUE(std::addressof(*o2) == std::addressof(v2));
      ASSERT_TRUE(std::addressof(*o2) != std::addressof(*o1));
   }
}

template <typename T>
void test_converting_ctor_for_noconst_const()
{
   typename concrete_type_of<T>::type v1(1), v2(2);
   
   {
      Optional<const T&> o1 = v1, o1_ = v1, o2 = v2;
      
      ASSERT_TRUE(o1);
      ASSERT_TRUE(std::addressof(*o1) == std::addressof(v1));
      ASSERT_TRUE(o1_);
      ASSERT_TRUE(std::addressof(*o1_) == std::addressof(v1));
      ASSERT_TRUE(std::addressof(*o1_) == std::addressof(*o1));
      
      ASSERT_TRUE(o2);
      ASSERT_TRUE(std::addressof(*o2) == std::addressof(v2));
      ASSERT_TRUE(std::addressof(*o2) != std::addressof(*o1));
   }
   {
      const Optional<const T&> o1 = v1, o1_ = v1, o2 = v2;
      
      ASSERT_TRUE(o1);
      ASSERT_TRUE(std::addressof(*o1) == std::addressof(v1));
      ASSERT_TRUE(o1_);
      ASSERT_TRUE(std::addressof(*o1_) == std::addressof(v1));
      ASSERT_TRUE(std::addressof(*o1_) == std::addressof(*o1));
      
      ASSERT_TRUE(o2);
      ASSERT_TRUE(std::addressof(*o2) == std::addressof(v2));
      ASSERT_TRUE(std::addressof(*o2) != std::addressof(*o1));
   }
}

template <typename T>
void test_all_const_cases_convert_ctor()
{
   test_converting_ctor<T>();
   test_converting_ctor<const T>();
   test_converting_ctor_for_noconst_const<T>();
}

TEST(OptionalRefTest, testRefAssignConstInt)
{
   test_copy_assignment_for_const<int>();
   test_copy_assignment_for_noconst_const<int>();
   test_rebinding_assignment_semantics_noconst_const<int>();
}

TEST(OptionalRefTest, testRefAssignMutableInt)
{
   test_copy_assignment_for<int>();
   test_rebinding_assignment_semantics<int>();
}

TEST(OptionalRefTest, testRefAssignPortableMinimum)
{
   test_optional_ref_assignment<ScopeGuard>();
   test_optional_ref_assignment<Abstract>();
   test_optional_ref_assignment<Optional<int> >();
}

TEST(OptionalRefTest, testRefConvertAssignConstInt)
{
   test_converting_assignment<const int, const int>();
}

TEST(OptionalRefTest, testRefConvertAssignMutableInt)
{
   test_converting_assignment<int, int>();
   test_converting_assignment<int, const int>();
}

TEST(OptionalRefTest, testRefConvertAssignNonInt)
{
   test_all_const_cases<ScopeGuard>();
   test_all_const_cases<Abstract>();
}

TEST(OptionalRefTest, testRefConvertCtor)
{
   test_all_const_cases<int>();
   test_all_const_cases<ScopeGuard>();
   test_all_const_cases<Abstract>();
   test_all_const_cases< Optional<int> >();
}

std::string global("text"); 

Optional<std::string&> make_optional_string_ref()
{
   return Optional<std::string&>(global);
}

std::string& return_global()
{
   return global;
}

TEST(OptionalRefTest, testRefMove)
{
   Optional<std::string&> opt;
   opt = make_optional_string_ref();
   ASSERT_TRUE(bool(opt));
   ASSERT_TRUE(*opt == global);
   ASSERT_TRUE(std::addressof(*opt) == std::addressof(global));
   
   {
      std::string& str = *make_optional_string_ref();
      ASSERT_TRUE(str == global);
      ASSERT_TRUE(std::addressof(str) == std::addressof(global));
   }
   
   {
      std::string& str = make_optional_string_ref().value();
      ASSERT_TRUE(str == global);
      ASSERT_TRUE(std::addressof(str) == std::addressof(global));
   }
   
   {
      std::string& str = make_optional_string_ref().valueOr(global);
      ASSERT_TRUE(str == global);
      ASSERT_TRUE(std::addressof(str) == std::addressof(global));
   }
   
   {
      std::string& str = make_optional_string_ref().valueOrEval(&return_global);
      ASSERT_TRUE(str == global);
      ASSERT_TRUE(std::addressof(str) == std::addressof(global));
   }   
}

struct CountingClass
{
   static int count;
   static int assign_count;
   CountingClass() { ++count; }
   CountingClass(const CountingClass&) { ++count; }
   CountingClass& operator=(const CountingClass&) { ++assign_count; return *this; }
   ~CountingClass() { ++count; }
};

int CountingClass::count = 0;
int CountingClass::assign_count = 0;

void test_no_object_creation()
{
   ASSERT_EQ(0, CountingClass::count);
   ASSERT_EQ(0, CountingClass::assign_count);
   {
      CountingClass v1, v2;
      Optional<CountingClass&> oA(v1);
      Optional<CountingClass&> oB;
      Optional<CountingClass&> oC = oA;
      oB = oA;
      *oB = v2;
      oC = pdk::stdext::none;
      oC = Optional<CountingClass&>(v2);
      oB = pdk::stdext::none;
      oA = oB;
   }
   ASSERT_EQ(4, CountingClass::count);
   ASSERT_EQ(1, CountingClass::assign_count);
}

template <typename T>
typename std::enable_if< has_arrow<T>::value >::type
test_arrow_const()
{
   const typename concrete_type_of<T>::type v(2);
   Optional<const T&> o(v);
   ASSERT_TRUE(o);
   ASSERT_EQ(o->val(), 2);
   ASSERT_TRUE(std::addressof(o->val()) == std::addressof(v.val()));
}

template <typename T>
typename pdk::stdext::DisableIf< has_arrow<T>::value >::type
test_arrow_const()
{
}

template <typename T>
typename std::enable_if< has_arrow<T>::value >::type
test_arrow_noconst_const()
{
   typename concrete_type_of<T>::type v(2);
   Optional<const T&> o(v);
   ASSERT_TRUE(o);
   ASSERT_EQ(o->val(), 2);
   ASSERT_TRUE(std::addressof(o->val()) == std::addressof(v.val()));
   
   v.val() = 1;
   ASSERT_TRUE(o);
   ASSERT_EQ(o->val(), 1);
   ASSERT_EQ(v.val(), 1);
   ASSERT_TRUE(std::addressof(o->val()) == std::addressof(v.val()));
}

template <typename T>
typename pdk::stdext::DisableIf<has_arrow<T>::value>::type
test_arrow_noconst_const()
{
}

template <typename T>
typename std::enable_if< has_arrow<T>::value >::type
test_arrow()
{
   typename concrete_type_of<T>::type v(2);
   Optional<T&> o(v);
   ASSERT_TRUE(o);
   ASSERT_EQ(o->val(), 2);
   ASSERT_TRUE(std::addressof(o->val()) == std::addressof(v.val()));
   
   v.val() = 1;
   ASSERT_TRUE(o);
   ASSERT_EQ(o->val(), 1);
   ASSERT_EQ(v.val(), 1);
   ASSERT_TRUE(std::addressof(o->val()) == std::addressof(v.val()));
   
   o->val() = 3;
   ASSERT_TRUE(o);
   ASSERT_EQ(o->val(), 3);
   ASSERT_EQ(v.val(), 3);
   ASSERT_TRUE(std::addressof(o->val()) == std::addressof(v.val()));
   
}

template <typename T>
typename pdk::stdext::DisableIf< has_arrow<T>::value >::type
test_arrow()
{
}

template <typename T>
void test_not_containing_value_for()
{
   Optional<T&> o1;
   Optional<T&> o2 = pdk::stdext::none;
   Optional<T&> o3 = o1;
   
   ASSERT_TRUE(!o1);
   ASSERT_TRUE(!o2);
   ASSERT_TRUE(!o3);
   
   ASSERT_TRUE(o1 == pdk::stdext::none);
   ASSERT_TRUE(o2 == pdk::stdext::none);
   ASSERT_TRUE(o3 == pdk::stdext::none);
}

template <typename T>
void test_direct_init_for_const()
{
   const typename concrete_type_of<T>::type v(2);
   Optional<const T&> o(v);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(o != pdk::stdext::none);
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 2);
}

template <typename T>
void test_direct_init_for_noconst_const()
{
   typename concrete_type_of<T>::type v(2);
   Optional<const T&> o(v);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(o != pdk::stdext::none);
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 2);
   
   val(v) = 9;
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 9);
   ASSERT_EQ(val(v), 9);
}

template <typename T>
void test_direct_init_for()
{
   typename concrete_type_of<T>::type v(2);
   Optional<T&> o(v);
   
   ASSERT_TRUE(o);
   ASSERT_TRUE(o != pdk::stdext::none);
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 2);
   
   val(v) = 9;
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 9);
   ASSERT_EQ(val(v), 9);
   
   val(*o) = 7;
   ASSERT_TRUE(std::addressof(*o) == std::addressof(v));
   ASSERT_EQ(val(*o), val(v));
   ASSERT_EQ(val(*o), 7);
   ASSERT_EQ(val(v), 7);
}

template <typename T, typename U>
void test_clearing_the_value()
{
   typename concrete_type_of<T>::type v(2);
   Optional<U&> o1(v), o2(v);
   
   ASSERT_TRUE(o1);
   ASSERT_TRUE(o1 != pdk::stdext::none);
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2 != pdk::stdext::none);
   
   o1 = pdk::stdext::none;
   ASSERT_TRUE(!o1);
   ASSERT_TRUE(o1 == pdk::stdext::none);
   ASSERT_TRUE(o2);
   ASSERT_TRUE(o2 != pdk::stdext::none);
   ASSERT_EQ(val(*o2), 2);
   ASSERT_TRUE(std::addressof(*o2) == std::addressof(v));
   ASSERT_EQ(val(v), 2);
}

template <typename T, typename U>
void test_equality()
{
   typename concrete_type_of<T>::type v1(1), v2(2), v2_(2), v3(3);
   Optional<U&> o1(v1), o2(v2), o2_(v2_), o3(v3), o3_(v3), oN, oN_;
   // o2 and o2_ point to different objects; o3 and o3_ point to the same object
   
   ASSERT_TRUE(oN  == oN);
   ASSERT_TRUE(oN  == oN_);
   ASSERT_TRUE(oN_ == oN);
   ASSERT_TRUE(o1  == o1);
   ASSERT_TRUE(o2  == o2);
   ASSERT_TRUE(o2  == o2_);
   ASSERT_TRUE(o2_ == o2);
   ASSERT_TRUE(o3  == o3);
   ASSERT_TRUE(o3  == o3_);
   ASSERT_TRUE(!(oN == o1));
   ASSERT_TRUE(!(o1 == oN));
   ASSERT_TRUE(!(o2 == o1));
   ASSERT_TRUE(!(o2 == oN));
   
   ASSERT_TRUE(!(oN  != oN));
   ASSERT_TRUE(!(oN  != oN_));
   ASSERT_TRUE(!(oN_ != oN));
   ASSERT_TRUE(!(o1  != o1));
   ASSERT_TRUE(!(o2  != o2));
   ASSERT_TRUE(!(o2  != o2_));
   ASSERT_TRUE(!(o2_ != o2));
   ASSERT_TRUE(!(o3  != o3));
   ASSERT_TRUE(!(o3  != o3_));
   ASSERT_TRUE( (oN  != o1));
   ASSERT_TRUE( (o1  != oN));
   ASSERT_TRUE( (o2  != o1));
   ASSERT_TRUE( (o2  != oN));
}

template <typename T, typename U>
void test_order()
{
   typename concrete_type_of<T>::type v1(1), v2(2), v2_(2), v3(3);
   Optional<U&> o1(v1), o2(v2), o2_(v2_), o3(v3), o3_(v3), oN, oN_;
   // o2 and o2_ point to different objects; o3 and o3_ point to the same object
   
   ASSERT_TRUE(!(oN  < oN));
   ASSERT_TRUE(!(oN  < oN_));
   ASSERT_TRUE(!(oN_ < oN));
   ASSERT_TRUE(!(o1  < o1));
   ASSERT_TRUE(!(o2  < o2));
   ASSERT_TRUE(!(o2  < o2_));
   ASSERT_TRUE(!(o2_ < o2));
   ASSERT_TRUE(!(o3  < o3));
   ASSERT_TRUE(!(o3  < o3_));
   
   ASSERT_TRUE( (oN  <= oN));
   ASSERT_TRUE( (oN  <= oN_));
   ASSERT_TRUE( (oN_ <= oN));
   ASSERT_TRUE( (o1  <= o1));
   ASSERT_TRUE( (o2  <= o2));
   ASSERT_TRUE( (o2  <= o2_));
   ASSERT_TRUE( (o2_ <= o2));
   ASSERT_TRUE( (o3  <= o3));
   ASSERT_TRUE( (o3  <= o3_));
   
   ASSERT_TRUE(!(oN  > oN));
   ASSERT_TRUE(!(oN  > oN_));
   ASSERT_TRUE(!(oN_ > oN));
   ASSERT_TRUE(!(o1  > o1));
   ASSERT_TRUE(!(o2  > o2));
   ASSERT_TRUE(!(o2  > o2_));
   ASSERT_TRUE(!(o2_ > o2));
   ASSERT_TRUE(!(o3  > o3));
   ASSERT_TRUE(!(o3  > o3_));
   
   ASSERT_TRUE( (oN  >= oN));
   ASSERT_TRUE( (oN  >= oN_));
   ASSERT_TRUE( (oN_ >= oN));
   ASSERT_TRUE( (o1  >= o1));
   ASSERT_TRUE( (o2  >= o2));
   ASSERT_TRUE( (o2  >= o2_));
   ASSERT_TRUE( (o2_ >= o2));
   ASSERT_TRUE( (o3  >= o3));
   ASSERT_TRUE( (o3  >= o3_));
   
   ASSERT_TRUE( (oN  < o1));
   ASSERT_TRUE( (oN_ < o1));
   ASSERT_TRUE( (oN  < o2));
   ASSERT_TRUE( (oN_ < o2));
   ASSERT_TRUE( (oN  < o2_));
   ASSERT_TRUE( (oN_ < o2_));
   ASSERT_TRUE( (oN  < o3));
   ASSERT_TRUE( (oN_ < o3));
   ASSERT_TRUE( (oN  < o3_));
   ASSERT_TRUE( (oN_ < o3_));
   ASSERT_TRUE( (o1  < o2));
   ASSERT_TRUE( (o1  < o2_));
   ASSERT_TRUE( (o1  < o3));
   ASSERT_TRUE( (o1  < o3_));
   ASSERT_TRUE( (o2  < o3));
   ASSERT_TRUE( (o2_ < o3));
   ASSERT_TRUE( (o2  < o3_));
   ASSERT_TRUE( (o2_ < o3_));
   
   ASSERT_TRUE( (oN  <= o1));
   ASSERT_TRUE( (oN_ <= o1));
   ASSERT_TRUE( (oN  <= o2));
   ASSERT_TRUE( (oN_ <= o2));
   ASSERT_TRUE( (oN  <= o2_));
   ASSERT_TRUE( (oN_ <= o2_));
   ASSERT_TRUE( (oN  <= o3));
   ASSERT_TRUE( (oN_ <= o3));
   ASSERT_TRUE( (oN  <= o3_));
   ASSERT_TRUE( (oN_ <= o3_));
   ASSERT_TRUE( (o1  <= o2));
   ASSERT_TRUE( (o1  <= o2_));
   ASSERT_TRUE( (o1  <= o3));
   ASSERT_TRUE( (o1  <= o3_));
   ASSERT_TRUE( (o2  <= o3));
   ASSERT_TRUE( (o2_ <= o3));
   ASSERT_TRUE( (o2  <= o3_));
   ASSERT_TRUE( (o2_ <= o3_));
   
   ASSERT_TRUE(!(oN  > o1));
   ASSERT_TRUE(!(oN_ > o1));
   ASSERT_TRUE(!(oN  > o2));
   ASSERT_TRUE(!(oN_ > o2));
   ASSERT_TRUE(!(oN  > o2_));
   ASSERT_TRUE(!(oN_ > o2_));
   ASSERT_TRUE(!(oN  > o3));
   ASSERT_TRUE(!(oN_ > o3));
   ASSERT_TRUE(!(oN  > o3_));
   ASSERT_TRUE(!(oN_ > o3_));
   ASSERT_TRUE(!(o1  > o2));
   ASSERT_TRUE(!(o1  > o2_));
   ASSERT_TRUE(!(o1  > o3));
   ASSERT_TRUE(!(o1  > o3_));
   ASSERT_TRUE(!(o2  > o3));
   ASSERT_TRUE(!(o2_ > o3));
   ASSERT_TRUE(!(o2  > o3_));
   ASSERT_TRUE(!(o2_ > o3_));
   
   ASSERT_TRUE(!(oN  >= o1));
   ASSERT_TRUE(!(oN_ >= o1));
   ASSERT_TRUE(!(oN  >= o2));
   ASSERT_TRUE(!(oN_ >= o2));
   ASSERT_TRUE(!(oN  >= o2_));
   ASSERT_TRUE(!(oN_ >= o2_));
   ASSERT_TRUE(!(oN  >= o3));
   ASSERT_TRUE(!(oN_ >= o3));
   ASSERT_TRUE(!(oN  >= o3_));
   ASSERT_TRUE(!(oN_ >= o3_));
   ASSERT_TRUE(!(o1  >= o2));
   ASSERT_TRUE(!(o1  >= o2_));
   ASSERT_TRUE(!(o1  >= o3));
   ASSERT_TRUE(!(o1  >= o3_));
   ASSERT_TRUE(!(o2  >= o3));
   ASSERT_TRUE(!(o2_ >= o3));
   ASSERT_TRUE(!(o2  >= o3_));
   ASSERT_TRUE(!(o2_ >= o3_));
   
   ASSERT_TRUE(!(o1  < oN));
   ASSERT_TRUE(!(o1  < oN_));
   ASSERT_TRUE(!(o2  < oN));
   ASSERT_TRUE(!(o2  < oN_));
   ASSERT_TRUE(!(o2_ < oN));
   ASSERT_TRUE(!(o2_ < oN_));
   ASSERT_TRUE(!(o3  < oN));
   ASSERT_TRUE(!(o3  < oN_));
   ASSERT_TRUE(!(o3_ < oN));
   ASSERT_TRUE(!(o3_ < oN_));
   ASSERT_TRUE(!(o2  < oN));
   ASSERT_TRUE(!(o2_ < oN_));
   ASSERT_TRUE(!(o3  < oN));
   ASSERT_TRUE(!(o3_ < oN_));
   ASSERT_TRUE(!(o3  < oN));
   ASSERT_TRUE(!(o3  < oN_));
   ASSERT_TRUE(!(o3_ < oN));
   ASSERT_TRUE(!(o3_ < oN_));
}

template <typename T, typename U>
void test_swap()
{
   typename concrete_type_of<T>::type v1(1), v2(2);
   Optional<U&> o1(v1), o1_(v1), o2(v2), o2_(v2), oN, oN_;
   
   swap(o1, o1);
   ASSERT_TRUE(o1);
   ASSERT_TRUE(std::addressof(*o1) == std::addressof(v1));
   
   swap(oN, oN_);
   ASSERT_TRUE(!oN);
   ASSERT_TRUE(!oN_);
   
   swap(o1, oN);
   ASSERT_TRUE(!o1);
   ASSERT_TRUE(oN);
   ASSERT_TRUE(std::addressof(*oN) == std::addressof(v1));
   
   swap(oN, o1);
   ASSERT_TRUE(!oN);
   ASSERT_TRUE(o1);
   ASSERT_TRUE(std::addressof(*o1) == std::addressof(v1));
   
   swap(o1_, o2_);
   ASSERT_TRUE(o1_);
   ASSERT_TRUE(o2_);
   ASSERT_TRUE(std::addressof(*o1_) == std::addressof(v2));
   ASSERT_TRUE(std::addressof(*o2_) == std::addressof(v1));
}

template <typename T, typename U>
void test_convertability_of_compatible_reference_types()
{
   typename concrete_type_of<T>::type v1(1);
   Optional<T&> oN, o1(v1);
   Optional<U&> uN(oN), u1(o1);
   ASSERT_TRUE(!uN);
   ASSERT_TRUE(u1);
   ASSERT_TRUE(std::addressof(*u1) == std::addressof(*o1));
   
   uN = o1;
   u1 = oN;
   ASSERT_TRUE(!u1);
   ASSERT_TRUE(uN);
   ASSERT_TRUE(std::addressof(*uN) == std::addressof(*o1));
}

template <typename T>
void test_optional_ref()
{
   test_not_containing_value_for<T>();
   test_direct_init_for<T>();
   test_clearing_the_value<T, T>();
   test_arrow<T>();
   test_equality<T, T>();
   test_order<T, T>();
   test_swap<T, T>();
}

template <typename T>
void test_optional_const_ref()
{
   test_not_containing_value_for<const T>();
   test_direct_init_for_const<T>();
   test_direct_init_for_noconst_const<T>();
   test_clearing_the_value<const T, const T>();
   test_clearing_the_value<T, const T>();
   test_arrow_const<T>();
   test_arrow_noconst_const<T>();
   test_equality<const T, const T>();
   test_equality<T, const T>();
   test_order<const T, const T>();
   test_order<T, const T>();
   test_swap<const T, const T>();
   test_swap<T, const T>();
}
TEST(OptionalRefTest, testPortableMinimum)
{
   test_optional_ref<int>();
   test_optional_ref<ScopeGuard>();
   test_optional_ref<Abstract>();
   test_optional_ref< Optional<int> >();
   
   test_optional_const_ref<int>();
   test_optional_const_ref<ScopeGuard>();
   test_optional_const_ref<Abstract>();
   test_optional_const_ref< Optional<int> >();
   
   test_convertability_of_compatible_reference_types<int, const int>();
   test_convertability_of_compatible_reference_types<Impl, Abstract>();
   test_convertability_of_compatible_reference_types<Impl, const Abstract>();
   test_convertability_of_compatible_reference_types<const Impl, const Abstract>();
   test_convertability_of_compatible_reference_types<Optional<int>, const Optional<int> >();
}

struct ValueCls
{
   int val;
   explicit ValueCls(int v) : val(v) {}
};

int val1(int const& i)
{
   return i;
}

int val1(ValueCls const& v)
{
   return v.val;
}

template <typename Tref>
Optional<Tref&> make_opt_ref(Tref& v)
{
   return Optional<Tref&>(v);
}

template <typename Tval, typename Tref>
void test_construct_from_optional_ref()
{
   Tref v1 (1), v2 (2);
   
   Optional<Tref&> opt_ref0;
   Optional<Tref&> opt_ref1 (v1);
   
   Optional<Tval> opt_val0 (opt_ref0);
   Optional<Tval> opt_val1 (opt_ref1);
   Optional<Tval> opt_val2 (make_opt_ref(v2));
   
   ASSERT_TRUE (!opt_val0);
   ASSERT_TRUE (opt_val1);
   ASSERT_TRUE (opt_val2);
   
   ASSERT_EQ (1, val1(*opt_val1));
   ASSERT_EQ (2, val1(*opt_val2));
   
   ASSERT_TRUE (std::addressof(*opt_val1) != std::addressof(v1));
   ASSERT_TRUE (std::addressof(*opt_val2) != std::addressof(v2));
}

template <typename Tval, typename Tref>
void test_assign_from_optional_ref()
{
   Tref v1 (1), v2 (2);
   
   Optional<Tref&> opt_ref0;
   Optional<Tref&> opt_ref1 (v1);
   
   Optional<Tval> opt_val0;
   Optional<Tval> opt_val1;
   Optional<Tval> opt_val2;
   
   opt_val0 = opt_ref0;
   opt_val1 = opt_ref1;
   opt_val2 = make_opt_ref(v2);
   
   ASSERT_TRUE (!opt_val0);
   ASSERT_TRUE (opt_val1);
   ASSERT_TRUE (opt_val2);
   
   ASSERT_EQ (1, val1(*opt_val1));
   ASSERT_EQ (2, val1(*opt_val2));
   
   ASSERT_TRUE (std::addressof(*opt_val1) != std::addressof(v1));
   ASSERT_TRUE (std::addressof(*opt_val2) != std::addressof(v2));
}

TEST(OptionalRefTest, testRefToVal)
{
   test_construct_from_optional_ref<int, int>();
   test_construct_from_optional_ref<int, int const>();
   test_construct_from_optional_ref<int const, int const>();
   test_construct_from_optional_ref<int const, int>();
   
   test_construct_from_optional_ref<ValueCls, ValueCls>();
   test_construct_from_optional_ref<ValueCls, ValueCls const>();
   test_construct_from_optional_ref<ValueCls const, ValueCls const>();
   test_construct_from_optional_ref<ValueCls const, ValueCls>();
   
   test_assign_from_optional_ref<int, int>();
   test_assign_from_optional_ref<int, int const>();
   
   test_assign_from_optional_ref<ValueCls, ValueCls>();
   test_assign_from_optional_ref<ValueCls, ValueCls const>();
}

// THIS TEST SHOULD FAIL TO COMPILE

TEST(OptionalRefTest, testFailure)
{
   
}
