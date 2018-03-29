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
// Created by softboy on 2017/01/08.

#include <limits.h>
#include <cstdio>
#include <utility>
#include <thread>
#include <list>
#include <chrono>

#include "gtest/gtest.h"
#include "pdk/base/os/thread/Semaphore.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdktest/PdkTest.h"

using pdk::os::thread::Semaphore;
using pdk::os::thread::Thread;
using pdk::kernel::ElapsedTimer;
using pdk::os::thread::SemaphoreReleaser;

static Semaphore *semaphore = nullptr;

class ThreadOne : public Thread
{
public:
   ThreadOne() {}
   
protected:
   void run()
   {
      int i = 0;
      while ( i < 100 ) {
         semaphore->acquire();
         i++;
         semaphore->release();
      }
   }
};

class ThreadN : public Thread
{
   int N;
   
public:
   ThreadN(int n) :N(n) { }
   
protected:
   void run()
   {
      int i = 0;
      while ( i < 100 ) {
         semaphore->acquire(N);
         i++;
         semaphore->release(N);
      }
   }
};

namespace {

void one_num_semaphore_handler()
{
   int i = 0;
   while ( i < 100 ) {
      semaphore->acquire();
      i++;
      semaphore->release();
   }
}

void n_num_semaphore_handler(int num)
{
   int i = 0;
   while ( i < 100 ) {
      semaphore->acquire(num);
      i++;
      semaphore->release(num);
   }
}

}

TEST(SemaphoreTest, testAcquire)
{
   {
      ASSERT_TRUE(!semaphore);
      semaphore = new Semaphore();
      semaphore->release();
      ThreadOne t1;
      ThreadOne t2;
      t1.start();
      t2.start();
      ASSERT_TRUE(t1.wait(4000));
      ASSERT_TRUE(t2.wait(4000));
      delete semaphore;
      semaphore = nullptr;
   }
   {
      ASSERT_TRUE(!semaphore);
      semaphore = new Semaphore();
      semaphore->release(4);
      ThreadN t1(2);
      ThreadN t2(3);
      t1.start();
      t2.start();
      ASSERT_TRUE(t1.wait(4000));
      ASSERT_TRUE(t2.wait(4000));
      delete semaphore;
      semaphore = nullptr;
   }
   Semaphore semaphore;
   ASSERT_EQ(semaphore.available(), 0);
   semaphore.release();
   ASSERT_EQ(semaphore.available(), 1);
   semaphore.release();
   ASSERT_EQ(semaphore.available(), 2);
   semaphore.release(10);
   ASSERT_EQ(semaphore.available(), 12);
   semaphore.release(10);
   ASSERT_EQ(semaphore.available(), 22);

   semaphore.acquire();
   ASSERT_EQ(semaphore.available(), 21);
   semaphore.acquire();
   ASSERT_EQ(semaphore.available(), 20);
   semaphore.acquire(10);
   ASSERT_EQ(semaphore.available(), 10);
   semaphore.acquire(10);
   ASSERT_EQ(semaphore.available(), 0);
}

TEST(SemaphoreTest, testTryAcquire)
{
   Semaphore semaphore;
   ASSERT_EQ(semaphore.available(), 0);
   semaphore.release();
   ASSERT_EQ(semaphore.available(), 1);
   ASSERT_TRUE(!semaphore.tryAcquire(2));
   ASSERT_EQ(semaphore.available(), 1);

   semaphore.release();
   ASSERT_EQ(semaphore.available(), 2);
   ASSERT_TRUE(!semaphore.tryAcquire(3));
   ASSERT_EQ(semaphore.available(), 2);

   semaphore.release(10);
   ASSERT_EQ(semaphore.available(), 12);
   ASSERT_TRUE(!semaphore.tryAcquire(100));
   ASSERT_EQ(semaphore.available(), 12);

   semaphore.release(10);
   ASSERT_EQ(semaphore.available(), 22);
   ASSERT_TRUE(!semaphore.tryAcquire(100));
   ASSERT_EQ(semaphore.available(), 22);

   semaphore.tryAcquire();
   ASSERT_EQ(semaphore.available(), 21);

   semaphore.tryAcquire();
   ASSERT_EQ(semaphore.available(), 20);

   semaphore.tryAcquire(10);
   ASSERT_EQ(semaphore.available(), 10);

   semaphore.tryAcquire(10);
   ASSERT_EQ(semaphore.available(), 0);

   ASSERT_TRUE(!semaphore.tryAcquire());
   ASSERT_EQ(semaphore.available(), 0);

   ASSERT_TRUE(!semaphore.tryAcquire(10));
   ASSERT_EQ(semaphore.available(), 0);

   ASSERT_TRUE(!semaphore.tryAcquire(10));
   ASSERT_EQ(semaphore.available(), 0);
}

