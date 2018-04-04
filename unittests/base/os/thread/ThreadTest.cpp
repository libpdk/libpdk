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
// Created by softboy on 2017/03/13.

#include "gtest/gtest.h"

#ifdef PDK_OS_UNIX
#include <pthread.h>
#endif
#if defined(PDK_OS_WIN)
#include <windows.h>
#if defined(PDK_OS_WIN32)
#include <process.h>
#endif
#endif
#include "pdk/kernel/CallableInvoker.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"

#include "pdk/kernel/CoreApplication.h"
#include "pdktest/TestEventLoop.h"
#include "pdk/kernel/Timer.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/base/time/Time.h"
#include "pdk/kernel/Pointer.h"
#include "pdk/base/lang/String.h"
#include <mutex>
#include <tuple>
#include <condition_variable>
#include <iostream>

#ifndef PDK_NO_EXCEPTIONS
#include <exception>
#endif
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/Semaphore.h"
#include "pdk/stdext/typetraits/Sequence.h"
#include "pdk/stdext/typetraits/CallableInfoTrait.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdktest/TestSystem.h"

int sg_argc;
char **sg_argv;

enum { one_minute = 60 * 1000, five_minutes = 5 * one_minute };

using pdk::os::thread::Thread;
using pdk::kernel::EventLoop;
using pdk::kernel::Object;
using pdk::kernel::Timer;
using pdk::kernel::Pointer;
using pdk::kernel::CoreApplication;
using pdk::kernel::CallableInvoker;
using pdk::time::Time;
using pdktest::TestEventLoop;
using pdk::os::thread::Semaphore;
using pdk::kernel::ElapsedTimer;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::utils::ScopedPointer;
using pdk::os::thread::AtomicInt;

class CurrentThread : public Thread
{
public:
   pdk::HANDLE m_id;
   Thread *m_thread;
   
   void run()
   {
      m_id = Thread::getCurrentThreadId();
      m_thread = Thread::getCurrentThread();
   }
};

class SimpleThread : public Thread
{
public:
   std::mutex m_mutex;
   std::condition_variable m_cond;
   
   void run()
   {
      std::lock_guard locker(m_mutex);
      m_cond.notify_one();
   }
};

class ExitObject : public Object
{
public:
   Thread *m_thread;
   int m_code;
   void slot() 
   { 
      m_thread->exit(m_code);
   }
};

class ExitThread : public SimpleThread
{
public:
   ExitObject *m_object;
   int m_code;
   int m_result;
   
   void run()
   {
      SimpleThread::run();
      if (nullptr != m_object) {
         m_object->m_thread = this;
         m_object->m_code = m_code;
         Timer::singleShot(100, m_object, &ExitObject::slot);
      }
      m_result = exec();
   }
};

class TerminateThread : public SimpleThread
{
public:
   void run()
   {
      setTerminationEnabled(false);
      {
         std::unique_lock locker(m_mutex);
         m_cond.notify_one();
         m_cond.wait_for(locker, std::chrono::milliseconds(five_minutes));
      }
      setTerminationEnabled(true);
      FAIL() << "ThreadTest : test case hung";
   }
};

class QuitObject : public Object
{
public:
   Thread *m_thread;
public:
   void slot()
   {
      m_thread->quit();
   }
};

class QuitThread : public SimpleThread
{
public:
   QuitObject *m_object;
   int m_result;
   
   void run()
   {
      SimpleThread::run();
      if (nullptr != m_object) {
         m_object->m_thread = this;
         Timer::singleShot(100, [&](){
            m_object->slot();
         });
      }
      m_result = exec();
   }
};

class SleepThread : public SimpleThread
{
public:
   enum SleepType { Second, Millisecond, Microsecond };
   
   SleepType m_sleepType;
   int m_interval;
   int m_elapsed; // result, in *MILLISECONDS*
   void run()
   {
      std::lock_guard<std::mutex> locker(m_mutex);
      m_elapsed = 0;
      Time time;
      time.start();
      switch (m_sleepType) {
      case Second:
         sleep(m_interval);
         break;
      case Millisecond:
         msleep(m_interval);
         break;
      case Microsecond:
         usleep(m_interval);
         break;
      }
      m_elapsed = time.elapsed();
      m_cond.notify_one();
   }
};

