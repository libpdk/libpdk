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

namespace  {

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

int& val(int& i) { return i; }
int& val(Abstract& a) { return a.val(); }
int& val(Impl& a) { return a.val(); }
template <typename T> int& val(T& o) { return *o; }

const int& val(const int& i) { return i; }
const int& val(const Abstract& a) { return a.val(); }
const int& val(const Impl& a) { return a.val(); }
template <typename T> const int& val(const T& o) { return *o; }

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
   ASSERT_TRUE(addressof(*o) == addressof(v));
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
   ASSERT_TRUE(addressof(*o) != addressof(v));
   ASSERT_NE(val(*o), val(v));
   ASSERT_NE(val(*o), 2);
   
   ASSERT_TRUE(addressof(*o) == addressof(w));
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

TEST(OptionalRefTest, testRefAssignConstInt)
{
   test_copy_assignment_for_const<int>();
   test_copy_assignment_for_noconst_const<int>();
   test_rebinding_assignment_semantics_noconst_const<int>();
}
