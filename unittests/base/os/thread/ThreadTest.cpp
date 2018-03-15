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

#include "pdk/base/os/thread/Thread.h"
#include <mutex>
#include <condition_variable>

#ifndef PDK_NO_EXCEPTIONS
#include <exception>
#endif

enum { one_minute = 60 * 1000, five_minutes = 5 * one_minute };

using pdk::os::thread::Thread;
using pdk::kernel::Object;

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