TEST(ThreadTest, testCurrentThreadId)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   CurrentThread thread;
   thread.m_id = 0;
   thread.m_thread = nullptr;
   thread.start();
   EXPECT_TRUE(thread.wait(five_minutes));
   EXPECT_TRUE(thread.m_id != 0);
   EXPECT_TRUE(thread.m_id != Thread::getCurrentThreadId());
   pdktest::sleep(1);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testCurrentThread)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   EXPECT_TRUE(Thread::getCurrentThread() != nullptr);
   CurrentThread thread;
   thread.m_id = 0;
   thread.m_thread = nullptr;
   thread.start();
   EXPECT_TRUE(thread.wait(five_minutes));
   EXPECT_EQ(thread.m_thread, dynamic_cast<Thread *>(&thread));
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testIdealThreadCount)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   EXPECT_TRUE(Thread::getIdealThreadCount() > 0);
   std::clog << "Ideal thread count:" << Thread::getIdealThreadCount();
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testIsFinished)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   SimpleThread thread;
   EXPECT_TRUE(!thread.isFinished());
   std::unique_lock<std::mutex> locker(thread.m_mutex);
   thread.start();
   EXPECT_TRUE(!thread.isFinished());
   thread.m_cond.wait(locker);
   EXPECT_TRUE(thread.wait(five_minutes));
   EXPECT_TRUE(thread.isFinished());
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testIsRunning)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   SimpleThread thread;
   EXPECT_TRUE(!thread.isRunning());
   std::unique_lock<std::mutex> locker;
   thread.start();
   EXPECT_TRUE(thread.isRunning());
   EXPECT_TRUE(thread.wait(five_minutes));
   EXPECT_TRUE(!thread.isRunning());
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testSetPriority)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   SimpleThread thread;
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::IdlePriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::LowestPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::LowPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::NormalPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::HighPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::HighestPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::TimeCriticalPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);

   std::unique_lock<std::mutex> locker(thread.m_mutex);
   thread.start();

   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::IdlePriority);
   EXPECT_EQ(thread.getPriority(), Thread::IdlePriority);
   thread.setPriority(Thread::LowestPriority);
   EXPECT_EQ(thread.getPriority(), Thread::LowestPriority);
   thread.setPriority(Thread::LowPriority);
   EXPECT_EQ(thread.getPriority(), Thread::LowPriority);
   thread.setPriority(Thread::NormalPriority);
   EXPECT_EQ(thread.getPriority(), Thread::NormalPriority);
   thread.setPriority(Thread::HighPriority);
   EXPECT_EQ(thread.getPriority(), Thread::HighPriority);
   thread.setPriority(Thread::HighestPriority);
   EXPECT_EQ(thread.getPriority(), Thread::HighestPriority);
   thread.setPriority(Thread::TimeCriticalPriority);
   EXPECT_EQ(thread.getPriority(), Thread::TimeCriticalPriority);

   thread.m_cond.wait(locker);
   EXPECT_TRUE(thread.wait(five_minutes));

   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::IdlePriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::LowestPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::LowPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::NormalPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::HighPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::HighestPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::TimeCriticalPriority);
   EXPECT_EQ(thread.getPriority(), Thread::InheritPriority);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testSetStackSize)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   SimpleThread thread;
   EXPECT_EQ(thread.getStackSize(), 0u);
   thread.setStackSize(8192u);
   EXPECT_EQ(thread.getStackSize(), 8192u);
   thread.setStackSize(0u);
   EXPECT_EQ(thread.getStackSize(), 0u);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testExit)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ExitThread thread;
   thread.m_object = new ExitObject;
   thread.m_object->moveToThread(&thread);
   thread.m_code = 42;
   thread.m_result = 0;
   EXPECT_TRUE(!thread.isFinished());
   EXPECT_TRUE(!thread.isRunning());
   std::unique_lock<std::mutex> locker(thread.m_mutex);
   thread.start();

   EXPECT_TRUE(thread.isRunning());
   EXPECT_TRUE(!thread.isFinished());
   thread.m_cond.wait(locker);
   EXPECT_TRUE(thread.wait(five_minutes));
   EXPECT_TRUE(thread.isFinished());
   EXPECT_TRUE(!thread.isRunning());
   EXPECT_EQ(thread.m_result, thread.m_code);
   delete thread.m_object;

   ExitThread thread2;
   thread2.m_object = nullptr;
   thread2.m_code = 53;
   thread2.m_result = 0;
   std::unique_lock<std::mutex> locker2(thread2.m_mutex);
   thread2.start();
   thread2.exit(thread2.m_code);
   thread2.m_cond.wait(locker2);
   EXPECT_TRUE(thread2.wait(five_minutes));
   EXPECT_EQ(thread2.m_result, thread2.m_code);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testStart)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Thread::Priority priorities[] = {
      Thread::Priority::IdlePriority,
      Thread::Priority::LowestPriority,
      Thread::Priority::LowPriority,
      Thread::Priority::NormalPriority,
      Thread::Priority::HighPriority,
      Thread::Priority::HighestPriority,
      Thread::Priority::TimeCriticalPriority,
      Thread::Priority::InheritPriority
   };

   const int priorityCount = sizeof(priorities) / sizeof(Thread::Priority);
   for (int i = 0; i < priorityCount; ++i) {
      SimpleThread thread;
      EXPECT_TRUE(!thread.isFinished());
      EXPECT_TRUE(!thread.isRunning());
      std::unique_lock<std::mutex> locker(thread.m_mutex);
      thread.start(priorities[i]);
      EXPECT_TRUE(thread.isRunning());
      EXPECT_TRUE(!thread.isFinished());
      thread.m_cond.wait(locker);
      EXPECT_TRUE(thread.wait(five_minutes));
      EXPECT_TRUE(thread.isFinished());
      EXPECT_TRUE(!thread.isRunning());
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testTerminate)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   TerminateThread thread;
   {
      std::unique_lock<std::mutex> locker(thread.m_mutex);
      thread.start();
      EXPECT_TRUE(std::cv_status::no_timeout == thread.m_cond.wait_for(locker, std::chrono::milliseconds(five_minutes)));
      thread.terminate();
      thread.m_cond.notify_one();
   }
   EXPECT_TRUE(thread.wait(five_minutes));
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testQuit)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   QuitThread thread;
   thread.m_object = new QuitObject;
   thread.m_object->moveToThread(&thread);
   thread.m_result = -1;
   EXPECT_TRUE(!thread.isFinished());
   EXPECT_TRUE(!thread.isRunning());

   std::unique_lock<std::mutex> locker(thread.m_mutex);
   thread.start();
   EXPECT_TRUE(!thread.isFinished());
   EXPECT_TRUE(thread.isRunning());
   thread.m_cond.wait(locker);
   EXPECT_TRUE(thread.wait(five_minutes));
   EXPECT_TRUE(thread.isFinished());
   EXPECT_TRUE(!thread.isRunning());
   EXPECT_EQ(thread.m_result, 0);
   delete thread.m_object;

   QuitThread thread2;
   thread2.m_object = 0;
   thread2.m_result = -1;
   std::unique_lock<std::mutex> locker2(thread2.m_mutex);
   thread2.start();
   thread2.quit();
   thread2.m_cond.wait(locker2);
   EXPECT_TRUE(thread2.wait(five_minutes));
   EXPECT_EQ(thread2.m_result, 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testStarted)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   SimpleThread thread;
   bool signalCatched = false;
   thread.connectStartedSignal([&](){
      signalCatched = true;
   }, CoreApplication::getInstance(), pdk::ConnectionType::DirectConnection);
   thread.start();
   EXPECT_TRUE(thread.wait(five_minutes));
   EXPECT_TRUE(signalCatched);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testFinished)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   SimpleThread thread;
   bool signalCatched = false;
   thread.connectFinishedSignal([&](){
      signalCatched = true;
   }, &thread, pdk::ConnectionType::DirectConnection);
   thread.start();
   EXPECT_TRUE(thread.wait(five_minutes));
   EXPECT_TRUE(signalCatched);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testTerminated)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   TerminateThread thread;
   bool signalCatched = false;
   thread.connectFinishedSignal([&](){
      signalCatched = true;
   });
   {
      std::unique_lock<std::mutex> locker(thread.m_mutex);
      thread.start();
      thread.m_cond.wait(locker);
      thread.terminate();
      thread.m_cond.notify_one();
   }
   EXPECT_TRUE(thread.wait(five_minutes));
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testExec)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   class MultipleExecThread : public Thread
   {
   public:
      int m_res1;
      int m_res2;

      MultipleExecThread()
         : m_res1(-2),
           m_res2(-2)
      {}

      void run()
      {
         {
            ExitObject o;
            o.m_thread = this;
            o.m_code = 1;
            Timer::singleShot(100, [&]() {
               o.slot();
            });
            m_res1 = exec();
         }
         {
            ExitObject o;
            o.m_thread = this;
            o.m_code = 2;
            Timer::singleShot(100, [&]() {
               o.slot();
            });
            m_res2 = exec();
         }
      }
   };
   MultipleExecThread thread;
   thread.start();
   EXPECT_TRUE(thread.wait());
   EXPECT_EQ(thread.m_res1, 1);
   EXPECT_EQ(thread.m_res2, 2);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testSleep)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   SleepThread thread;
   thread.m_sleepType = SleepThread::Second;
   thread.m_interval = 2;
   thread.start();
   EXPECT_TRUE(thread.wait(five_minutes));
   EXPECT_TRUE(thread.m_elapsed >= 2000);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testMSleep)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   SleepThread thread;
   thread.m_sleepType = SleepThread::Millisecond;
   thread.m_interval = 120;
   thread.start();
   EXPECT_TRUE(thread.wait(five_minutes));
