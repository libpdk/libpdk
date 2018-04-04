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

#include "gtest/gtest.h"
#include "pdk/base/os/thread/ReadWriteLock.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/Semaphore.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/time/Time.h"

#include <limits.h>
#include <cstdio>
#include <list>
#include <utility>
#include <tuple>
#include <condition_variable>
#include <mutex>

#ifdef PDK_OS_UNIX
#include <unistd.h>
#endif

//on solaris, threads that loop on the release bool variable
//needs to sleep more than 1 usec.
#ifdef PDK_OS_SOLARIS
# define RWTESTSLEEP usleep(10);
#else
# define RWTESTSLEEP usleep(1);
#endif

using pdk::os::thread::ReadWriteLock;
using pdk::os::thread::Semaphore;
using pdk::os::thread::AtomicInt;
using pdk::os::thread::Thread;
using pdk::time::Time;

TEST(ReadWriteLockTest, testConstructDestruct)
{
   {
      ReadWriteLock rwlock;
   }
}

TEST(ReadWriteLockTest, testReadLockUnlock)
{
   ReadWriteLock rwlock;
   rwlock.lockForRead();
   rwlock.unlock();
}

TEST(ReadWriteLockTest, testWriteLockUnlock)
{
   ReadWriteLock rwlock;
   rwlock.lockForWrite();
   rwlock.unlock();
}

TEST(ReadWriteLockTest, testReadLockUnlockLoop)
{
   ReadWriteLock rwlock;
   int runs = 10000;
   for (int i = 0; i < runs; ++i) {
      rwlock.lockForRead();
      rwlock.unlock();
   }
}

TEST(ReadWriteLockTest, testWriteLockUnlockLoop)
{
   ReadWriteLock rwlock;
   int runs = 10000;
   for (int i = 0; i < runs; ++i) {
      rwlock.lockForWrite();
      rwlock.unlock();
   }
}

TEST(ReadWriteLockTest, testReadLockLoop)
{
   ReadWriteLock rwlock;
   int runs = 10000;
   for (int i = 0; i < runs; ++i) {
      rwlock.lockForRead();
   }
   for (int i = 0; i < runs; ++i) {
      rwlock.unlock();
   }
}

TEST(ReadWriteLockTest, testWriteLockLoop)
{
   /*
   * If you include this, the test should print one line
   * and then block.
   */
#if 0
   ReadWriteLock rwlock;
   int runs = 10000;
   for (int i = 0; i < runs; ++i) {
      rwlock.lockForWrite();
   }
#endif
}

TEST(ReadWriteLockTest, testReadWriteLockUnlockLoop)
{
   ReadWriteLock rwlock;
   int runs = 10000;
   for (int i = 0; i < runs; ++i) {
      rwlock.lockForRead();
      rwlock.unlock();
      rwlock.lockForWrite();
      rwlock.unlock();
   }
}

AtomicInt sg_lockCount(0);
ReadWriteLock sg_readWriteLock;
Semaphore sg_testsTurn;
Semaphore sg_threadsTurn;

