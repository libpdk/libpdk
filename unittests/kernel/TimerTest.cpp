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
// Created by softboy on 2018/03/28.

#include "gtest/gtest.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/global/Global.h"
#include "pdktest/PdkTest.h"
#include "pdk/kernel/Timer.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/time/Time.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/kernel/CallableInvoker.h"

#if defined PDK_OS_UNIX
#include <unistd.h>
#endif

using pdk::kernel::Object;
using pdk::kernel::Timer;
using pdk::kernel::CoreApplication;
using pdk::lang::String;
using pdktest::TestEventLoop;
using pdk::kernel::Event;
using pdk::kernel::TimerEvent;
using pdk::kernel::EventLoop;
using pdk::kernel::Pointer;
using pdk::os::thread::Thread;
using pdk::kernel::BasicTimer;
using pdk::time::Time;
using pdk::utils::ScopedPointer;
using pdk::kernel::CallableInvoker;

class TimerHelper : public Object
{
public:
   TimerHelper() 
      : m_count(0),
        m_remainingTime(-1)
   {
   }
   
   int m_count;
   int m_remainingTime;
   
public:
   void timeout();
   void fetchRemainingTime(Timer::SignalType signal, Object *sender);
};

void TimerHelper::timeout()
{
   ++m_count;
}

void TimerHelper::fetchRemainingTime(Timer::SignalType signal, Object *sender)
{
   Timer *timer = static_cast<Timer *>(sender);
   m_remainingTime = timer->getRemainingTime();
}

TEST(TimerTest, testZeroTimer)
{
   TimerHelper helper;
   Timer timer;
   timer.setInterval(0);
   timer.start();
   timer.connectTimeoutSignal(&helper, &TimerHelper::timeout);
   CoreApplication::processEvents();
   ASSERT_EQ(helper.m_count, 1);
}

TEST(TimerTest, testSingleShotTimeout)
{
   TimerHelper helper;
   Timer timer;
   timer.setSingleShot(true);
   timer.connectTimeoutSignal(&helper, &TimerHelper::timeout);
   timer.start(100);
   pdktest::wait(500);
   ASSERT_EQ(helper.m_count, 1);
   pdktest::wait(500);
   ASSERT_EQ(helper.m_count, 1);
}

#define TIMEOUT_TIMEOUT 200

TEST(TimerTest, testTimeout)
{
   TimerHelper helper;
   Timer timer;
   timer.connectTimeoutSignal(&helper, &TimerHelper::timeout);
   timer.start(100);
   ASSERT_EQ(helper.m_count, 0);
   PDK_TRY_VERIFY_WITH_TIMEOUT(helper.m_count > 0, TIMEOUT_TIMEOUT);
   int oldCount = helper.m_count;
   PDK_TRY_VERIFY_WITH_TIMEOUT(helper.m_count > oldCount, TIMEOUT_TIMEOUT);
}

TEST(TimerTest, testRemainingTime)
{
   TimerHelper helper;
   Timer timer;
   timer.connectTimeoutSignal(&helper, &TimerHelper::timeout);
   timer.setTimerType(pdk::TimerType::PreciseTimer);
   timer.start(200);
   ASSERT_EQ(helper.m_count, 0);
   pdktest::wait(50);
   ASSERT_EQ(helper.m_count, 0);
   int remainingTime = timer.getRemainingTime();
   ASSERT_TRUE(std::abs(remainingTime - 150) < 50) << pdk_printable(String::number(remainingTime));
   // wait for the timer to actually fire now
   timer.connectTimeoutSignal(&TestEventLoop::instance(), &TestEventLoop::exitLoop);
   TestEventLoop::instance().enterLoop(5);
   ASSERT_TRUE(!TestEventLoop::instance().getTimeout());
   ASSERT_EQ(helper.m_count, 1);
   // the timer is still active, so it should have a non-zero remaining time
   remainingTime = timer.getRemainingTime();
   ASSERT_TRUE(remainingTime > 150) << pdk_printable(String::number(remainingTime));
}

void remaining_time_during_activation_data(std::list<bool> &data)
{
   data.push_back(true);
   data.push_back(true);
   data.push_back(false);
}

