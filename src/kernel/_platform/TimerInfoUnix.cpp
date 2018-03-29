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

#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/CoreUnixPrivate.h"
#include "pdk/kernel/internal/TimerInfoUnixPrivate.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/kernel/internal/AbstractEventDispatcherPrivate.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/lang/Character.h"
#include "pdk/base/os/thread/Thread.h"

#ifdef PDK_TIMERINFO_DEBUG
#  include "pdk/base/io/Debug.h" 
#  include "pdk/base/os/thread/Thread.h"
#endif
#include <sys/times.h>

namespace pdk {
namespace kernel {
namespace internal {

using pdk::kernel::ElapsedTimer;
using pdk::io::Debug;
using pdk::io::DebugStateSaver;
using pdk::lang::Character;
using pdk::os::thread::Thread;

PDK_CORE_EXPORT bool g_pdkDisableLowpriorityTimers = false;

// Internal functions for manipulating timer data structures.  The
// timerBitVec array is used for keeping track of timer identifiers.

TimerInfoList::TimerInfoList()
{
#if (_POSIX_MONOTONIC_CLOCK-0 <= 0) && !defined(PDK_OS_MAC) && !defined(PDK_OS_NACL)
   if (!ElapsedTimer::isMonotonic()) {
      // not using monotonic timers, initialize the timeChanged() machinery
      m_previousTime = pdk::kernel::get_time();
      tms unused;
      m_previousTicks = times(&unused);
      m_ticksPerSecond = sysconf(_SC_CLK_TCK);
      m_msPerTick = 1000/m_ticksPerSecond;
   } else {
      // detected monotonic timers
      previousTime.tv_sec = m_previousTime.tv_nsec = 0;
      m_previousTicks = 0;
      m_ticksPerSecond = 0;
      m_msPerTick = 0;
   }
#endif
   m_firstTimerInfo = nullptr;
}

timespec TimerInfoList::updateCurrentTime()
{
   return (m_currentTime = pdk::kernel::get_time());
}

#if ((_POSIX_MONOTONIC_CLOCK-0 <= 0) && !defined(PDK_OS_MAC) && !defined(PDK_OS_INTEGRITY))
timespec abs_timespec(const timespec &t)
{
   timespec tmp = t;
   if (tmp.tv_sec < 0) {
      tmp.tv_sec = -tmp.tv_sec - 1;
      tmp.tv_nsec -= 1000000000;
   }
   if (tmp.tv_sec == 0 && tmp.tv_nsec < 0) {
      tmp.tv_nsec = -tmp.tv_nsec;
   }
   return normalized_timespec(tmp);
}

bool TimerInfoList::timeChanged(timespec *delta)
{
#ifdef PDK_OS_NACL
   PDK_UNUSED(delta)
         return false; // Calling "times" crashes.
#endif
   struct tms unused;
   clock_t currentTicks = times(&unused);
   clock_t elapsedTicks = currentTicks - m_previousTicks;
   timespec elapsedTime = m_currentTime - m_previousTime;
   timespec elapsedTimeTicks;
   elapsedTimeTicks.tv_sec = elapsedTicks / m_ticksPerSecond;
   elapsedTimeTicks.tv_nsec = (((elapsedTicks * 1000) / m_ticksPerSecond) % 1000) * 1000 * 1000;
   timespec dummy;
   if (!delta) {
      delta = &dummy;
   }
   *delta = elapsedTime - elapsedTimeTicks;
   m_previousTicks = currentTicks;
   m_previousTime = m_currentTime;
   // If tick drift is more than 10% off compared to realtime, we assume that the clock has
   // been set. Of course, we have to allow for the tick granularity as well.
   timespec tickGranularity;
   tickGranularity.tv_sec = 0;
   tickGranularity.tv_nsec = msPerTick * 1000 * 1000;
   return elapsedTimeTicks < ((abs_timespec(*delta) - tickGranularity) * 10);
}

void TimerInfoList::timerRepair(const timespec &diff)
{
   // repair all timers
   for (size_t i = 0; i < size(); ++i) {
      TimerInfo *t = at(i);
      t->m_timeout = t->m_timeout + diff;
   }
}

void TimerInfoList::repairTimersIfNeeded()
{
   if (ElapsedTimer::isMonotonic()) {
      return;
   }
   timespec delta;
   if (timeChanged(&delta)) {
      timerRepair(delta);
   }
}

#else


void TimerInfoList::repairTimersIfNeeded()
{
}

#endif

/*
  insert timer info into list
*/
void TimerInfoList::timerInsert(TimerInfo *timeInfo)
{
   TimerInfoList::iterator iter = begin();
   TimerInfoList::iterator endMark = end();
   while (iter != endMark) {
      const TimerInfo * const t = *iter;
      if (!(timeInfo->m_timeout < t->m_timeout)) {
         break;
      }   
      ++iter;
   }
   std::advance(iter, 1);
   insert(iter, timeInfo);
}

inline timespec &operator+=(timespec &t1, int ms)
{
   t1.tv_sec += ms / 1000;
   t1.tv_nsec += ms % 1000 * 1000 * 1000;
   return normalized_timespec(t1);
}

inline timespec operator+(const timespec &t1, int ms)
{
   timespec t2 = t1;
   return t2 += ms;
}

namespace {

timespec round_to_millisecond(timespec val)
{
   // always round up
   // worst case scenario is that the first trigger of a 1-ms timer is 0.999 ms late
   int ns = val.tv_nsec % (1000 * 1000);
   val.tv_nsec += 1000 * 1000 - ns;
   return normalized_timespec(val);
}

void calculate_coarse_timer_timeout(TimerInfo *timeInfo, timespec currentTime)
{
   // The coarse timer works like this:
   //  - interval under 40 ms: round to even
   //  - between 40 and 99 ms: round to multiple of 4
   //  - otherwise: try to wake up at a multiple of 25 ms, with a maximum error of 5%
   //
   // We try to wake up at the following second-fraction, in order of preference:
   //    0 ms
   //  500 ms
   //  250 ms or 750 ms
   //  200, 400, 600, 800 ms
   //  other multiples of 100
   //  other multiples of 50
   //  other multiples of 25
   //
   // The objective is to make most timers wake up at the same time, thereby reducing CPU wakeups.
   
   uint interval = uint(timeInfo->m_interval);
   uint msec = uint(timeInfo->m_timeout.tv_nsec) / 1000 / 1000;
   PDK_ASSERT(interval >= 20);
   
   // Calculate how much we can round and still keep within 5% error
   uint absMaxRounding = interval / 20;
   
   if (interval < 100 && interval != 25 && interval != 50 && interval != 75) {
      // special mode for timers of less than 100 ms
      if (interval < 50) {
         // round to even
         // round towards multiples of 50 ms
         bool roundUp = (msec % 50) >= 25;
         msec >>= 1;
         msec |= uint(roundUp);
         msec <<= 1;
      } else {
         // round to multiple of 4
         // round towards multiples of 100 ms
         bool roundUp = (msec % 100) >= 50;
         msec >>= 2;
         msec |= uint(roundUp);
         msec <<= 2;
      }
   } else {
      uint min = std::max<int>(0, msec - absMaxRounding);
      uint max = std::min(1000u, msec + absMaxRounding);
      
      // find the boundary that we want, according to the rules above
      // extra rules:
      // 1) whatever the interval, we'll take any round-to-the-second timeout
      if (min == 0) {
         msec = 0;
         goto recalculate;
      } else if (max == 1000) {
         msec = 1000;
         goto recalculate;
      }
      
      uint wantedBoundaryMultiple;
      
      // 2) if the interval is a multiple of 500 ms and > 5000 ms, we'll always round
      //    towards a round-to-the-second
      // 3) if the interval is a multiple of 500 ms, we'll round towards the nearest
      //    multiple of 500 ms
      if ((interval % 500) == 0) {
         if (interval >= 5000) {
            msec = msec >= 500 ? max : min;
            goto recalculate;
         } else {
            wantedBoundaryMultiple = 500;
         }
      } else if ((interval % 50) == 0) {
         // 4) same for multiples of 250, 200, 100, 50
         uint mult50 = interval / 50;
         if ((mult50 % 4) == 0) {
            // multiple of 200
            wantedBoundaryMultiple = 200;
         } else if ((mult50 % 2) == 0) {
            // multiple of 100
            wantedBoundaryMultiple = 100;
         } else if ((mult50 % 5) == 0) {
            // multiple of 250
            wantedBoundaryMultiple = 250;
         } else {
            // multiple of 50
            wantedBoundaryMultiple = 50;
         }
      } else {
         wantedBoundaryMultiple = 25;
      }
      
      uint base = msec / wantedBoundaryMultiple * wantedBoundaryMultiple;
      uint middlepoint = base + wantedBoundaryMultiple / 2;
      if (msec < middlepoint) {
         msec = std::max(base, min);
      } else {
         msec = std::min(base + wantedBoundaryMultiple, max);
      }
   }
   
recalculate:
   if (msec == 1000u) {
      ++timeInfo->m_timeout.tv_sec;
      timeInfo->m_timeout.tv_nsec = 0;
   } else {
      timeInfo->m_timeout.tv_nsec = msec * 1000 * 1000;
   }
   if (timeInfo->m_timeout < currentTime) {
      timeInfo->m_timeout += interval;
   }
}

void calculate_next_timeout(TimerInfo *timeInfo, timespec currentTime)
{
   switch (timeInfo->m_timerType) {
   case pdk::TimerType::PreciseTimer:
   case pdk::TimerType::CoarseTimer:
      timeInfo->m_timeout += timeInfo->m_interval;
      if (timeInfo->m_timeout < currentTime) {
         timeInfo->m_timeout = currentTime;
         timeInfo->m_timeout += timeInfo->m_interval;
      }
#ifdef PDK_TIMERINFO_DEBUG
      timeInfo->m_expected += timeInfo->m_interval;
      if (timeInfo->m_expected < currentTime) {
         timeInfo->m_expected = currentTime;
         timeInfo->m_expected += timeInfo->m_interval;
      }
#endif
      if (timeInfo->m_timerType == pdk::TimerType::CoarseTimer) {
         calculate_coarse_timer_timeout(timeInfo, currentTime);
      }
      return;
   case pdk::TimerType::VeryCoarseTimer:
      // we don't need to take care of the microsecond component of t->interval
      timeInfo->m_timeout.tv_sec += timeInfo->m_interval;
      if (timeInfo->m_timeout.tv_sec <= currentTime.tv_sec) {
         timeInfo->m_timeout.tv_sec = currentTime.tv_sec + timeInfo->m_interval;
      }
#ifdef PDK_TIMERINFO_DEBUG
      timeInfo->m_expected.tv_sec += t->interval;
      if (timeInfo->m_expected.tv_sec <= currentTime.tv_sec) {
         timeInfo->m_expected.tv_sec = currentTime.tv_sec + timeInfo->m_interval;
      }
#endif
      return;
   }
   
#ifdef PDK_TIMERINFO_DEBUG
   if (timeInfo->m_timerType != pdk::TimerType::PreciseTimer)
      debug_stream() << "timer" << timeInfo->m_timerType << pdk::io::hex << timeInfo->m_id << pdk::io::dec << "interval" << timeInfo->m_interval
                     << "originally expected at" << timeInfo->m_expected << "will fire at" << timeInfo->m_timeout
                     << "or" << (timeInfo->m_timeout - timeInfo->m_expected) << "s late";
#endif
}

} // anonymous namespace

bool TimerInfoList::timerWait(timespec &tm)
{
   timespec currentTime = updateCurrentTime();
   repairTimersIfNeeded();
   // Find first waiting timer not already active
   TimerInfo *t = nullptr;
   for (TimerInfoList::const_iterator iter = cbegin(); iter != cend(); ++iter) {
      if (!(*iter)->m_activateRef) {
         t = *iter;
         break;
      }
   }
   if (!t) {
      return false;
   }
   if (currentTime < t->m_timeout) {
      // time to wait
      tm = round_to_millisecond(t->m_timeout - currentTime);
   } else {
      // no time to wait
      tm.tv_sec  = 0;
      tm.tv_nsec = 0;
   }
   return true;
}

int TimerInfoList::timerRemainingTime(int timerId)
{
   timespec currentTime = updateCurrentTime();
   repairTimersIfNeeded();
   timespec tm = {0, 0};
   TimerInfoList::iterator iter = begin();
   TimerInfoList::iterator endMark = end();
   while (iter != endMark) {
      TimerInfo *t = *iter;
      if (t->m_id == timerId) {
         if (currentTime < t->m_timeout) {
            // time to wait
            tm = round_to_millisecond(t->m_timeout - currentTime);
            return tm.tv_sec*1000 + tm.tv_nsec/1000/1000;
         } else {
            return 0;
         }
      }
      ++iter;
   }
   
#ifndef PDK_NO_DEBUG
   warning_stream("TimerInfoList::timerRemainingTime: timer id %i not found", timerId);
#endif
   return -1;
}

void TimerInfoList::registerTimer(int timerId, int interval, pdk::TimerType timerType, Object *object)
{
   TimerInfo *t = new TimerInfo;
   t->m_id = timerId;
   t->m_interval = interval;
   t->m_timerType = timerType;
   t->m_obj = object;
   t->m_activateRef = 0;
   timespec expected = updateCurrentTime() + interval;
   switch (timerType) {
   case pdk::TimerType::PreciseTimer:
      // high precision timer is based on millisecond precision
      // so no adjustment is necessary
      t->m_timeout = expected;
      break;
      
   case pdk::TimerType::CoarseTimer:
      // this timer has up to 5% coarseness
      // so our boundaries are 20 ms and 20 s
      // below 20 ms, 5% inaccuracy is below 1 ms, so we convert to high precision
      // above 20 s, 5% inaccuracy is above 1 s, so we convert to VeryCoarseTimer
      if (interval >= 20000) {
         t->m_timerType = pdk::TimerType::VeryCoarseTimer;
      } else {
         t->m_timeout = expected;
         if (interval <= 20) {
            t->m_timerType = pdk::TimerType::PreciseTimer;
            // no adjustment is necessary
         } else if (interval <= 20000) {
            calculate_coarse_timer_timeout(t, m_currentTime);
         }
         break;
      }
      PDK_FALLTHROUGH();
   case pdk::TimerType::VeryCoarseTimer:
      // the very coarse timer is based on full second precision,
      // so we keep the interval in seconds (round to closest second)
      t->m_interval /= 500;
      t->m_interval += 1;
      t->m_interval >>= 1;
      t->m_timeout.tv_sec = m_currentTime.tv_sec + t->m_interval;
      t->m_timeout.tv_nsec = 0;
      
      // if we're past the half-second mark, increase the timeout again
      if (m_currentTime.tv_nsec > 500*1000*1000) {
         ++t->m_timeout.tv_sec;
      }
   }
   timerInsert(t);
#ifdef PDK_TIMERINFO_DEBUG
   t->m_expected = expected;
   t->m_cumulativeError = 0;
   t->m_count = 0;
   if (t->m_timerType != pdk::TimerType::PreciseTimer)
      debug_stream() << "timer" << t->m_timerType << pdk::io::hex <<t->m_id << pdk::io::dec << "interval" << t->m_interval << "expected at"
                     << t->m_expected << "will fire first at" << t->m_timeout;
#endif
}

#ifdef PDK_TIMERINFO_DEBUG
Debug operator<<(Debug s, timeval tv)
{
   DebugStateSaver saver(s);
   s.nospace() << tv.tv_sec << "." 
               << pdk::io::set_field_width(6) 
               << pdk::io::set_pad_char(Character(48))
               << tv.tv_usec << pdk::io::reset;
   return s;
}

Debug operator<<(Debug s, pdk::TimerType t)
{
   DebugStateSaver saver(s);
   s << (t == pdk::TimerType::PreciseTimer ? "P" :
                                             t == pdk::TimerType::CoarseTimer ? "C" : "VC");
   return s;
}
#endif

bool TimerInfoList::unregisterTimer(int timerId)
{
   // set timer inactive
   TimerInfoList::iterator iter = begin();
   TimerInfoList::iterator endMark = end();
   while (iter != endMark) {
      TimerInfo *t = *iter;
      if (t->m_id == timerId) {
         // found it
         erase(iter);
         if (t == m_firstTimerInfo) {
            m_firstTimerInfo = nullptr;
         }
         if (t->m_activateRef) {
            *(t->m_activateRef) = 0;
         }            
         delete t;
         return true;
      }
      ++iter;
   }
   // id not found
   return false;
}

bool TimerInfoList::unregisterTimers(Object *object)
{
   if (empty()) {
      return false;
   }
   TimerInfoList::iterator iter = begin();
   while (iter != end()) {
      TimerInfo *t = *iter;
      if (t->m_obj == object) {
         // object found
         iter = erase(iter);
         if (t == m_firstTimerInfo) {
            m_firstTimerInfo = nullptr;
         }  
         if (t->m_activateRef) {
            *(t->m_activateRef) = 0;
         }
         delete t;
         // move back one so that we don't skip the new current item
      } else {
         ++iter;
      }
   }
   return true;
}

std::list<AbstractEventDispatcher::TimerInfo> 
TimerInfoList::getRegisteredTimers(Object *object) const
{
   std::list<AbstractEventDispatcher::TimerInfo> list;
   TimerInfoList::const_iterator iter = begin();
   TimerInfoList::const_iterator endMark = end();
   while (iter != endMark) {
      const TimerInfo * const t = *iter;
      if (t->m_obj == object) {
         list.push_back(AbstractEventDispatcher::TimerInfo(t->m_id,
                                                           (t->m_timerType == pdk::TimerType::VeryCoarseTimer
                                                            ? t->m_interval * 1000
                                                            : t->m_interval),
                                                           t->m_timerType));
      }
      ++iter;
   }
   return list;
}

int TimerInfoList::getActivateTimers()
{
   if (g_pdkDisableLowpriorityTimers || empty()) {
      return 0; // nothing to do
   }
   int n_act = 0;
   int maxCount = 0;
   m_firstTimerInfo = nullptr;
   timespec currentTime = updateCurrentTime();
#ifdef PDK_TIMERINFO_DEBUG
   // debug_stream() << "Thread" << Thread::getCurrentThreadId() << "woken up at" << currentTime;
#endif
   repairTimersIfNeeded();   
   // Find out how many timer have expired
   for (TimerInfoList::const_iterator iter = cbegin(); iter != cend(); ++iter) {
      if (currentTime < (*iter)->m_timeout) {
         break;
      }
      maxCount++;
   }
   //fire the timers.
   while (maxCount--) {
      if (empty()) {
         break;
      }
      TimerInfo *currentTimerInfo = front();
      if (currentTime < currentTimerInfo->m_timeout) {
         break; // no timer has expired
      }
      if (!m_firstTimerInfo) {
         m_firstTimerInfo = currentTimerInfo;
      } else if (m_firstTimerInfo == currentTimerInfo) {
         // avoid sending the same timer multiple times
         break;
      } else if (currentTimerInfo->m_interval <  m_firstTimerInfo->m_interval
                 || currentTimerInfo->m_interval == m_firstTimerInfo->m_interval) {
         m_firstTimerInfo = currentTimerInfo;
      }
      
      // remove from list
      pop_front();
      
#ifdef PDK_TIMERINFO_DEBUG
      float diff;
      if (currentTime < currentTimerInfo->m_expected) {
         // early
         timeval early = currentTimerInfo->m_expected - currentTime;
         diff = -(early.tv_sec + early.tv_usec / 1000000.0);
      } else {
         timeval late = currentTime - currentTimerInfo->m_expected;
         diff = late.tv_sec + late.tv_usec / 1000000.0;
      }
      currentTimerInfo->m_cumulativeError += diff;
      ++currentTimerInfo->m_count;
      if (currentTimerInfo->m_timerType != pdk::TimerType::PreciseTimer) {
         debug_stream() << "timer" << currentTimerInfo->m_timerType << pdk::io::hex << currentTimerInfo->m_id << pdk::io::dec << "interval"
                        << currentTimerInfo->m_interval << "firing at" << currentTime
                        << "(orig" << currentTimerInfo->m_expected << "scheduled at" << currentTimerInfo->m_timeout
                        << ") off by" << diff << "activation" << currentTimerInfo->m_count
                        << "avg error" << (currentTimerInfo->m_cumulativeError / currentTimerInfo->m_count);
      }
      
#endif
      // determine next timeout time
      calculate_next_timeout(currentTimerInfo, currentTime);
      // reinsert timer
      timerInsert(currentTimerInfo);
      if (currentTimerInfo->m_interval > 0) {
         n_act++;
      }
      if (!currentTimerInfo->m_activateRef) {
         // send event, but don't allow it to recurse
         currentTimerInfo->m_activateRef = &currentTimerInfo;
         TimerEvent e(currentTimerInfo->m_id);
         CoreApplication::sendEvent(currentTimerInfo->m_obj, &e);
         if (currentTimerInfo) {
            currentTimerInfo->m_activateRef = 0;
         }
      }
   }
   m_firstTimerInfo = 0;
   // debug_stream() << "Thread" << Thread::getCurrentThreadId() << "activated" << n_act << "timers";
   return n_act;
}

} // internal
} // kernel
} // pdk
