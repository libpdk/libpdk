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

using pdk::os::thread::Semaphore;

static Semaphore *semaphore = 0;

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
      std::thread thread1(one_num_semaphore_handler);
      std::thread thread2(one_num_semaphore_handler);
      thread1.join();
      thread2.join();
      delete semaphore;
      semaphore = nullptr;
   }
   {
      ASSERT_TRUE(!semaphore);
      semaphore = new Semaphore(4);
      semaphore->release();
      std::thread thread1(n_num_semaphore_handler, 2);
      std::thread thread2(n_num_semaphore_handler, 3);
      thread1.join();
      thread2.join();
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
   Semaphore startup;
   Semaphore *semaphorePtr;
   int amountToConsume;
   int timeout;
   Semaphore semaphore;
   semaphore.release(1);
   semaphorePtr = &semaphore;
   amountToConsume = 1;
   timeout = 1000;
   std::thread thread([&](Semaphore &startup, Semaphore *semaphorePtr){
      startup.release();
      while(true) {
         if (!semaphorePtr->tryAcquire(amountToConsume, timeout))
            break;
         semaphorePtr->release(amountToConsume);
      }
   }, std::ref(startup), semaphorePtr);
   startup.acquire();
   ASSERT_TRUE(!semaphore.tryAcquire(amountToConsume * 2, timeout * 2));
   semaphore.acquire();
   thread.join();
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

TEST(SemaphoreTest, testSemaphoreTest)
{
   std::thread producer([&](){
      for (int i = 0; i < DataSize; ++i) {
         freeSpace.acquire();
         buffer[i % BufferSize] = alphabet[i % AlphabetSize];
         usedSpace.release();
      }
      for (int i = 0; i < DataSize; ++i) {
         if ((i % ProducerChunkSize) == 0)
            freeSpace.acquire(ProducerChunkSize);
         buffer[i % BufferSize] = alphabet[i % AlphabetSize];
         if ((i % ProducerChunkSize) == (ProducerChunkSize - 1))
            usedSpace.release(ProducerChunkSize);
      }
   });
   std::thread consumer([&](){
      for (int i = 0; i < DataSize; ++i) {
         usedSpace.acquire();
         ASSERT_EQ(buffer[i % BufferSize], alphabet[i % AlphabetSize]);
         freeSpace.release();
      }
      for (int i = 0; i < DataSize; ++i) {
         if ((i % ConsumerChunkSize) == 0)
            usedSpace.acquire(ConsumerChunkSize);
         ASSERT_EQ(buffer[i % BufferSize], alphabet[i % AlphabetSize]);
         if ((i % ConsumerChunkSize) == (ConsumerChunkSize - 1))
            freeSpace.release(ConsumerChunkSize);
      }
   });
   producer.join();
   consumer.join();
}