#if defined(PDK_OS_WIN)
   // Since the resolution of Time is so coarse...
   EXPECT_TRUE(thread.m_elapsed >= 100)
      #else
   EXPECT_TRUE(thread.m_elapsed >= 120);
#endif
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testUSleep)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   SleepThread thread;
   thread.m_sleepType = SleepThread::Microsecond;
   thread.m_interval = 120000;
   thread.start();
   EXPECT_TRUE(thread.wait(five_minutes));
#if defined(PDK_OS_WIN)
   // Since the resolution of Time is so coarse...
   EXPECT_TRUE(thread.m_elapsed >= 100)
      #else
   EXPECT_TRUE(thread.m_elapsed >= 120);
#endif
   PDKTEST_END_APP_CONTEXT();
}

using FuncPtr = void (*)(void *);
void noop(void*) {}

#if defined PDK_OS_UNIX
using ThreadHandle = pthread_t;
#elif defined PDK_OS_WIN
using ThreadHandle = HANDLE;
#endif

#ifdef PDK_OS_WIN
#define WIN_FIX_STDCALL __stdcall
#else
#define WIN_FIX_STDCALL
#endif

class NativeThreadWrapper
{
public:
   NativeThreadWrapper()
      : m_pdkthread(0),
        m_locker(m_mutex, std::defer_lock),
        m_waitForStop(false)
   {}
   
   void start(FuncPtr functionPointer = noop, void *data = nullptr);
   void startAndWait(FuncPtr functionPointer = noop, void *data = nullptr);
   void join();
   void setWaitForStop()
   {
      m_waitForStop = true;
   }
   
   void stop();
   
   ThreadHandle m_nativeThreadHandle;
   Thread *m_pdkthread;
   std::condition_variable m_startCondition;
   std::mutex m_mutex;
   std::unique_lock<std::mutex> m_locker;
   bool m_waitForStop;
   std::condition_variable m_stopCondition;
protected:
   static void *runUnix(void *data);
   static unsigned WIN_FIX_STDCALL runWin(void *data);
   
   FuncPtr m_functionPointer;
   void *m_data;
};

void NativeThreadWrapper::start(FuncPtr functionPointer, void *data)
{
   this->m_functionPointer = functionPointer;
   this->m_data = data;
#if defined PDK_OS_UNIX
   const int state = pthread_create(&m_nativeThreadHandle, 0, NativeThreadWrapper::runUnix, this);
   PDK_UNUSED(state);
#elif defined(PDK_OS_WINRT)
   nativeThreadHandle = CreateThread(NULL, 0 , (LPTHREAD_START_ROUTINE)NativeThreadWrapper::runWin , this, 0, NULL);
#elif defined PDK_OS_WIN
   unsigned thrdid = 0;
   m_nativeThreadHandle = (pdk::HANDLE) _beginthreadex(NULL, 0, NativeThreadWrapper::runWin, this, 0, &thrdid);
#endif
}

void NativeThreadWrapper::startAndWait(FuncPtr functionPointer, void *data)
{
   std::unique_lock<std::mutex> locker(m_mutex);
   start(functionPointer, data);
   m_startCondition.wait(locker);
}

void NativeThreadWrapper::join()
{
#if defined PDK_OS_UNIX
   pthread_join(m_nativeThreadHandle, 0);
#elif defined PDK_OS_WIN
   WaitForSingleObjectEx(m_nativeThreadHandle, INFINITE, FALSE);
   CloseHandle(nativeThreadHandle);
#endif
}

void *NativeThreadWrapper::runUnix(void *that)
{
   NativeThreadWrapper *nativeThreadWrapper = reinterpret_cast<NativeThreadWrapper*>(that);
   // Adopt thread, create Thread object.
   nativeThreadWrapper->m_pdkthread = Thread::getCurrentThread();
   // Release main thread.
   {
      std::lock_guard<std::mutex> lock(nativeThreadWrapper->m_mutex);
      nativeThreadWrapper->m_startCondition.notify_one();
   }
   // Run function.
   nativeThreadWrapper->m_functionPointer(nativeThreadWrapper->m_data);
   // Wait for stop.
   {
      std::unique_lock<std::mutex> lock(nativeThreadWrapper->m_mutex);
      if (nativeThreadWrapper->m_waitForStop) {
         nativeThreadWrapper->m_stopCondition.wait(lock);
      } 
   }
   return nullptr;
}

unsigned WIN_FIX_STDCALL NativeThreadWrapper::runWin(void *data)
{
   runUnix(data);
   return 0;
}

void NativeThreadWrapper::stop()
{
   std::lock_guard<std::mutex> lock(m_mutex);
   m_waitForStop = false;
   m_stopCondition.notify_one();
}

bool threadAdoptedOk = false;
Thread *mainThread;

void test_native_thread_adoption(void *)
{
   threadAdoptedOk = (Thread::getCurrentThreadId() != 0
         && Thread::getCurrentThread() != nullptr
         && Thread::getCurrentThread() != mainThread);
}

TEST(ThreadTest, testNativeThreadAdoption)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   threadAdoptedOk = false;
   mainThread = Thread::getCurrentThread();
   NativeThreadWrapper nativeThread;
   nativeThread.setWaitForStop();
   nativeThread.startAndWait(test_native_thread_adoption);
   EXPECT_TRUE(nativeThread.m_pdkthread);

   nativeThread.stop();
   nativeThread.join();

   EXPECT_TRUE(threadAdoptedOk);
   PDKTEST_END_APP_CONTEXT();
}

