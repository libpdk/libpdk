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