TEST(ReadWriteLockTest, testTryReadLock)
{
   ReadWriteLock rwlock;
   ASSERT_TRUE(rwlock.tryLockForRead());
   rwlock.unlock();
   ASSERT_TRUE(rwlock.tryLockForRead());
   rwlock.unlock();
   
   rwlock.lockForRead();
   rwlock.lockForRead();
   ASSERT_TRUE(rwlock.tryLockForRead());
   rwlock.unlock();
   rwlock.unlock();
   rwlock.unlock();
   
   rwlock.lockForWrite();
   ASSERT_TRUE(!rwlock.tryLockForRead());
   rwlock.unlock();
   
   // functionality test
   {
      class MyThread : public Thread
      {
      public:
         void run()
         {
            sg_testsTurn.release();
            
            sg_threadsTurn.acquire();
            ASSERT_TRUE(!sg_readWriteLock.tryLockForRead());
            sg_testsTurn.release();
            
            sg_threadsTurn.acquire();
            ASSERT_TRUE(sg_readWriteLock.tryLockForRead());
            sg_lockCount.ref();
            ASSERT_TRUE(sg_readWriteLock.tryLockForRead());
            sg_lockCount.ref();
            sg_lockCount.deref();
            sg_readWriteLock.unlock();
            sg_lockCount.deref();
            sg_readWriteLock.unlock();
            sg_testsTurn.release();
            
            sg_threadsTurn.acquire();
            Time timer;
            timer.start();
            ASSERT_TRUE(!sg_readWriteLock.tryLockForRead(1000));
            ASSERT_TRUE(timer.elapsed() >= 1000);
            sg_testsTurn.release();
            
            sg_threadsTurn.acquire();
            timer.start();
            ASSERT_TRUE(sg_readWriteLock.tryLockForRead(1000));
            ASSERT_TRUE(timer.elapsed() <= 1000);
            sg_lockCount.ref();
            ASSERT_TRUE(sg_readWriteLock.tryLockForRead(1000));
            sg_lockCount.ref();
            sg_lockCount.deref();
            sg_readWriteLock.unlock();
            sg_lockCount.deref();
            sg_readWriteLock.unlock();
            sg_testsTurn.release();
            
            sg_threadsTurn.acquire();
         }
      };
      MyThread thread;
      thread.start();
      
      sg_testsTurn.acquire();
      sg_readWriteLock.lockForWrite();
      ASSERT_TRUE(sg_lockCount.testAndSetRelaxed(0, 1));
      sg_threadsTurn.release();
      
      sg_testsTurn.acquire();
      ASSERT_TRUE(sg_lockCount.testAndSetRelaxed(1, 0));
      sg_readWriteLock.unlock();
      sg_threadsTurn.release();
      
      sg_testsTurn.acquire();
      sg_readWriteLock.lockForWrite();
      ASSERT_TRUE(sg_lockCount.testAndSetRelaxed(0, 1));
      sg_threadsTurn.release();
      
      sg_testsTurn.acquire();
      ASSERT_TRUE(sg_lockCount.testAndSetRelaxed(1, 0));
      sg_readWriteLock.unlock();
      sg_threadsTurn.release();
      
      sg_testsTurn.acquire();
      sg_threadsTurn.release();
      
      thread.wait();
   }
}

TEST(ReadWriteLockTest, testTryWriteLock)
{
   {
      ReadWriteLock rwlock;
      ASSERT_TRUE(rwlock.tryLockForWrite());
      rwlock.unlock();
      ASSERT_TRUE(rwlock.tryLockForWrite());
      rwlock.unlock();
      
      rwlock.lockForWrite();
      ASSERT_TRUE(!rwlock.tryLockForWrite());
      ASSERT_TRUE(!rwlock.tryLockForWrite());
      rwlock.unlock();
      
      rwlock.lockForRead();
      ASSERT_TRUE(!rwlock.tryLockForWrite());
      rwlock.unlock();
   }
   
   {
      ReadWriteLock rwlock(ReadWriteLock::RecursionMode::Recursion);
      ASSERT_TRUE(rwlock.tryLockForWrite());
      rwlock.unlock();
      ASSERT_TRUE(rwlock.tryLockForWrite());
      rwlock.unlock();
      
      rwlock.lockForWrite();
      ASSERT_TRUE(rwlock.tryLockForWrite());
      ASSERT_TRUE(rwlock.tryLockForWrite());
      rwlock.unlock();
      rwlock.unlock();
      rwlock.unlock();
      
      rwlock.lockForRead();
      ASSERT_TRUE(rwlock.tryLockForWrite());
      rwlock.unlock();
   }
   
   // functionality test
   {
      class MyThread : public Thread
      {
      public:
         MyThread() : m_failureCount(0) { }
         void run()
         {
            sg_testsTurn.release();
            
            sg_threadsTurn.acquire();
            if (sg_readWriteLock.tryLockForWrite()) {
               m_failureCount++;
            }
            
            sg_testsTurn.release();
            
            sg_threadsTurn.acquire();
            if (!sg_readWriteLock.tryLockForWrite())
               m_failureCount++;
            if (!sg_lockCount.testAndSetRelaxed(0, 1))
               m_failureCount++;
            if (!sg_lockCount.testAndSetRelaxed(1, 0))
               m_failureCount++;
            sg_readWriteLock.unlock();
            sg_testsTurn.release();
            
            sg_threadsTurn.acquire();
            if (sg_readWriteLock.tryLockForWrite(1000)) {
               m_failureCount++;
            }
            
            sg_testsTurn.release();
            
            sg_threadsTurn.acquire();
            if (!sg_readWriteLock.tryLockForWrite(1000))
               m_failureCount++;
            if (!sg_lockCount.testAndSetRelaxed(0, 1))
               m_failureCount++;
            if (!sg_lockCount.testAndSetRelaxed(1, 0))
               m_failureCount++;
            sg_readWriteLock.unlock();
            sg_testsTurn.release();
            
            sg_threadsTurn.acquire();
         }
         
         int m_failureCount;
      };
      
      MyThread thread;
      thread.start();
      
      sg_testsTurn.acquire();
      sg_readWriteLock.lockForRead();
      sg_lockCount.ref();
      sg_threadsTurn.release();
      
      sg_testsTurn.acquire();
      sg_lockCount.deref();
      sg_readWriteLock.unlock();
      sg_threadsTurn.release();
      
      sg_testsTurn.acquire();
      sg_readWriteLock.lockForRead();
      sg_lockCount.ref();
      sg_threadsTurn.release();
      
      sg_testsTurn.acquire();
      sg_lockCount.deref();
      sg_readWriteLock.unlock();
      sg_threadsTurn.release();
      
      // stop thread
      sg_testsTurn.acquire();
      sg_threadsTurn.release();
      thread.wait();
      
      ASSERT_EQ(thread.m_failureCount, 0);
   }
}

