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
#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/kernel/Pointer.h"

namespace pdk {
namespace kernel {

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
      // @TODO emit signal
      // emit timeout(PrivateSignal());
   }
}

class SingleShotTimer : public Object
{
public:
   ~SingleShotTimer();
   SingleShotTimer(int msec, pdk::TimerType timerType, const Object *r, const char * m);
   SingleShotTimer(int msec, pdk::TimerType timerType, const Object *r/*, QtPrivate::QSlotObjectBase *slotObj*/);
   
   // SIGNALS:
   // void timeout();
protected:
   void timerEvent(TimerEvent *) override;
   
private:
   int m_timerId;
   bool m_hasValidReceiver;
   Pointer<const Object> m_receiver;
};

SingleShotTimer::SingleShotTimer(int msec, pdk::TimerType timerType, const Object *r, const char *member)
   : Object(AbstractEventDispatcher::getInstance()), 
     m_hasValidReceiver(true)
{
}

SingleShotTimer::SingleShotTimer(int msec, pdk::TimerType timerType, const Object *r/*, QtPrivate::QSlotObjectBase *slotObj*/)
   : Object(AbstractEventDispatcher::getInstance()),
     m_hasValidReceiver(r), 
     m_receiver(std::shared_ptr<const Object>(r)) /*,slotObj(slotObj) */
{
}

SingleShotTimer::~SingleShotTimer()
{
}

void SingleShotTimer::timerEvent(TimerEvent *)
{
}

void Timer::singleShotImpl(int msec, pdk::TimerType timerType,
                           const Object *receiver
                           /* QtPrivate::QSlotObjectBase *slotObj*/)
{
}

void Timer::singleShot(int msec, const Object *receiver, const char *member)
{
}

void Timer::singleShot(int msec, pdk::TimerType timerType, const Object *receiver,
                       const char *member)
{
}

void Timer::setInterval(int msec)
{
}

int Timer::getRemainingTime() const
{
}

} // kernel
} // pdk
