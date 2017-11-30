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
// Created by softboy on 2017/11/29.

#include <limits.h>
#include <cstdio>
#include <list>
#include <utility>
#include <tuple>

#include "gtest/gtest.h"
#include "pdk/base/os/thread/Atomic.h"

using pdk::os::thread::BasicAtomicInt;
using pdk::os::thread::BasicAtomicInteger;
using pdk::os::thread::AtomicInt;

namespace
{

int puts( const char *str )
{
   return 1;
}

template <int Int>
inline void assembly_marker(void *ptr = 0)
{
   puts(reinterpret_cast<char *>(ptr) + Int);
}

template <typename T, typename Atomic>
void warning_free_helper_template()
{
   T expectedValue = 0;
   T newValue = 0;
   T valueToAdd = 0;
   assembly_marker<0>();
   Atomic i = PDK_BASIC_ATOMIC_INITIALIZER(0);
   //std::printf("%d\n", static_cast<int>(i.loadAcquire()));
   ASSERT_EQ(0, static_cast<int>(i.loadAcquire()));
   assembly_marker<1>(&i);
   i.load();
   assembly_marker<11>(&i);
   i.loadAcquire();
   assembly_marker<12>(&i);
   
   i.store(newValue);
   assembly_marker<21>(&i);
   i.storeRelease(newValue);
   assembly_marker<22>(&i);
   
   i.ref();
   assembly_marker<31>(&i);
   i.deref();
   assembly_marker<32>(&i);
   
   i.testAndSetRelaxed(expectedValue, newValue);
   assembly_marker<41>(&i);
   i.testAndSetAcquire(expectedValue, newValue);
   assembly_marker<42>(&i);
   i.testAndSetRelease(expectedValue, newValue);
   assembly_marker<43>(&i);
   i.testAndSetOrdered(expectedValue, newValue);
   assembly_marker<44>(&i);
   
   i.fetchAndStoreRelaxed(newValue);
   assembly_marker<51>(&i);
   i.fetchAndStoreAcquire(newValue);
   assembly_marker<52>(&i);
   i.fetchAndStoreRelease(newValue);
   assembly_marker<53>(&i);
   i.fetchAndStoreOrdered(newValue);
   assembly_marker<54>(&i);
   
   i.fetchAndAddRelaxed(valueToAdd);
   assembly_marker<61>(&i);
   i.fetchAndAddAcquire(valueToAdd);
   assembly_marker<62>(&i);
   i.fetchAndAddRelease(valueToAdd);
   assembly_marker<63>(&i);
   i.fetchAndAddOrdered(valueToAdd);
   assembly_marker<64>(&i);
}

template <bool>
inline void boolean_helper()
{}

template <typename Atomic>
void constexpr_functions_helper_template()
{
   
#ifdef PDK_COMPILER_CONSTEXPR
   boolean_helper<Atomic::isRefCountingNative()>();
   boolean_helper<Atomic::isRefCountingWaitFree()>();
   boolean_helper<Atomic::isTestAndSetNative()>();
   boolean_helper<Atomic::isTestAndSetWaitFree()>();
   boolean_helper<Atomic::isFetchAndStoreNative()>();
   boolean_helper<Atomic::isFetchAndStoreWaitFree()>();
   boolean_helper<Atomic::isFetchAndAddNative()>();
   boolean_helper<Atomic::isFetchAndAddWaitFree()>();
#endif
   
}

void warning_free_helper()
{
   warning_free_helper_template<int, BasicAtomicInt>();
   warning_free_helper_template<int, BasicAtomicInteger<int>>();
   warning_free_helper_template<unsigned int, BasicAtomicInteger<unsigned int>>();
   constexpr_functions_helper_template<BasicAtomicInteger<int>>();
   constexpr_functions_helper_template<BasicAtomicInteger<unsigned int>>();
   
#ifdef PDK_COMPILER_UNICODE_STRINGS
   warning_free_helper_template<pdk::pint16, BasicAtomicInteger<char32_t>>();
   constexpr_functions_helper_template<BasicAtomicInteger<char32_t>>();
#endif
   
   // pointer-sized integers are always supported:
   warning_free_helper_template<int, BasicAtomicInteger<pdk::ptrdiff>>();
   warning_free_helper_template<unsigned int, BasicAtomicInteger<pdk::uintptr>>();
   constexpr_functions_helper_template<BasicAtomicInteger<pdk::ptrdiff>>();
   constexpr_functions_helper_template<BasicAtomicInteger<pdk::uintptr>>();
   
   // long is always supported because it's either 32-bit or pointer-sized:
   warning_free_helper_template<int, BasicAtomicInteger<long int>>();
   warning_free_helper_template<unsigned int, BasicAtomicInteger<unsigned long int>>();
   constexpr_functions_helper_template<BasicAtomicInteger<long int>>();
   constexpr_functions_helper_template<BasicAtomicInteger<unsigned long int>>();
   
#ifdef PDK_ATOMIC_INT16_IS_SUPPORTED
   warning_free_helper_template<pdk::pint16, BasicAtomicInteger<pdk::pint16>>();
   warning_free_helper_template<pdk::puint16, BasicAtomicInteger<pdk::puint16>>();
   constexpr_functions_helper_template<BasicAtomicInteger<pdk::pint16>>();
   constexpr_functions_helper_template<BasicAtomicInteger<pdk::puint16>>();
   
#  ifdef PDK_COMPILER_UNICODE_STRINGS
   warning_free_helper_template<pdk::pint16, BasicAtomicInteger<char16_t>>();
   constexpr_functions_helper_template<BasicAtomicInteger<char16_t>>();
#  endif
   
#endif
   
#ifdef PDK_ATOMIC_INT8_IS_SUPPORTED
   warning_free_helper_template<char, BasicAtomicInteger<char>>();
   warning_free_helper_template<signed char, BasicAtomicInteger<signed char>>();
   warning_free_helper_template<unsigned char, BasicAtomicInteger<unsigned char>>();
   constexpr_functions_helper_template<BasicAtomicInteger<char>>();
   constexpr_functions_helper_template<BasicAtomicInteger<signed char>>();
   constexpr_functions_helper_template<BasicAtomicInteger<unsigned char>>();
#endif
   
#ifdef PDK_ATOMIC_INT64_IS_SUPPORTED
#  if !defined(__i386__) || (defined(PDK_CC_GNU) && defined(__OPTIMIZE__))
   warning_free_helper_template<pdk::plonglong, BasicAtomicInteger<pdk::plonglong>>();
   warning_free_helper_template<pdk::pulonglong, BasicAtomicInteger<pdk::pulonglong>>();
   constexpr_functions_helper_template<BasicAtomicInteger<pdk::plonglong>>();
   constexpr_functions_helper_template<BasicAtomicInteger<pdk::pulonglong>>();
#  endif
#endif
}

}