bool sg_threadDone;
AtomicInt sg_release;

namespace {

/*
    read-lock
    unlock
    set threadone
*/
class WriteLockThread : public Thread
{
public:
   ReadWriteLock &m_testRwlock;
   inline WriteLockThread(ReadWriteLock &locker)
      :m_testRwlock(locker)
   {}
   
   void run()
   {
      m_testRwlock.lockForWrite();
      m_testRwlock.unlock();
      sg_threadDone = true;
   }
};

/*
    read-lock
    unlock
    set threadone
*/
class ReadLockThread : public Thread
{
public:
   ReadWriteLock &m_testRwlock;
   inline ReadLockThread(ReadWriteLock &locker)
      : m_testRwlock(locker)
   {}
   
   void run()
   {
      m_testRwlock.lockForRead();
      m_testRwlock.unlock();
      sg_threadDone = true;
   }
};

/*
    write-lock
    wait for release==true
    unlock
*/
class WriteLockReleasableThread : public Thread
{
public:
   ReadWriteLock &m_testRwlock;
   inline WriteLockReleasableThread(ReadWriteLock &locker)
      : m_testRwlock(locker)
   {}
   
   void run()
   {
      m_testRwlock.lockForWrite();
      while(sg_release.load() == false) {
         RWTESTSLEEP
      }
      m_testRwlock.unlock();
   }
};

/*
    read-lock
    wait for release==true
    unlock
*/
class ReadLockReleasableThread : public Thread
{
public:
   ReadWriteLock &m_testRwlock;
   inline ReadLockReleasableThread(ReadWriteLock &locker)
      : m_testRwlock(locker)
   {}
   
   void run()
   {
      m_testRwlock.lockForRead();
      while(sg_release.load() == false) {
         RWTESTSLEEP
      }
      m_testRwlock.unlock();
   }
};

/*
    for(runTime msecs)
        read-lock
        msleep(holdTime msecs)
        release lock
        msleep(waitTime msecs)
*/
class ReadLockLoopThread : public Thread
{
public:
   ReadWriteLock &m_testRwlock;
   int m_runTime;
   int m_holdTime;
   int m_waitTime;
   bool m_print;
   Time m_time;
   inline ReadLockLoopThread(ReadWriteLock &locker, int runTime, int holdTime = 0, 
                             int waitTime = 0, bool print = false)
      : m_testRwlock(locker),
        m_runTime(runTime),
        m_holdTime(holdTime),
        m_waitTime(waitTime),
        m_print(print)
   {}
   
