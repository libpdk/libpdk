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
// Created by softboy on 2018/03/27.


#include "gtest/gtest.h"
#include "pdk/base/os/thread/Runnable.h"
#include "pdk/base/os/thread/ThreadPool.h"
#include "pdk/base/os/thread/Semaphore.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/lang/String.h"
#include "pdktest/PdkTest.h"
#include <mutex>
#include <iostream>

using FunctionPointer =  void (*)();
using pdk::os::thread::Runnable;
using pdk::os::thread::ThreadPool;
using pdk::os::thread::AtomicInt;
using pdk::os::thread::Semaphore;
using pdk::os::thread::Thread;

class FunctionPointerTask : public Runnable
{
public:
   FunctionPointerTask(FunctionPointer function)
      :function(function)
   {}
   
   void run()
   {
      function();
   }
private:
   FunctionPointer function;
};

Runnable *create_task(FunctionPointer pointer)
{
   return new FunctionPointerTask(pointer);
}

static std::mutex sg_funcTestMutex;

int testFunctionCount;

void sleep_test_function()
{
   pdktest::sleep(1000);
   ++testFunctionCount;
}

void empty_function()
{}

void no_sleep_test_function()
{
   ++testFunctionCount;
}

void sleep_test_function_mutex()
{
   pdktest::sleep(1000);
   sg_funcTestMutex.lock();
   ++testFunctionCount;
   sg_funcTestMutex.unlock();
}

void no_sleep_test_function_mutex()
{
   sg_funcTestMutex.lock();
   ++testFunctionCount;
   sg_funcTestMutex.unlock();
}

//TEST(ThreadPoolTest, testRunFunction)
//{
//   {
//      ThreadPool manager;
//      testFunctionCount = 0;
//      manager.start(create_task(no_sleep_test_function));
//   }
//   ASSERT_EQ(testFunctionCount, 1);
//}

//TEST(ThreadPoolTest, testCreateThreadRunFunction)
//{
//   {
//      ThreadPool manager;
//      testFunctionCount = 0;
//      manager.start(create_task(no_sleep_test_function));
//   }
//   ASSERT_EQ(testFunctionCount, 1);
//}

//TEST(ThreadPoolTest, testRunMultiple)
//{
//   const int runs = 10;
//   {
//      ThreadPool manager;
//      testFunctionCount = 0;
//      for (int i = 0; i < runs; ++i) {
//         manager.start(create_task(sleep_test_function_mutex));
//      }
//   }
//   ASSERT_EQ(testFunctionCount, runs);

//   {
//      ThreadPool manager;
//      testFunctionCount = 0;
//      for (int i = 0; i < runs; ++i) {
//         manager.start(create_task(sleep_test_function_mutex));
//      }
//   }
//   ASSERT_EQ(testFunctionCount, runs);
//   {
//      ThreadPool manager;
//      for (int i = 0; i < 500; ++i) {
//         manager.start(create_task(empty_function));
//      }
//   }
//}

//TEST(ThreadPoolTest, testWaitcomplete)
//{
//   testFunctionCount = 0;
//   const int runs = 500;
//   for (int i = 0; i < 500; ++i) {
//      ThreadPool pool;
//      pool.start(create_task(no_sleep_test_function));
//   }
//   ASSERT_EQ(testFunctionCount, runs);
//}

AtomicInt ran; // bool
class TestTask : public Runnable
{
public:
   void run()
   {
      ran.store(true);
   }
};

//TEST(ThreadPoolTest, testRunTask)
//{
//   ThreadPool manager;
//   ran.store(false);
//   manager.start(new TestTask());
//   PDK_TRY_VERIFY(ran.load());
//}

//TEST(ThreadPoolTest, testSingleton)
//{
//   ran.store(false);
//   ThreadPool::getGlobalInstance()->start(new TestTask());
//   PDK_TRY_VERIFY(ran.load());
//}

AtomicInt *value = nullptr;
class IntAccessor : public Runnable
{
public:
   void run()
   {
      for (int i = 0; i < 100; ++i) {
         value->ref();
         pdktest::sleep(1);
      }
   }
};

/*
    Test that the ThreadManager destructor waits until
    all threads have completed.
*/
//TEST(ThreadPoolTest, testDestruction)
//{
//   value = new AtomicInt;
//   ThreadPool *threadManager = new ThreadPool();
//   threadManager->start(new IntAccessor());
//   threadManager->start(new IntAccessor());
//   delete threadManager;
//   ASSERT_EQ(*value, 200);
//   delete value;
//   value = 0;
//}