template <typename T> 
struct TypeInStruct
{ 
   T type; 
};

TEST(AtomicIntTest, testWarningFreeHelper)
{
   warning_free_helper();
}

TEST(AtomicIntTest, testWarningFree)
{
   void (*foo)() = &warning_free_helper;
   (void)foo;
}

TEST(AtomicIntTest, testAlignment)
{
   // this will cause a build error if the alignment isn't the same
   char dummy1[alignof(BasicAtomicInt) == alignof(TypeInStruct<int>) ? 1 : -1];
   char dummy2[alignof(AtomicInt) == alignof(TypeInStruct<int>) ? 1 : -1];
   (void)dummy1;
   (void)dummy2;
   
#ifdef PDK_ATOMIC_INT8_IS_SUPPORTED
   ASSERT_EQ(alignof(BasicAtomicInteger<char>), alignof(TypeInStruct<char>));
#endif
   
#ifdef PDK_ATOMIC_INT16_IS_SUPPORTED
   ASSERT_EQ(alignof(BasicAtomicInteger<short>), alignof(TypeInStruct<short>));
#endif
   
#ifdef PDK_ATOMIC_INT32_IS_SUPPORTED
   ASSERT_EQ(alignof(BasicAtomicInteger<int>), alignof(TypeInStruct<int>));
#endif
   
#ifdef PDK_ATOMIC_INT64_IS_SUPPORTED
   ASSERT_EQ(alignof(BasicAtomicInteger<pdk::plonglong>), alignof(TypeInStruct<pdk::plonglong>));
#endif
   
}

