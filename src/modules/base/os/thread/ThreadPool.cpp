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

#include "pdk/base/os/thread/ThreadPool.h"
#include "pdk/base/os/thread/internal/ThreadPoolPrivate.h"
#include "pdk/base/lang/String.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/global/Logging.h"
#include "pdk/global/Random.h"
#include "pdk/stdext/utility/Algorithms.h"
#include <algorithm>
#include <chrono>

namespace pdk {
namespace os {
namespace thread {

using internal::ThreadPoolPrivate;
using internal::QueuePage;
using pdk::lang::Latin1String;
using pdk::kernel::ElapsedTimer;

PDK_GLOBAL_STATIC(ThreadPool, sg_theInstance);

namespace internal {

/*
 * Thread wrapper, provides synchronization against a ThreadPool
 */
class ThreadPoolThread : public Thread
{
public:
   ThreadPoolThread(ThreadPoolPrivate *manager);
   void run() override;
   void registerThreadInactive();
   
   std::condition_variable m_runnableReady;
   ThreadPoolPrivate *m_manager;
   Runnable *m_runnable;
};

ThreadPoolThread::ThreadPoolThread(ThreadPoolPrivate *manager)
   : m_manager(manager),
     m_runnable(nullptr)
{
   setStackSize(manager->m_stackSize);
}

void ThreadPoolThread::run()
{
   std::unique_lock<std::mutex> locker(m_manager->m_mutex);
   for(;;) {
      Runnable *runnable = m_runnable;
      m_runnable = nullptr;
      do {
         if (runnable) {
            const bool autoDelete = runnable->autoDelete();
            // run the task
            locker.unlock();
            try {
               runnable->run();
            } catch (...) {
               warning_stream("pdk Concurrent has caught an exception thrown from a worker thread.\n"
                              "This is not supported, exceptions thrown in worker threads must be\n"
                              "caught before control returns to Qt Concurrent.");
               registerThreadInactive();
               throw;
            }
            locker.lock();
            if (autoDelete && !--runnable->m_ref) {
               delete runnable;
            }
         }
         // if too many threads are active, expire this thread
         if (m_manager->tooManyThreadsActive()) {
            break;
         }
         if (m_manager->m_queue.empty()) {
            runnable = nullptr;
            break;
         }
         QueuePage *page = m_manager->m_queue.front();
         runnable = page->pop();
         if (page->isFinished()) {
            m_manager->m_queue.pop_front();
            delete page;
         }
      } while (true);
      
      if (m_manager->m_isExiting) {
         registerThreadInactive();
         break;
      }
      
      // if too many threads are active, expire this thread
      bool expired = m_manager->tooManyThreadsActive();
      if (!expired) {
         m_manager->m_waitingThreads.push_back(this);
         registerThreadInactive();
         // wait for work, exiting after the expiry timeout is reached
         // @TODO issue not fixed
         // here the fix method is a temporary workaround, not very good, but it works
         // i will come back
         m_runnableReady.wait_for(locker, 
                                  std::chrono::milliseconds(m_manager->m_expiryTimeout + pdk::RandomGenerator::global()->bounded(64)));
         ++m_manager->m_activeThreads;
         auto iter = std::find(m_manager->m_waitingThreads.begin(), 
                               m_manager->m_waitingThreads.end(), this);
         if (iter != m_manager->m_waitingThreads.end()) {
            size_t oldSize = m_manager->m_waitingThreads.size();
            m_manager->m_waitingThreads.erase(iter);
            if (oldSize != m_manager->m_waitingThreads.size()){
               expired = true;
            }
         }
      }
      if (expired) {
         m_manager->m_expiredThreads.push_back(this);
         registerThreadInactive();
         break;
      }
   }
}

void ThreadPoolThread::registerThreadInactive()
{
   if (--m_manager->m_activeThreads == 0) {
      m_manager->m_noActiveThreads.notify_all();
   } 
}

ThreadPoolPrivate:: ThreadPoolPrivate()
{}

bool ThreadPoolPrivate::tryStart(Runnable *task)
{
   PDK_ASSERT(task != nullptr);
   if (m_allThreads.empty()) {
      // always create at least one thread
      startThread(task);
      return true;
   }
   // can't do anything if we're over the limit
   if (getActiveThreadCount() >= m_maxThreadCount) {
      return false;
   }
   if (!m_waitingThreads.empty()) {
      // recycle an available thread
      enqueueTask(task);
      ThreadPoolThread *thread = m_waitingThreads.front();
      m_waitingThreads.pop_front();
      thread->m_runnableReady.notify_one();
      return true;
   }
   if (!m_expiredThreads.empty()) {
      // restart an expired thread
      ThreadPoolThread *thread = m_expiredThreads.front();
      m_expiredThreads.pop_front();
      PDK_ASSERT(thread->m_runnable == nullptr);
      ++m_activeThreads;
      if (task->autoDelete()) {
         ++task->m_ref;
      }
      thread->m_runnable = task;
      thread->start();
      return true;
   }
   
   // start a new thread
   startThread(task);
   return true;
}

inline bool compare_priority(int priority, const QueuePage *p)
{
   return p->getPriority() < priority;
}

void ThreadPoolPrivate::enqueueTask(Runnable *runnable, int priority)
{
   PDK_ASSERT(runnable != nullptr);
   if (runnable->autoDelete()) {
      ++runnable->m_ref;
   }
   
   for (QueuePage *page : std::as_const(m_queue)) {
      if (page->getPriority() == priority && !page->isFull()) {
         page->push(runnable);
         return;
      }
   }
   auto iter = std::upper_bound(m_queue.cbegin(), m_queue.cend(), priority, compare_priority);
   m_queue.insert(iter, new QueuePage(runnable, priority));
}

int ThreadPoolPrivate::getActiveThreadCount() const
{
   return (m_allThreads.size()
           - m_expiredThreads.size()
           - m_waitingThreads.size()
           + m_reservedThreads);
}

void ThreadPoolPrivate::tryToStartMoreThreads()
{
   // try to push tasks on the queue to any available threads
   while (!m_queue.empty()) {
      QueuePage *page = m_queue.front();
      if (!tryStart(page->first())) {
         break;
      }
      page->pop();
      if (page->isFinished()) {
         m_queue.pop_front();
         delete page;
      }
   }
}

bool ThreadPoolPrivate::tooManyThreadsActive() const
{
   const int activeThreadCount = this->getActiveThreadCount();
   return activeThreadCount > m_maxThreadCount && (activeThreadCount - m_reservedThreads) > 1;
}

void ThreadPoolPrivate::startThread(Runnable *runnable)
{
   PDK_ASSERT(runnable != nullptr);
   pdk::utils::ScopedPointer <ThreadPoolThread> thread(new ThreadPoolThread(this));
   thread->setObjectName(Latin1String("Thread (pooled)"));
   PDK_ASSERT(m_allThreads.end() == std::find(m_allThreads.begin(), m_allThreads.end(), thread.getData())); 
   // if this assert hits, we have an ABA problem (deleted threads don't get removed here)
   m_allThreads.push_back(thread.getData());
   ++m_activeThreads;
   if (runnable->autoDelete()) {
      ++runnable->m_ref;
   }
   thread->m_runnable = runnable;
   thread.take()->start();
}

void ThreadPoolPrivate::reset()
{
   std::unique_lock<std::mutex> locker(m_mutex);
   m_isExiting = true;
   while (!m_allThreads.empty()) {
      // move the contents of the set out so that we can iterate without the lock
      std::list<ThreadPoolThread *> allThreadsCopy;
      allThreadsCopy.swap(m_allThreads);
      locker.unlock();
      for (ThreadPoolThread *thread : std::as_const(allThreadsCopy)) {
         thread->m_runnableReady.notify_all();
         thread->wait();
         delete thread;
      }
      locker.lock();
      // repeat until all newly arrived threads have also completed
   }
   m_waitingThreads.clear();
   m_expiredThreads.clear();
   m_isExiting = false;
}

bool ThreadPoolPrivate::waitForDone(int msecs)
{
   std::unique_lock<std::mutex> locker(m_mutex);
   if (msecs < 0) {
      while (!(m_queue.empty() && m_activeThreads == 0)) {
         m_noActiveThreads.wait(locker);
      } 
   } else {
      ElapsedTimer timer;
      timer.start();
      int t;
      while (!(m_queue.empty() && m_activeThreads == 0) &&
             ((t = msecs - timer.getElapsed()) > 0)) {
         m_noActiveThreads.wait_for(locker, std::chrono::milliseconds(t));
      }
   }
   return m_queue.empty() && m_activeThreads == 0;
}

void ThreadPoolPrivate::clear()
{
   std::unique_lock<std::mutex> locker(m_mutex);
   for (QueuePage *page : std::as_const(m_queue)) {
      while (!page->isFinished()) {
         Runnable *runnable = page->pop();
         if (runnable && runnable->autoDelete() && !--runnable->m_ref) {
            delete runnable;
         }
      }
   }
   pdk::stdext::delete_all(m_queue);
   m_queue.clear();
}

void ThreadPoolPrivate::stealAndRunRunnable(Runnable *runnable)
{
   PDK_Q(ThreadPool);
   if (!apiPtr->tryTake(runnable)) {
      return;
   }
   const bool del = runnable->autoDelete() && !runnable->m_ref; // tryTake already deref'ed
   runnable->run();
   if (del) {
      delete runnable;
   }
}

} // internal

bool ThreadPool::tryTake(Runnable *runnable)
{
   PDK_D(ThreadPool);
   if (runnable == nullptr) {
      return false;
   }
   {
      std::unique_lock<std::mutex> locker(implPtr->m_mutex);
      for (QueuePage *page : std::as_const(implPtr->m_queue)) {
         if (page->tryTake(runnable)) {
            if (page->isFinished()) {
               auto iter = std::find(implPtr->m_queue.begin(), implPtr->m_queue.end(), page);
               if (iter != implPtr->m_queue.end()) {
                  implPtr->m_queue.erase(iter);
               }
               delete page;
            }
            if (runnable->autoDelete()) {
               --runnable->m_ref; // undo ++ref in start()
            }
            return true;
         }
      }
   }
   return false;
}

ThreadPool::ThreadPool(Object *parent)
   : Object(*new ThreadPoolPrivate, parent)
{}

ThreadPool::~ThreadPool()
{
   waitForDone();
}

ThreadPool *ThreadPool::getGlobalInstance()
{
   return sg_theInstance();
}

void ThreadPool::start(Runnable *runnable, int priority)
{
   if (!runnable) {
      return;
   }
   PDK_D(ThreadPool);
   std::unique_lock<std::mutex> locker(implPtr->m_mutex);
   if (!implPtr->tryStart(runnable)) {
      implPtr->enqueueTask(runnable, priority);
      if (!implPtr->m_waitingThreads.empty()) {
         ThreadPoolThread *thread = implPtr->m_waitingThreads.front();
         implPtr->m_waitingThreads.pop_front();
         thread->m_runnableReady.notify_one();
      }
   }
   
}

bool ThreadPool::tryStart(Runnable *runnable)
{
   if (!runnable) {
      return false;
   }
   PDK_D(ThreadPool);
   std::lock_guard<std::mutex> locker(implPtr->m_mutex);
   if (!implPtr->m_allThreads.empty() && 
       implPtr->getActiveThreadCount() >= implPtr->m_maxThreadCount) {
      return false;
   }  
   return implPtr->tryStart(runnable);
}

int ThreadPool::getExpiryTimeout() const
{
   PDK_D(const ThreadPool);
   return implPtr->m_expiryTimeout;
}

void ThreadPool::setExpiryTimeout(int expiryTimeout)
{
   PDK_D(ThreadPool);
   if (implPtr->m_expiryTimeout == expiryTimeout) {
      return;
   } 
   implPtr->m_expiryTimeout = expiryTimeout;
}

int ThreadPool::getMaxThreadCount() const
{
   PDK_D(const ThreadPool);
   return implPtr->m_maxThreadCount;
}

void ThreadPool::setMaxThreadCount(int maxThreadCount)
{
   PDK_D(ThreadPool);
   std::lock_guard<std::mutex> locker(implPtr->m_mutex);
   if (maxThreadCount == implPtr->m_maxThreadCount) {
      return;
   }
   implPtr->m_maxThreadCount = maxThreadCount;
   implPtr->tryToStartMoreThreads();
}

int ThreadPool::getActiveThreadCount() const
{
   PDK_D(const ThreadPool);
   std::lock_guard<std::mutex> locker(implPtr->m_mutex);
   return implPtr->getActiveThreadCount();
}

void ThreadPool::reserveThread()
{
   PDK_D(ThreadPool);
   std::lock_guard<std::mutex> locker(implPtr->m_mutex);
   ++implPtr->m_reservedThreads;
}

void ThreadPool::setStackSize(uint stackSize)
{
   PDK_D(ThreadPool);
   implPtr->m_stackSize = stackSize;
}

uint ThreadPool::getStackSize() const
{
   PDK_D(const ThreadPool);
   return implPtr->m_stackSize;
}

void ThreadPool::releaseThread()
{
   PDK_D(ThreadPool);
   std::lock_guard<std::mutex> locker(implPtr->m_mutex);
   --implPtr->m_reservedThreads;
   implPtr->tryToStartMoreThreads();
}

bool ThreadPool::waitForDone(int msecs)
{
   PDK_D(ThreadPool);
   bool rc = implPtr->waitForDone(msecs);
   if (rc) {
      implPtr->reset();
   }
   return rc;
}

void ThreadPool::clear()
{
   PDK_D(ThreadPool);
   implPtr->clear();
}

} // thread
} // os
} // pdk
