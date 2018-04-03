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
// Created by softboy on 2018/01/25.

#include "pdk/kernel/Timer.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/kernel/internal/SingleShotTimerPrivate.h"

namespace pdk {
namespace kernel {

using pdk::kernel::CallableInvoker;
static constexpr int INV_TIMER = -1;// invalid timer id

Timer::Timer(Object *parent)
   : Object(parent),
     m_id(INV_TIMER),
     m_interval(0),
     m_single(0),
     m_nullTimer(0),
     m_type(pdk::as_integer<pdk::TimerType>(pdk::TimerType::CoarseTimer))
{}

Timer::~Timer()
{
   if (m_id != INV_TIMER) {
      stop();
   }
}

void Timer::start()
{
   if (m_id != INV_TIMER) {
      stop();
   }
   m_nullTimer = (!m_interval && m_single);
   m_id = Object::startTimer(m_interval, pdk::TimerType(m_type));
}

void Timer::start(int msec)
{
   m_interval = msec;
   start();
}

void Timer::stop()
{
   if (m_id != INV_TIMER) {
      Object::killTimer(m_id);
      m_id = INV_TIMER;
   }
}

void Timer::timerEvent(TimerEvent *event)
{
   if (event->getTimerId() == m_id) {
      if (m_single) {
         stop();
      }
      // emit signal
      emitTimeoutSignal();
   }
}

namespace internal {

SingleShotTimer::~SingleShotTimer()
{
   if (m_timerId > 0) {
      killTimer(m_timerId);
   }
}

void SingleShotTimer::timerEvent(TimerEvent *)
{
   if (m_timerId > 0) {
      killTimer(m_timerId);
   }
   m_timerId = -1;
   if (m_slotMode) {
      // If the receiver was destroyed, skip this part
      if (PDK_LIKELY(!m_receiver.isNull() || !m_hasValidReceiver)) {
         m_slotFunc();
      }
   } else {
      emitTimeoutSignal();
   }
   // we would like to use delete later here, but it feels like a
   // waste to post a new event to handle this event, so we just unset the flag
   // and explicitly delete...
   internal::delete_in_event_handler(this);
}

} // internal

void Timer::setInterval(int msec)
{
   m_interval = msec;
   if (m_id != INV_TIMER) {                        // create new timer
      Object::killTimer(m_id);                        // restart timer
      m_id = Object::startTimer(msec, pdk::TimerType(m_type));
   }
}

int Timer::getRemainingTime() const
{
   if (m_id != INV_TIMER) {
      return AbstractEventDispatcher::getInstance()->getRemainingTime(m_id);
   }
   return -1;
}

} // kernel
} // pdk
