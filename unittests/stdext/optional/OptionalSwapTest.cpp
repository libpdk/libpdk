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
#include <utility>
#include <iostream>

using pdk::stdext::none;
using pdk::stdext::optional::Optional;

#define ARG(T) (static_cast< T const* >(0))

namespace optional_swap_test
{
class default_ctor_exception : public std::exception {} ;
class copy_ctor_exception : public std::exception {} ;
class assignment_exception : public std::exception {} ;

//
// Base class for swap test classes.  Its assignment should not be called, when swapping
// optional<T> objects.  (The default std::swap would do so.)
//
class base_class_with_forbidden_assignment
{
public:
   base_class_with_forbidden_assignment & operator=(const base_class_with_forbidden_assignment &)
   {
      std::cerr<<"The assignment should not be used while swapping!";
      throw assignment_exception();
   }
   
   virtual ~base_class_with_forbidden_assignment() {}
};

//
// Class without default constructor
//
class class_without_default_ctor : public base_class_with_forbidden_assignment
{
public:
   char data;
   explicit class_without_default_ctor(char arg) : data(arg) {}
};

//
// Class whose default constructor should not be used by optional::swap!
//
class class_whose_default_ctor_should_not_be_used : public base_class_with_forbidden_assignment
{
public:
   char data;
   explicit class_whose_default_ctor_should_not_be_used(char arg) : data(arg) {}
   
   class_whose_default_ctor_should_not_be_used()
   {
      std::cerr<<"This default constructor should not be used while swapping!";
      throw default_ctor_exception();
   }
};

//
// Class whose default constructor should be used by optional::swap.
// Its copy constructor should be avoided!
//
class class_whose_default_ctor_should_be_used : public base_class_with_forbidden_assignment
{
public:
   char data;
   explicit class_whose_default_ctor_should_be_used(char arg) : data(arg) { }
   
   class_whose_default_ctor_should_be_used() : data('\0') { }
   
   class_whose_default_ctor_should_be_used(const class_whose_default_ctor_should_be_used &)
   {
      std::cerr<<"This copy constructor should not be used while swapping!";
      throw copy_ctor_exception();
   }
};

//
// Class template whose default constructor should be used by optional::swap.
// Its copy constructor should be avoided!
//
template <class T>
class template_whose_default_ctor_should_be_used : public base_class_with_forbidden_assignment
{
public:
   T data;
   explicit template_whose_default_ctor_should_be_used(T arg) : data(arg) { }
   
   template_whose_default_ctor_should_be_used() : data('\0') { }
   
   template_whose_default_ctor_should_be_used(const template_whose_default_ctor_should_be_used &)
   {
      std::cerr<<"This copy constructor should not be used while swapping!";
      throw copy_ctor_exception();
   }
};

//
// Class whose explicit constructor should be used by optional::swap.
// Its other constructors should be avoided!
//
class class_whose_explicit_ctor_should_be_used : public base_class_with_forbidden_assignment
{
public:
   char data;
   explicit class_whose_explicit_ctor_should_be_used(char arg) : data(arg) { }
   
   class_whose_explicit_ctor_should_be_used()
   {
      std::cerr<<"This default constructor should not be used while swapping!";
      throw default_ctor_exception();
   }
   
   class_whose_explicit_ctor_should_be_used(const class_whose_explicit_ctor_should_be_used &)
   {
      std::cerr<<"This copy constructor should not be used while swapping!";
      throw copy_ctor_exception();
   }
};

void swap(class_whose_default_ctor_should_not_be_used & lhs, class_whose_default_ctor_should_not_be_used & rhs)
{
   std::swap(lhs.data, rhs.data);
}

void swap(class_whose_default_ctor_should_be_used & lhs, class_whose_default_ctor_should_be_used & rhs)
{
   std::swap(lhs.data, rhs.data);
}

void swap(class_without_default_ctor & lhs, class_without_default_ctor & rhs)
{
   std::swap(lhs.data, rhs.data);
}

void swap(class_whose_explicit_ctor_should_be_used & lhs, class_whose_explicit_ctor_should_be_used & rhs)
{
   std::swap(lhs.data, rhs.data);
}

template <class T>
void swap(template_whose_default_ctor_should_be_used<T> & lhs, template_whose_default_ctor_should_be_used<T> & rhs)
{
   std::swap(lhs.data, rhs.data);
}

//
// optional<T>::swap should be customized when neither the copy constructor
// nor the default constructor of T are supposed to be used when swapping, e.g.,
// for the following type T = class_whose_explicit_ctor_should_be_used.
//
void swap(Optional<class_whose_explicit_ctor_should_be_used> & x, Optional<class_whose_explicit_ctor_should_be_used> & y)
{
   bool hasX(x);
   bool hasY(y);
   
   if ( !hasX && !hasY )
      return;
   
//   if( !hasX )
//      x = boost::in_place('\0');
//   else if ( !hasY )
//      y = boost::in_place('\0');
   
   optional_swap_test::swap(*x,*y);
   
   if( !hasX )
      y = none ;
   else if( !hasY )
      x = none ;
}


} // End of namespace optional_swap_test.