namespace 
{

void make_data_list(std::list<int> &data)
{
   data.push_back(31337);
   data.push_back(0);
   data.push_back(1);
   data.push_back(-1);
   data.push_back(2);
   data.push_back(-2);
   data.push_back(3);
   data.push_back(-3);
   data.push_back(INT_MAX);
   data.push_back(INT_MIN + 1);
}

}

TEST(AtomicIntTest, testConstructor)
{
   std::list<int> data;
   make_data_list(data);
   
   std::list<int>::iterator begin = data.begin();
   std::list<int>::iterator end = data.end();
   while (begin != end) {
      int value = *begin;
      AtomicInt atomic1(value);
      ASSERT_EQ(atomic1.load(), value);
      AtomicInt atomic2 = value;
      ASSERT_EQ(atomic2.load(), value);
      ++begin;
   }
}

TEST(AtomicIntTest, testCopyConstructor)
{
   std::list<int> data;
   make_data_list(data);
   std::list<int>::iterator begin = data.begin();
   std::list<int>::iterator end = data.end();
   while (begin != end) {
      int value = *begin;
      AtomicInt atomic1(value);
      ASSERT_EQ(atomic1.load(), value);
      AtomicInt atomic2(atomic1);
      ASSERT_EQ(atomic2.load(), value);
      AtomicInt atomic3 = atomic1;
      ASSERT_EQ(atomic3.load(), value);
      AtomicInt atomic4(atomic2);
      ASSERT_EQ(atomic4.load(), value);
      AtomicInt atomic5 = atomic2;
      ASSERT_EQ(atomic5.load(), value);
      ++begin;
   }
}

TEST(AtomicIntTest, testAssignmentOperator)
{
   std::list<std::pair<int, int>> data;
   data.push_back(std::make_pair(0, 1));
   data.push_back(std::make_pair(1, 0));
   data.push_back(std::make_pair(0, -1));
   data.push_back(std::make_pair(-1, 0));
   data.push_back(std::make_pair(-1, 1));
   data.push_back(std::make_pair(1, -1));
   std::list<std::pair<int, int>>::iterator begin = data.begin();
   std::list<std::pair<int, int>>::iterator end = data.end();
   while (begin != end) {
      std::pair<int, int> pair = *begin;
      int value = pair.first;
      int newValue = pair.second;
      AtomicInt atomic1 = value;
      atomic1 = newValue;
      ASSERT_EQ(atomic1.load(), newValue);
      atomic1 = value;
      ASSERT_EQ(atomic1.load(), value);
      AtomicInt atomic2 = newValue;
      atomic1 = atomic2;
      ASSERT_EQ(atomic1.load(), atomic2.load());
      ++begin;
   }
}

TEST(AtomicIntTest, testIsRefCountingNative)
{
#if defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE)
   ASSERT_TRUE(AtomicInt::isRefCountingNative());
#  if (defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE) \
   || (defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_NEVER_NATIVE)))
#     error "Define only one of PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_{ALWAYS, SOMETIMES, NEVER}_NATIVE"
#  endif
#elif defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE)
   ASSERT_TRUE(AtomicInt::isRefCountingNative() || !AtomicInt::isRefCountingNative());
#  if (defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE) \
   || (defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_NEVER_NATIVE)))
#     error "Define only one of PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_{ALWAYS, SOMETIMES, NEVER}_NATIVE"
#  endif
#elif defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_NEVER_NATIVE)
   ASSERT_TRUE(!AtomicInt::isRefCountingNative());
#  if (defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE) \
   || (defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE)))
#     error "Define only one of PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_{ALWAYS, SOMETIMES, NEVER}_NATIVE"
#  endif
#else
#  error "Define only one of PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_{ALWAYS, SOMETIMES, NEVER}_NATIVE is not defined"
#endif
}

TEST(AtomicIntTest, testIsRefCountingWaitFree)
{
#if defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_WAIT_FREE)
   ASSERT_TRUE(AtomicInt::isRefCountingWaitFree());
   ASSERT_TRUE(AtomicInt::isRefCountingNative());
#  if defined(PDK_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
   ASSERT_TRUE(!AtomicInt::isRefCountingWaitFree());
#endif
}

