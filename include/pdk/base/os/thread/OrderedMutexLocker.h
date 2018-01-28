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
// Created by softboy on 2018/01/27.

#ifndef PDK_M_BASE_OS_THREAD_ORDERED_MUTEX_LOCKER_LOCK_H
#define PDK_M_BASE_OS_THREAD_ORDERED_MUTEX_LOCKER_LOCK_H

#include "pdk/global/Global.h"

#include <mutex>

namespace pdk {
namespace os {
namespace thread {

// Locks 2 mutexes in a defined order, avoiding a recursive lock if
// we're trying to lock the same mutex twice.
class OrderedMutexLocker
{
public:
   OrderedMutexLocker(std::mutex *mutex1, std::mutex *mutex2)
      : m_mutex1((mutex1 == mutex2) ? mutex1 : (std::less<std::mutex *>()(mutex1, mutex2) ? mutex1 : mutex2)),
        m_mutex2((mutex1 == mutex2) ?  0 : (std::less<std::mutex *>()(mutex1, mutex2) ? mutex2 : mutex1)),
        m_locked(false)
   {
      relock();
   }
   ~OrderedMutexLocker()
   {
      unlock();
   }
   
   void relock()
   {
      if (!locked) {
         if (m_mutex1) {
            m_mutex1->lock();
         }
         if (m_mutex2) {
            m_mutex2->lock();
         }
         locked = true;
      }
   }
   
   void unlock()
   {
      if (locked) {
         if (m_mutex2) {
            m_mutex2->unlock();
         }
         if (m_mutex1) {
            m_mutex1->unlock();
         }
         locked = false;
      }
   }
   
   static bool relock(std::mutex *m_mutex1, std::mutex *m_mutex2)
   {
      // m_mutex1 is already locked, m_mutex2 not... do we need to unlock and relock?
      if (m_mutex1 == m_mutex2)
         return false;
      if (std::less<std::mutex *>()(m_mutex1, m_mutex2)) {
         m_mutex2->lock();
         return true;
      }
      if (!m_mutex2->try_lock()) {
         m_mutex1->unlock();
         m_mutex2->lock();
         m_mutex1->lock();
      }
      return true;
   }
   
private:
   std::mutex *m_mutex1;
   std::mutex *m_mutex2;
   bool m_locked;
};

} // thread
} // os
} // pdk


#endif // PDK_M_BASE_OS_THREAD_ORDERED_MUTEX_LOCKER_LOCK_H
