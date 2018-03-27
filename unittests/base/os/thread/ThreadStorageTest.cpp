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
#include "pdktest/PdkTest.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/base/lang/String.h"

#include <mutex>
#include <condition_variable>

using pdk::os::thread::ThreadStorage;
using pdk::os::thread::Thread;
using pdk::os::thread::BasicAtomicInt;
using pdk::lang::String;
using pdk::lang::Latin1String;
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
   
   PDK_TRY_COMPARE(Pointer::sm_count, c);
}

BasicAtomicInt cleanupOrder = PDK_BASIC_ATOMIC_INITIALIZER(0);

class First
{
public:
   ~First()
   {
      sm_order = cleanupOrder.fetchAndAddRelaxed(1);
   }
   static int sm_order;
};
int First::sm_order = -1;

class Second
{
public:
   ~Second()
   {
      sm_order = cleanupOrder.fetchAndAddRelaxed(1);
   }
   static int sm_order;
};
int Second::sm_order = -1;

TEST(ThreadStorageTest, testEnsureCleanupOrder)
{
   class MyThread : public Thread
   {
   public:
      ThreadStorage<First *> &m_first;
      ThreadStorage<Second *> &m_second;
      
      MyThread(ThreadStorage<First *> &first,
               ThreadStorage<Second *> &second)
         : m_first(first),
           m_second(second)
      {}
      
      void run()
      {
         // set in reverse order, but shouldn't matter, the data
         // will be deleted in the order the thread storage objects
         // were created
         m_second.setLocalData(new Second);
         m_first.setLocalData(new First);
      }
   };
   
   ThreadStorage<Second *> second;
   ThreadStorage<First *> first;
   MyThread thread(first, second);
   thread.start();
   thread.wait();
   
   ASSERT_TRUE(First::sm_order < Second::sm_order);
}

class SPointer
{
public:
   static BasicAtomicInt sm_count;
   inline SPointer()
   {
      sm_count.ref();
   }
   inline ~SPointer()
   {
      sm_count.deref();
   }
   
   inline SPointer(const SPointer & /* other */)
   {
      sm_count.ref();
   }
};

BasicAtomicInt SPointer::sm_count = PDK_BASIC_ATOMIC_INITIALIZER(0);
PDK_GLOBAL_STATIC(ThreadStorage<SPointer *>, sg_threadStoragePointers1);
PDK_GLOBAL_STATIC(ThreadStorage<SPointer *>, sg_threadStoragePointers2);

class ThreadStorageLocalDataTester
{
public:
   SPointer m_member;
   inline ~ThreadStorageLocalDataTester()
   {
      assertConditions();
   }
   void assertConditions()
   {
      ASSERT_TRUE(!sg_threadStoragePointers1()->hasLocalData());
      ASSERT_TRUE(!sg_threadStoragePointers2()->hasLocalData());
      sg_threadStoragePointers2()->setLocalData(new SPointer);
      sg_threadStoragePointers1()->setLocalData(new SPointer);
      ASSERT_TRUE(sg_threadStoragePointers1()->hasLocalData());
      ASSERT_TRUE(sg_threadStoragePointers2()->hasLocalData());
   }
};

TEST(ThreadSTorageTest, testLeakInDestructor)
{
   class MyThread : public Thread
   {
   public:
      ThreadStorage<ThreadStorageLocalDataTester *> &m_tls;
      
      MyThread(ThreadStorage<ThreadStorageLocalDataTester *> &t) : m_tls(t)
      {}
      
      void run()
      {
         ASSERT_TRUE(!m_tls.hasLocalData());
         m_tls.setLocalData(new ThreadStorageLocalDataTester);
         ASSERT_TRUE(m_tls.hasLocalData());
      }
   };
   int c = SPointer::sm_count.load();
   
   ThreadStorage<ThreadStorageLocalDataTester *> tls;
   
   ASSERT_TRUE(!sg_threadStoragePointers1()->hasLocalData());
   ThreadStorage<int *> tls2; //add some more tls to make sure ids are not following each other too much
   ThreadStorage<int *> tls3;
   ASSERT_TRUE(!tls2.hasLocalData());
   ASSERT_TRUE(!tls3.hasLocalData());
   ASSERT_TRUE(!tls.hasLocalData());
   
   MyThread t1(tls);
   MyThread t2(tls);
   MyThread t3(tls);
   
   t1.start();
   t2.start();
   t3.start();
   
   ASSERT_TRUE(t1.wait());
   ASSERT_TRUE(t2.wait());
   ASSERT_TRUE(t3.wait());
   
   ASSERT_EQ(int(SPointer::sm_count.load()), c);
}