   void run()
   {
      m_time.start();
      while (m_time.elapsed() < m_runTime)  {
         m_testRwlock.lockForRead();
         if(m_print) {
            printf("reading\n");
         }
         if (m_holdTime) {
            msleep(m_holdTime);
         }
         m_testRwlock.unlock();
         if (m_waitTime) {
            msleep(m_waitTime);
         }
      }
   }
};

/*
    for(runTime msecs)
        write-lock
        msleep(holdTime msecs)
        release lock
        msleep(waitTime msecs)
*/
class WriteLockLoopThread : public Thread
{
public:
   ReadWriteLock &m_testRwlock;
   int m_runTime;
   int m_holdTime;
   int m_waitTime;
   bool m_print;
   Time m_time;
   inline WriteLockLoopThread(ReadWriteLock &locker, int runTime, int holdTime = 0, 
                              int waitTime = 0, bool print = false)
      : m_testRwlock(locker),
        m_runTime(runTime),
        m_holdTime(holdTime),
        m_waitTime(waitTime),
        m_print(print)
   {}
   
   void run()
   {
      m_time.start();
      while (m_time.elapsed() < m_runTime)  {
         m_testRwlock.lockForWrite();
         if (m_print) {
            printf(".");
         }
         if (m_holdTime) {
            msleep(m_holdTime);
         }
         m_testRwlock.unlock();
         if (m_waitTime) {
            msleep(m_waitTime);
         }
      }
   }
};

volatile int sg_count = 0;

/*
    for(runTime msecs)
        write-lock
        count to maxval
        set count to 0
        release lock
        msleep waitTime
*/
class WriteLockCountThread : public Thread
{
public:
   ReadWriteLock &m_testRwlock;
   int m_runTime;
   int m_waitTime;
   int m_maxval;
   Time m_time;
   inline WriteLockCountThread(ReadWriteLock &locker, int runTime, int waitTime, int maxval)
      : m_testRwlock(locker),
        m_runTime(runTime),
        m_waitTime(waitTime),
        m_maxval(maxval)
   {}
   
   void run()
   {
      m_time.start();
      while (m_time.elapsed() < m_runTime)  {
         m_testRwlock.lockForWrite();
         if(sg_count) {
            std::cerr << "Non-zero count at start of write! (" << sg_count << ")";
            //            printf(".");
         }
         
         int i;
         for(i=0; i < m_maxval; ++i) {
            volatile int lc = sg_count;
            ++lc;
            sg_count = lc;
         }
         sg_count = 0;
         m_testRwlock.unlock();
         msleep(m_waitTime);
      }
   }
};

/*
    for(runTime msecs)
        read-lock
        verify count==0
        release lock
        msleep waitTime
*/
class ReadLockCountThread : public Thread
{
public:
   ReadWriteLock &m_testRwlock;
   int m_runTime;
   int m_waitTime;
   Time m_time;
   inline ReadLockCountThread(ReadWriteLock &locker, int runTime, int waitTime)
      : m_testRwlock(locker),
        m_runTime(runTime),
        m_waitTime(waitTime)
   {}
   
   void run()
   {
      m_time.start();
      while (m_time.elapsed() < m_runTime)  {
         m_testRwlock.lockForRead();
         if(sg_count) {
            std::cerr << "Non-zero count at Read! (" << sg_count << ")";
         }
         m_testRwlock.unlock();
         msleep(m_waitTime);
      }
   }
};

} // anonymous namespace

/*
    A writer acquires a read-lock, a reader locks
    the writer releases the lock, the reader gets the lock
*/
TEST(ReadWriteLockTest, testReadLockBlockRelease)
{
   ReadWriteLock testLock;
   testLock.lockForWrite();
   sg_threadDone = false;
   ReadLockThread rlt(testLock);
   rlt.start();
   sleep(1);
   testLock.unlock();
   rlt.wait();
   ASSERT_TRUE(sg_threadDone);
}

/*
    writer1 acquires a read-lock, writer2 blocks,
    writer1 releases the lock, writer2 gets the lock
*/
TEST(ReadWriteLockTest, testWriteLockBlockRelease)
{
   ReadWriteLock testLock;
   testLock.lockForWrite();
   sg_threadDone = false;
   WriteLockThread wlt(testLock);
   wlt.start();
   sleep(1);
   testLock.unlock();
   wlt.wait();
   ASSERT_TRUE(sg_threadDone);
}

/*
    Two readers acquire a read-lock, one writer attempts a write block,
    the readers release their locks, the writer gets the lock.
*/
TEST(ReadWriteLockTest, testMultipleReadersBlockRelease)
{
   ReadWriteLock testLock;
   sg_release.store(false);
   sg_threadDone = false;
   ReadLockReleasableThread rlt1(testLock);
   ReadLockReleasableThread rlt2(testLock);
   rlt1.start();
   rlt2.start();
   sleep(1);
   WriteLockThread wlt(testLock);
   wlt.start();
   sleep(1);
   sg_release.store(true);
   wlt.wait();
   rlt1.wait();
   rlt2.wait();
   ASSERT_TRUE(sg_threadDone);
}


