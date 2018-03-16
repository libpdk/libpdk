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

#include "pdk/base/os/thread/Thread.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/Timer.h"
#include <mutex>
#include <condition_variable>

#ifndef PDK_NO_EXCEPTIONS
#include <exception>
#endif

enum { one_minute = 60 * 1000, five_minutes = 5 * one_minute };

using pdk::os::thread::Thread;
using pdk::kernel::Object;
using pdk::kernel::Timer;
using pdk::kernel::CoreApplication;
using pdk::kernel::CallableInvoker;

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
      if (m_object) {
         m_object->m_thread = this;
         m_object->m_code = m_code;
         Timer::singleShot(100, [&](){
            m_object->slot();
         });
      }
      m_result = exec();
   }
};

TEST(ThreadTest, testCurrentThreadId)
{
   CurrentThread thread;
   thread.m_id = 0;
   thread.m_thread = nullptr;
   thread.start();
   ASSERT_TRUE(thread.wait(five_minutes));
   ASSERT_TRUE(thread.m_id != 0);
   ASSERT_TRUE(thread.m_id != Thread::getCurrentThreadId());
}

TEST(ThreadTest, testCurrentThread)
{
   ASSERT_TRUE(Thread::getCurrentThread() != nullptr);
   CurrentThread thread;
   thread.m_id = 0;
   thread.m_thread = nullptr;
   thread.start();
   ASSERT_TRUE(thread.wait(five_minutes));
   ASSERT_EQ(thread.m_thread, dynamic_cast<Thread *>(&thread));
}

TEST(ThreadTest, testIdealThreadCount)
{
   ASSERT_TRUE(Thread::getIdealThreadCount() > 0);
   std::clog << "Ideal thread count:" << Thread::getIdealThreadCount();
}

TEST(ThreadTest, testIsFinished)
{
   SimpleThread thread;
   ASSERT_TRUE(!thread.isFinished());
   std::unique_lock<std::mutex> locker(thread.m_mutex);
   thread.start();
   ASSERT_TRUE(!thread.isFinished());
   thread.m_cond.wait(locker);
   ASSERT_TRUE(thread.wait(five_minutes));
   ASSERT_TRUE(thread.isFinished());
}

TEST(ThreadTest, testIsRunning)
{
   SimpleThread thread;
   ASSERT_TRUE(!thread.isRunning());
   std::unique_lock<std::mutex> locker;
   thread.start();
   ASSERT_TRUE(thread.isRunning());
   ASSERT_TRUE(thread.wait(five_minutes));
   ASSERT_TRUE(!thread.isRunning());
}

TEST(ThreadTest, testSetPriority)
{
   SimpleThread thread;
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::IdlePriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::LowestPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::LowPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::NormalPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::HighPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::HighestPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::TimeCriticalPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   
   std::unique_lock<std::mutex> locker(thread.m_mutex);
   thread.start();
   
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::IdlePriority);
   ASSERT_EQ(thread.getPriority(), Thread::IdlePriority);
   thread.setPriority(Thread::LowestPriority);
   ASSERT_EQ(thread.getPriority(), Thread::LowestPriority);
   thread.setPriority(Thread::LowPriority);
   ASSERT_EQ(thread.getPriority(), Thread::LowPriority);
   thread.setPriority(Thread::NormalPriority);
   ASSERT_EQ(thread.getPriority(), Thread::NormalPriority);
   thread.setPriority(Thread::HighPriority);
   ASSERT_EQ(thread.getPriority(), Thread::HighPriority);
   thread.setPriority(Thread::HighestPriority);
   ASSERT_EQ(thread.getPriority(), Thread::HighestPriority);
   thread.setPriority(Thread::TimeCriticalPriority);
   ASSERT_EQ(thread.getPriority(), Thread::TimeCriticalPriority);
   
   thread.m_cond.wait(locker);
   ASSERT_TRUE(thread.wait(five_minutes));
   
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::IdlePriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::LowestPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::LowPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::NormalPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::HighPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::HighestPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
   thread.setPriority(Thread::TimeCriticalPriority);
   ASSERT_EQ(thread.getPriority(), Thread::InheritPriority);
}

TEST(ThreadTest, testSetStackSize)
{
   SimpleThread thread;
   ASSERT_EQ(thread.getStackSize(), 0u);
   thread.setStackSize(8192u);
   ASSERT_EQ(thread.getStackSize(), 8192u);
   thread.setStackSize(0u);
   ASSERT_EQ(thread.getStackSize(), 0u);
}

TEST(ThreadTest, testExit)
{
   ExitThread thread;
   thread.m_object = new ExitObject;
   thread.m_object->moveToThread(&thread);
   thread.m_code = 42;
   thread.m_result = 0;
   ASSERT_TRUE(!thread.isFinished());
   ASSERT_TRUE(!thread.isRunning());
   std::unique_lock<std::mutex> locker(thread.m_mutex);
   thread.start();
   
   ASSERT_TRUE(thread.isRunning());
   ASSERT_TRUE(!thread.isFinished());
   thread.m_cond.wait(locker);
   ASSERT_TRUE(thread.wait(five_minutes));
   ASSERT_TRUE(thread.isFinished());
   ASSERT_TRUE(!thread.isRunning());
}

int main(int argc, char **argv)
{
   CoreApplication app(argc, argv);
   int retCode = 0;
   ::testing::InitGoogleTest(&argc, argv);
   retCode = RUN_ALL_TESTS();   
   app.exec();
   return retCode;
}
