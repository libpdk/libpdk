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

} // internal
} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_INTERNAL_THREAD_PRIVATE_H