/*
    Multiple readers locks and unlocks a lock.
*/
TEST(ReadWriteLockTest, testMultipleReadersLoop)
{
   int time = 500;
   int hold = 250;
   int wait = 0;
#if defined (PDK_OS_HPUX)
   const int numthreads = 50;
#elif defined(PDK_OS_VXWORKS)
   const int numthreads = 40;
#else
   const int numthreads = 75;
#endif
   ReadWriteLock testLock;
   ReadLockLoopThread *threads[numthreads];
   int i;
   for (i = 0; i < numthreads; ++i)
      threads[i] = new ReadLockLoopThread(testLock, time, hold, wait);
   for (i = 0; i< numthreads; ++i) {
      threads[i]->start();
   }
   for (i = 0; i< numthreads; ++i) {
      threads[i]->wait();
   }
   for (i = 0; i< numthreads; ++i) {
      delete threads[i];
   }
}

/*
    Multiple writers locks and unlocks a lock.
*/
TEST(ReadWriteLockTest, testMultipleWritersLoop)
{
   int time = 500;
   int wait = 0;
   int hold = 0;
   const int numthreads = 50;
   ReadWriteLock testLock;
   WriteLockLoopThread *threads[numthreads];
   int i;
   for (i=0; i<numthreads; ++i) {
      threads[i] = new WriteLockLoopThread(testLock, time, hold, wait);
   }
   for (i=0; i<numthreads; ++i) {
      threads[i]->start();
   }
   for (i=0; i<numthreads; ++i) {
      threads[i]->wait();
   }
   for (i=0; i<numthreads; ++i) {
      delete threads[i];
   }
}

/*
    Multiple readers and writers locks and unlocks a lock.
*/
TEST(ReadWriteLockTest, testMultipleReadersWritersLoop)
{
   //int time=INT_MAX;
   int time = 10000;
   int readerThreads = 20;
   int readerWait = 0;
   int readerHold = 1;

   int writerThreads = 2;
   int writerWait = 500;
   int writerHold = 50;

   ReadWriteLock testLock;
   ReadLockLoopThread  *readers[1024];
   WriteLockLoopThread *writers[1024];
   int i;

   for (i=0; i<readerThreads; ++i) {
      readers[i] = new ReadLockLoopThread(testLock, time, readerHold, readerWait, false);
   }
   for (i=0; i<writerThreads; ++i) {
      writers[i] = new WriteLockLoopThread(testLock, time, writerHold, writerWait, false);
   }
   for (i=0; i<readerThreads; ++i) {
      readers[i]->start(Thread::NormalPriority);
   }
   for (i=0; i<writerThreads; ++i) {
      writers[i]->start(Thread::IdlePriority);
   }
   for (i=0; i<readerThreads; ++i) {
      readers[i]->wait();
   }
   for (i=0; i<writerThreads; ++i) {
      writers[i]->wait();
   }
   for (i=0; i<readerThreads; ++i) {
      delete readers[i];
   }
   for (i=0; i<writerThreads; ++i) {
      delete writers[i];
   }
}

/*
    Writers increment a variable from 0 to maxval, then reset it to 0.
    Readers verify that the variable remains at 0.
*/
TEST(ReadWriteLockTest, testCounting)
{
   //int time=INT_MAX;
   int time = 10000;
   int readerThreads = 20;
   int readerWait = 1;

   int writerThreads = 3;
   int writerWait = 150;
   int maxval = 10000;

   ReadWriteLock testLock;
   ReadLockCountThread  *readers[1024];
   WriteLockCountThread *writers[1024];
   int i;

   for (i=0; i<readerThreads; ++i) {
      readers[i] = new ReadLockCountThread(testLock, time,  readerWait);
   }
   for (i=0; i<writerThreads; ++i) {
      writers[i] = new WriteLockCountThread(testLock, time,  writerWait, maxval);
   }
   for (i=0; i<readerThreads; ++i) {
      readers[i]->start(Thread::NormalPriority);
   }
   for (i=0; i<writerThreads; ++i) {
      writers[i]->start(Thread::LowestPriority);
   }
   for (i=0; i<readerThreads; ++i) {
      readers[i]->wait();
   }
   for (i=0; i<writerThreads; ++i) {
      writers[i]->wait();
   }
   for (i=0; i<readerThreads; ++i) {
      delete readers[i];
   }
   for (i=0; i<writerThreads; ++i) {
      delete writers[i];
   }

}

