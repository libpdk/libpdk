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






