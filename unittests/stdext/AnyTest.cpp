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
// Created by softboy on 2017/01/12.

#include "gtest/gtest.h"
#include "pdk/stdext/Any.h"
#include <type_traits>

using pdk::stdext::Any;
using pdk::stdext::any_cast;
using pdk::stdext::unsafe_any_cast;
using pdk::stdext::BadAnyCast;

TEST(AnyTest, testDefaultConstructor)
{
   const Any value;
   ASSERT_TRUE(value.empty());
   ASSERT_EQ(any_cast<int>(&value) ,nullptr);
   ASSERT_EQ(value.getType(), typeid(void));
}

TEST(AnyTest, testConvertingCtor)
{
   std::string text = "test message";
   Any value = text;
   ASSERT_FALSE(value.empty());
   ASSERT_EQ(value.getType(), typeid(std::string));
   ASSERT_EQ(any_cast<int>(&value) ,nullptr);
   ASSERT_TRUE(any_cast<std::string>(&value) != nullptr);
   ASSERT_EQ(any_cast<std::string>(value), text);
   ASSERT_NE(any_cast<std::string>(&value), &text);
}

TEST(AnyTest, testCopyCtor)
{
   std::string text = "test message";
   Any original = text;
   Any copy = original;
   ASSERT_FALSE(copy.empty());
   ASSERT_EQ(original.getType(), copy.getType());
   ASSERT_EQ(any_cast<std::string>(original), any_cast<std::string>(copy));
   ASSERT_EQ(text, any_cast<std::string>(copy));
   ASSERT_NE(any_cast<std::string>(&original), any_cast<std::string>(&copy));
}

TEST(AnyTest, testCopyAssign)
{
   std::string text = "test message";
   Any original = text, copy;
   Any *assignResult = &(copy = original);
   ASSERT_FALSE(copy.empty());
   ASSERT_EQ(original.getType(), copy.getType());
   ASSERT_EQ(any_cast<std::string>(original), any_cast<std::string>(copy));
   ASSERT_EQ(text, any_cast<std::string>(copy));
   ASSERT_NE(any_cast<std::string>(&original), any_cast<std::string>(&copy));
   ASSERT_EQ(assignResult, &copy);
}

TEST(AnyTest, testConvertingAssign)
{
   std::string text = "test message";
   Any value;
   Any *assignResult = &(value = text);
   ASSERT_EQ(value.getType(), typeid(std::string));
   ASSERT_EQ(any_cast<int>(&value), nullptr);
   ASSERT_TRUE(any_cast<std::string>(&value) != nullptr);
   ASSERT_EQ(any_cast<std::string>(value), text);
   ASSERT_NE(any_cast<std::string>(&value), &text);
   ASSERT_EQ(assignResult, &value);
}

TEST(AnyTest, testBadCast)
{
   std::string text = "test message";
   Any value = text;
   ASSERT_THROW(any_cast<const char *>(value), BadAnyCast);
}

namespace {
struct CopyCounter
{
   
public:
   
   CopyCounter() {}
   CopyCounter(const CopyCounter&) { ++count; }
   CopyCounter& operator=(const CopyCounter&) { ++count; return *this; }
   static int getCount() { return count; }
   
private:
   
   static int count;
   
};

int CopyCounter::count = 0;
}

TEST(AnyTest, testSwap)
{
   std::string text = "test message";
   Any original = text, swapped;
   std::string *originalPtr = any_cast<std::string>(&original);
   Any  *swapResult = &original.swap(swapped);
   ASSERT_TRUE(original.empty());
   ASSERT_FALSE(swapped.empty());
   ASSERT_EQ(swapped.getType(), typeid(std::string));
   ASSERT_EQ(text, any_cast<std::string>(swapped));
   ASSERT_TRUE(originalPtr != nullptr);
   ASSERT_EQ(originalPtr, any_cast<std::string>(&swapped));
   ASSERT_EQ(swapResult, &original);
   Any copy1 = CopyCounter();
   Any copy2 = CopyCounter();
   int count = CopyCounter::getCount();
   swap(copy1, copy2);
   ASSERT_EQ(count, CopyCounter::getCount());
}

TEST(AnyTest, testNullCopying)
{
   const Any null;
   Any copied = null;
   Any assigned;
   assigned = null;
   ASSERT_TRUE(null.empty());
   ASSERT_TRUE(copied.empty());
   ASSERT_TRUE(assigned.empty());
}