namespace {


/*
    Test a race-condition that may happen if one thread is in unlock() while
    another thread deletes the rw-lock.
    
    MainThread              DeleteOnUnlockThread
    
    write-lock
    unlock
      |                     write-lock
      |                     unlock
      |                     delete lock
    deref d inside unlock
*/
class DeleteOnUnlockThread : public Thread
{
public:
   DeleteOnUnlockThread(ReadWriteLock **lock, std::condition_variable *startup, std::mutex *waitMutex)
      :m_lock(lock),
        m_startup(startup),
        m_waitMutex(waitMutex)
   {}
   
   void run()
   {
      m_waitMutex->lock();
      m_startup->notify_all();
      m_waitMutex->unlock();
      
      // DeleteOnUnlockThread and the main thread will race from this point
      (*m_lock)->lockForWrite();
      (*m_lock)->unlock();
      delete *m_lock;
   }
private:
   ReadWriteLock **m_lock;
   std::condition_variable *m_startup;
   std::mutex *m_waitMutex;
};


} // anonymous namespace

TEST(ReadWriteLockTest, testDeleteOnUnlock)
{
   ReadWriteLock *lock = 0;
   std::condition_variable startup;
   std::mutex waitMutex;
   std::unique_lock mutexLocker(waitMutex, std::defer_lock);
   DeleteOnUnlockThread thread2(&lock, &startup, &waitMutex);
   
   Time time;
   time.start();
   while(time.elapsed() < 4000) {
      lock = new ReadWriteLock();
      mutexLocker.lock();
      lock->lockForWrite();
      thread2.start();
      startup.wait(mutexLocker);
      mutexLocker.unlock();
      
      // DeleteOnUnlockThread and the main thread will race from this point
      lock->unlock();
      
      thread2.wait();
   }
}

TEST(ReadWriteLockTest, testUncontendedLocks)
{
   
   uint read = 0;
   uint write = 0;
   uint count = 0;
   int millisecs = 1000;
   {
      Time time;
      time.start();
      while(time.elapsed() < millisecs)
      {
         ++count;
      }
   }
   {
      ReadWriteLock rwlock;
      Time time;
      time.start();
      while(time.elapsed() < millisecs)
      {
         rwlock.lockForRead();
         rwlock.unlock();
         ++read;
      }
   }
   {
      ReadWriteLock rwlock;
      Time time;
      time.start();
      while(time.elapsed() < millisecs)
      {
         rwlock.lockForWrite();
         rwlock.unlock();
         ++write;
      }
   }
   
   std::cout << "during "<< millisecs << " millisecs:" << std::endl;
   std::cout << "counted to " << count << std::endl;
   std::cout << read << " uncontended read locks/unlocks" << std::endl;
   std::cout << write << " uncontended write locks/unlocks" << std::endl;
}

enum { RecursiveLockCount = 10 };