void adopted_thread_affinity_function(void *arg)
{
   Thread **affinity = reinterpret_cast<Thread **>(arg);
   Thread *current = Thread::getCurrentThread();
   affinity[0] = current;
   affinity[1] = current->getThread();
}

TEST(ThreadTest, testAdoptedThreadAffinity)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Thread *affinity[2] = { 0, 0 };
   NativeThreadWrapper thread;
   thread.startAndWait(adopted_thread_affinity_function, affinity);
   thread.join();
   // adopted thread should have affinity to itself
   EXPECT_EQ(affinity[0], affinity[1]);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testAdoptedThreadSetPriority)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   NativeThreadWrapper nativeThread;
   nativeThread.setWaitForStop();
   nativeThread.startAndWait();

   // change the priority of a running thread
   EXPECT_EQ(nativeThread.m_pdkthread->getPriority(), Thread::InheritPriority);
   nativeThread.m_pdkthread->setPriority(Thread::IdlePriority);
   EXPECT_EQ(nativeThread.m_pdkthread->getPriority(), Thread::IdlePriority);
   nativeThread.m_pdkthread->setPriority(Thread::LowestPriority);
   EXPECT_EQ(nativeThread.m_pdkthread->getPriority(), Thread::LowestPriority);
   nativeThread.m_pdkthread->setPriority(Thread::LowPriority);
   EXPECT_EQ(nativeThread.m_pdkthread->getPriority(), Thread::LowPriority);
   nativeThread.m_pdkthread->setPriority(Thread::NormalPriority);
   EXPECT_EQ(nativeThread.m_pdkthread->getPriority(), Thread::NormalPriority);
   nativeThread.m_pdkthread->setPriority(Thread::HighPriority);
   EXPECT_EQ(nativeThread.m_pdkthread->getPriority(), Thread::HighPriority);
   nativeThread.m_pdkthread->setPriority(Thread::HighestPriority);
   EXPECT_EQ(nativeThread.m_pdkthread->getPriority(), Thread::HighestPriority);
   nativeThread.m_pdkthread->setPriority(Thread::TimeCriticalPriority);
   EXPECT_EQ(nativeThread.m_pdkthread->getPriority(), Thread::TimeCriticalPriority);

   nativeThread.stop();
   nativeThread.join();
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testAdoptedThreadExit)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   NativeThreadWrapper nativeThread;
   nativeThread.setWaitForStop();

   nativeThread.startAndWait();
   EXPECT_TRUE(nativeThread.m_pdkthread);
   EXPECT_TRUE(nativeThread.m_pdkthread->isRunning());
   EXPECT_TRUE(!nativeThread.m_pdkthread->isFinished());

   nativeThread.stop();
   nativeThread.join();
   PDKTEST_END_APP_CONTEXT();
}

void adopted_thread_exec_function(void *)
{
   Thread  * const adoptedThread = Thread::getCurrentThread();
   EventLoop eventLoop(adoptedThread);
   const int code = 1;
   ExitObject o;
   o.m_thread = adoptedThread;
   o.m_code = code;
   Timer::singleShot(100, [&](){
      o.slot();
   });
   const int result = eventLoop.exec();
   EXPECT_EQ(result, code);
}

TEST(ThreadTest, testAdoptedThreadExec)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   NativeThreadWrapper nativeThread;
   nativeThread.start(adopted_thread_exec_function);
   nativeThread.join();
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testAdoptedThreadFinished)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   NativeThreadWrapper nativeThread;
   nativeThread.setWaitForStop();
   nativeThread.startAndWait();
   nativeThread.m_pdkthread->connectFinishedSignal(&TestEventLoop::instance(), &TestEventLoop::exitLoop, pdk::ConnectionType::DirectConnection);
   nativeThread.stop();
   nativeThread.join();
   TestEventLoop::instance().enterLoop(5);
   EXPECT_TRUE(!TestEventLoop::instance().getTimeout());
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testAdoptedThreadExecFinished)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   NativeThreadWrapper nativeThread;
   nativeThread.setWaitForStop();
   nativeThread.startAndWait(adopted_thread_exec_function);
   nativeThread.m_pdkthread->connectFinishedSignal(&TestEventLoop::instance(), &TestEventLoop::exitLoop, pdk::ConnectionType::DirectConnection);
   nativeThread.stop();
   nativeThread.join();
   TestEventLoop::instance().enterLoop(5);
   EXPECT_TRUE(!TestEventLoop::instance().getTimeout());
   PDKTEST_END_APP_CONTEXT();
}

class SignalRecorder : public Object
{
public:
   AtomicInt m_activationCount;
   inline SignalRecorder()
      : m_activationCount(0)
   {}
   
   bool wasActivated()
   {
      return m_activationCount.load() > 0;
   }
   
public:
   void slot();
};

void SignalRecorder::slot()
{
   m_activationCount.ref();
}

