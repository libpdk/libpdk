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
#include "pdk/base/os/thread/ThreadStorage.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdktest/TestEventLoop.h"

#include <mutex>
#include <condition_variable>

using pdk::os::thread::ThreadStorage;
using pdk::os::thread::Thread;
using pdktest::TestEventLoop;

class Pointer
{
public:
   static int sm_count;
   inline Pointer()
   {
      ++sm_count;
   }
   
   inline ~Pointer()
   {
      --sm_count;
   }
};

int Pointer::sm_count = 0;

TEST(ThreadStorageTest, testHasLocalData)
{
   ThreadStorage<Pointer *> pointers;
   ASSERT_TRUE(!pointers.hasLocalData());
   pointers.setLocalData(new Pointer);
   ASSERT_TRUE(pointers.hasLocalData());
   pointers.setLocalData(0);
   ASSERT_TRUE(!pointers.hasLocalData());
}

TEST(ThreadStorageTest, testGetLocalData)
{
   ThreadStorage<Pointer*> pointers;
   Pointer *p = new Pointer;
   ASSERT_TRUE(!pointers.hasLocalData());
   pointers.setLocalData(p);
   ASSERT_TRUE(pointers.hasLocalData());
   ASSERT_EQ(pointers.getLocalData(), p);
   pointers.setLocalData(0);
   ASSERT_EQ(pointers.getLocalData(), (Pointer *)0);
   ASSERT_TRUE(!pointers.hasLocalData());
}

TEST(ThreadStorageTest, testGetLocalDataConst)
{
   ThreadStorage<Pointer *> pointers;
   const ThreadStorage<Pointer *> &const_pointers = pointers;
   Pointer *p = new Pointer;
   ASSERT_TRUE(!pointers.hasLocalData());
   pointers.setLocalData(p);
   ASSERT_TRUE(pointers.hasLocalData());
   ASSERT_EQ(const_pointers.getLocalData(), p);
   pointers.setLocalData(0);
   ASSERT_EQ(const_pointers.getLocalData(), (Pointer *)0);
   ASSERT_TRUE(!pointers.hasLocalData());
}

TEST(ThreadStorageTest, testSetLocalData)
{
   ThreadStorage<Pointer *> pointers;
   ASSERT_TRUE(!pointers.hasLocalData());
   pointers.setLocalData(new Pointer);
   ASSERT_TRUE(pointers.hasLocalData());
   pointers.setLocalData(0);
   ASSERT_TRUE(!pointers.hasLocalData());
}

class MyThread : public Thread
{
public:
   ThreadStorage<Pointer *> &m_pointers;
   
   std::mutex m_mutex;
   std::condition_variable m_cond;
   
   MyThread(ThreadStorage<Pointer *> &p)
      : m_pointers(p)
   {}
   
   void run()
   {
      m_pointers.setLocalData(new Pointer);
      std::unique_lock<std::mutex> locker(m_mutex);
      m_cond.notify_one();
      m_cond.wait(locker);
   }
};

TEST(ThreadTest, testAutoDelete)
{
   ThreadStorage<Pointer *> pointers;
   ASSERT_TRUE(!pointers.hasLocalData());
   MyThread thread(pointers);
   int c = Pointer::sm_count;
   {
      std::unique_lock<std::mutex> locker(thread.m_mutex);
      thread.start();
      thread.m_cond.wait(locker);
      ASSERT_EQ(Pointer::sm_count, c + 1);
      thread.m_cond.notify_one();
   }
   thread.wait();
   ASSERT_EQ(Pointer::sm_count, c);
}

bool threadStorageOk;
void test_adopted_thread_storage_win(void *p)
{
   ThreadStorage<Pointer *> *pointers = reinterpret_cast<ThreadStorage<Pointer *> *>(p);
   if (pointers->hasLocalData()) {
      threadStorageOk = false;
      return;
   }
   Pointer *pointer = new Pointer();
   pointers->setLocalData(pointer);
   if (pointers->hasLocalData() == false) {
      threadStorageOk = false;
      return;
   }
   if (pointers->getLocalData() != pointer) {
      threadStorageOk = false;
      return;
   }
   Thread::getCurrentThread()->connectFinishedSignal(&TestEventLoop::instance(), &TestEventLoop::exitLoop);
}

void *test_adopted_thread_storage_unix(void *pointers)
{
   test_adopted_thread_storage_win(pointers);
   return 0;
}

TEST(ThreadStorageTest, testAdoptedThreads)
{
    TestEventLoop::instance(); // Make sure the instance is created in this thread.
    ThreadStorage<Pointer *> pointers;
    int c = Pointer::sm_count;
    threadStorageOk = true;
    {
#ifdef PDK_OS_UNIX
        pthread_t thread;
        const int state = pthread_create(&thread, 0, test_adopted_thread_storage_unix, &pointers);
        ASSERT_EQ(state, 0);
        pthread_join(thread, 0);
#elif defined PDK_OS_WIN
        HANDLE thread;
        thread = (HANDLE)_beginthread(test_adopted_thread_storage_win, 0, &pointers);
        ASSERT_TRUE(thread);
        WaitForSingleObject(thread, INFINITE);
#endif
    }
    ASSERT_TRUE(threadStorageOk);
    TestEventLoop::instance().enterLoop(2);
    ASSERT_TRUE(!TestEventLoop::instance().getTimeout());

    //QTRY_COMPARE(Pointer::count, c);
}

