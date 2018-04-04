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
// Created by softboy on 2018/01/05.

#include "pdk/base/os/thread/ReadWriteLock.h"
#include "pdk/base/os/thread/internal/ReadWriteLockPrivate.h"
#include "pdk/utils/internal/LockFreeListPrivate.h"
#include "pdk/global/GlobalStatic.h"

#include <thread>
#include <chrono>
#include <condition_variable>
#include <iostream>

namespace pdk {
namespace os {
namespace thread {

/**
 * Implementation details of ReadWriteLock:
 *
 * Depending on the valued of m_implPtr, the lock is in the following state:
 *  - when m_implPtr == 0x0: Unlocked (no readers, no writers) and non-recursive.
 *  - when m_implPtr & 0x1: If the least significant bit is set, we are locked for read.
 *    In that case, m_implPtr>>4 represents the number of reading threads minus 1. No writers
 *    are waiting, and the lock is not recursive.
 *  - when m_implPtr == 0x2: We are locked for write and nobody is waiting. (no contention)
 *  - In any other case, m_implPtr points to an actual QReadWriteLockPrivate.
 */

using internal::ReadWriteLockPrivate;
using SystemClock = std::chrono::system_clock;

namespace
{

enum {
   StateMask = 0x3,
   StateLockedForRead = 0x1,
   StateLockedForWrite = 0x2,
};

ReadWriteLockPrivate *const DUMMY_LOCKED_FOR_READ = reinterpret_cast<ReadWriteLockPrivate *>(static_cast<pdk::uintptr>(StateLockedForRead));
ReadWriteLockPrivate *const DUMMY_LOCKED_FOR_WRITE = reinterpret_cast<ReadWriteLockPrivate *>(static_cast<pdk::uintptr>(StateLockedForWrite));

inline bool is_uncontended_locked(const ReadWriteLockPrivate *ptr)
{ 
   return reinterpret_cast<pdk::uintptr>(ptr) & StateMask; 
}

namespace {

using pdk::utils::internal::LockFreeListDefaultConstants;
using pdk::utils::internal::LockFreeList;

struct FreeListConstants : LockFreeListDefaultConstants
{
   enum { 
      BlockCount = 4, 
      MaxIndex=0xffff
   };
   static const int sm_sizes[BlockCount];
};

const int FreeListConstants::sm_sizes[FreeListConstants::BlockCount] = {
   16,
   128,
   1024,
   FreeListConstants::MaxIndex - (16 + 128 + 1024)
};

typedef LockFreeList<ReadWriteLockPrivate, FreeListConstants> FreeList;
PDK_GLOBAL_STATIC(FreeList, freeList);

}

} // anonymous

namespace internal {

bool ReadWriteLockPrivate::lockForRead(int timeout, std::unique_lock<std::mutex> &mutexLocker)
{
   typename SystemClock::time_point start;
   if (timeout > 0) {
      start = SystemClock::now();
   }
   while (m_waitingWriters || m_writerCount) {
      if (timeout == 0) {
         return false;
      }
      if (timeout > 0) {
         int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(SystemClock::now() - start).count();
         if (elapsed > timeout) {
            return false;
         }
         ++m_waitingReaders;
         m_readerCond.wait_for(mutexLocker, std::chrono::milliseconds(timeout - elapsed));
      } else {
         ++m_waitingReaders;
         m_readerCond.wait(mutexLocker);
      }
      --m_waitingReaders;
   }
   ++m_readerCount;
   PDK_ASSERT(m_writerCount == 0);
   return true;
}

bool ReadWriteLockPrivate::lockForWrite(int timeout, std::unique_lock<std::mutex> &mutexLocker)
{
   typename SystemClock::time_point start;
   if (timeout > 0) {
      start = SystemClock::now();
   }
   while (m_readerCount || m_writerCount) {
      if (timeout == 0) {
         return false;
      }
      if (timeout > 0) {
         int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(SystemClock::now() - start).count();
         if (elapsed > timeout) {
            if (m_waitingReaders && !m_waitingWriters && !m_writerCount) {
               // We timed out and now there is no more writers or waiting writers, but some
               // readers were queueud (probably because of us). Wake the waiting readers.
               m_readerCond.notify_all();
            }
            return false;
         }
         ++m_waitingWriters;
         m_writerCond.wait_for(mutexLocker, std::chrono::milliseconds(timeout - elapsed));
      } else {
         ++m_waitingWriters;
         m_writerCond.wait(mutexLocker);
      }
      --m_waitingWriters;
   }
   PDK_ASSERT(m_writerCount == 0);
   PDK_ASSERT(m_readerCount == 0);
   m_writerCount = 1;
   return true;
}

void ReadWriteLockPrivate::unlock(std::unique_lock<std::mutex> &mutexLocker)
{
   if (m_waitingWriters) {
      m_writerCond.notify_one();
   } else if (m_waitingReaders) {
      m_readerCond.notify_all();
   }
}

bool ReadWriteLockPrivate::recursiveLockForRead(int timeout)
{
   PDK_ASSERT(m_recursive);
   std::unique_lock<std::mutex> locker(m_mutex);
   std::thread::id self = std::this_thread::get_id();
   std::map<std::thread::id, int>::iterator iter = m_currentReaders.find(self);
   if (iter != m_currentReaders.end()) {
      iter->second = iter->second + 1;
      return true;
   }
   if (!lockForRead(timeout, locker)) {
      return false;
   }
   m_currentReaders[self] = 1;
   return true;
}

bool ReadWriteLockPrivate::recursiveLockForWrite(int timeout)
{
   PDK_ASSERT(m_recursive);
   std::unique_lock<std::mutex> locker(m_mutex);
   std::thread::id self = std::this_thread::get_id();
   if (m_currentWriter == self) {
      ++m_writerCount;
      return true;
   }
   if (!lockForWrite(timeout, locker)) {
      return false;
   }
   m_currentWriter = self;
   return true;
}

void ReadWriteLockPrivate::recursiveUnlock()
{
   PDK_ASSERT(m_recursive);
   std::unique_lock<std::mutex> locker(m_mutex);
   std::thread::id self = std::this_thread::get_id();
   if (self == m_currentWriter) {
      if (--m_writerCount > 0) {
         return;
      }
      m_writerCount = 0;
   } else {
      std::map<std::thread::id, int>::iterator iter = m_currentReaders.find(self);
      if (iter == m_currentReaders.end()) {
         // @TODO warning("ReadWriteLock::unlock: unlocking from a thread that did not lock");
         std::cerr << "ReadWriteLock::unlock: unlocking from a thread that did not lock" << std::endl;
         return;
      } else {
         // @TODO ref or value?
         if (--iter->second <= 0) {
            m_currentReaders.erase(iter);
            --m_readerCount;
         }
         if (m_readerCount) {
            return;
         }
      }
   }
   unlock(locker);
}

ReadWriteLockPrivate *ReadWriteLockPrivate::allocate()
{
   int i = freeList->next();
   ReadWriteLockPrivate *data = &(*freeList)[i];
   data->m_id = i;
   PDK_ASSERT(!data->m_recursive);
   PDK_ASSERT(!data->m_waitingReaders && !data->m_waitingWriters && 
              !data->m_readerCount && !data->m_writerCount);
   return data;
}

void ReadWriteLockPrivate::release()
{
   PDK_ASSERT(!m_recursive);
   PDK_ASSERT(!m_waitingReaders && !m_waitingWriters && !m_readerCount && !m_writerCount);
   freeList->release(m_id);
}

} // internal

ReadWriteLock::ReadWriteLock(RecursionMode recursionMode)
   : m_implPtr(recursionMode == RecursionMode::Recursion ? new ReadWriteLockPrivate(true) : nullptr)
{
   PDK_ASSERT_X(!(reinterpret_cast<pdk::uintptr>(m_implPtr.load()) & StateMask),
                "ReadWriteLock::ReadWriteLock", "bad m_implPtr alignment");
}

ReadWriteLock::~ReadWriteLock()
{
   auto data = m_implPtr.load();
   if (is_uncontended_locked(data)) {
      std::cerr << "ReadWriteLock: destroying locked ReadWriteLock" << std::endl;
      return;
   }
   delete data;
}

void ReadWriteLock::lockForRead()
{
   if (m_implPtr.testAndSetAcquire(nullptr, DUMMY_LOCKED_FOR_READ)) {
      return;
   }
   tryLockForRead(-1);
}

bool ReadWriteLock::tryLockForRead()
{
   return tryLockForRead(0);
}

bool ReadWriteLock::tryLockForRead(int timeout)
{
   ReadWriteLockPrivate *dptr = nullptr;
   
   if (m_implPtr.testAndSetAcquire(nullptr, DUMMY_LOCKED_FOR_READ, dptr)) {
      return true;
   }
   while (true) {
      if (nullptr == dptr) {
         if (!m_implPtr.testAndSetAcquire(nullptr, DUMMY_LOCKED_FOR_READ, dptr)) {
            continue;
         }
         return true;
      }
      if ((reinterpret_cast<pdk::uintptr>(dptr) & StateMask) == StateLockedForRead) {
         // locked for read, increase the counter
         const auto val = reinterpret_cast<ReadWriteLockPrivate *>(reinterpret_cast<pdk::uintptr>(dptr) + (1U << 4));
         PDK_ASSERT_X(reinterpret_cast<pdk::uintptr>(val) > (1U << 4),
                      "ReadWriteLock::tryLockForRead()",
                      "Overflow in lock counter");
         if (!m_implPtr.testAndSetAcquire(dptr, val, dptr)) {
            continue;
         }
         return true;
      }
      if (dptr == DUMMY_LOCKED_FOR_WRITE) {
         if (!timeout) {
            return false;
         }
         // locked for write, assign a m_implPtr and wait.
         ReadWriteLockPrivate * val = ReadWriteLockPrivate::allocate();
         val->m_writerCount = 1;
         if (!m_implPtr.testAndSetOrdered(dptr, val, dptr)) {
            val->m_writerCount = 0;
            val->release();
            continue;
         }
         dptr = val;
      }
      PDK_ASSERT(!is_uncontended_locked(dptr));
      // dptr is an actual pointer;
      if (dptr->m_recursive) {
         return dptr->recursiveLockForRead(timeout);
      }
      std::unique_lock<std::mutex> locker(dptr->m_mutex);
      if (dptr != m_implPtr.load()) {
         // m_implPtr has changed: this ReadWriteLock was unlocked before we had
         // time to lock m_implPtr->m_mutex.
         // We are holding a lock to a mutex within a ReadWriteLockPrivate
         // that is already released (or even is already re-used). That's ok
         // because the FreeList never frees them.
         // Just unlock m_implPtr->m_mutex (at the end of the scope) and retry.
         dptr = m_implPtr.loadAcquire();
         continue;
      }
      return dptr->lockForRead(timeout, locker);
   }
}

void ReadWriteLock::lockForWrite()
{
   tryLockForWrite(-1);
}

bool ReadWriteLock::tryLockForWrite()
{
   return tryLockForWrite(0);
}

bool ReadWriteLock::tryLockForWrite(int timeout)
{
   // Fast case: non contended:
   ReadWriteLockPrivate *dptr;
   if (m_implPtr.testAndSetAcquire(nullptr, DUMMY_LOCKED_FOR_WRITE, dptr)) {
      return true;
   }
   while (true) {
      if (nullptr == dptr) {
         if (!m_implPtr.testAndSetAcquire(dptr, DUMMY_LOCKED_FOR_WRITE, dptr)) {
            continue;
         }
         return true;
      }
      if (is_uncontended_locked(dptr)) {
         if (!timeout) {
            return false;
         }
         // locked for either read or write, assign a m_implPtr and wait.
         auto val = ReadWriteLockPrivate::allocate();
         if (dptr == DUMMY_LOCKED_FOR_WRITE) {
            val->m_writerCount = 1;
         } else {
            val->m_readerCount = (reinterpret_cast<pdk::uintptr>(dptr) >> 4) + 1;
         }
         if (!m_implPtr.testAndSetOrdered(dptr, val, dptr)) {
            val->m_writerCount = val->m_readerCount = 0;
            val->release();
            continue;
         }
         dptr = val;
      }
      PDK_ASSERT(!is_uncontended_locked(dptr));
      // dptr is an actual pointer;
      if (dptr->m_recursive) {
         return dptr->recursiveLockForWrite(timeout);
      }
      std::unique_lock<std::mutex> locker(dptr->m_mutex);
      if (dptr != m_implPtr.load()) {
         // The mutex was unlocked before we had time to lock the mutex.
         // We are holding to a mutex within a ReadWriteLockPrivate that is already released
         // (or even is already re-used) but that's ok because the FreeList never frees them.
         dptr = m_implPtr.loadAcquire();
         continue;
      }
      return dptr->lockForWrite(timeout, locker);
   }
}

void ReadWriteLock::unlock()
{
   ReadWriteLockPrivate *dptr = m_implPtr.loadAcquire();
   while (true) {
      PDK_ASSERT_X(dptr, "ReadWriteLock::unlock()", "Cannot unlock an unlocked lock");
      // Fast case: no contention: (no waiters, no other readers)
      if (reinterpret_cast<pdk::uintptr>(dptr) <= 2) {
         // 1 or 2 (StateLockedForRead or StateLockedForWrite)
         if (!m_implPtr.testAndSetOrdered(dptr, nullptr, dptr)) {
            continue;  
         }
         return;
      }
      
      if (reinterpret_cast<pdk::uintptr>(dptr) & StateLockedForRead) {
         PDK_ASSERT(reinterpret_cast<pdk::uintptr>(dptr) > (1U << 4));// otherwise that would be the fast case
         auto val = reinterpret_cast<ReadWriteLockPrivate *>(reinterpret_cast<pdk::uintptr>(dptr) - (1U<<4));
         if (!m_implPtr.testAndSetOrdered(dptr, val, dptr)) {
            continue;
         }
         return;
      }
      
      PDK_ASSERT(!is_uncontended_locked(dptr));
      if (dptr->m_recursive) {
         dptr->recursiveUnlock();
         return;
      }
      std::unique_lock<std::mutex> locker(dptr->m_mutex);
      if (dptr->m_writerCount) {
         PDK_ASSERT(dptr->m_writerCount == 1);
         PDK_ASSERT(dptr->m_readerCount == 0);
         dptr->m_writerCount = 0;
      } else {
         PDK_ASSERT(dptr->m_readerCount > 0);
         --dptr->m_readerCount;
         if (dptr->m_readerCount > 0) {
            return;
         }
      }
      if (dptr->m_waitingReaders || dptr->m_waitingWriters) {
         dptr->unlock(locker);
      } else {
         PDK_ASSERT(m_implPtr.load() == dptr);
         m_implPtr.storeRelease(nullptr);
         dptr->release();
      }
      return;
   }
}

ReadWriteLock::StateForWaitCondition ReadWriteLock::stateForWaitCondition() const
{
   ReadWriteLockPrivate *dptr = m_implPtr.load();
   switch (reinterpret_cast<pdk::uintptr>(dptr) & StateMask) {
   case StateLockedForRead:
      return StateForWaitCondition::LockedForRead;
   case StateLockedForWrite:
      return StateForWaitCondition::LockedForWrite;
   }
   if (!dptr) {
      return StateForWaitCondition::Unlocked;
   } else if (dptr->m_writerCount > 1) {
      return StateForWaitCondition::RecursivelyLocked;
   } else if (dptr->m_writerCount == 1) {
      return StateForWaitCondition::LockedForWrite;
   }
   return StateForWaitCondition::LockedForRead;
}

} // thread
} // os
} // pdk
