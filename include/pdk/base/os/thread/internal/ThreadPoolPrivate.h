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
// Created by softboy on 2018/03/02.

#ifndef PDK_M_BASE_OS_THREAD_INTERNAL_THREAD_POOL_PRIVATE_H
#define PDK_M_BASE_OS_THREAD_INTERNAL_THREAD_POOL_PRIVATE_H

#include "pdk/base/os/thread/Thread.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include <mutex>
#include <condition_variable>
#include <set>
#include <deque>

namespace pdk {
namespace os {
namespace thread {

// forward declare class
class Runnable;

namespace internal {

using pdk::kernel::internal::ObjectPrivate;

class QueuePage {
public:
   enum {
      MaxPageSize = 256
   };
   
   QueuePage(Runnable *runnable, int pri)
      : m_priority(pri)
   {
      push(runnable);
   }
   
   bool isFull()
   {
      return m_lastIndex >= MaxPageSize - 1;
   }
   
   bool isFinished()
   {
      return m_firstIndex > m_lastIndex;
   }
   
   void push(Runnable *runnable) {
      PDK_ASSERT(runnable != nullptr);
      PDK_ASSERT(!isFull());
      m_lastIndex += 1;
      m_entries[m_lastIndex] = runnable;
   }
   
   void skipToNextOrEnd()
   {
      while (!isFinished() && m_entries[m_firstIndex] == nullptr) {
         m_firstIndex += 1;
      }
   }
   
   Runnable *first()
   {
      PDK_ASSERT(!isFinished());
      Runnable *runnable = m_entries[m_firstIndex];
      PDK_ASSERT(runnable);
      return runnable;
   }
   
   Runnable *pop() {
      PDK_ASSERT(!isFinished());
      Runnable *runnable = first();
      PDK_ASSERT(runnable);
      // clear the entry although this should not be necessary
      m_entries[m_firstIndex] = nullptr;
      m_firstIndex += 1;
      // make sure the next runnable returned by first() is not a nullptr
      skipToNextOrEnd();
      return runnable;
   }
   
   bool tryTake(Runnable *runnable)
   {
      PDK_ASSERT(!isFinished());
      for (int i = m_firstIndex; i <= m_lastIndex; i++) {
         if (m_entries[i] == runnable) {
            m_entries[i] = nullptr;
            if (i == m_firstIndex) {
               // make sure first() does not return a nullptr
               skipToNextOrEnd();
            }
            return true;
         }
      }
      return false;
   }
   
   int getPriority() const
   {
      return m_priority;
   }
   
private:
   int m_priority = 0;
   int m_firstIndex = 0;
   int m_lastIndex = -1;
   Runnable *m_entries[MaxPageSize];
};

class ThreadPoolThread;
class PDK_CORE_EXPORT ThreadPoolPrivate : public ObjectPrivate
{
   PDK_DECLARE_PUBLIC(ThreadPool);
   friend class ThreadPoolThread;
   
public:
   ThreadPoolPrivate();
   
   bool tryStart(Runnable *task);
   void enqueueTask(Runnable *task, int priority = 0);
   int getActiveThreadCount() const;
   
   void tryToStartMoreThreads();
   bool tooManyThreadsActive() const;
   
   void startThread(Runnable *runnable = 0);
   void reset();
   bool waitForDone(int msecs);
   void clear();
   void stealAndRunRunnable(Runnable *runnable);
   void deletePageIfFinished(QueuePage *page);
   
   mutable std::mutex m_mutex;
   std::list<ThreadPoolThread *> m_allThreads;
   std::deque<ThreadPoolThread *> m_waitingThreads;
   std::deque<ThreadPoolThread *> m_expiredThreads;
   std::deque<QueuePage*> m_queue;
   std::condition_variable m_noActiveThreads;
   
   int m_expiryTimeout = 30000;
   int m_maxThreadCount = Thread::getIdealThreadCount();
   int m_reservedThreads = 0;
   int m_activeThreads = 0;
   uint m_stackSize = 0;
   bool m_isExiting = false;
};

} // internal
} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_INTERNAL_THREAD_POOL_PRIVATE_H