TEST(AtomicIntTest, testRef)
{
   std::list<std::tuple<int, int, int>> data;
   data.push_back(std::make_tuple(0, 1, 1));
   data.push_back(std::make_tuple(-1, 0, 0));
   data.push_back(std::make_tuple(1, 1, 2));
   std::list<std::tuple<int, int, int>>::iterator begin = data.begin();
   std::list<std::tuple<int, int, int>>::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      int value = std::get<0>(item);
      int result = std::get<1>(item);
      int expected = std::get<2>(item);
      AtomicInt atomic = value;
      ASSERT_EQ(atomic.ref() ? 1 : 0, result);
      ASSERT_EQ(atomic.load(), expected);
      ++begin;
   }
}

TEST(AtomicIntTest, testDeref)
{
   std::list<std::tuple<int, int, int>> data;
   data.push_back(std::make_tuple(0, 1, -1));
   data.push_back(std::make_tuple(1, 0, 0));
   data.push_back(std::make_tuple(2, 1, 1));
   std::list<std::tuple<int, int, int>>::iterator begin = data.begin();
   std::list<std::tuple<int, int, int>>::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      int value = std::get<0>(item);
      int result = std::get<1>(item);
      int expected = std::get<2>(item);
      AtomicInt atomic = value;
      ASSERT_EQ(atomic.deref() ? 1 : 0, result);
      ASSERT_EQ(atomic.load(), expected);
      ++begin;
   }
}

TEST(AtomicIntTest, testAtomicIsTestAndSetNative)
{
#if defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE)
   ASSERT_TRUE(AtomicInt::isTestAndSetNative());
#  if (defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_SOMETIMES_NATIVE) \
   || defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_NEVER_NATIVE))
#     error "Define only one of PDK_ATOMIC_INT_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#elif defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_SOMTIMES_NATIVE)
   ASSERT_TRUE(AtomicInt::isTestAndSetNative() || !AtomicInt::isTestAndSetNative());
#  if (defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE) \
   || defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_NEVER_NATIVE))
#     error "Define only one of PDK_ATOMIC_INT_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#elif defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_NEVER_NATIVE)
   ASSERT_TRUE(!AtomicInt::isTestAndSetNative());
#  if (defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE) \
   || defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_SOMTIMES_NATIVE))
#     error "Define only one of PDK_ATOMIC_INT_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#else
#  error "PDK_ATOMIC_INT_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE is not defined"
#endif
}

TEST(AtomicIntTest, testAtomicIsTestAndSetWaitFree)
{
#if defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_WAIT_FREE)
   ASSERT_TRUE(AtomicInt::isTestAndSetWaitFree());
   ASSERT_TRUE(AtomicInt::isTestAndSetNative());
#  if defined(PDK_ATOMIC_INT_TEST_AND_SET_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
   ASSERT_TRUE(!AtomicInt::isTestAndSetWaitFree());
#endif
}

