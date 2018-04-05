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
// Created by softboy on 2018/04/05.

#include "gtest/gtest.h"
#include "pdk/base/os/thread/ReadWriteLock.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/Semaphore.h"
#include "pdk/kernel/CoreApplication.h"
#include <iostream>

using pdk::kernel::Object;
using pdk::os::thread::Thread;
using pdk::os::thread::Semaphore;
using pdk::os::thread::ReadWriteLock;
using pdk::os::thread::WriteLocker;

namespace {

class WriteLockerThread : public Thread
{
public:
   ReadWriteLock m_lock;
   Semaphore m_semaphore;
   Semaphore m_testSemaphore;
   
   void waitForTest()
   {
      m_semaphore.release();
      m_testSemaphore.acquire();
   }
};

WriteLockerThread *sg_thread = nullptr;

void wait_for_thread()
{
   sg_thread->m_semaphore.acquire();
}
void release_thread()
{
   sg_thread->m_testSemaphore.release();
}

} // anonymous namespace

TEST(WriteLockerTest, testScope)
{
   class ScopeTestThread : public WriteLockerThread
   {
   public:
      void run()
      {
         waitForTest();
         
         {
            WriteLocker locker(&m_lock);
            waitForTest();
         }
         
         waitForTest();
      }
   };
   
   sg_thread = new ScopeTestThread;
   sg_thread->start();
   
   wait_for_thread();
   // lock should be unlocked before entering the scope that creates the WriteLocker
   ASSERT_TRUE(sg_thread->m_lock.tryLockForWrite());
   sg_thread->m_lock.unlock();
   release_thread();
   
   wait_for_thread();
   // lock should be locked by the WriteLocker
   ASSERT_TRUE(!sg_thread->m_lock.tryLockForWrite());
   release_thread();
   
   wait_for_thread();
   // lock should be unlocked when the WriteLocker goes out of scope
   ASSERT_TRUE(sg_thread->m_lock.tryLockForWrite());
   sg_thread->m_lock.unlock();
   release_thread();
   
   ASSERT_TRUE(sg_thread->wait());
   
   delete sg_thread;
   sg_thread = nullptr;
}

TEST(WriteLockerTest, testUnlockAndRelock)
{
   class UnlockAndRelockThread : public WriteLockerThread
   {
   public:
      void run()
      {
         WriteLocker locker(&m_lock);
         waitForTest();
         locker.unlock();
         waitForTest();
         locker.relock();
         waitForTest();
      }
   };
   sg_thread = new UnlockAndRelockThread;
   sg_thread->start();
   wait_for_thread();
   // lock should be locked by the WriteLocker
   ASSERT_TRUE(!sg_thread->m_lock.tryLockForWrite());
   release_thread();
   wait_for_thread();
   // lock has been explicitly unlocked via WriteLocker
   ASSERT_TRUE(sg_thread->m_lock.tryLockForWrite());
   sg_thread->m_lock.unlock();
   release_thread();
   wait_for_thread();
   // lock has been explicitly relocked via WriteLocker
   ASSERT_TRUE(!sg_thread->m_lock.tryLockForWrite());
   release_thread();
   
   ASSERT_TRUE(sg_thread->wait());
   delete sg_thread;
   sg_thread = nullptr;
}

TEST(WriteLockerTest, testLockerState)
{
   class LockerStateThread : public WriteLockerThread
   {
   public:
      void run()
      {
         {
            WriteLocker locker(&m_lock);
            locker.relock();
            locker.unlock();
            waitForTest();
         }
         waitForTest();
      }
   };

   sg_thread = new LockerStateThread;
   sg_thread->start();
   wait_for_thread();
   // even though we relock() after creating the WriteLocker, it shouldn't lock the lock more than once
   ASSERT_TRUE(sg_thread->m_lock.tryLockForWrite());
   sg_thread->m_lock.unlock();
   release_thread();
   wait_for_thread();
   // if we call WriteLocker::unlock(), its destructor should do nothing
   ASSERT_TRUE(sg_thread->m_lock.tryLockForWrite());
   sg_thread->m_lock.unlock();
   release_thread();
   ASSERT_TRUE(sg_thread->wait());
   delete sg_thread;
   sg_thread = nullptr;
}

