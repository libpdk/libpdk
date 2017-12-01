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
// Created by softboy on 2017/12/01.

#include <limits.h>
#include <cstdio>
#include <list>
#include <utility>
#include <tuple>

#include "gtest/gtest.h"
#include "pdk/base/os/thread/Atomic.h"

using pdk::os::thread::AtomicPointer;
using pdk::os::thread::BasicAtomicPointer;

namespace
{

struct WarnFreeHandler
{
   void bar() {}
};

void warning_free_helper()
{
   
}

}

TEST(AtomicPointerTest, testWarningFreeHandler)
{
   BasicAtomicPointer<WarnFreeHandler> p = PDK_BASIC_ATOMIC_INITIALIZER(0);
   
   p.load()->bar();
   WarnFreeHandler *expectedValue = 0;
   WarnFreeHandler *newValue = 0;
   pdk::ptrdiff valueToAdd = 0;
   
   p.testAndSetRelaxed(expectedValue, newValue);
   p.testAndSetAcquire(expectedValue, newValue);
   p.testAndSetRelease(expectedValue, newValue);
   p.testAndSetOrdered(expectedValue, newValue);
   
   p.fetchAndStoreRelaxed(newValue);
   p.fetchAndStoreAcquire(newValue);
   p.fetchAndStoreRelease(newValue);
   p.fetchAndStoreOrdered(newValue);
   
   p.fetchAndAddRelaxed(valueToAdd);
   p.fetchAndAddAcquire(valueToAdd);
   p.fetchAndAddRelease(valueToAdd);
   p.fetchAndAddOrdered(valueToAdd);
}

TEST(AtomicPointerTest, testWarningFree)
{
   void (*foo)() = &warning_free_helper;
   (void)foo;
}

TEST(AtomicPointerTest, testAlignment)
{
   char dummy[alignof(BasicAtomicPointer<void>) == alignof(void *) ? 1 : -1];
   (void)dummy;
}

TEST(AtomicPointerTest, testConstructor)
{
   void *oneLevel = this;
   AtomicPointer<void> atomic1 = oneLevel;
   ASSERT_EQ(atomic1.load(), oneLevel);
   
   void *twoLevel = &oneLevel;
   AtomicPointer<void> atomic2 = twoLevel;
   ASSERT_EQ(atomic2.load(), twoLevel);
   
   void *threeLevel = &twoLevel;
   AtomicPointer<void> atomic3 = threeLevel;
   ASSERT_EQ(atomic3.load(), threeLevel);
}

TEST(AtomicPointerTest, testCopyConstructor)
{
   void *oneLevel = this;
   AtomicPointer<void> atomic1 = oneLevel;
   AtomicPointer<void> atomic1Copy = atomic1;
   ASSERT_EQ(atomic1.load(), oneLevel);
   ASSERT_EQ(atomic1.load(), atomic1Copy.load());
   
   void *twoLevel = &oneLevel;
   AtomicPointer<void> atomic2 = twoLevel;
   AtomicPointer<void> atomic2Copy = atomic2;
   ASSERT_EQ(atomic2.load(), twoLevel);
   ASSERT_EQ(atomic2.load(), atomic2Copy.load());
   
   void *threeLevel = &twoLevel;
   AtomicPointer<void> atomic3 = threeLevel;
   AtomicPointer<void> atomic3Copy = atomic3;
   ASSERT_EQ(atomic3.load(), threeLevel);
   ASSERT_EQ(atomic3.load(), atomic3Copy.load());
}

TEST(AtomicPointerTest, testAssignmentOperator)
{
   void *oneLevel = this;
   void *twoLevel = &oneLevel;
   void *threeLevel = &twoLevel;
   
   AtomicPointer<void> atomic1 = oneLevel;
   AtomicPointer<void> atomic2 = twoLevel;
   AtomicPointer<void> atomic3 = threeLevel;
   
   ASSERT_EQ(atomic1.load(), oneLevel);
   ASSERT_EQ(atomic2.load(), twoLevel);
   ASSERT_EQ(atomic3.load(), threeLevel);
   
   atomic1 = twoLevel;
   atomic2 = threeLevel;
   atomic3 = oneLevel;
   
   ASSERT_EQ(atomic1.load(), twoLevel);
   ASSERT_EQ(atomic2.load(), threeLevel);
   ASSERT_EQ(atomic3.load(), oneLevel);
}

TEST(AtomicPointerTest, testIsTestAndSetNative)
{
#if defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE)
   ASSERT_TRUE(AtomicPointer<void>::isTestAndSetNative());
#  if (defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE)     \
   || defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_NEVER_NATIVE))
#    error "Define only one of PDK_ATOMIC_POINTER_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#elif defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE)
   ASSERT_TRUE(AtomicPointer<void>::isTestAndSetNative() || !AtomicPointer<void>::isTestAndSetNative());
#  if (defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE)     \
   || defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_NEVER_NATIVE))