TEST(ThreadTest, testAdoptMultipleThreads)
{
   PDKTEST_BEGIN_APP_CONTEXT();
#if defined(PDK_OS_WIN)
   // need to test lots of threads, so that we exceed MAXIMUM_WAIT_OBJECTS in qt_adopted_thread_watcher()
   const int numThreads = 200;
#else
   const int numThreads = 5;
#endif
   std::vector<NativeThreadWrapper*> nativeThreads;
   SignalRecorder recorder;
   for (int i = 0; i < numThreads; ++i) {
      nativeThreads.push_back(new NativeThreadWrapper());
      nativeThreads.at(i)->setWaitForStop();
      nativeThreads.at(i)->startAndWait();
      nativeThreads.at(i)->m_pdkthread->connectFinishedSignal(&recorder, &SignalRecorder::slot);
   }
   nativeThreads.at(numThreads - 1)->m_pdkthread->connectFinishedSignal(&TestEventLoop::instance(), &TestEventLoop::exitLoop);
   for (int i = 0; i < numThreads; ++i) {
      nativeThreads.at(i)->stop();
      nativeThreads.at(i)->join();
      delete nativeThreads.at(i);
   }

   TestEventLoop::instance().enterLoop(5);
   EXPECT_TRUE(!TestEventLoop::instance().getTimeout());
   EXPECT_EQ(recorder.m_activationCount.load(), numThreads);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testAdoptMultipleThreadsOverlap)
{
   PDKTEST_BEGIN_APP_CONTEXT();
#if defined(PDK_OS_WIN)
   // need to test lots of threads, so that we exceed MAXIMUM_WAIT_OBJECTS in qt_adopted_thread_watcher()
   const int numThreads = 200;
#else
   const int numThreads = 5;
#endif
   std::vector<NativeThreadWrapper*> nativeThreads;

   SignalRecorder recorder;

   for (int i = 0; i < numThreads; ++i) {
      nativeThreads.push_back(new NativeThreadWrapper());
      nativeThreads.at(i)->setWaitForStop();
      nativeThreads.at(i)->m_locker.lock();
      nativeThreads.at(i)->start();
   }
   for (int i = 0; i < numThreads; ++i) {
      nativeThreads.at(i)->m_startCondition.wait(nativeThreads.at(i)->m_locker);
      nativeThreads.at(i)->m_pdkthread->connectFinishedSignal(&recorder, &SignalRecorder::slot);
      nativeThreads.at(i)->m_locker.unlock();
   }
   nativeThreads.at(numThreads - 1)->m_pdkthread->connectFinishedSignal(&TestEventLoop::instance(), &TestEventLoop::exitLoop);

   for (int i = 0; i < numThreads; ++i) {
      nativeThreads.at(i)->stop();
      nativeThreads.at(i)->join();
      delete nativeThreads.at(i);
   }
   TestEventLoop::instance().enterLoop(5);
   EXPECT_TRUE(!TestEventLoop::instance().getTimeout());
   EXPECT_EQ(recorder.m_activationCount.load(), numThreads);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testStress)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Time t;
   t.start();
   while (t.elapsed() < one_minute) {
      CurrentThread t;
      t.start();
      t.wait(one_minute);
   }
   PDKTEST_END_APP_CONTEXT();
}

class Syncronizer : public Object
{
public:
   PDK_DEFINE_SIGNAL_ENUMS(PropChanged);
   void setProp(int p) {
      if(m_prop != p) {
         m_prop = p;
         emitPropChangedSignal(std::move(p));
      }
   }
   ~Syncronizer()
   {

   }
   using PropChangedHandlerType = void(int);
   PDK_DEFINE_SIGNAL_BINDER(PropChanged)
   PDK_DEFINE_SIGNAL_EMITTER(PropChanged)

   public:
      Syncronizer() 
    : m_prop(42)
   {}

   int m_prop;
};

class CurrentThread1;

class CurrentThread1 : public Thread
{
public:
   pdk::HANDLE m_id;
   Thread *m_thread;

   void run()
   {
      exec();
   }

   void quit()
   {

      Thread::quit();
   }

   void propChanged(int val)
   {

   }
};

TEST(ThreadTest, testExitAndStart)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Thread thread;
   thread.exit(555); //should do nothing
   thread.start();
   Syncronizer sync1;
   sync1.moveToThread(&thread);
   Syncronizer sync2;
   sync2.moveToThread(&thread);
   sync2.connectPropChangedSignal(&sync1, &Syncronizer::setProp, pdk::ConnectionType::QueuedConnection);
   sync1.connectPropChangedSignal(&thread, &Thread::quit, pdk::ConnectionType::QueuedConnection);
   CallableInvoker::invokeAsync([&sync2](int value) {
      sync2.setProp(value);
   }, &sync2, 89);

   pdktest::wait(50);
   while(!thread.wait(10)) {
      pdktest::wait(50);
   }

   EXPECT_EQ(sync2.m_prop, 89);
   EXPECT_EQ(sync1.m_prop, 89);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testExitAndExec)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   class MyThread : public Thread {
   public:
      Semaphore m_sem1;
      Semaphore m_sem2;
      volatile int m_value;
      void run() {
         m_sem1.acquire();
         m_value = exec();  //First entrence
         m_sem2.release();
         m_value = exec(); // Second loop
      }
   };
   MyThread thread;
   thread.m_value = 0;
   thread.start();
   thread.exit(556);
   thread.m_sem1.release();
   thread.m_sem2.acquire();
   int v = thread.m_value;
   EXPECT_EQ(v, 556);
   Syncronizer sync1;
   sync1.moveToThread(&thread);
   Syncronizer sync2;
   sync2.moveToThread(&thread);
   sync2.connectPropChangedSignal(&sync1, &Syncronizer::setProp, pdk::ConnectionType::QueuedConnection);
   sync1.connectPropChangedSignal(&thread, &MyThread::quit, pdk::ConnectionType::QueuedConnection);
   CallableInvoker::invokeAsync([&sync2](int value) {
      sync2.setProp(value);
   }, &sync2, 89);

   pdktest::wait(50);
   while(!thread.wait(10)) {
      pdktest::wait(10);
   }
   EXPECT_EQ(sync2.m_prop, 89);
   EXPECT_EQ(sync1.m_prop, 89);
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testConnectThreadFinishedSignalToObjectDeleteLaterSlot)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Thread thread;
   Object *object = new Object;
   Pointer<Object> p = object;
   EXPECT_TRUE(!p.isNull());
   thread.connectStartedSignal(&thread, &Thread::quit, pdk::ConnectionType::DirectConnection);
   thread.connectFinishedSignal(object, &Object::deleteLater);
   object->moveToThread(&thread);
   thread.start();
   EXPECT_TRUE(thread.wait(30000));
   EXPECT_TRUE(p.isNull());
   PDKTEST_END_APP_CONTEXT();
}

class WaitingThread : public Thread
{
public:
   enum { WaitTime = 800 };
   std::mutex m_mutex;
   std::condition_variable m_cond1;
   std::condition_variable m_cond2;

   void run()
   {
      std::unique_lock locker(m_mutex);
      m_cond1.wait(locker);
      m_cond2.wait_for(locker, std::chrono::milliseconds(WaitTime));
   }
};

TEST(ThreadTest, testWait2)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   ElapsedTimer timer;
   WaitingThread thread;
   thread.start();
   timer.start();
   EXPECT_TRUE(!thread.wait(WaitingThread::WaitTime));
   pdk::pint64 elapsed = timer.getElapsed();
   EXPECT_TRUE(elapsed >= WaitingThread::WaitTime - 10) << pdk_printable(String::fromLatin1("elapsed: %1").arg(elapsed));
   timer.start();
   thread.m_cond1.notify_one();
   EXPECT_TRUE(thread.wait(/*WaitingThread::WaitTime * 1.4*/));
   elapsed = timer.getElapsed();
   EXPECT_TRUE(elapsed - WaitingThread::WaitTime >= -1) << pdk_printable(String::fromLatin1("elapsed: %1").arg(elapsed));
   PDKTEST_END_APP_CONTEXT();
}

