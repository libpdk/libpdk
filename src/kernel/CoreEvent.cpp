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
// Created by softboy on 2018/01/23.

#include "pdk/kernel/CoreEvent.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"

namespace pdk {
namespace kernel {

namespace {

using pdk::os::thread::BasicAtomicInteger;
using pdk::kernel::CoreApplication;
using pdk::kernel::CoreApplicationPrivate;

template <size_t N>
struct BasicAtomicBitField
{
   enum {
      BitsPerInt = std::numeric_limits<uint>::digits,
      NumInts = (N + BitsPerInt - 1) / BitsPerInt,
      NumBits = N
   };
   
   BasicAtomicInteger<uint> m_next;
   BasicAtomicInteger<uint> m_data[NumInts];
   
   bool allocateSpecific(int which) noexcept
   {
      BasicAtomicInteger<uint> &entry = m_data[which / BitsPerInt];
      const uint old = entry.load();
      const uint bit = 1U << (which % BitsPerInt);
      return !(old & bit) // wasn't taken
            && entry.testAndSetRelaxed(old, old | bit); // still wasn't taken
      
      // don't update 'next' here - it's unlikely that it will need
      // to be updated, in the general case, and having 'next'
      // trailing a bit is not a problem, as it is just a starting
      // hint for allocateNext(), which, when wrong, will just
      // result in a few more rounds through the allocateNext()
      // loop.
   }
   
   int allocateNext() noexcept
   {
      // Unroll loop to iterate over ints, then bits? Would save
      // potentially a lot of cmpxchgs, because we can scan the
      // whole int before having to load it again.
      
      // Then again, this should never execute many iterations, so
      // leave like this for now:
      for (uint i = m_next.load(); i < NumBits; ++i) {
         if (allocateSpecific(i)) {
            // remember next (possibly) free id:
            const uint oldNext = m_next.load();
            m_next.testAndSetRelaxed(oldNext, std::max(i + 1, oldNext));
            return i;
         }
      }
      return -1;
   }
};

using UserEventTypeRegistry = BasicAtomicBitField<static_cast<int>(Event::Type::MaxUser) - static_cast<int>(Event::Type::User) + 1>;
static UserEventTypeRegistry sg_userEventTypeRegistry;

static inline int register_event_type_zero_based(int id) noexcept
{
   // if the type hint hasn't been registered yet, take it:
   if (id < UserEventTypeRegistry::NumBits && id >= 0 && sg_userEventTypeRegistry.allocateSpecific(id)) {
      return id;
   }
   // otherwise, ignore hint:
   return sg_userEventTypeRegistry.allocateNext();
}

}

Event::Event(Type type)
   : m_implPtr(nullptr),
     m_type(static_cast<int>(type)), 
     m_posted(false),
     m_spont(false),
     m_accept(true)
{}

Event::Event(const Event &other)
   : m_implPtr(other.m_implPtr),
     m_type(other.m_type),
     m_posted(other.m_posted),
     m_spont(other.m_spont),
     m_accept(other.m_accept)
{
   // if EventPrivate becomes available, make sure to implement a
   // virtual EventPrivate *clone() const; function so we can copy here
   PDK_ASSERT_X(!m_implPtr, "Event", "Impossible, this can't happen: EventPrivate isn't defined anywhere");
}

Event &Event::operator=(const Event &other)
{
   // if EventPrivate becomes available, make sure to implement a
   // virtual QEventPrivate *clone() const; function so we can copy here
   PDK_ASSERT_X(!other.m_implPtr, "Event", "Impossible, this can't happen: EventPrivate isn't defined anywhere");
   
   m_type = other.m_type;
   m_posted = other.m_posted;
   m_spont = other.m_spont;
   m_accept = other.m_accept;
   return *this;
}

int Event::registerEventType(int hint) noexcept
{
   const int result = register_event_type_zero_based(static_cast<int>(Event::Type::MaxUser) - hint);
   return result < 0 ? -1 : static_cast<int>(Event::Type::MaxUser) - result;
}

Event::~Event()
{
   if (m_posted && CoreApplication::getInstance()) {
      CoreApplicationPrivate::removePostedEvent(this);  
   }
   PDK_ASSERT_X(!m_implPtr, "Event", "Impossible, this can't happen: EventPrivate isn't defined anywhere");
}

TimerEvent::TimerEvent(int timerId)
   : Event(Type::Timer),
     m_id(timerId)
{}

TimerEvent::~TimerEvent()
{}

ChildEvent::ChildEvent(Type type, Object *child)
    : Event(type),
      m_child(child)
{}

ChildEvent::~ChildEvent()
{}

DeferredDeleteEvent::DeferredDeleteEvent()
   : Event(Type::DeferredDelete),
     m_level(0)
{}

DeferredDeleteEvent::~DeferredDeleteEvent()
{}

} // kernel
} // pdk