class ThreadStorageResetLocalDataTester
{
public:
   SPointer m_member;
   ~ThreadStorageResetLocalDataTester();
};

PDK_GLOBAL_STATIC(ThreadStorage<ThreadStorageResetLocalDataTester *>, sg_threadStorageResetLocalDataTesterTls);

ThreadStorageResetLocalDataTester::~ThreadStorageResetLocalDataTester()
{
   //Quite stupid, but WTF::ThreadSpecific<T>::destroy does it.
   sg_threadStorageResetLocalDataTesterTls()->setLocalData(this);
}

TEST(ThreadStorageTest, testResetInDestructor)
{
   class MyThread : public Thread
   {
   public:
      void run()
      {
         ASSERT_TRUE(!sg_threadStorageResetLocalDataTesterTls()->hasLocalData());
         sg_threadStorageResetLocalDataTesterTls()->setLocalData(new ThreadStorageResetLocalDataTester);
         ASSERT_TRUE(sg_threadStorageResetLocalDataTesterTls()->hasLocalData());
      }
   };
   int c = SPointer::sm_count.load();
   
   MyThread t1;
   MyThread t2;
   MyThread t3;
   t1.start();
   t2.start();
   t3.start();
   ASSERT_TRUE(t1.wait());
   ASSERT_TRUE(t2.wait());
   ASSERT_TRUE(t3.wait());
   
   //check all the constructed things have been destructed
   ASSERT_EQ(int(SPointer::sm_count.load()), c);
}

TEST(ThreadStorageTest, testvalueBased)
{
   struct MyThread : Thread
   {
      ThreadStorage<SPointer> &tlsSPointer;
      ThreadStorage<String> &tlsString;
      ThreadStorage<int> &tlsInt;
      
      int someNumber;
      String someString;
      MyThread(ThreadStorage<SPointer> &t1, ThreadStorage<String> &t2, ThreadStorage<int> &t3)
         : tlsSPointer(t1), tlsString(t2), tlsInt(t3)
      {}
      
      void run()
      {
         /*ASSERT_TRUE(!tlsSPointer.hasLocalData());
            ASSERT_TRUE(!tlsString.hasLocalData());
            ASSERT_TRUE(!tlsInt.hasLocalData());*/
         SPointer pointercopy = tlsSPointer.getLocalData();
         
         //Default constructed values
         ASSERT_TRUE(tlsString.getLocalData().isNull());
         ASSERT_EQ(tlsInt.getLocalData(), 0);
         
         //setting
         tlsString.setLocalData(someString);
         tlsInt.setLocalData(someNumber);
         
         ASSERT_EQ(tlsString.getLocalData(), someString);
         ASSERT_EQ(tlsInt.getLocalData(), someNumber);
         
         //changing
         tlsSPointer.setLocalData(SPointer());
         tlsInt.getLocalData() += 42;
         tlsString.getLocalData().append(Latin1String(" world"));
         
         ASSERT_EQ(tlsString.getLocalData(), (someString + Latin1String(" world")));
         ASSERT_EQ(tlsInt.getLocalData(), (someNumber + 42));
         
         // operator=
         tlsString.getLocalData() = String::number(someNumber);
         ASSERT_EQ(tlsString.getLocalData().toInt(), someNumber);
      }
   };
   
   ThreadStorage<SPointer> tlsSPointer;
   ThreadStorage<String> tlsString;
   ThreadStorage<int> tlsInt;
   
   int c = SPointer::sm_count.load();
   
   MyThread t1(tlsSPointer, tlsString, tlsInt);
   MyThread t2(tlsSPointer, tlsString, tlsInt);
   MyThread t3(tlsSPointer, tlsString, tlsInt);
   t1.someNumber = 42;
   t2.someNumber = -128;
   t3.someNumber = 78;
   t1.someString = Latin1String("hello");
   t2.someString = Latin1String("australia");
   t3.someString = Latin1String("nokia");
   
   t1.start();
   t2.start();
   t3.start();
   
   ASSERT_TRUE(t1.wait());
   ASSERT_TRUE(t2.wait());
   ASSERT_TRUE(t3.wait());
   
   ASSERT_EQ(c, int(SPointer::sm_count.load()));
   
}

