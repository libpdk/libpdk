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
// Created by softboy on 2017/01/23.

#include "pdk/kernel/BasicTimer.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/internal/AbstractEventDispatcherPrivate.h"
#include "pdk/global/Logging.h"

namespace pdk {
namespace kernel {

using internal::AbstractEventDispatcherPrivate;

void BasicTimer::start(int msec, Object *obj)
{
   AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance();
   if (PDK_UNLIKELY(!eventDispatcher)) {
      warning_stream("BasicTimer::start: BasicTimer can only be used with threads started with Thread");
      return;
   }
   if (PDK_UNLIKELY(obj && obj->getThread() != eventDispatcher->getThread())) {
      warning_stream("BasicTimer::start: Timers cannot be started from another thread");
      return;
   }
   if (m_id) {
      if (PDK_LIKELY(eventDispatcher->unregisterTimer(m_id))) {
         AbstractEventDispatcherPrivate::releaseTimerId(m_id);
      } else {
         warning_stream("BasicTimer::start: Stopping previous timer failed. Possibly trying to stop from a different thread");
      }  
   }
   m_id = 0;
   if (obj) {
      m_id = eventDispatcher->registerTimer(msec, pdk::TimerType::CoarseTimer, obj);
   }
}

void BasicTimer::start(int msec, pdk::TimerType timerType, Object *obj)
{
   AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance();
   if (PDK_UNLIKELY(msec < 0)) {
      warning_stream("BasicTimer::start: Timers cannot have negative timeouts");
      return;
   }
   if (PDK_UNLIKELY(!eventDispatcher)) {
      warning_stream("BasicTimer::start: BasicTimer can only be used with threads started with Thread");
      return;
   }
   if (PDK_UNLIKELY(obj && obj->getThread() != eventDispatcher->getThread())) {
      warning_stream("BasicTimer::start: Timers cannot be started from another thread");
      return;
   }
   if (m_id) {
      if (PDK_LIKELY(eventDispatcher->unregisterTimer(m_id))) {
         AbstractEventDispatcherPrivate::releaseTimerId(m_id);
      } else {
         warning_stream("BasicTimer::start: Stopping previous timer failed. Possibly trying to stop from a different thread");
      }
   }
   m_id = 0;
   if (obj) {
      m_id = eventDispatcher->registerTimer(msec, timerType, obj);
   }
}

void BasicTimer::stop()
{
   if (m_id) {
      AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance();
      if (eventDispatcher) {
         if (PDK_UNLIKELY(!eventDispatcher->unregisterTimer(m_id))) {
            warning_stream("BasicTimer::stop: Failed. Possibly trying to stop from a different thread");
            return;
         }
         AbstractEventDispatcherPrivate::releaseTimerId(m_id);
      }
   }
   m_id = 0;
}

} // kernel
} // pdk