Semaphore threadRecyclingSemaphore;
Thread *recycledThread = nullptr;

class ThreadRecorderTask : public Runnable
{
public:
   void run()
   {
      recycledThread = Thread::getCurrentThread();
      threadRecyclingSemaphore.release();
   }
};

/*
    Test that the thread pool really reuses threads.
*/
//TEST(ThreadPoopTest, testThreadRecycling)
//{
//   ThreadPool threadPool;
//   threadPool.start(new ThreadRecorderTask());
//   threadRecyclingSemaphore.acquire();
//   Thread *thread1 = recycledThread;
//   pdktest::sleep(100);
//   threadPool.start(new ThreadRecorderTask());
//   threadRecyclingSemaphore.acquire();
//   Thread *thread2 = recycledThread;
//   ASSERT_EQ(thread1, thread2);
//   pdktest::sleep(100);
//   threadPool.start(new ThreadRecorderTask());
//   threadRecyclingSemaphore.acquire();
//   Thread *thread3 = recycledThread;
//   ASSERT_EQ(thread2, thread3);
//}

class ExpiryTimeoutTask : public Runnable
{
public:
   Thread *m_thread;
   AtomicInt m_runCount;
   Semaphore m_semaphore;
   
   ExpiryTimeoutTask()
      : m_thread(nullptr),
        m_runCount(0)
   {
      setAutoDelete(false);
   }
   
   void run()
   {
      m_thread = Thread::getCurrentThread();
      m_runCount.ref();
      m_semaphore.release();
   }
};

//TEST(ThreadPoopTest, testExpiryTimeout)
//{
//   ExpiryTimeoutTask task;
   
//   ThreadPool threadPool;
//   threadPool.setMaxThreadCount(1);
   
//   int expiryTimeout = threadPool.getExpiryTimeout();
//   threadPool.setExpiryTimeout(1000);
//   ASSERT_EQ(threadPool.getExpiryTimeout(), 1000);
   
//   // run the task
//   threadPool.start(&task);
//   ASSERT_TRUE(task.m_semaphore.tryAcquire(1, 10000));
//   ASSERT_EQ(task.m_runCount.load(), 1);
//   ASSERT_TRUE(!task.m_thread->wait(100));
//   // thread should expire
//   Thread *firstThread = task.m_thread;
//   ASSERT_TRUE(task.m_thread->wait(10000));
   
//   // run task again, thread should be restarted
//   threadPool.start(&task);
//   ASSERT_TRUE(task.m_semaphore.tryAcquire(1, 10000));
//   ASSERT_EQ(task.m_runCount.load(), 2);
//   ASSERT_TRUE(!task.m_thread->wait(100));
//   // thread should expire again
//   ASSERT_TRUE(task.m_thread->wait(10000));
   
//   // thread pool should have reused the expired thread (instead of
//   // starting a new one)
//   ASSERT_EQ(firstThread, task.m_thread);
   
//   threadPool.setExpiryTimeout(expiryTimeout);
//   ASSERT_EQ(threadPool.getExpiryTimeout(), expiryTimeout);
//}


TEST(ThreadPoolTest, testExpiryTimeoutRace)
{
#ifdef PDK_OS_WIN
   SUCCEED("This test is unstable on Windows. See BUG-3786.");
#endif
   ExpiryTimeoutTask task;
   ThreadPool threadPool;
   threadPool.setMaxThreadCount(1);
   threadPool.setExpiryTimeout(50);
   const int numTasks = 20;
   for (int i = 0; i < numTasks; ++i) {
      threadPool.start(&task);
      Thread::msleep(50); // exactly the same as the expiry timeout
   }
   ASSERT_TRUE(task.m_semaphore.tryAcquire(numTasks, 10000));
   ASSERT_EQ(task.m_runCount.load(), numTasks);
   ASSERT_TRUE(threadPool.waitForDone(2000));
}

class ExceptionTask : public Runnable
{
public:
   void run()
   {
      throw new int;
   }
};

TEST(ThreadPoolTest, testExceptions)
{
   ExceptionTask task;
   {
      ThreadPool threadPool;
      //  Uncomment this for a nice crash.
      //        threadPool.start(&task);
   }
}