// @TODO re think the mean
TEST(TimerTest, testRemainingTimeDuringActivation)
{
   std::list<bool> data;
   remaining_time_during_activation_data(data);
   for (bool singleShot : data) {
      TimerHelper helper;
      Timer timer;
      // 20 ms is short enough and should not round down to 0 in any timer mode
      const int timeout = 20;
      timer.connectTimeoutSignal(&helper, &TimerHelper::fetchRemainingTime);
      timer.connectTimeoutSignal(&TestEventLoop::instance(), &TestEventLoop::exitLoop);
      timer.start(timeout);
      timer.setSingleShot(singleShot);
      TestEventLoop::instance().enterLoop(5);
      ASSERT_TRUE(!TestEventLoop::instance().getTimeout());
      if (singleShot) {
         ASSERT_EQ(helper.m_remainingTime, -1);     // timer not running
      } else {
         // ASSERT_EQ(helper.m_remainingTime, timeout);
         ASSERT_EQ(helper.m_remainingTime, timer.getRemainingTime());
      }
      if (!singleShot) {
         // do it again - see BUG-46940
         helper.m_remainingTime = -1;
         TestEventLoop::instance().enterLoop(5);
         ASSERT_TRUE(!TestEventLoop::instance().getTimeout());
         // ASSERT_EQ(helper.m_remainingTime, timeout);
         ASSERT_EQ(helper.m_remainingTime, timer.getRemainingTime());
      }
   }
}

namespace {

template <typename T>
std::chrono::milliseconds to_ms(T t)
{
   return std::chrono::duration_cast<std::chrono::milliseconds>(t);
}

} // unnamed namespace

TEST(TimerTest, testBasicChrono)
{
   // duplicates zeroTimer, singleShotTimeout, interval and remainingTime
   using namespace std::chrono;
   TimerHelper helper;
   Timer timer;
   timer.setInterval(to_ms(nanoseconds(0)));
   timer.start();
   ASSERT_EQ(timer.intervalAsDuration().count(), milliseconds::rep(0));
   ASSERT_EQ(timer.remainingTimeAsDuration().count(), milliseconds::rep(0));
   timer.connectTimeoutSignal(&helper, &TimerHelper::timeout);
   CoreApplication::processEvents();
   ASSERT_EQ(helper.m_count, 1);
   helper.m_count = 0;
   timer.start(milliseconds(100));
   ASSERT_EQ(helper.m_count, 0);
   pdktest::wait(TIMEOUT_TIMEOUT);
   ASSERT_TRUE(helper.m_count > 0);
   int oldCount = helper.m_count;
   pdktest::wait(TIMEOUT_TIMEOUT);
   ASSERT_TRUE(helper.m_count > oldCount);

   helper.m_count = 0;
   timer.start(to_ms(microseconds(200000)));
   ASSERT_EQ(timer.intervalAsDuration().count(), milliseconds::rep(200));
   pdktest::wait(50);
   ASSERT_EQ(helper.m_count, 0);

   milliseconds rt = timer.remainingTimeAsDuration();
   ASSERT_TRUE(std::abs(rt.count() - 150) < 50) << pdk_printable(String::number(rt.count()));

   helper.m_count = 0;
   timer.setSingleShot(true);
   timer.start(milliseconds(100));
   pdktest::wait(500);
   ASSERT_EQ(helper.m_count, 1);
   pdktest::wait(500);
   ASSERT_EQ(helper.m_count, 1);
   helper.m_count = 0;
}

void live_lock_data(std::list<int> &data)
{
   data.push_back(0);
   data.push_back(1);
   data.push_back(20);
}

class LiveLockTester : public Object
{
public:
   LiveLockTester(int i)
      : m_interval(i),
        m_timeoutsForFirst(0),
        m_timeoutsForExtra(0),
        m_timeoutsForSecond(0),
        m_postEventAtRightTime(false)
   {
      m_firstTimerId = startTimer(m_interval);
      m_extraTimerId = startTimer(m_interval + 80);
      m_secondTimerId = -1; // started later
   }
   
   bool event(Event *event) {
      if (pdk::as_integer<Event::Type>(event->getType()) == 4002) {
         // got the posted event
         if (m_timeoutsForFirst == 1 && m_timeoutsForSecond == 0) {
            m_postEventAtRightTime = true;
         }
         return true;
      }
      return Object::event(event);
   }
   