TEST(SemaphoreTest, testTryAcquireWithTimeout)
{
   std::list<int> data{1000};
   for (int timeout : data) {
      int fuzz = 50;
      Semaphore semaphore;
#define FUZZYCOMPARE(a,e) \
   do { \
   int a1 = a; \
   int e1 = e; \
   ASSERT_TRUE(std::abs(a1-e1) < fuzz) << "("<< #a << "=" << a1<<") is more than "<<fuzz<< \
   " milliseconds different from ( #e =" << e1<<")";\
   } while (0);

      ASSERT_EQ(semaphore.available(), 0);
      semaphore.release();
      ASSERT_EQ(semaphore.available(), 1);
      std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
      ASSERT_TRUE(!semaphore.tryAcquire(2, timeout));
      int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, timeout);
      ASSERT_EQ(semaphore.available(), 1);

      semaphore.release();
      ASSERT_EQ(semaphore.available(), 2);
      start = std::chrono::system_clock::now();
      ASSERT_TRUE(!semaphore.tryAcquire(3, timeout));
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, timeout);
      ASSERT_EQ(semaphore.available(), 2);

      semaphore.release(10);
      ASSERT_EQ(semaphore.available(), 12);
      start = std::chrono::system_clock::now();
      ASSERT_TRUE(!semaphore.tryAcquire(100, timeout));
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, timeout);
      ASSERT_EQ(semaphore.available(), 12);

      semaphore.release(10);
      ASSERT_EQ(semaphore.available(), 22);
      start = std::chrono::system_clock::now();
      ASSERT_TRUE(!semaphore.tryAcquire(100, timeout));
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, timeout);
      ASSERT_EQ(semaphore.available(), 22);

      start = std::chrono::system_clock::now();
      semaphore.tryAcquire(1, timeout);
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, 0);
      ASSERT_EQ(semaphore.available(), 21);

      start = std::chrono::system_clock::now();
      semaphore.tryAcquire(1, timeout);
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, 0);
      ASSERT_EQ(semaphore.available(), 20);

      start = std::chrono::system_clock::now();
      semaphore.tryAcquire(10, timeout);
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, 0);
      ASSERT_EQ(semaphore.available(), 10);

      start = std::chrono::system_clock::now();
      semaphore.tryAcquire(10, timeout);
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, 0);
      ASSERT_EQ(semaphore.available(), 0);

      start = std::chrono::system_clock::now();
      ASSERT_TRUE(!semaphore.tryAcquire(1, timeout));
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, timeout);
      ASSERT_EQ(semaphore.available(), 0);

      start = std::chrono::system_clock::now();
      ASSERT_TRUE(!semaphore.tryAcquire(1, timeout));
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, timeout);
      ASSERT_EQ(semaphore.available(), 0);

      start = std::chrono::system_clock::now();
      ASSERT_TRUE(!semaphore.tryAcquire(10, timeout));
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, timeout);
      ASSERT_EQ(semaphore.available(), 0);

      start = std::chrono::system_clock::now();
      ASSERT_TRUE(!semaphore.tryAcquire(10, timeout));
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count();
      FUZZYCOMPARE(elapsed, timeout);
      ASSERT_EQ(semaphore.available(), 0);
#undef FUZZYCOMPARE
   }
}

TEST(SemaphoreTest, testTryAcquireTimeoutStarvation)
{
   class MyThread : public Thread
   {
   public:
      Semaphore m_startup;
      Semaphore *m_semaphore;
      int m_amountToConsume;
      int m_timeout;

      void run()
      {
         m_startup.release();
         while(true) {
            if (!m_semaphore->tryAcquire(m_amountToConsume, m_timeout)) {
               break;
            }
            m_semaphore->release(m_amountToConsume);
         }
      }
   };
   Semaphore semaphore;
   semaphore.release(1);
   MyThread consumer;
   consumer.m_semaphore = &semaphore;
   consumer.m_amountToConsume = 1;
   consumer.m_timeout = 1000;

   // start the thread and wait for it to start consuming
   consumer.start();
   consumer.m_startup.acquire();

   ASSERT_TRUE(!semaphore.tryAcquire(consumer.m_amountToConsume * 2, consumer.m_timeout * 2));
   semaphore.acquire();
   ASSERT_TRUE(consumer.wait());
}

namespace {

void try_acquire_with_timeout_forever_data(std::list<int> &data)
{
   data.push_back(-1);
   data.push_back(INT_MIN);
}

} // anonymous namespace

