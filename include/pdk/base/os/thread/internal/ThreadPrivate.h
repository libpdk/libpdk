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

} // internal
} // thread
} // os
} // pdk

PDK_DECLARE_TYPEINFO(pdk::os::thread::internal::PostEvent, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_OS_THREAD_INTERNAL_THREAD_PRIVATE_H
