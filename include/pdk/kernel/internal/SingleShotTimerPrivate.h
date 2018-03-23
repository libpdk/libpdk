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
// Created by softboy on 2018/3/23.

#ifndef PDK_KERNEL_INTERNAL_SINGLE_SHOT_TIMER_PRIVATE_H
#define PDK_KERNEL_INTERNAL_SINGLE_SHOT_TIMER_PRIVATE_H

#include "pdk/kernel/Object.h"
#include "pdk/kernel/internal/ObjectDefsPrivate.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/Pointer.h"
#include "pdk/kernel/AbstractEventDispatcher.h"

namespace pdk {
namespace kernel {
namespace internal {

class SingleShotTimer : public Object
{
public:
   using TimeoutHandlerType = void();
   using SlotFuncHandlerType = TimeoutHandlerType;
   ~SingleShotTimer();
   template <typename SlotFuncType,
             typename = std::enable_if_t<pdk::stdext::IsCallable<SlotFuncType>::value>>
   SingleShotTimer(int msec, pdk::TimerType timerType, const Object *context, SlotFuncType &&slotFunc);
   template <typename MemberFuncPointer,
             typename = std::enable_if_t<std::is_member_function_pointer<MemberFuncPointer>::value>>
   SingleShotTimer(int msec, pdk::TimerType timerType, const Object *receiver, MemberFuncPointer memberFunc);
   
   PDK_DEFINE_SIGNAL_BINDER(Timeout)
   PDK_DEFINE_SIGNAL_EMITTER(Timeout)
   
protected:
   void timerEvent(TimerEvent *) override;
   
private:
   int m_timerId;
   bool m_hasValidReceiver;
   Pointer<const Object> m_receiver;
   std::function<SlotFuncHandlerType> m_slotFunc;
   bool m_slotMode;
};

template <typename MemberFuncPointer, typename>
SingleShotTimer::SingleShotTimer(int msec, pdk::TimerType timerType, const Object *receiver, MemberFuncPointer memberFunc)
   : Object(AbstractEventDispatcher::getInstance()), 
     m_hasValidReceiver(true),
     m_slotMode(false)
{
   m_timerId = startTimer(msec, timerType);
   connectTimeoutSignal(receiver, memberFunc);
}

template <typename SlotFuncType, typename>
SingleShotTimer::SingleShotTimer(int msec, pdk::TimerType timerType, const Object *context, SlotFuncType &&slotFunc)
   : Object(AbstractEventDispatcher::getInstance()),
     m_hasValidReceiver(context), 
     m_receiver(context),
     m_slotFunc(slotFunc),
     m_slotMode(true)
{
   m_timerId = startTimer(msec, timerType);
   if (context && getThread() != context->getThread()) {
      // Avoid leaking the SingleShotTimer instance in case the application exits before the timer fires
      CoreApplication::getInstance()->connectAboutToQuitSignal(this, &Object::deleteLater);
      setParent(nullptr);
      moveToThread(context->getThread());
   }
}

} // internal
} // kernel
} // pdk

#endif
