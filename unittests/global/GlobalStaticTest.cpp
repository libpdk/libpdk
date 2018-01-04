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
// Created by softboy on 2018/01/04.

#include "gtest/gtest.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/base/os/thread/Atomic.h"
#include <list>
#include <utility>
#include <thread>

#if defined(PDK_OS_UNIX)
#include <sys/resource.h>
#endif

class GlobalStaticTest : public ::testing::Test {
protected:
   static void SetUpTestCase()
   {
#if defined(PDK_OS_UNIX)
      // The tests create a lot of threads, which require file descriptors. On systems like
      // OS X low defaults such as 256 as the limit for the number of simultaneously
      // open files is not sufficient.
      struct rlimit numFiles;
      if (getrlimit(RLIMIT_NOFILE, &numFiles) == 0 && numFiles.rlim_cur < 1024) {
         numFiles.rlim_cur = std::min(rlim_t(1024), numFiles.rlim_max);
         setrlimit(RLIMIT_NOFILE, &numFiles);
      }
#endif
   }
};

PDK_GLOBAL_STATIC_WITH_ARGS(const int, constInt, (42));
PDK_GLOBAL_STATIC_WITH_ARGS(volatile int, volatileInt, (-47));

void otherFunction()
{
   // never called
   constInt();
   volatileInt();
}

PDK_GLOBAL_STATIC(int, checkedBeforeInitialization);

TEST_F(GlobalStaticTest, testBeforeInitialization)
{
   ASSERT_FALSE(checkedBeforeInitialization.exists());
   ASSERT_FALSE(checkedBeforeInitialization.exists());
}

struct Type {
   int i;
};

PDK_GLOBAL_STATIC(Type, checkedAfterInitialization);

TEST_F(GlobalStaticTest, testApi)
{
   ASSERT_TRUE(static_cast<Type *>(checkedAfterInitialization));
   ASSERT_TRUE(checkedAfterInitialization());
   *checkedAfterInitialization = Type();
   *checkedAfterInitialization() = Type();
   
   checkedAfterInitialization()->i = 47;
   checkedAfterInitialization->i = 42;
   ASSERT_EQ(checkedAfterInitialization()->i, 42);
   ASSERT_EQ(checkedAfterInitialization->i, 42);
   checkedAfterInitialization()->i = 47;
   ASSERT_EQ(checkedAfterInitialization->i, 47);
   
   ASSERT_TRUE(checkedAfterInitialization.exists());
   ASSERT_TRUE(!checkedAfterInitialization.isDestroyed());
}

TEST_F(GlobalStaticTest, testConstVolatile)
{
   ASSERT_EQ(*constInt(), 42);
   ASSERT_EQ(static_cast<int>(*volatileInt()), -47);
   ASSERT_EQ(*constInt(), 42);
   ASSERT_EQ(static_cast<int>(*volatileInt()), -47);
}

using pdk::os::thread::BasicAtomicInt;

struct ThrowingType
{
   static BasicAtomicInt sm_constructedCount;
   static BasicAtomicInt sm_destructedCount;
   
   ThrowingType()
   {
      throw 0;
   }
   
   ThrowingType(BasicAtomicInt &throwControl)
   {
      sm_constructedCount.ref();
      if(throwControl.fetchAndAddRelaxed(-1) != 0) {
         throw 0;
      }
   }
   
   ~ThrowingType()
   {
      sm_destructedCount.ref();
   }
};

BasicAtomicInt ThrowingType::sm_constructedCount = PDK_BASIC_ATOMIC_INITIALIZER(0);
BasicAtomicInt ThrowingType::sm_destructedCount = PDK_BASIC_ATOMIC_INITIALIZER(0);

PDK_GLOBAL_STATIC(ThrowingType, throwingGS);

TEST_F(GlobalStaticTest, testException)
{
   bool exceptionCaught = false;
   try {
      throwingGS();
   } catch (int) {
      exceptionCaught = true;
   }
   ASSERT_TRUE(exceptionCaught);
   ASSERT_EQ(PDK_GS_throwingGS::guard.load(), 0);
   ASSERT_TRUE(!throwingGS.exists());
   ASSERT_TRUE(!throwingGS.isDestroyed());
}

BasicAtomicInt exceptionControlVar = PDK_BASIC_ATOMIC_INITIALIZER(1);
PDK_GLOBAL_STATIC_WITH_ARGS(ThrowingType, exceptionGS, (exceptionControlVar));

TEST_F(GlobalStaticTest, testCatchExceptionAndRetry)
{
   if (exceptionControlVar.load() != 1) {
      FAIL() << "This test cannot be run more than once";
   }
   ThrowingType::sm_constructedCount.store(0);
   ThrowingType::sm_destructedCount.store(0);
   bool exceptionCaught = false;
   try {
      exceptionGS();
   } catch (int) {
      exceptionCaught = true;
   }
   ASSERT_EQ(ThrowingType::sm_constructedCount.load(), 1);
   ASSERT_TRUE(exceptionCaught);
   exceptionGS();
   ASSERT_EQ(ThrowingType::sm_constructedCount.load(), 2);
}

BasicAtomicInt threadStressTestControlVar = PDK_BASIC_ATOMIC_INITIALIZER(5);
PDK_GLOBAL_STATIC_WITH_ARGS(ThrowingType, threadStressTestGS, (threadStressTestControlVar));

TEST_F(GlobalStaticTest, testThreadStressTest)
{
   ThrowingType::sm_constructedCount.store(0);
   ThrowingType::sm_destructedCount.store(0);
   int expectedConstructionCount = threadStressTestControlVar.load() + 1;
   if (expectedConstructionCount <= 0) {
      FAIL() << "This test cannot be run more than once";
   }
   const int numThreads = 200;
   
}