class SlowSlotObject : public Object {
public:
   std::mutex m_mutex;
   std::condition_variable m_cond;
   void slowSlot(Thread::SignalType signal, Object *sender)
   {
      std::unique_lock locker(m_mutex);
      m_cond.wait(locker);
   }
};

TEST(ThreadTest, testWait3SlowDestructor)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   SlowSlotObject slow;
   Thread thread;
   thread.connectFinishedSignal(&slow, &SlowSlotObject::slowSlot, pdk::ConnectionType::DirectConnection);
   thread.connectFinishedSignal([](Thread::SignalType signal, Object *sender) {}, &slow, pdk::ConnectionType::DirectConnection);
   enum { WaitTime = 1800 };
   ElapsedTimer timer;
   thread.start();
   thread.quit();
   timer.start();
   EXPECT_TRUE(!thread.wait(WaitingThread::WaitTime));
   pdk::pint64 elapsed = timer.getElapsed();
   EXPECT_TRUE(elapsed >= WaitingThread::WaitTime - 1) << pdk_printable(String::fromLatin1("elapsed: %1").arg(elapsed));
   slow.m_cond.notify_one();
   //now the thread should finish quickly
   EXPECT_TRUE(thread.wait(one_minute));
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testDestroyFinishRace)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   class MyThread : public Thread { void run() {} };
   for (int i = 0; i < 15; i++) {
      MyThread *thread = new MyThread;
      thread->connectFinishedSignal(thread, &MyThread::deleteLater);
      Pointer<Thread> weak(static_cast<Thread *>(thread));
      thread->start();
      while (weak) {
         PDK_RETRIEVE_APP_INSTANCE()->processEvents();
         PDK_RETRIEVE_APP_INSTANCE()->processEvents();
         PDK_RETRIEVE_APP_INSTANCE()->processEvents();
         PDK_RETRIEVE_APP_INSTANCE()->processEvents();
      }
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testStartFinishRace)
{
   class MyThread : public Thread {
   public:
      MyThread() : m_i (50) {}
      void run() {
         m_i--;
         if (0 == m_i) {
            disconnectFinishedSignal(m_connection);
         }
      }
      void start()
      {
         Thread::start(Priority::InheritPriority);
      }
      int m_i;
      pdk::kernel::signal::Connection m_connection;
   };
   PDKTEST_BEGIN_APP_CONTEXT();
   for (int i = 0; i < 15; i++) {
      MyThread thread;
      thread.m_connection = thread.connectFinishedSignal(&thread, &MyThread::start, pdk::ConnectionType::QueuedConnection);
      thread.start();
      while (!thread.isFinished() || thread.m_i != 0) {
         PDK_RETRIEVE_APP_INSTANCE()->processEvents();
         PDK_RETRIEVE_APP_INSTANCE()->processEvents();
         PDK_RETRIEVE_APP_INSTANCE()->processEvents();
         PDK_RETRIEVE_APP_INSTANCE()->processEvents();
      }
      EXPECT_EQ(thread.m_i, 0);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testStartAndQuitCustomEventLoop)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   struct MyThread : Thread {
      void run() { EventLoop().exec(); }
   };

   for (int i = 0; i < 5; i++) {
      MyThread t;
      t.start();
      t.quit();
      t.wait();
   }
   PDKTEST_END_APP_CONTEXT();
}

class FinishedTestObject : public Object {
public:
   FinishedTestObject()
      : m_ok(false)
   {}
   bool m_ok;
public:
   void slotFinished(Thread::SignalType, Object *sender)
   {
      Thread *t = dynamic_cast<Thread *>(sender);
      m_ok = t != nullptr && t->isFinished() && !t->isRunning();
   }
};

TEST(ThreadTest, testIsRunningInFinished)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   for (int i = 0; i < 15; i++) {
      Thread thread;
      thread.start();
      FinishedTestObject localObject;
      FinishedTestObject inThreadObject;
      localObject.setObjectName(Latin1String("..."));
      inThreadObject.moveToThread(&thread);
      thread.connectFinishedSignal(&localObject, &FinishedTestObject::slotFinished);
      thread.connectFinishedSignal(&inThreadObject, &FinishedTestObject::slotFinished);
      EventLoop loop;
      thread.connectFinishedSignal(&loop, &EventLoop::quit);
      CallableInvoker::invokeAsync(&thread, &Thread::quit);
      loop.exec();
      EXPECT_TRUE(!thread.isRunning());
      EXPECT_TRUE(thread.isFinished());
      EXPECT_TRUE(localObject.m_ok);
      EXPECT_TRUE(inThreadObject.m_ok);
   }
   PDKTEST_END_APP_CONTEXT();
}

namespace pdk {
namespace kernel {
PDK_CORE_EXPORT uint global_posted_events_count();
} // kernel
} // pdk

using pdk::kernel::global_posted_events_count;

using pdk::kernel::AbstractEventDispatcher;
using pdk::kernel::SocketNotifier;
using pdk::os::thread::BasicAtomicInt;

class DummyEventDispatcher : public AbstractEventDispatcher
{
public:
   DummyEventDispatcher()
   {}

   bool processEvents(EventLoop::ProcessEventsFlags) override
   {
      m_visited.store(true);
      emitAwakeSignal();
      CoreApplication::sendPostedEvents();
      return false;
   }

   bool hasPendingEvents() override
   {
      return global_posted_events_count() != 0;
   }

   void registerSocketNotifier(SocketNotifier *) override
   {}

   void unregisterSocketNotifier(SocketNotifier *) override
   {}

   void registerTimer(int, int, pdk::TimerType, Object *) override
   {}

   bool unregisterTimer(int ) override
   {
      return false;
   }

   bool unregisterTimers(Object *) override
   {
      return false;
   }

   std::list<TimerInfo> getRegisteredTimers(Object *) const override
   {
      return std::list<TimerInfo>();
   }

   int getRemainingTime(int) override
   {
      return 0;
   }

   void wakeUp() override
   {}

   void interrupt() override
   {}

   void flush() override
   {}

   //#ifdef PDK_OS_WIN
   //    bool registerEventNotifier(WinEventNotifier *) { return false; }
   //    void unregisterEventNotifier(WinEventNotifier *) { }
   //#endif

   BasicAtomicInt m_visited; // bool
};