TEST(ReadWriteLockTest, testRecursiveReadLock)
{
   // thread to attempt locking for writing while the test recursively locks for reading
   class RecursiveReadLockThread : public Thread
   {
   public:
      ReadWriteLock *m_lock;
      bool m_tryLockForWriteResult;
      
      void run()
      {
         sg_testsTurn.release();
         
         // test is recursively locking for writing
         for (int i = 0; i < RecursiveLockCount; ++i) {
            sg_threadsTurn.acquire();
            m_tryLockForWriteResult = m_lock->tryLockForWrite();
            sg_testsTurn.release();
         }
         
         // test is releasing recursive write lock
         for (int i = 0; i < RecursiveLockCount - 1; ++i) {
            sg_threadsTurn.acquire();
            m_tryLockForWriteResult = m_lock->tryLockForWrite();
            sg_testsTurn.release();
         }
         
         // after final unlock in test, we should get the lock
         sg_threadsTurn.acquire();
         m_tryLockForWriteResult = m_lock->tryLockForWrite();
         sg_testsTurn.release();
         
         // cleanup
         sg_threadsTurn.acquire();
         m_lock->unlock();
         sg_testsTurn.release();
         
         // test will lockForRead(), then we will lockForWrite()
         // (and block), purpose is to ensure that the test can
         // recursive lockForRead() even with a waiting writer
         sg_threadsTurn.acquire();
         // testsTurn.release(); // ### do not release here, the test uses tryAcquire()
         m_lock->lockForWrite();
         m_lock->unlock();
      }
   };
   
   // init
   ReadWriteLock lock(ReadWriteLock::RecursionMode::Recursion);
   RecursiveReadLockThread thread;
   thread.m_lock = &lock;
   thread.start();
   
   sg_testsTurn.acquire();
   
   // verify that we can get multiple read locks in the same thread
   for (int i = 0; i < RecursiveLockCount; ++i) {
      ASSERT_TRUE(lock.tryLockForRead());
      sg_threadsTurn.release();
      
      sg_testsTurn.acquire();
      ASSERT_TRUE(!thread.m_tryLockForWriteResult);
   }
   
   // have to unlock the same number of times that we locked
   for (int i = 0;i < RecursiveLockCount - 1; ++i) {
      lock.unlock();
      sg_threadsTurn.release();
      
      sg_testsTurn.acquire();
      ASSERT_TRUE(!thread.m_tryLockForWriteResult);
   }
   
   // after the final unlock, we should be able to get the write lock
   lock.unlock();
   sg_threadsTurn.release();
   
   sg_testsTurn.acquire();
   ASSERT_TRUE(thread.m_tryLockForWriteResult);
   sg_threadsTurn.release();
   
   // check that recursive read locking works even when we have a waiting writer
   sg_testsTurn.acquire();
   ASSERT_TRUE(lock.tryLockForRead());
   sg_threadsTurn.release();
   
   sg_testsTurn.tryAcquire(1, 1000);
   ASSERT_TRUE(lock.tryLockForRead());
   lock.unlock();
   lock.unlock();
   
   // cleanup
   ASSERT_TRUE(thread.wait());
}

TEST(ReadWriteLockTest, testRecursiveWriteLock)
{
    // thread to attempt locking for reading while the test recursively locks for writing
    class RecursiveWriteLockThread : public Thread
    {
    public:
        ReadWriteLock *m_lock;
        bool m_tryLockForReadResult;

        void run()
        {
            sg_testsTurn.release();

            // test is recursively locking for writing
            for (int i = 0; i < RecursiveLockCount; ++i) {
                sg_threadsTurn.acquire();
                m_tryLockForReadResult = m_lock->tryLockForRead();
                sg_testsTurn.release();
            }

            // test is releasing recursive write lock
            for (int i = 0; i < RecursiveLockCount - 1; ++i) {
                sg_threadsTurn.acquire();
                m_tryLockForReadResult = m_lock->tryLockForRead();
                sg_testsTurn.release();
            }

            // after final unlock in test, we should get the lock
            sg_threadsTurn.acquire();
            m_tryLockForReadResult = m_lock->tryLockForRead();
            sg_testsTurn.release();

            // cleanup
            m_lock->unlock();
        }
    };

    // init
    ReadWriteLock lock(ReadWriteLock::RecursionMode::Recursion);
    RecursiveWriteLockThread thread;
    thread.m_lock = &lock;
    thread.start();

    sg_testsTurn.acquire();

    // verify that we can get multiple read locks in the same thread
    for (int i = 0; i < RecursiveLockCount; ++i) {
        ASSERT_TRUE(lock.tryLockForWrite());
        sg_threadsTurn.release();

        sg_testsTurn.acquire();
        ASSERT_TRUE(!thread.m_tryLockForReadResult);
    }

    // have to unlock the same number of times that we locked
    for (int i = 0;i < RecursiveLockCount - 1; ++i) {
        lock.unlock();
        sg_threadsTurn.release();

        sg_testsTurn.acquire();
        ASSERT_TRUE(!thread.m_tryLockForReadResult);
    }

    // after the final unlock, thread should be able to get the read lock
    lock.unlock();
    sg_threadsTurn.release();

    sg_testsTurn.acquire();
    ASSERT_TRUE(thread.m_tryLockForReadResult);

    // cleanup
    ASSERT_TRUE(thread.wait());
}