TEST(AtomicIntTest, testTestAndSet)
{
   std::list<std::tuple<int, int, int, bool>> data;
   data.push_back(std::make_tuple(0, 0, 0, true));
   data.push_back(std::make_tuple(0, 0, 1, true));
   data.push_back(std::make_tuple(0, 0, -1, true));
   data.push_back(std::make_tuple(1, 1, 0, true));
   data.push_back(std::make_tuple(1, 1, 1, true));
   data.push_back(std::make_tuple(1, 1, -1, true));
   data.push_back(std::make_tuple(-1, -1, 0, true));
   data.push_back(std::make_tuple(-1, -1, 1, true));
   data.push_back(std::make_tuple(-1, -1, -1, true));
   data.push_back(std::make_tuple(INT_MIN + 1, INT_MIN + 1, INT_MIN + 1, true));
   data.push_back(std::make_tuple(INT_MIN + 1, INT_MIN + 1, 1, true));
   data.push_back(std::make_tuple(INT_MIN + 1, INT_MIN + 1, -1, true));
   data.push_back(std::make_tuple(INT_MAX, INT_MAX, INT_MAX, true));
   data.push_back(std::make_tuple(INT_MAX, INT_MAX, 1, true));
   data.push_back(std::make_tuple(INT_MAX, INT_MAX, -1, true));
   
   data.push_back(std::make_tuple(0, 1, ~0, false));
   data.push_back(std::make_tuple(0, -1, ~0, false));
   data.push_back(std::make_tuple(1, 0, ~0, false));
   data.push_back(std::make_tuple(-1, 0, ~0, false));
   data.push_back(std::make_tuple(1, -1, ~0, false));
   data.push_back(std::make_tuple(-1, 1, ~0, false));
   data.push_back(std::make_tuple(INT_MIN + 1, INT_MAX, ~0, false));
   data.push_back(std::make_tuple(INT_MAX, INT_MIN + 1, ~0, false));
   
   std::list<std::tuple<int, int, int, bool>>::iterator begin = data.begin();
   std::list<std::tuple<int, int, int, bool>>::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      int value = std::get<0>(item);
      int expected = std::get<1>(item);
      int newValue = std::get<2>(item);
      int result = std::get<3>(item);
      {
         AtomicInt atomic = value;
         ASSERT_EQ(atomic.testAndSetRelaxed(expected, newValue), result);
      }
      {
         AtomicInt atomic = value;
         ASSERT_EQ(atomic.testAndSetAcquire(expected, newValue), result);
      }
      {
         AtomicInt atomic = value;
         ASSERT_EQ(atomic.testAndSetRelease(expected, newValue), result);
      }
      {
         AtomicInt atomic = value;
         ASSERT_EQ(atomic.testAndSetOrdered(expected, newValue), result);
      }
#ifdef PDK_ATOMIC_INT32_IS_SUPPORTED
      {
         AtomicInt atomic = value;
         int currentval = 0xdeadbeef;
         ASSERT_EQ(atomic.testAndSetRelaxed(expected, newValue, currentval), result);
         if (!result) {
            ASSERT_EQ(currentval, value);
         }
      }
      
      {
         AtomicInt atomic = value;
         int currentval = 0xdeadbeef;
         ASSERT_EQ(atomic.testAndSetAcquire(expected, newValue, currentval), result);
         if (!result) {
            ASSERT_EQ(currentval, value);
         }
      }
      
      {
         AtomicInt atomic = value;
         int currentval = 0xdeadbeef;
         ASSERT_EQ(atomic.testAndSetRelease(expected, newValue, currentval), result);
         if (!result) {
            ASSERT_EQ(currentval, value);
         }
      }
      
      {
         AtomicInt atomic = value;
         int currentval = 0xdeadbeef;
         ASSERT_EQ(atomic.testAndSetOrdered(expected, newValue, currentval), result);
         if (!result) {
            ASSERT_EQ(currentval, value);
         }
      }
#endif
      ++begin;
   }
}

TEST(AtomicIntTest, testIsFetchAndStoreNative)
{
#if defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE)
   ASSERT_TRUE(AtomicInt::isFetchAndStoreNative());
#  if (defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_SOMETIMES_NATIVE) \
   || defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_NEVER_NATIVE))
#     error "Define only one of PDK_ATOMIC_INT_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#elif defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_SOMTIMES_NATIVE)
   ASSERT_TRUE(AtomicInt::isFetchAndStoreNative() || !AtomicInt::isFetchAndStoreNative());
#  if (defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE) \
   || defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_NEVER_NATIVE))
#     error "Define only one of PDK_ATOMIC_INT_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#elif defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_NEVER_NATIVE)
   ASSERT_TRUE(!AtomicInt::isFetchAndStoreNative());
#  if (defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE) \
   || defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_SOMTIMES_NATIVE))
#     error "Define only one of PDK_ATOMIC_INT_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#else
#  error "PDK_ATOMIC_INT_FETCH_AND_STORE_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE is not defined"
#endif
}

TEST(AtomicIntTest, testAtomicIsFetchAndStoreWaitFree)
{
#if defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_WAIT_FREE)
   ASSERT_TRUE(AtomicInt::isFetchAndStoreWaitFree());
   ASSERT_TRUE(AtomicInt::isFetchAndStoreNative());
#  if defined(PDK_ATOMIC_INT_FETCH_AND_STORE_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
   ASSERT_TRUE(!AtomicInt::isFetchAndStoreWaitFree());
#endif
}