   void timerEvent(TimerEvent *timerEvent) {
      if (timerEvent->getTimerId() == m_firstTimerId) {
         if (++m_timeoutsForFirst == 1) {
            killTimer(m_extraTimerId);
            m_extraTimerId = -1;
            CoreApplication::postEvent(this, new Event(static_cast<Event::Type>(4002)));
            m_secondTimerId = startTimer(m_interval);
         }
      } else if (timerEvent->getTimerId() == m_secondTimerId) {
         ++m_timeoutsForSecond;
      } else if (timerEvent->getTimerId() == m_extraTimerId) {
         ++m_timeoutsForExtra;
      }      
      // sleep for 2ms
      pdktest::sleep(2);
      killTimer(timerEvent->getTimerId());
   }
   
   const int m_interval;
   int m_firstTimerId;
   int m_secondTimerId;
   int m_extraTimerId;
   int m_timeoutsForFirst;
   int m_timeoutsForExtra;
   int m_timeoutsForSecond;
   bool m_postEventAtRightTime;
};

TEST(TimerTest, testLivelock)
{
   std::list<int> data;
   live_lock_data(data);
   /*
      New timers created in timer event handlers should not be sent
      until the next iteration of the eventloop.  Note: this test
      depends on the fact that we send posted events before timer
      events (since new posted events are not sent until the next
      iteration of the eventloop either).
    */
   for (int interval: data) {
      LiveLockTester tester(interval);
      pdktest::wait(180); // we have to use wait here, since we're testing timers with a non-zero timeout
      PDK_TRY_COMPARE(tester.m_timeoutsForFirst, 1);
      ASSERT_EQ(tester.m_timeoutsForExtra, 0);
      PDK_TRY_COMPARE(tester.m_timeoutsForSecond, 1);
      ASSERT_TRUE(tester.m_postEventAtRightTime);
   }
}

class TimerInfiniteRecursionObject : public Object
{
public:
   bool m_inTimerEvent;
   bool m_timerEventRecursed;
   int m_interval;
   
   TimerInfiniteRecursionObject(int interval)
      : m_inTimerEvent(false),
        m_timerEventRecursed(false),
        m_interval(interval)
   {}
   
   void timerEvent(TimerEvent *timerEvent)
   {
      m_timerEventRecursed = m_inTimerEvent;
      if (m_timerEventRecursed) {
         // bug detected!
         return;
      }
      m_inTimerEvent = true;
      EventLoop eventLoop;
      Timer::singleShot(std::max(100, m_interval * 2), &eventLoop, &EventLoop::quit);
      eventLoop.exec();
      m_inTimerEvent = false;
      killTimer(timerEvent->getTimerId());
   }
};

void timer_infinite_recursion_data(std::list<int> &data)
{
   data.push_back(0);
   data.push_back(1);
   data.push_back(10);
   data.push_back(11);
   data.push_back(100);
   data.push_back(1000);
}

TEST(TimerTest, testTimerInfiniteRecursion)
{
   std::list<int> data;
   timer_infinite_recursion_data(data);
   for (bool interval: data) {
      TimerInfiniteRecursionObject object(interval);
      (void) object.startTimer(interval);
      EventLoop eventLoop;
      Timer::singleShot(std::max(100, interval * 2), &eventLoop, &EventLoop::quit);
      eventLoop.exec();
      ASSERT_TRUE(!object.m_timerEventRecursed);
   }
}

class RecurringTimerObject : public Object
{
public:
   using DoneHandlerType = void ();
   PDK_DEFINE_SIGNAL_ENUMS(Done);
   PDK_DEFINE_SIGNAL_BINDER(Done)
   PDK_DEFINE_SIGNAL_EMITTER(Done)
   int m_times;
   int m_target;
   bool m_recurse;
   RecurringTimerObject(int target)
      : m_times(0),
        m_target(target),
        m_recurse(false)
   {}
   
   void timerEvent(TimerEvent *timerEvent)
   {
      if (++m_times == m_target) {
         killTimer(timerEvent->getTimerId());
         emitDoneSignal();
      } if (m_recurse) {
         EventLoop eventLoop;
         Timer::singleShot(100, &eventLoop, &EventLoop::quit);
         eventLoop.exec();
      }
   }
};

void recurring_timer_data(std::list<int> &data)
{
   data.push_back(0);
   data.push_back(1);
}

