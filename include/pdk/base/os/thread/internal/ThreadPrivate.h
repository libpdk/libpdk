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
// Created by softboy on 2017/01/24.

#ifndef PDK_M_BASE_OS_THREAD_INTERNAL_THREAD_PRIVATE_H
#define PDK_M_BASE_OS_THREAD_INTERNAL_THREAD_PRIVATE_H

#include "pdk/base/os/thread/Thread.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/base/os/thread/Atomic.h"
#include <stack>
#include <map>
#include <mutex>
#include <vector>
#include <condition_variable>

namespace pdk {

namespace kernel {
class AbstractEventDispatcher;
class EventLoop;
class Event;
} // kernel

namespace os {
namespace thread {
namespace internal {

using pdk::kernel::Event;
using pdk::kernel::Object;
using pdk::kernel::EventLoop;
using pdk::kernel::AbstractEventDispatcher;
using pdk::kernel::internal::ObjectPrivate;
using pdk::os::thread::AtomicInt;

class PostEvent
{
public:
   inline PostEvent()
      : m_receiver(nullptr),
        m_event(nullptr),
        m_priority(0)
   {}
   
   inline PostEvent(Object *r, Event *event, int priority)
      : m_receiver(r),
        m_event(event),
        m_priority(priority)
   {}
   
public:
   Object *m_receiver;
   Event *m_event;
   int m_priority;
};

inline bool operator<(const PostEvent &lhs, const PostEvent &rhs)
{
   return lhs.m_priority > rhs.m_priority;
}

// This class holds the list of posted events.
//  The list has to be kept sorted by priority
class PostEventList : public std::vector<PostEvent>
{
public:   
   inline PostEventList()
      : std::vector<PostEvent>(),
        m_recursion(0),
        m_startOffset(0),
        m_insertionOffset(0)
   {}
   
   void addEvent(const PostEvent &event) {
      int priority = event.m_priority;
      if (isEmpty() ||
          (*--cend()).m_priority >= priority ||
          m_insertionOffset >= size()) {
         // optimization: we can simply append if the last event in
         // the queue has higher or equal priority
         append(event);
      } else {
         // insert event in descending priority order, using upper
         // bound for a given priority (to ensure proper ordering
         // of events with the same priority)
         PostEventList::iterator at = std::upper_bound(begin() + m_insertionOffset, end(), event);
         insert(at, event);
      }
   }
   
public:
   // recursion == recursion count for sendPostedEvents()
   int m_recursion;
   // sendOffset == the current event to start sending
   int m_startOffset;
   // insertionOffset == set by sendPostedEvents to tell postEvent() where to start insertions
   int m_insertionOffset;
   std::mutex m_mutex;
private:
   //hides because they do not keep that list sorted. addEvent must be used
   using std::vector<PostEvent>::push_back;
   using std::vector<PostEvent>::insert;
};

class PDK_CORE_EXPORT DaemonThread : public Thread
{
public:
   DaemonThread(Object *parent = nullptr);
   ~DaemonThread();
};

class ThreadPrivate : public ObjectPrivate
{
   PDK_DECLARE_PUBLIC(Thread);
public:
   static Thread *threadForId(int id);
   static void createEventDispatcher(ThreadData *data);
#ifdef PDK_OS_WIN
   static unsigned int __stdcall start(void *);
   static void finish(void *, bool lockAnyway=true);
#endif // PDK_OS_WIN
#ifdef PDK_OS_UNIX
   static void *start(void *arg);
   static void finish(void *);
#endif // PDK_OS_UNIX
   
public:
   ThreadPrivate(ThreadData *d = 0);
   ~ThreadPrivate();
   void setPriority(Thread::Priority priority);
   void ref()
   {
      m_quitLockRef.ref();
   }
   
   void deref()
   {
      if (!m_quitLockRef.deref() && m_running) {
         CoreApplication::getInstance()->postEvent(m_apiPtr, new Event(Event::Type::Quit));
      }
   }
   
public:
#ifdef PDK_OS_UNIX
   std::condition_variable m_threadDone;
#endif // PDK_OS_UNIX
#ifdef PDK_OS_WIN
   pdk::HANDLE m_handle;
   unsigned int m_id;
   int m_waiters;
   bool m_terminationEnabled;
   bool m_terminatePending;
#endif // PDK_OS_WIN
   ThreadData *m_data;
   mutable std::mutex m_mutex;
   AtomicInt m_quitLockRef;   
   bool m_running;
   bool m_finished;
   bool m_isInFinish; //when in ThreadPrivate::finish
   bool m_interruptionRequested;
   bool m_exited;
   int m_returnCode;
   uint m_stackSize;
   Thread::Priority m_priority;
};

class ThreadData
{
public:
   ThreadData(int initialRefCount = 1);
   ~ThreadData();
   
   static PDK_UNITTEST_EXPORT ThreadData *current(bool createIfNecessary = true);
   static void clearCurrentThreadData();
   static QThreadData *get(Thread *thread)
   {
      PDK_ASSERT_X(thread != 0, "Thread", "internal error");
      return thread->getImplPtr()->m_data;
   }
   void ref();
   void deref();
   inline bool hasEventDispatcher() const
   {
      return m_eventDispatcher.load() != 0;
   }
   
   bool canWaitLocked()
   {
      std::scoped_lock locker(&m_postEventList.m_mutex);
      return m_canWait;
   }
   
   // This class provides per-thread (by way of being a QThreadData
   // member) storage for qFlagLocation()
   class FlaggedDebugSignatures
   {
      static constexpr uint COUNT = 2;
      uint m_idx;
      const char* m_locations[Count];
      
   public:
      FlaggedDebugSignatures()
         : m_idx(0)
      {
         std::fill_n(m_locations, COUNT, static_cast<char *>(nullptr));
      }
      
      void store(const char *method)
      {
         m_locations[m_idx++ % COUNT] = method;
      }
      
      bool contains(const char *method) const
      {
         return std::find(m_locations, m_locations + COUNT, method) != m_locations + COUNT;
      }
   };
   
public:
   int m_loopLevel;
   int m_scopeLevel;
   
   std::stack<EventLoop *> m_eventLoops;
   PostEventList m_postEventList;
   AtomicPointer<Thread> m_thread;
   pdk::HANDLE m_threadId;
   AtomicPointer<AbstractEventDispatcher> m_eventDispatcher;
   std::vector<void *> m_tls;
   
   FlaggedDebugSignatures m_flaggedSignatures;
   
   bool m_quitNow;
   bool m_canWait;
   bool m_isAdopted;
   bool m_requiresCoreApplication;
   
private:
   AtomicInt m_ref;
};

class ScopedScopeLevelCounter
{
   ThreadData *m_threadData;
public:
   inline ScopedScopeLevelCounter(ThreadData *threadData)
      : m_threadData(threadData)
   {
      ++threadData->m_scopeLevel;
   }
   
   inline ~ScopedScopeLevelCounter()
   {
      --threadData->m_scopeLevel;
   }
};

// thread wrapper for the main() thread
class AdoptedThread : public Thread
{
   PDK_DECLARE_PRIVATE(Thread);
   
public:
   AdoptedThread(ThreadData *data = nullptr);
   ~AdoptedThread();
   void init();
   
private:
   void run() override;
};

} // internal
} // thread
} // os
} // pdk

PDK_DECLARE_TYPEINFO(pdk::os::thread::internal::PostEvent, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_OS_THREAD_INTERNAL_THREAD_PRIVATE_H