class ThreadObj : public Object
{
public:
   using VisitedHandlerType = void();
   PDK_DEFINE_SIGNAL_ENUMS(Visited);
   PDK_DEFINE_SIGNAL_EMITTER(Visited)
   PDK_DEFINE_SIGNAL_BINDER(Visited)

   void visit()
   {
      emitVisitedSignal();
   }
public:
   void visited();
};

TEST(ThreadTest, testCustomEventDispatcher)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Thread thread;
   // there should be no ED yet
   EXPECT_TRUE(!thread.getEventDispatcher());
   DummyEventDispatcher *ed = new DummyEventDispatcher;
   thread.setEventDispatcher(ed);
   // the new ED should be set
   EXPECT_EQ(thread.getEventDispatcher(), ed);
   // test the alternative API of AbstractEventDispatcher
   EXPECT_EQ(AbstractEventDispatcher::getInstance(&thread), ed);
   thread.start();
   // start() should not overwrite the ED
   EXPECT_EQ(thread.getEventDispatcher(), ed);

   ThreadObj obj;
   obj.moveToThread(&thread);
   // move was successful?
   EXPECT_EQ(obj.getThread(), &thread);
   EventLoop loop;
   obj.connectVisitedSignal(&loop, &EventLoop::quit, pdk::ConnectionType::QueuedConnection);
   CallableInvoker::invokeAsync([&obj]() {
      obj.visit();
   }, &obj);
   loop.exec();
   // test that the ED has really been used
   EXPECT_TRUE(ed->m_visited.load());

   Pointer<DummyEventDispatcher> weak_ed(ed);
   EXPECT_TRUE(!weak_ed.isNull());
   thread.quit();
   // wait for thread to be stopped
   EXPECT_TRUE(thread.wait(30000));
   // test that ED has been deleted
   EXPECT_TRUE(weak_ed.isNull());
   PDKTEST_END_APP_CONTEXT();
}

using pdk::kernel::EventLoopLocker;

class Job : public Object
{
public:
   Job(Thread *thread, int deleteDelay, bool *flag, Object *parent = nullptr)
      : Object(parent),
        m_quitLocker(thread),
        m_exitThreadCalled(*flag)
   {
      m_exitThreadCalled = false;
      moveToThread(thread);
      //      std::cout << "Job current thread " << getThread() << std::endl;
      Timer::singleShot(deleteDelay, this, &Job::deleteLater);
      Timer::singleShot(1000, this, &Job::exitThread);
   }
   void deleteLater()
   {
      //      std::cout << "deleteLater >> " << Thread::getCurrentThread() << std::endl;
      //      std::cout << "job >> " << this << std::endl;
      Object::deleteLater();
   }
   ~Job()
   {
      //      std::cout << "destroy" << std::endl;
   }

public:
   void exitThread()
   {
      //      std::cout << "exitThread" << std::endl;
      m_exitThreadCalled = true;
      getThread()->exit(1);
   }

private:
   EventLoopLocker m_quitLocker;
public:
   bool &m_exitThreadCalled;
};

class MyEventLoop : public EventLoop
{
public:
   void quit()
   {
      //      std::cout << "quit" << std::endl;
      exit(0);
   }
};

TEST(ThreadTest, testQuitLock)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Object *x = new Object;
   Thread thread;
   delete x;
   bool exitThreadCalled;
   MyEventLoop loop;
   thread.connectFinishedSignal(&loop, &MyEventLoop::quit);
   Job *job;
   thread.start();
   job = new Job(&thread, 500, &exitThreadCalled);
   EXPECT_EQ(job->getThread(), &thread);
   loop.exec();

   EXPECT_TRUE(!exitThreadCalled);
   thread.start();
   job = new Job(&thread, 1000, &exitThreadCalled);
   EXPECT_EQ(job->getThread(), &thread);
   loop.exec();
   EXPECT_TRUE(exitThreadCalled);
   PDKTEST_END_APP_CONTEXT();
}

class StopableJob : public Object
{
public:
   using FinishedHandlerType = void();
   PDK_DEFINE_SIGNAL_ENUMS(Finished);
   PDK_DEFINE_SIGNAL_EMITTER(Finished)
   PDK_DEFINE_SIGNAL_BINDER(Finished)
   StopableJob (Semaphore &sem)
      : m_sem(sem) {}
   Semaphore &m_sem;
public:
   void run()
   {
      m_sem.release();
      while (!getThread()->isInterruptionRequested()) {
         PDK_RETRIEVE_APP_INSTANCE()->getThread()->msleep(10);
      }
      m_sem.release();
      emitFinishedSignal();
   }
};