TEST(TimerTest, testRecurringTimer)
{
   const int target = 5;
   std::list<int> data;
   recurring_timer_data(data);
   for (int interval: data){

      {
         RecurringTimerObject object(target);
         object.connectDoneSignal(&TestEventLoop::instance(), &TestEventLoop::exitLoop);
         (void) object.startTimer(interval);
         TestEventLoop::instance().enterLoop(5);
         ASSERT_EQ(object.m_times, target);
      }
      {
         // make sure that eventloop recursion doesn't effect timer recurrance
         RecurringTimerObject object(target);
         object.m_recurse = true;
         object.connectDoneSignal(&TestEventLoop::instance(), &TestEventLoop::exitLoop);
         (void) object.startTimer(interval);
         TestEventLoop::instance().enterLoop(5);
         ASSERT_EQ(object.m_times, target);
      }
   }
}

TEST(TimerTest, testDeleteLaterOnTimer)
{
   Timer *timer = new Timer;
   timer->connectTimeoutSignal(timer, &Timer::deleteLater);
   timer->connectDestroyedSignal(&TestEventLoop::instance(), &TestEventLoop::exitLoop);
   timer->setInterval(1);
   timer->setSingleShot(true);
   timer->start();
   Pointer<Timer> pointer = timer;
   TestEventLoop::instance().enterLoop(5);
   ASSERT_TRUE(pointer.isNull());
}

#define MOVETOTHREAD_TIMEOUT 200
#define MOVETOTHREAD_WAIT 300

TEST(TimerTest, testMoveToThread)
{
#if defined(PDK_OS_WIN32)
   SUCCEED() << "Does not work reliably on Windows :(";
#elif defined(PDK_OS_MACOS)
   if (__builtin_available(macOS 10.12, *))
      SUCCEED() << "Does not work reliably on macOS 10.12 (BUG-59679)";
#endif
   Timer timer1;
   Timer timer2;
   timer1.start(MOVETOTHREAD_TIMEOUT);
   timer2.start(MOVETOTHREAD_TIMEOUT);
   ASSERT_TRUE((timer1.getTimerId() & 0xffffff) != (timer2.getTimerId() & 0xffffff));
   Thread thread;
   timer1.moveToThread(&thread);
   timer1.connectTimeoutSignal(&thread, &Thread::quit);
   thread.start();
   Timer timer3;
   timer3.start(MOVETOTHREAD_TIMEOUT);
   ASSERT_TRUE((timer3.getTimerId() & 0xffffff) != (timer2.getTimerId() & 0xffffff));
   ASSERT_TRUE((timer3.getTimerId() & 0xffffff) != (timer1.getTimerId() & 0xffffff));
   pdktest::wait(MOVETOTHREAD_WAIT);
   ASSERT_TRUE(thread.wait());
   timer2.stop();
   Timer timer4;
   timer4.start(MOVETOTHREAD_TIMEOUT);
   timer3.stop();
   timer2.start(MOVETOTHREAD_TIMEOUT);
   timer3.start(MOVETOTHREAD_TIMEOUT);
   ASSERT_TRUE((timer4.getTimerId() & 0xffffff) != (timer2.getTimerId() & 0xffffff));
   ASSERT_TRUE((timer3.getTimerId() & 0xffffff) != (timer2.getTimerId() & 0xffffff));
   ASSERT_TRUE((timer3.getTimerId() & 0xffffff) != (timer1.getTimerId() & 0xffffff));
}

class RestartedTimerFiresTooSoonObject : public Object
{  
public:
   BasicTimer m_timer;
   
   int m_interval;
   Time m_startedTime;
   EventLoop m_eventLoop;
   
   inline RestartedTimerFiresTooSoonObject()
      : Object(), m_interval(0)
   {}
   
   void timerFired()
   {
      static int interval = 1000;
      m_interval = interval;
      m_startedTime.start();
      m_timer.start(interval, this);
      // alternate between single-shot and 1 sec
      interval = interval ? 0 : 1000;
   }
   
