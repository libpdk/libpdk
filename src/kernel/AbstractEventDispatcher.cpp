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

#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/internal/AbstractEventDispatcherPrivate.h"
#include "pdk/kernel/AbstractNativeEventFilter.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"

#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/utils/internal/LockFreeListPrivate.h"

namespace pdk {
namespace kernel {

using pdk::utils::internal::LockFreeListDefaultConstants;
using pdk::utils::internal::LockFreeList;
using pdk::os::thread::internal::ThreadData;
using pdk::os::thread::internal::ScopedScopeLevelCounter;
using pdk::kernel::internal::AbstractEventDispatcherPrivate;

struct PdkTimerIdFreeListConstants : public LockFreeListDefaultConstants
{
   enum {
      InitialNextValue = 1,
      BlockCount = 6
   };
   static const int Sizes[BlockCount];
};

enum {
   Offset0 = 0x00000000,
   Offset1 = 0x00000040,
   Offset2 = 0x00000100,
   Offset3 = 0x00001000,
   Offset4 = 0x00010000,
   Offset5 = 0x00100000,
   
   Size0 = Offset1  - Offset0,
   Size1 = Offset2  - Offset1,
   Size2 = Offset3  - Offset2,
   Size3 = Offset4  - Offset3,
   Size4 = Offset5  - Offset4,
   Size5 = PdkTimerIdFreeListConstants::MaxIndex - Offset5
};

const int PdkTimerIdFreeListConstants::Sizes[PdkTimerIdFreeListConstants::BlockCount] = {
   Size0,
   Size1,
   Size2,
   Size3,
   Size4,
   Size5
};

typedef LockFreeList<void, PdkTimerIdFreeListConstants> PdkTimerIdFreeList;
PDK_GLOBAL_STATIC(PdkTimerIdFreeList, sg_timerIdFreeList);

namespace internal {

int AbstractEventDispatcherPrivate::allocateTimerId()
{
   return sg_timerIdFreeList()->next();
}

void AbstractEventDispatcherPrivate::releaseTimerId(int timerId)
{
   if (PdkTimerIdFreeList *fl = sg_timerIdFreeList()) {
      fl->release(timerId);
   }
}

} // internal

AbstractEventDispatcher::AbstractEventDispatcher(Object *parent)
   : Object(*new AbstractEventDispatcherPrivate, parent)
{}

AbstractEventDispatcher::AbstractEventDispatcher(AbstractEventDispatcherPrivate &dd,
                                                 Object *parent)
   : Object(dd, parent)
{}

AbstractEventDispatcher::~AbstractEventDispatcher()
{}

AbstractEventDispatcher *AbstractEventDispatcher::getInstance(Thread *thread)
{
   ThreadData *data = thread ? ThreadData::get(thread) : ThreadData::current();
   return data->m_eventDispatcher.load();
}

int AbstractEventDispatcher::registerTimer(int interval, pdk::TimerType timerType, Object *object)
{
   int id = AbstractEventDispatcherPrivate::allocateTimerId();
   registerTimer(id, interval, timerType, object);
   return id;
}

void AbstractEventDispatcher::startingUp()
{}

void AbstractEventDispatcher::closingDown()
{}

void AbstractEventDispatcher::installNativeEventFilter(AbstractNativeEventFilter *filterObj)
{
   PDK_D(AbstractEventDispatcher);
   // clean up unused items in the list
   implPtr->m_eventFilters.remove(nullptr);
   implPtr->m_eventFilters.remove(filterObj);
   implPtr->m_eventFilters.push_front(filterObj);
}

void AbstractEventDispatcher::removeNativeEventFilter(AbstractNativeEventFilter *filter)
{
   PDK_D(AbstractEventDispatcher);
   for (size_t i = 0; i < implPtr->m_eventFilters.size(); ++i) {
      implPtr->m_eventFilters.remove_if([&filter](AbstractNativeEventFilter *cur){
         return filter == cur;
      });
   }
}

bool AbstractEventDispatcher::filterNativeEvent(const ByteArray &eventType, void *message, long *result)
{
   PDK_D(AbstractEventDispatcher);
   if (!implPtr->m_eventFilters.empty()) {
      // Raise the loopLevel so that deleteLater() calls in or triggered
      // by event_filter() will be processed from the main event loop.
      ScopedScopeLevelCounter scopeLevelCounter(implPtr->m_threadData);
      auto iter = implPtr->m_eventFilters.begin();
      auto end = implPtr->m_eventFilters.end();
      while (iter != end) {
         AbstractNativeEventFilter *filter = *iter;
         if (!filter) {
            continue;
         }
         if (filter->nativeEventFilter(eventType, message, result)) {
            return true;
         }
         ++iter;
      }
   }
   return false;
}

} // kernel
} // pdk
