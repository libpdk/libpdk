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

#ifndef PDK_KERNEL_TIMER_INFO_UNIX_H
#define PDK_KERNEL_TIMER_INFO_UNIX_H

#include "pdk/global/Global.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#include <sys/time.h>
#include <list>

namespace pdk {
namespace kernel {

// internal timer info
struct TimerInfo {
   int m_id;           // - timer identifier
   int m_interval;     // - timer interval in milliseconds
   pdk::TimerType m_timerType; // - timer type
   timespec m_timeout;  // - when to actually fire
   Object *m_obj;     // - object to receive event
   TimerInfo **m_activateRef; // - ref from activateTimers
   
#ifdef PDK_TIMERINFO_DEBUG
   timeval m_expected; // when timer is expected to fire
   float m_cumulativeError;
   uint m_count;
#endif
};

class PDK_CORE_EXPORT TimerInfoList : public std::list<TimerInfo *>
{
#if ((_POSIX_MONOTONIC_CLOCK-0 <= 0) && !defined(PDK_OS_MAC))
   timespec m_previousTime;
   clock_t m_previousTicks;
   int m_ticksPerSecond;
   int m_msPerTick;
   
   bool timeChanged(timespec *delta);
   void timerRepair(const timespec &);
#endif
   
public:
   TimerInfoList();
   timespec updateCurrentTime();
   // must call updateCurrentTime() first!
   void repairTimersIfNeeded();
   bool timerWait(timespec &);
   void timerInsert(TimerInfo *);
   int timerRemainingTime(int timerId);
   void registerTimer(int timerId, int interval, pdk::TimerType timerType, Object *object);
   bool unregisterTimer(int timerId);
   bool unregisterTimers(Object *object);
   std::list<AbstractEventDispatcher::TimerInfo> registeredTimers(Object *object) const;
   int activateTimers();
public:
    timespec m_currentTime;
private:
   // state variables used by activateTimers()
   TimerInfo *m_firstTimerInfo;
};

} // kernel
} // pdk

#endif // PDK_KERNEL_TIMER_INFO_UNIX_H