   void timerEvent(TimerEvent* event)
   {
      if (event->getTimerId() != m_timer.getTimerId()) {
         return;
      }
      m_timer.stop();
      int elapsed = m_startedTime.elapsed();
      if (elapsed < m_interval / 2) {
         // severely too early!
         m_timer.stop();
         m_eventLoop.exit(-1);
         return;
      }
      timerFired();
      // don't do this forever
      static int count = 0;
      if (count++ > 20) {
         m_timer.stop();
         m_eventLoop.quit();
         return;
      }
   }
};

TEST(TimerTest, testRestartedTimerFiresTooSoon)
{
    RestartedTimerFiresTooSoonObject object;
    object.timerFired();
    ASSERT_EQ(object.m_eventLoop.exec(), 0);
}

class LongLastingSlotClass : public Object
{
public:
   LongLastingSlotClass(Timer *timer)
      : m_count(0),
        m_timer(timer)
   {}
   
public:
   void longLastingSlot()
   {
      // Don't use timers for this, because we are testing them.
      Time time;
      time.start();
      while (time.elapsed() < 200) {
         for (int c = 0; c < 100000; c++) {} // Mindless looping.
      }
      if (++m_count >= 2) {
         m_timer->stop();
      }
   }
   
public:
   int m_count;
   Timer *m_timer;
};

void timer_fires_only_once_per_process_events_data(std::list<int> &data)
{
   data.push_back(0);
   data.push_back(10);
}

TEST(TimerTest, testTimerFiresOnlyOncePerProcessEvents)
{
   std::list<int> data;
   timer_fires_only_once_per_process_events_data(data);
   for(int interval: data) {
      Timer timer;
      LongLastingSlotClass longSlot(&timer);
      timer.start(interval);
      timer.connectTimeoutSignal(&longSlot, &LongLastingSlotClass::longLastingSlot);
      // Loop because there may be other events pending.
      while (longSlot.m_count == 0) {
          CoreApplication::processEvents(EventLoop::WaitForMoreEvents);
      }
      ASSERT_EQ(longSlot.m_count, 1);
   }
}

class TimerIdPersistsAfterThreadExitThread : public Thread
{
public:
   Timer *m_timer;
   int m_timerId;
   int m_returnValue;
   
   TimerIdPersistsAfterThreadExitThread()
      : m_timer(nullptr),
        m_timerId(-1),
        m_returnValue(-1)
   {}
   ~TimerIdPersistsAfterThreadExitThread()
   {
      delete m_timer;
   }
   
   void run()
   {
      EventLoop eventLoop;
      m_timer = new Timer;
      m_timer->connectTimeoutSignal(&eventLoop, &EventLoop::quit);
      m_timer->start(100);
      m_timerId = m_timer->getTimerId();
      m_returnValue = eventLoop.exec();
   }
};

TEST(TimerTest, testTimerIdPersistsAfterThreadExit)
{
   TimerIdPersistsAfterThreadExitThread thread;
   thread.start();
   ASSERT_TRUE(thread.wait(30000));
   ASSERT_EQ(thread.m_returnValue, 0);
   // even though the thread has exited, and the event dispatcher destroyed, the timer is still
   // "active", meaning the timer id should NOT be reused (i.e. the event dispatcher should not
   // have unregistered it)
   int timerId = thread.startTimer(100);
   ASSERT_TRUE((timerId & 0xffffff) != (thread.m_timerId & 0xffffff));
}

TEST(TimerTest, testCancelLongTimer)
{
   Timer timer;
   timer.setSingleShot(true);
   timer.start(1000 * 60 * 60); //set timer for 1 hour
   CoreApplication::processEvents();
   ASSERT_TRUE(timer.isActive()); //if the timer completes immediately with an error, then this will fail
   timer.stop();
   ASSERT_TRUE(!timer.isActive());
}

TEST(TimerTest, testSingleShotStaticFunctionZeroTimeout)
{
   TimerHelper helper;
   Timer::singleShot(0, &helper, &TimerHelper::timeout);
   pdktest::wait(500);
   ASSERT_EQ(helper.m_count, 1);
   pdktest::wait(500);
   ASSERT_EQ(helper.m_count, 1);
   TimerHelper nhelper;
   Timer::singleShot(0, &nhelper, &TimerHelper::timeout);
   CoreApplication::processEvents();
   ASSERT_EQ(nhelper.m_count, 1);
   CoreApplication::processEvents();
   ASSERT_EQ(nhelper.m_count, 1);
}