TEST(ThreadTest, testCreate)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   {
      const auto &function = [](){};
      ScopedPointer<Thread> thread(Thread::create(function));
      EXPECT_TRUE(thread);
      EXPECT_TRUE(!thread->isRunning());
      thread->start();
      EXPECT_TRUE(thread->wait());
   }
   {
      // no side effects before starting
      int i = 0;
      const auto &function = [&i]() { i = 42; };
      ScopedPointer<Thread> thread(Thread::create(function));
      EXPECT_TRUE(thread);
      EXPECT_TRUE(!thread->isRunning());
      EXPECT_EQ(i, 0);
      thread->start();
      EXPECT_TRUE(thread->wait());
      EXPECT_EQ(i, 42);
   }

   {
      // control thread progress
      Semaphore semaphore1;
      Semaphore semaphore2;
      const auto &function = [&semaphore1, &semaphore2]() -> void
      {
         semaphore1.acquire();
         semaphore2.release();
      };
      ScopedPointer<Thread> thread(Thread::create(function));
      EXPECT_TRUE(thread);
      thread->start();
      Thread::getCurrentThread()->msleep(5000);
      EXPECT_TRUE(thread->isRunning());
      semaphore1.release();
      semaphore2.acquire();
      EXPECT_TRUE(thread->wait());
      EXPECT_TRUE(!thread->isRunning());
   }

   {
      // ignore return values
      const auto &function = []() { return 42; };
      ScopedPointer<Thread> thread(Thread::create(function));
      EXPECT_TRUE(thread);
      EXPECT_TRUE(!thread->isRunning());
      thread->start();
      EXPECT_TRUE(thread->wait());
   }

   {
      // move-only parameters
      struct MoveOnlyValue
      {
         explicit MoveOnlyValue(int v) : v(v) {}
         ~MoveOnlyValue() = default;
         MoveOnlyValue(const MoveOnlyValue &) = delete;
         MoveOnlyValue(MoveOnlyValue &&) = default;
         MoveOnlyValue &operator=(const MoveOnlyValue &) = delete;
         MoveOnlyValue &operator=(MoveOnlyValue &&) = default;
         int v;
      };

      struct MoveOnlyFunctor {
         explicit MoveOnlyFunctor(int *i) : i(i) {}
         ~MoveOnlyFunctor() = default;
         MoveOnlyFunctor(const MoveOnlyFunctor &) = delete;
         MoveOnlyFunctor(MoveOnlyFunctor &&) = default;
         MoveOnlyFunctor &operator=(const MoveOnlyFunctor &) = delete;
         MoveOnlyFunctor &operator=(MoveOnlyFunctor &&) = default;
         int operator()() { return (*i = 42); }
         int *i;
      };

      {
         int i = 0;
         MoveOnlyFunctor f(&i);
         ScopedPointer<Thread> thread(Thread::create(std::move(f)));
         EXPECT_TRUE(thread);
         EXPECT_TRUE(!thread->isRunning());
         thread->start();
         EXPECT_TRUE(thread->wait());
         EXPECT_EQ(i, 42);
      }

      {
         int i = 0;
         MoveOnlyValue mo(123);
         auto moveOnlyFunction = [&i, mo = std::move(mo)]() { i = mo.v; };
         ScopedPointer<Thread> thread(Thread::create(std::move(moveOnlyFunction)));
         EXPECT_TRUE(thread);
         EXPECT_TRUE(!thread->isRunning());
         thread->start();
         EXPECT_TRUE(thread->wait());
         EXPECT_EQ(i, 123);
      }

      {
         int i = 0;
         const auto &function = [&i](MoveOnlyValue &&mo) { i = mo.v; };
         ScopedPointer<Thread> thread(Thread::create(function, MoveOnlyValue(123)));
         EXPECT_TRUE(thread);
         EXPECT_TRUE(!thread->isRunning());
         thread->start();
         EXPECT_TRUE(thread->wait());
         EXPECT_EQ(i, 123);
      }

      {
         int i = 0;
         const auto &function = [&i](MoveOnlyValue &&mo) { i = mo.v; };
         MoveOnlyValue mo(-1);
         ScopedPointer<Thread> thread(Thread::create(function, std::move(mo)));
         EXPECT_TRUE(thread);
         EXPECT_TRUE(!thread->isRunning());
         thread->start();
         EXPECT_TRUE(thread->wait());
         EXPECT_EQ(i, -1);
      }
   } // move
   {
      // simple parameter passing
      int i = 0;
      const auto &function = [&i](int j, int k) { i = j * k; };
      ScopedPointer<Thread> thread(Thread::create(function, 3, 4));
      EXPECT_TRUE(thread);
      EXPECT_TRUE(!thread->isRunning());
      EXPECT_EQ(i, 0);
      thread->start();
      EXPECT_TRUE(thread->wait());
      EXPECT_EQ(i, 12);
   }

   {
      // ignore return values (with parameters)
      const auto &function = [](double d) { return d * 2.0; };
      ScopedPointer<Thread> thread(Thread::create(function, 3.14));
      EXPECT_TRUE(thread);
      EXPECT_TRUE(!thread->isRunning());
      thread->start();
      EXPECT_TRUE(thread->wait());
   }

   {
      // handling of pointers to member functions, std::ref, etc.
      struct S {
         S() : v(0) {}
         void doSomething() { ++v; }
         int v;
      };

      S object;

      EXPECT_EQ(object.v, 0);

      ScopedPointer<Thread> thread;
      thread.reset(Thread::create(&S::doSomething, object));
      EXPECT_TRUE(thread);
      EXPECT_TRUE(!thread->isRunning());
      thread->start();
      EXPECT_TRUE(thread->wait());

      EXPECT_EQ(object.v, 0); // a copy was passed, this should still be 0

      thread.reset(Thread::create(&S::doSomething, std::ref(object)));
      EXPECT_TRUE(thread);
      EXPECT_TRUE(!thread->isRunning());
      thread->start();
      EXPECT_TRUE(thread->wait());

      EXPECT_EQ(object.v, 1);

      thread.reset(Thread::create(&S::doSomething, &object));
      EXPECT_TRUE(thread);
      EXPECT_TRUE(!thread->isRunning());
      thread->start();
      EXPECT_TRUE(thread->wait());

      EXPECT_EQ(object.v, 2);
   }

   {
      // std::ref into ordinary reference
      int i = 42;
      const auto &function = [](int &i) { i *= 2; };
      ScopedPointer<Thread> thread(Thread::create(function, std::ref(i)));
      EXPECT_TRUE(thread);
      thread->start();
      EXPECT_TRUE(thread->wait());
      EXPECT_EQ(i, 84);
   }
   {
      // exceptions when copying/decaying the arguments are thrown at build side and won't terminate
      class ThreadException : public std::exception
      {
      };

      struct ThrowWhenCopying
      {
         ThrowWhenCopying() = default;
         ThrowWhenCopying(const ThrowWhenCopying &)
         {
            throw ThreadException();
         }
         ~ThrowWhenCopying() = default;
         ThrowWhenCopying &operator=(const ThrowWhenCopying &) = default;
      };

      const auto &function = [](const ThrowWhenCopying &){};
      ScopedPointer<Thread> thread;
      ThrowWhenCopying t;
      ASSERT_THROW(thread.reset(Thread::create(function, t)), ThreadException);
      EXPECT_TRUE(!thread);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST(ThreadTest, testRequestTermination)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Thread thread;
   EXPECT_TRUE(!thread.isInterruptionRequested());
   Semaphore sem;
   StopableJob *j  = new StopableJob(sem);
   j->moveToThread(&thread);
   thread.connectStartedSignal(j, &StopableJob::run);
   j->connectFinishedSignal(&thread, &Thread::quit, pdk::ConnectionType::DirectConnection);
   thread.connectFinishedSignal(j, &Object::deleteLater);
   thread.start();
   EXPECT_TRUE(!thread.isInterruptionRequested());
   sem.acquire();
   EXPECT_TRUE(!thread.wait(1000));
   thread.requestInterruption();
   sem.acquire();
   EXPECT_TRUE(thread.wait(1000));
   EXPECT_TRUE(!thread.isInterruptionRequested());
   PDKTEST_END_APP_CONTEXT();
}

int main(int argc, char **argv)
{
   sg_argc = argc;
   sg_argv = argv;
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
