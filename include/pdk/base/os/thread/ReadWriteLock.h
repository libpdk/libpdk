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

#ifndef PDK_M_BASE_OS_THREAD_READWRITE_LOCK_H
#define PDK_M_BASE_OS_THREAD_READWRITE_LOCK_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Atomic.h"

namespace pdk {
namespace os {
namespace thread {

namespace internal {
class ReadWriteLockPrivate;
} // internal

class PDK_CORE_EXPORT ReadWriteLock
{
public:
   enum class RecursionMode
   {
      NonRecursion,
      Recursion
   };
   
   explicit ReadWriteLock(RecursionMode recursionMode = RecursionMode::NonRecursion);
   ~ReadWriteLock();
   
   void lockForRead();
   bool tryLockForRead();
   bool tryLockForRead(int timeout);
   
   void lockForWrite();
   bool tryLockForWrite();
   bool tryLockForWrite(int timeout);
   
   void unlock();
   
private:
   PDK_DISABLE_COPY(ReadWriteLock);
   AtomicPointer<internal::ReadWriteLockPrivate> m_implPtr;
   enum class StateForWaitCondition
   {
      LockedForRead,
      LockedForWrite,
      Unlocked,
      RecursivelyLocked
   };
   StateForWaitCondition stateForWaitCondition() const;
};

#if defined(PDK_CC_MSVC)
#pragma warning( push )
#pragma warning( disable : 4312 ) // ignoring the warning from /Wp64
#endif

class PDK_CORE_EXPORT ReadLocker
{
public:
   inline ReadLocker(ReadWriteLock *readWriteLock);
   inline ~ReadLocker()
   {
      unlock();
   }
   
   inline void unlock()
   {
      if (m_qval) {
         if ((m_qval & static_cast<pdk::uintptr>(1u)) == static_cast<pdk::uintptr>(1u)){
            m_qval &= ~static_cast<pdk::uintptr>(1u);
            readWriteLock()->unlock();
         }
      }
   }
   
   inline void relock()
   {
      if (m_qval) {
         if ((m_qval & static_cast<pdk::uintptr>(1u)) == static_cast<pdk::uintptr>(0u)) {
            readWriteLock()->lockForRead();
            m_qval |= static_cast<pdk::uintptr>(1u);
         }
      }
   }
   
   inline ReadWriteLock *readWriteLock() const
   {
      return reinterpret_cast<ReadWriteLock *>(m_qval & ~static_cast<pdk::uintptr>(1u));
   }
   
private:
   PDK_DISABLE_COPY(ReadLocker);
   pdk::uintptr m_qval;
};

inline ReadLocker::ReadLocker(ReadWriteLock *readWriteLock)
   : m_qval(reinterpret_cast<pdk::uintptr>(readWriteLock))
{
   PDK_ASSERT_X((m_qval & static_cast<pdk::uintptr>(1u)) == static_cast<pdk::uintptr>(0u),
                "ReadLocker", "ReadWriteLock pointer is misaligned");
   relock();
}

class PDK_CORE_EXPORT WriteLocker
{
public:
   inline WriteLocker(ReadWriteLock *readWriteLock);
   inline ~WriteLocker()
   {
      unlock();
   }
   
   inline void unlock()
   {
      if (m_qval) {
         if ((m_qval & static_cast<pdk::uintptr>(1u)) == static_cast<pdk::uintptr>(1u)){
            m_qval &= ~static_cast<pdk::uintptr>(1u);
            readWriteLock()->unlock();
         }
      }
   }
   
   inline void relock()
   {
      if (m_qval) {
         if ((m_qval & static_cast<pdk::uintptr>(1u)) == static_cast<pdk::uintptr>(0u)) {
            readWriteLock()->lockForWrite();
            m_qval |= static_cast<pdk::uintptr>(1u);
         }
      }
   }
   
   inline ReadWriteLock *readWriteLock() const
   {
      return reinterpret_cast<ReadWriteLock *>(m_qval & ~static_cast<pdk::uintptr>(1u));
   }
   
private:
   PDK_DISABLE_COPY(WriteLocker);
   pdk::uintptr m_qval;
};

inline WriteLocker::WriteLocker(ReadWriteLock *readWriteLock)
   : m_qval(reinterpret_cast<pdk::uintptr>(readWriteLock))
{
   PDK_ASSERT_X((m_qval & static_cast<pdk::uintptr>(1u)) == static_cast<pdk::uintptr>(0u),
                "ReadLocker", "ReadWriteLock pointer is misaligned");
   relock();
}

#if defined(PDK_CC_MSVC)
#pragma warning( pop )
#endif

} // pdk
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_READWRITE_LOCK_H