TEST(AnyTest, testCastToReference)
{
   Any a(137);
   const Any b(a);
   
   int &                ra    = any_cast<int &>(a);
   int const &          ra_c  = any_cast<int const &>(a);
   int volatile &       ra_v  = any_cast<int volatile &>(a);
   int const volatile & ra_cv = any_cast<int const volatile&>(a);
   
   ASSERT_TRUE(&ra == &ra_c && &ra == &ra_v && &ra == &ra_cv);
   
   int const &          rb_c  = any_cast<int const &>(b);
   int const volatile & rb_cv = any_cast<int const volatile &>(b);
   
   ASSERT_TRUE(&rb_c == &rb_cv);
   ASSERT_TRUE(&ra != &rb_c);
   
   ++ra;
   int incremented = any_cast<int>(a);
   ASSERT_TRUE(incremented == 138);
   ASSERT_THROW(any_cast<char &>(a), BadAnyCast);
   ASSERT_THROW(any_cast<const char &>(b), BadAnyCast);
}

TEST(AnyTest, testWithArray)
{
   Any value1("Char array");
   Any value2;
   value2 = "Char array";
   
   ASSERT_FALSE(value1.empty());
   ASSERT_FALSE(value2.empty());
   
   ASSERT_EQ(value1.getType(), typeid(const char*));
   ASSERT_EQ(value2.getType(), typeid(const char*));
   
   ASSERT_TRUE(any_cast<const char*>(&value1) != nullptr);
   ASSERT_TRUE(any_cast<const char*>(&value2) != nullptr);
}

namespace {

const std::string& returning_string1() 
{
   static const std::string ret("foo"); 
   return ret;
}

std::string returning_string2() 
{
   static const std::string ret("foo"); 
   return ret;
}

}

TEST(AnyTest, testWithFunc)
{
   std::string s;
   s = any_cast<std::string>(returning_string1());
   s = any_cast<const std::string&>(returning_string1());
   
   s = any_cast<std::string>(returning_string2());
   s = any_cast<const std::string&>(returning_string2());
   
#if !defined(__INTEL_COMPILER) && !defined(__ICL) && (!defined(_MSC_VER) || _MSC_VER != 1600)
   // Intel compiler thinks that it must choose the `any_cast(const any&)` function 
   // instead of the `any_cast(const any&&)`.
   // Bug was not reported because of missing premier support account + annoying 
   // registrations requirements.
   
   // MSVC-10 had a bug:
   //
   // any.hpp(291) : error C2440: 'return' : cannot convert.
   // Conversion loses qualifiers
   // any_test.cpp(304) : see reference to function template instantiation
   //
   // This issue was fixed in MSVC-11.
   
   s = any_cast<std::string&&>(returning_string1());
#endif
   
   s = any_cast<std::string&&>(returning_string2());
   PDK_UNUSED(s);
}

TEST(AnyTest, testClear)
{
   std::string text = "test message";
   Any value = text;
   
   ASSERT_FALSE(value.empty());
   
   value.clear();
   ASSERT_TRUE(value.empty());
   
   value.clear();
   ASSERT_TRUE(value.empty());
   
   value = text;
   ASSERT_FALSE(value.empty());
   
   value.clear();
   ASSERT_TRUE(value.empty());
}

Any makeVec() 
{
   return std::vector<int>(100 /*size*/, 7 /*value*/);
}


TEST(AnyTest, testVectors)
{
   const std::vector<int>& vec = any_cast<std::vector<int> >(makeVec()); 
   ASSERT_EQ(vec.size(), 100u); 
   ASSERT_EQ(vec.back(), 7);
   ASSERT_EQ(vec.front(), 7);
   
   std::vector<int> vec1 = any_cast<std::vector<int> >(makeVec()); 
   ASSERT_EQ(vec1.size(), 100u); 
   ASSERT_EQ(vec1.back(), 7);
   ASSERT_EQ(vec1.front(), 7);
}

namespace {

template<typename T>
class classWithAddressOp {
public:
   classWithAddressOp(const T* p)
      : ptr(p)
   {}
   
   const T** operator &() {
      return &ptr;
   }
   
   const T* get() const {
      return ptr;
   }
   
private:
   const T* ptr;
};

}

TEST(AnyTest, testAddressof)
{
   int val = 10;
   const int* ptr = &val;
   classWithAddressOp<int> obj(ptr);
   Any test_val(obj);
   
   classWithAddressOp<int> returned_obj = any_cast<classWithAddressOp<int> >(test_val);
   ASSERT_EQ(&val, returned_obj.get());
   
   ASSERT_TRUE(!!any_cast<classWithAddressOp<int> >(&test_val));
   ASSERT_EQ(unsafe_any_cast<classWithAddressOp<int> >(&test_val)->get(), ptr);
}