//
// Tests whether the swap function works properly for optional<T>.
// Assumes that T has one data member, of type char.
// Returns true iff the test is passed.
//
template <class T>
void test_swap_function( T const* )
{
  try
  {
    Optional<T> obj1;
    Optional<T> obj2('a');

    // Self-swap should not have any effect.
    swap(obj1, obj1);
    swap(obj2, obj2);
    ASSERT_TRUE(!obj1);
    ASSERT_TRUE(!!obj2 && obj2->data == 'a');

    // Call non-member swap.
    swap(obj1, obj2);

    // Test if obj1 and obj2 are really swapped.
    ASSERT_TRUE(!!obj1 && obj1->data == 'a');
    ASSERT_TRUE(!obj2);

    // Call non-member swap one more time.
    swap(obj1, obj2);

    // Test if obj1 and obj2 are swapped back.
    ASSERT_TRUE(!obj1);
    ASSERT_TRUE(!!obj2 && obj2->data == 'a');
  }
  catch(const std::exception &)
  {
    // The swap function should not throw, for our test cases.
    FAIL() << "throw in swap";
  }
}

//
// Tests whether the optional<T>::swap member function works properly.
// Assumes that T has one data member, of type char.
// Returns true iff the test is passed.
//
template <class T>
void test_swap_member_function( T const* )
{
  try
  {
    Optional<T> obj1;
    Optional<T> obj2('a');

    // Self-swap should not have any effect.
    obj1.swap(obj1);
    obj2.swap(obj2);
    ASSERT_TRUE(!obj1);
    ASSERT_TRUE(!!obj2 && obj2->data == 'a');

    // Call member swap.
    obj1.swap(obj2);

    // Test if obj1 and obj2 are really swapped.
    ASSERT_TRUE(!!obj1 && obj1->data == 'a');
    ASSERT_TRUE(!obj2);

    // Call member swap one more time.
    obj1.swap(obj2);

    // Test if obj1 and obj2 are swapped back.
    ASSERT_TRUE(!obj1);
    ASSERT_TRUE(!!obj2 && obj2->data == 'a');
  }
  catch(const std::exception &)
  {
    FAIL() << "throw in swap";
  }
}

void test_swap_tweaking()
{
  ( test_swap_function( ARG(optional_swap_test::class_without_default_ctor) ) );
  ( test_swap_function( ARG(optional_swap_test::class_whose_default_ctor_should_be_used) ) );
  ( test_swap_function( ARG(optional_swap_test::class_whose_default_ctor_should_not_be_used) ) );
  ( test_swap_function( ARG(optional_swap_test::class_whose_explicit_ctor_should_be_used) ) );
  ( test_swap_function( ARG(optional_swap_test::template_whose_default_ctor_should_be_used<char>) ) );
  ( test_swap_member_function( ARG(optional_swap_test::class_without_default_ctor) ) );
  ( test_swap_member_function( ARG(optional_swap_test::class_whose_default_ctor_should_be_used) ) );
  ( test_swap_member_function( ARG(optional_swap_test::class_whose_default_ctor_should_not_be_used) ) );
  ( test_swap_member_function( ARG(optional_swap_test::class_whose_explicit_ctor_should_be_used) ) );
  ( test_swap_member_function( ARG(optional_swap_test::template_whose_default_ctor_should_be_used<char>) ) );
}


TEST(OptionalSwapTest, testSwap)
{
   test_swap_tweaking();
}