class RecursOnTimeoutAndStopTimerTimer : public Object
{
public:
   Timer *m_one;
   Timer *m_two;
   
public:
   void onetrigger()
   {
      CoreApplication::processEvents();
   }
   
   void twotrigger()
   {
      m_one->stop();
   }
};

TEST(TimerTest, testRecurseOnTimeoutAndStopTimer)
{
   EventLoop eventLoop;
   Timer::singleShot(1000, &eventLoop, &EventLoop::quit);
   RecursOnTimeoutAndStopTimerTimer t;
   t.m_one = new Timer(&t);
   t.m_two = new Timer(&t);
   t.m_one->connectTimeoutSignal(&t, &RecursOnTimeoutAndStopTimerTimer::onetrigger);
   t.m_two->connectTimeoutSignal(&t, &RecursOnTimeoutAndStopTimerTimer::twotrigger);
   t.m_two->setSingleShot(true);
   t.m_one->start();
   t.m_two->start();
   (void) eventLoop.exec();
   ASSERT_TRUE(!t.m_one->isActive());
   ASSERT_TRUE(!t.m_two->isActive());
}

struct CountedStruct
{
   CountedStruct(int *count, Thread *t = nullptr)
      : m_count(count),
        m_thread(t)
   {}
   
   ~CountedStruct()
   {}
   
   void operator()() const
   {
      ++(*m_count);
      if (m_thread) {
         ASSERT_EQ(Thread::getCurrentThread(), m_thread);
      }
   }
   
   int *m_count;
   Thread *m_thread;
};

static ScopedPointer<EventLoop> sg_eventLoop;
static Thread *sg_thread = nullptr;

class StaticEventLoop
{
public:
   static void quitEventLoop()
   {
      quitEventLoopNoexcept();
   }
   
   static void quitEventLoopNoexcept() noexcept
   {
      ASSERT_TRUE(!sg_eventLoop.isNull());
      sg_eventLoop->quit();
      if (sg_thread) {
         ASSERT_EQ(Thread::getCurrentThread(), sg_thread);
      }
   }
};

TEST(TimerTest, testSingleShotToFunctors)
{
   int count = 0;
   sg_eventLoop.reset(new EventLoop);
   EventLoop e;

   Timer::singleShot(0, CountedStruct(&count));
   CoreApplication::processEvents();
   ASSERT_EQ(count, 1);

   Timer::singleShot(0, &StaticEventLoop::quitEventLoop);
   ASSERT_EQ(sg_eventLoop->exec(), 0);

   Timer::singleShot(0, &StaticEventLoop::quitEventLoopNoexcept);
   ASSERT_EQ(sg_eventLoop->exec(), 0);

   Thread t1;
   Object c1;
   c1.moveToThread(&t1);
   t1.connectStartedSignal(&e, &EventLoop::quit);
   t1.start();
   ASSERT_EQ(e.exec(), 0);

   Timer::singleShot(0, &c1, CountedStruct(&count, &t1));
   pdktest::wait(500);
   ASSERT_EQ(count, 2);

   t1.quit();
   t1.wait();

   sg_thread = new Thread;
   Object c2;
   c2.moveToThread(sg_thread);
   sg_thread->connectStartedSignal(&e, &EventLoop::quit);
   sg_thread->start();
   ASSERT_EQ(e.exec(), 0);

   Timer::singleShot(0, &c2, &StaticEventLoop::quitEventLoop);
   ASSERT_EQ(sg_eventLoop->exec(), 0);

   sg_thread->quit();
   sg_thread->wait();
   sg_thread->deleteLater();
   sg_thread = nullptr;

   {
      Object c3;
      Timer::singleShot(500, &c3, CountedStruct(&count));
   }
   pdktest::wait(800);
   ASSERT_EQ(count, 2);

   Timer::singleShot(0, [&count] { ++count; });
   CoreApplication::processEvents();
   ASSERT_EQ(count, 3);

   Object context;
   Thread thread;

   context.moveToThread(&thread);
   thread.connectStartedSignal(&e, &EventLoop::quit);
   thread.start();
   ASSERT_EQ(e.exec(), 0);

   Timer::singleShot(0, &context, [&count, &thread]()
   { 
      ++count;
      ASSERT_EQ(Thread::getCurrentThread(), &thread);
   });
   pdktest::wait(500);
   ASSERT_EQ(count, 4);

   thread.quit();
   thread.wait();

   sg_eventLoop.reset();
   sg_thread = nullptr;
}