TEST(AtomicIntTest, testFetchAndStore)
{
   std::list<std::tuple<int, int>> data;
   data.push_back(std::make_tuple(0, 1));
   data.push_back(std::make_tuple(1, 2));
   data.push_back(std::make_tuple(3, 8));
   std::list<std::tuple<int, int>>::iterator begin = data.begin();
   std::list<std::tuple<int, int>>::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      int value = std::get<0>(item);
      int newValue = std::get<1>(item);
      {
         AtomicInt atomic = value;
         ASSERT_EQ(atomic.fetchAndStoreRelaxed(newValue), value);
         ASSERT_EQ(atomic.load(), newValue);
      }
      
      {
         AtomicInt atomic = value;
         ASSERT_EQ(atomic.fetchAndStoreAcquire(newValue), value);
         ASSERT_EQ(atomic.load(), newValue);
      }
      
      {
         AtomicInt atomic = value;
         ASSERT_EQ(atomic.fetchAndStoreRelease(newValue), value);
         ASSERT_EQ(atomic.load(), newValue);
      }
      
      {
         AtomicInt atomic = value;
         ASSERT_EQ(atomic.fetchAndStoreOrdered(newValue), value);
         ASSERT_EQ(atomic.load(), newValue);
      }
      
      ++begin;
   }
}


TEST(AtomicIntTest, testIsFetchAndAddNative)
{
#if defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE)
   ASSERT_TRUE(AtomicInt::isFetchAndAddNative());
#  if (defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_SOMETIMES_NATIVE) \
   || defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_NEVER_NATIVE))
#     error "Define only one of PDK_ATOMIC_INT_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#elif defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_SOMTIMES_NATIVE)
   ASSERT_TRUE(AtomicInt::isFetchAndAddNative() || !AtomicInt::isFetchAndAddNative());
#  if (defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE) \
   || defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_NEVER_NATIVE))
#     error "Define only one of PDK_ATOMIC_INT_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#elif defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_NEVER_NATIVE)
   ASSERT_TRUE(!AtomicInt::isFetchAndAddNative());
#  if (defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE) \
   || defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_SOMTIMES_NATIVE))
#     error "Define only one of PDK_ATOMIC_INT_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#else
#  error "PDK_ATOMIC_INT_FETCH_AND_ADD_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE is not defined"
#endif
}

TEST(AtomicIntTest, testAtomicIsFetchAndAddWaitFree)
{
#if defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_WAIT_FREE)
   ASSERT_TRUE(AtomicInt::isFetchAndAddWaitFree());
   ASSERT_TRUE(AtomicInt::isFetchAndAddNative());
#  if defined(PDK_ATOMIC_INT_FETCH_AND_ADD_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
   ASSERT_TRUE(!AtomicInt::isFetchAndAddWaitFree());
#endif
}