#    error "Define only one of PDK_ATOMIC_POINTER_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#elif defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_NEVER_NATIVE)
   ASSERT_TRUE(!AtomicPointer<void>::isTestAndSetNative());
#  if (defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE)     \
   || defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_SOMTIMES_NATIVE))
#    error "Define only one of PDK_ATOMIC_POINTER_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE"
#  endif
#else
#  error "PDK_ATOMIC_POINTER_TEST_AND_SET_IS_{ALWAYS,SOMTIMES,NEVER}_NATIVE is not defined"
#endif
}

TEST(AtomicPointerTest, testIsTestAndSetWaitFree)
{
#if defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_WAIT_FREE)
   // the runtime test should say the same thing
   ASSERT_EQ(AtomicPointer<void>::isTestAndSetWaitFree());
   ASSERT_EQ(AtomicPointer<void>::isTestAndSetNative());
#  if defined(PDK_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE)
#    error "Reference counting cannot be wait-free and unsupported at the same time!"
#  endif
#else
   ASSERT_TRUE(!AtomicPointer<void>::isTestAndSetWaitFree());
#endif
}

TEST(AtomicPointerTest, testTestAndSet)
{
   void *oneLevel = this;
   void *twoLevel = &oneLevel;
   void *threeLevel = &twoLevel;
   {
      AtomicPointer<void> atomic1 = oneLevel;
      AtomicPointer<void> atomic2 = twoLevel;
      AtomicPointer<void> atomic3 = threeLevel;
      
      ASSERT_EQ(atomic1.load(), oneLevel);
      ASSERT_EQ(atomic2.load(), twoLevel);
      ASSERT_EQ(atomic3.load(), threeLevel);
      
      ASSERT_TRUE(atomic1.testAndSetRelaxed(oneLevel, twoLevel));
      ASSERT_TRUE(atomic2.testAndSetRelaxed(twoLevel, threeLevel));
      ASSERT_TRUE(atomic3.testAndSetRelaxed(threeLevel, oneLevel));
      
      ASSERT_EQ(atomic1.load(), twoLevel);
      ASSERT_EQ(atomic2.load(), threeLevel);
      ASSERT_EQ(atomic3.load(), oneLevel);
   }
   
   {
      AtomicPointer<void> atomic1 = oneLevel;
      AtomicPointer<void> atomic2 = twoLevel;
      AtomicPointer<void> atomic3 = threeLevel;
      
      ASSERT_EQ(atomic1.load(), oneLevel);
      ASSERT_EQ(atomic2.load(), twoLevel);
      ASSERT_EQ(atomic3.load(), threeLevel);
      
      ASSERT_TRUE(atomic1.testAndSetAcquire(oneLevel, twoLevel));
      ASSERT_TRUE(atomic2.testAndSetAcquire(twoLevel, threeLevel));
      ASSERT_TRUE(atomic3.testAndSetAcquire(threeLevel, oneLevel));
      
      ASSERT_EQ(atomic1.load(), twoLevel);
      ASSERT_EQ(atomic2.load(), threeLevel);
      ASSERT_EQ(atomic3.load(), oneLevel);
   }
   
   {
      AtomicPointer<void> atomic1 = oneLevel;
      AtomicPointer<void> atomic2 = twoLevel;
      AtomicPointer<void> atomic3 = threeLevel;
      
      ASSERT_EQ(atomic1.load(), oneLevel);
      ASSERT_EQ(atomic2.load(), twoLevel);
      ASSERT_EQ(atomic3.load(), threeLevel);
      
      ASSERT_TRUE(atomic1.testAndSetRelease(oneLevel, twoLevel));
      ASSERT_TRUE(atomic2.testAndSetRelease(twoLevel, threeLevel));
      ASSERT_TRUE(atomic3.testAndSetRelease(threeLevel, oneLevel));
      
      ASSERT_EQ(atomic1.load(), twoLevel);
      ASSERT_EQ(atomic2.load(), threeLevel);
      ASSERT_EQ(atomic3.load(), oneLevel);
   }
   
   {
      AtomicPointer<void> atomic1 = oneLevel;
      AtomicPointer<void> atomic2 = twoLevel;
      AtomicPointer<void> atomic3 = threeLevel;
      
      ASSERT_EQ(atomic1.load(), oneLevel);
      ASSERT_EQ(atomic2.load(), twoLevel);
      ASSERT_EQ(atomic3.load(), threeLevel);
      
      ASSERT_TRUE(atomic1.testAndSetOrdered(oneLevel, twoLevel));
      ASSERT_TRUE(atomic2.testAndSetOrdered(twoLevel, threeLevel));
      ASSERT_TRUE(atomic3.testAndSetOrdered(threeLevel, oneLevel));
      
      ASSERT_EQ(atomic1.load(), twoLevel);
      ASSERT_EQ(atomic2.load(), threeLevel);
      ASSERT_EQ(atomic3.load(), oneLevel);
   }
}