TEST(TimerTest, testSingleShotChrono)
{
   // duplicates singleShotStaticFunctionZeroTimeout and singleShotToFunctors
   using namespace std::chrono;
   TimerHelper helper;

   Timer::singleShot(hours(0), &helper, &TimerHelper::timeout);
   pdktest::wait(500);
   ASSERT_EQ(helper.m_count, 1);
   pdktest::wait(500);
   ASSERT_EQ(helper.m_count, 1);

   TimerHelper nhelper;

   Timer::singleShot(seconds(0), &nhelper, &TimerHelper::timeout);
   CoreApplication::processEvents();
   ASSERT_EQ(nhelper.m_count, 1);
   CoreApplication::processEvents();
   ASSERT_EQ(nhelper.m_count, 1);

   int count = 0;
   Timer::singleShot(to_ms(microseconds(0)), CountedStruct(&count));
   CoreApplication::processEvents();
   ASSERT_EQ(count, 1);

   sg_eventLoop.reset(new EventLoop);
   Timer::singleShot(0, &StaticEventLoop::quitEventLoop);
   ASSERT_EQ(sg_eventLoop->exec(), 0);

   Object c3;
   Timer::singleShot(milliseconds(500), &c3, CountedStruct(&count));
   pdktest::wait(800);
   ASSERT_EQ(count, 2);

   Timer::singleShot(0, [&count] { ++count; });
   CoreApplication::processEvents();
   ASSERT_EQ(count, 3);

   sg_eventLoop.reset();
}

class DontBlockEvents : public Object
{
public:
   DontBlockEvents();
   void timerEvent(TimerEvent *);
   
   int m_count;
   int m_total;
   BasicTimer m_timer;
   
public:
   void paintEvent();
   
};

DontBlockEvents::DontBlockEvents()
{
   m_count = 0;
   m_total = 0;
   
   // need a few unrelated timers running to reproduce the bug.
   (new Timer(this))->start(2000);
   (new Timer(this))->start(2500);
   (new Timer(this))->start(3000);
   (new Timer(this))->start(5000);
   (new Timer(this))->start(1000);
   (new Timer(this))->start(2000);
   
   m_timer.start(1, this);
}

void DontBlockEvents::timerEvent(TimerEvent* event)
{
   if (event->getTimerId() == m_timer.getTimerId()) {
      CallableInvoker::invokeAsync(this, &DontBlockEvents::paintEvent);
      m_timer.start(0, this);
      m_count++;
      ASSERT_EQ(m_count, 1);
      m_total++;
   }
}

void DontBlockEvents::paintEvent()
{
   m_count--;
   ASSERT_EQ(m_count, 0);
}

// This is a regression test for BUG-13633, where a timer with a zero
// timeout that was restarted by the event handler could starve other timers.
TEST(TimerTest, testDontBlockEvents)
{
    DontBlockEvents t;
    pdktest::wait(60);
    PDK_TRY_VERIFY(t.m_total > 2);
}

class SlotRepeater : public Object {
public:
   SlotRepeater() {}
   
public:
   void repeatThisSlot()
   {
      CallableInvoker::invokeAsync(this, &SlotRepeater::repeatThisSlot);
   }
};

TEST(TimerTest, testPostedEventsShouldNotStarveTimers)
{
   TimerHelper timerHelper;
   Timer timer;
   timer.connectTimeoutSignal(&timerHelper, &TimerHelper::timeout);
   timer.setInterval(0);
   timer.setSingleShot(false);
   timer.start();
   SlotRepeater slotRepeater;
   slotRepeater.repeatThisSlot();
   pdktest::wait(100);
   ASSERT_TRUE(timerHelper.m_count > 5);
}

struct DummyFunctor {
    void operator()() {}
};

TEST(TimerTest, testCrossThreadSingleShotToFunctor)
{
    // We're testing for crashes here, so the test simply running to
    // completion is considered a success
    Thread t;
    t.start();
    Object* o = new Object();
    o->moveToThread(&t);

    for (int i = 0; i < 10000; i++) {
        Timer::singleShot(0, o, DummyFunctor());
    }
    t.quit();
    t.wait();
    delete o;
}