TEST(AtomicIntTest, testFetchAndAdd)
{
   std::list<std::tuple<int, int>> data;
   data.push_back(std::make_tuple(0, 1));
   data.push_back(std::make_tuple(1, 0));
   data.push_back(std::make_tuple(2, 1));
   data.push_back(std::make_tuple(10, 21));
   data.push_back(std::make_tuple(31, 40));
   data.push_back(std::make_tuple(51, 62));
   data.push_back(std::make_tuple(72, 81));
   data.push_back(std::make_tuple(810, 721));
   data.push_back(std::make_tuple(631, 540));
   data.push_back(std::make_tuple(451, 362));
   data.push_back(std::make_tuple(272, 181));
   data.push_back(std::make_tuple(1810, 8721));
   data.push_back(std::make_tuple(3631, 6540));
   data.push_back(std::make_tuple(5451, 4362));
   data.push_back(std::make_tuple(7272, 2181));
   
   data.push_back(std::make_tuple(0, -1));
   data.push_back(std::make_tuple(1, 0));
   data.push_back(std::make_tuple(1, -2));
   data.push_back(std::make_tuple(2, -1));
   data.push_back(std::make_tuple(10, -21));
   data.push_back(std::make_tuple(31, -40));
   data.push_back(std::make_tuple(51, -62));
   data.push_back(std::make_tuple(72, -81));
   data.push_back(std::make_tuple(810, -721));
   data.push_back(std::make_tuple(631, -540));
   data.push_back(std::make_tuple(451, -362));
   data.push_back(std::make_tuple(272, -181));
   data.push_back(std::make_tuple(1810, -8721));
   data.push_back(std::make_tuple(3631, -6540));
   data.push_back(std::make_tuple(5451, -4362));
   data.push_back(std::make_tuple(7272, -2181));
   
   data.push_back(std::make_tuple(0, 1));
   data.push_back(std::make_tuple(-1, 0));
   data.push_back(std::make_tuple(-1, 2));
   data.push_back(std::make_tuple(-2, 1));
   data.push_back(std::make_tuple(-10, 21));
   data.push_back(std::make_tuple(-31, 40));
   data.push_back(std::make_tuple(-51, 62));
   data.push_back(std::make_tuple(-72, 81));
   data.push_back(std::make_tuple(-810, 721));
   data.push_back(std::make_tuple(-631, 540));
   data.push_back(std::make_tuple(-451, 362));
   data.push_back(std::make_tuple(-272, 181));
   data.push_back(std::make_tuple(-1810, 8721));
   data.push_back(std::make_tuple(-3631, 6540));
   data.push_back(std::make_tuple(-5451, 4362));
   data.push_back(std::make_tuple(-7272, 2181));
   
   std::list<std::tuple<int, int>>::iterator begin = data.begin();
   std::list<std::tuple<int, int>>::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      int value1 = std::get<0>(item);
      int value2 = std::get<1>(item);
      int result;
      {
         AtomicInt atomic = value1;
         result = atomic.fetchAndAddRelaxed(value2);
         ASSERT_EQ(result, value1);
         ASSERT_EQ(atomic.load(), value1 + value2);
      }
      
      {
         AtomicInt atomic = value1;
         result = atomic.fetchAndAddAcquire(value2);
         ASSERT_EQ(result, value1);
         ASSERT_EQ(atomic.load(), value1 + value2);
      }
      
      {
         AtomicInt atomic = value1;
         result = atomic.fetchAndAddRelease(value2);
         ASSERT_EQ(result, value1);
         ASSERT_EQ(atomic.load(), value1 + value2);
      }
      
      {
         AtomicInt atomic = value1;
         result = atomic.fetchAndAddOrdered(value2);
         ASSERT_EQ(result, value1);
         ASSERT_EQ(atomic.load(), value1 + value2);
      }
      
      ++begin;
   }
}

TEST(AtomicIntTest, testOperators)
{
   {
      BasicAtomicInt atomic = PDK_BASIC_ATOMIC_INITIALIZER(0);
      atomic = 1;
      ASSERT_EQ(static_cast<int>(atomic), 1);
   }
   AtomicInt atomic = 0;
   int x = ++atomic;
   ASSERT_EQ(static_cast<int>(atomic), x);
   ASSERT_EQ(static_cast<int>(atomic), 1);
   
   x = atomic++;
   ASSERT_EQ(static_cast<int>(atomic), x + 1);
   ASSERT_EQ(static_cast<int>(atomic), 2);
   
   x = atomic--;
   ASSERT_EQ(static_cast<int>(atomic), x - 1);
   ASSERT_EQ(static_cast<int>(atomic), 1);
   
   x = --atomic;
   ASSERT_EQ(static_cast<int>(atomic), x);
   ASSERT_EQ(static_cast<int>(atomic), 0);
   
   x = (atomic += 1);
   ASSERT_EQ(static_cast<int>(atomic), x);
   ASSERT_EQ(static_cast<int>(atomic), 1);
   
   x = (atomic -= 1);
   ASSERT_EQ(static_cast<int>(atomic), x);
   ASSERT_EQ(static_cast<int>(atomic), 0);
   
   x = (atomic |= 0xf);
   ASSERT_EQ(static_cast<int>(atomic), x);
   ASSERT_EQ(static_cast<int>(atomic), 0xf);
   
   x = (atomic &= 0x17);
   ASSERT_EQ(static_cast<int>(atomic), x);
   ASSERT_EQ(static_cast<int>(atomic), 7);
   
   x = (atomic ^= 0x14);
   ASSERT_EQ(static_cast<int>(atomic), x);
   ASSERT_EQ(static_cast<int>(atomic), 0x13);
}


