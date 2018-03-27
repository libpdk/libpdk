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
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/Semaphore.h"
#include "ThreadOnce.h"
#include <condition_variable>

using pdk::kernel::Object;
using pdk::os::thread::Thread;
using pdk::os::thread::BasicAtomicInt;
using pdkunittest::Singleton;
using pdk::os::thread::Semaphore;

class SingletonObject: public Object
{
public:
   static int sm_runCount;
   SingletonObject()
   {
      m_val.store(42);
      ++sm_runCount;
   }
   
   ~SingletonObject()
   {}
   
   BasicAtomicInt m_val;
};

class IncrementThread: public Thread
{
public:
   static BasicAtomicInt sm_runCount;
   static Singleton<SingletonObject> sm_singleton;
   Semaphore &m_sem1;
   Semaphore &m_sem2;
   int &m_var;
   
   IncrementThread(Semaphore *psem1, Semaphore *psem2, int *pvar, Object *parent)
      : Thread(parent),
        m_sem1(*psem1),
        m_sem2(*psem2),
        m_var(*pvar)
   {
      start();
   }
   
   ~IncrementThread() {
      wait();
   }
   
protected:
   void run()
   {
      m_sem2.release();
      m_sem1.acquire();             // synchronize
      
      PDK_ONCE {
         ++m_var;
      }
      sm_runCount.ref();
      sm_singleton->m_val.ref();
   }
};
int SingletonObject::sm_runCount = 0;
BasicAtomicInt IncrementThread::sm_runCount = PDK_BASIC_ATOMIC_INITIALIZER(0);
Singleton<SingletonObject> IncrementThread::sm_singleton;

namespace {

void samethread_data(std::list<int> &data)
{
   SingletonObject::sm_runCount = 0;
   data.push_back(42);
   data.push_back(43);
}

} // anonymous namespace

TEST(ThreadOnceTest, testSameThread)
{
   static int controlVariable = 0;
   PDK_ONCE {
      ASSERT_EQ(controlVariable, 0);
      ++controlVariable;
   }
   ASSERT_EQ(controlVariable, 1);
   static Singleton<SingletonObject> s;
   std::list<int> data;
   samethread_data(data);
   for (const int v : data) {
      ASSERT_EQ((int)s->m_val.load(), v);
      s->m_val.ref();
   }
   ASSERT_EQ(SingletonObject::sm_runCount, 1);
}

TEST(ThreadOnceTest, testMultipleThreads)
{
   const int NumberOfThreads = 100;
   int controlVariable = 0;
   Semaphore sem1;
   Semaphore sem2(NumberOfThreads);
   Object *parent = new Object;
   for (int i = 0; i < NumberOfThreads; ++i) {
      new IncrementThread(&sem1, &sem2, &controlVariable, parent);
   }
   ASSERT_EQ(controlVariable, 0); // nothing must have set them yet
   SingletonObject::sm_runCount = 0;
   IncrementThread::sm_runCount.store(0);
   // wait for all of them to be ready
   sem2.acquire(NumberOfThreads);
   // unleash the threads
   sem1.release(NumberOfThreads);
   // wait for all of them to terminate:
   delete parent;
   ASSERT_EQ(controlVariable, 1);
   ASSERT_EQ((int)IncrementThread::sm_runCount.load(), NumberOfThreads);
   ASSERT_EQ(SingletonObject::sm_runCount, 1);
}

TEST(ThreadOnceTest, testNesting)
{
   int variable = 0;
   PDK_ONCE {
      PDK_ONCE {
         ++variable;
      }
   }
   ASSERT_EQ(variable, 1);
}

namespace {
void reentrant(int control, int &counter)
{
   PDK_ONCE {
      if (counter) {
         reentrant(--control, counter);
      }
      ++counter;
   }
   static Singleton<SingletonObject> s;
   s->m_val.ref();
}

void exception_helper(int &val)
{
   PDK_ONCE {
      if (val++ == 0) throw 0;
   }
}

}

TEST(ThreadOnceTest, testReentering)
{
   const int WantedRecursions = 5;
   int count = 0;
   SingletonObject::sm_runCount = 0;
   reentrant(WantedRecursions, count);
   // reentrancy is undefined behavior:
   ASSERT_TRUE(count == 1 || count == WantedRecursions);
   ASSERT_EQ(SingletonObject::sm_runCount, 1);
}

TEST(ThreadOnceTest, testException)
{
   int count = 0;
   
   try {
      exception_helper(count);
   } catch (...) {
      // nothing
   }
   ASSERT_EQ(count, 1);
   
   try {
      exception_helper(count);
   } catch (...) {
      ASSERT_TRUE(false) << "Exception shouldn't have been thrown...";
   }
   ASSERT_EQ(count, 2);
}