TEST(SemaphoreTest, testTryAcquireWithTimeoutForever)
{
   enum { WaitTime = 1000 };
   class MyThread : public Thread {
   public:
      Semaphore m_sem;

      void run() override
      {
         pdktest::wait(WaitTime);
         m_sem.release(2);
      }
   };

   std::list<int> data;
   try_acquire_with_timeout_forever_data(data);
   for (int timeout: data) {
      MyThread thread;
      thread.m_sem.release(11);
      ASSERT_TRUE(thread.m_sem.tryAcquire(1, timeout));
      ASSERT_TRUE(thread.m_sem.tryAcquire(10, timeout));
      ElapsedTimer timer;
      timer.start();
      thread.start();
      ASSERT_TRUE(thread.wait());
      ASSERT_TRUE(thread.m_sem.tryAcquire(1, timeout));
      // @TODO i have no idea why wake up ahead of schedul
      ASSERT_TRUE(timer.getElapsed() >= WaitTime - 10);
      ASSERT_EQ(thread.m_sem.available(), 1);
   }
}

const char alphabet[] = "ACGTH";
const int AlphabetSize = sizeof(alphabet) - 1;

const int BufferSize = 4096; // GCD of BufferSize and alphabet size must be 1
char buffer[BufferSize];

const int ProducerChunkSize = 3;
const int ConsumerChunkSize = 7;
const int Multiplier = 10;

// note: the code depends on the fact that DataSize is a multiple of
// ProducerChunkSize, ConsumerChunkSize, and BufferSize
const int DataSize = ProducerChunkSize * ConsumerChunkSize * BufferSize * Multiplier;

Semaphore freeSpace(BufferSize);
Semaphore usedSpace;

class Producer : public Thread
{
public:
   void run();
};

static const int Timeout = 60 * 1000; // 1min


void Producer::run()
{
   for (int i = 0; i < DataSize; ++i) {
      ASSERT_TRUE(freeSpace.tryAcquire(1, Timeout));
      buffer[i % BufferSize] = alphabet[i % AlphabetSize];
      usedSpace.release();
   }
   for (int i = 0; i < DataSize; ++i) {
      if ((i % ProducerChunkSize) == 0) {
         ASSERT_TRUE(freeSpace.tryAcquire(ProducerChunkSize, Timeout));
      }
      buffer[i % BufferSize] = alphabet[i % AlphabetSize];
      if ((i % ProducerChunkSize) == (ProducerChunkSize - 1)) {
         usedSpace.release(ProducerChunkSize);
      }
   }
}

class Consumer : public Thread
{
public:
   void run();
};

void Consumer::run()
{
   for (int i = 0; i < DataSize; ++i) {
      usedSpace.acquire();
      ASSERT_EQ(buffer[i % BufferSize], alphabet[i % AlphabetSize]);
      freeSpace.release();
   }
   for (int i = 0; i < DataSize; ++i) {
      if ((i % ConsumerChunkSize) == 0) {
         usedSpace.acquire(ConsumerChunkSize);
      }
      ASSERT_EQ(buffer[i % BufferSize], alphabet[i % AlphabetSize]);
      if ((i % ConsumerChunkSize) == (ConsumerChunkSize - 1)) {
         freeSpace.release(ConsumerChunkSize);
      }
   }
}

TEST(SemaphoreTest, testProducerConsumer)
{
   Producer producer;
   Consumer consumer;
   producer.start();
   consumer.start();
   producer.wait();
   consumer.wait();
}

TEST(SemaphoreTest, testRaii)
{
   Semaphore sem;
   ASSERT_EQ(sem.available(), 0);
   // basic operation:
   {
      SemaphoreReleaser r0;
      const SemaphoreReleaser r1(sem);
      const SemaphoreReleaser r2(sem, 2);
      
      ASSERT_EQ(r0.getSemaphore(), nullptr);
      ASSERT_EQ(r1.getSemaphore(), &sem);
      ASSERT_EQ(r2.getSemaphore(), &sem);
   }
   ASSERT_EQ(sem.available(), 3);
   // cancel:
   {
      const SemaphoreReleaser r1(sem);
      SemaphoreReleaser r2(sem, 2);
      
      ASSERT_EQ(r2.cancel(), &sem);
      ASSERT_EQ(r2.getSemaphore(), nullptr);
   }
   ASSERT_EQ(sem.available(), 4);
   
   // move-assignment:
   {
      const SemaphoreReleaser r1(sem);
      SemaphoreReleaser r2(sem, 2);
      ASSERT_EQ(sem.available(), 4);
      r2 = SemaphoreReleaser();
      ASSERT_EQ(sem.available(), 6);
      r2 = SemaphoreReleaser(sem, 42);
      ASSERT_EQ(sem.available(), 6);
   }
   ASSERT_EQ(sem.available(), 49);
}
