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
// Created by softboy on 2018/03/29.

#include "gtest/gtest.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/CoreEvent.h"
#include "pdk/kernel/EventLoop.h"
#include "pdk/kernel/CallableInvoker.h"
#include "pdk/kernel/internal/EventLoopPrivate.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#if defined(PDK_OS_UNIX)
#include "pdk/kernel/internal/CoreUnixPrivate.h"
#include "pdk/kernel/EventDispatcherUnix.h"
#endif
#include "pdk/base/os/thread/Thread.h"
#include "pdk/kernel/Timer.h"
#include "pdktest/PdkTest.h"
#include <condition_variable>
#include <mutex>
#include <iostream>

using pdk::kernel::EventLoop;
using pdk::kernel::Object;
using pdk::os::thread::Thread;
using pdk::kernel::Event;
using pdk::kernel::internal::EventLoopPrivate;
using pdk::kernel::TimerEvent;
using pdk::kernel::Timer;
using pdk::kernel::internal::ObjectPrivate;
using pdk::kernel::AbstractEventDispatcher;
using pdk::kernel::EventDispatcherUNIX;
using pdk::kernel::CoreApplication;
using pdk::kernel::CallableInvoker;
using pdk::kernel::EventLoopLocker;

class EventLoopExiter : public Object
{
   EventLoop *m_eventLoop;
public:
   inline EventLoopExiter(EventLoop *el)
      : m_eventLoop(el)
   {}
public:
   void exit();
   void exit1();
   void exit2();
};

void EventLoopExiter::exit()
{
   m_eventLoop->exit();
}

void EventLoopExiter::exit1()
{
   m_eventLoop->exit(1);
}

void EventLoopExiter::exit2()
{
   m_eventLoop->exit(2);
}

class EventLoopThread : public Thread
{
public:
   using CheckPointHandlerType = void();
   PDK_DEFINE_SIGNAL_ENUMS(CheckPoint);
   PDK_DEFINE_SIGNAL_BINDER(CheckPoint)
   PDK_DEFINE_SIGNAL_EMITTER(CheckPoint)
   
   public:
      EventLoop *m_eventLoop;
   void run();
};

void EventLoopThread::run()
{
   m_eventLoop = new EventLoop;
   emitCheckPointSignal();
   (void) m_eventLoop->exec();
   delete m_eventLoop;
   m_eventLoop = nullptr;
}

class MultipleExecThread : public Thread
{
   using CheckPointHandlerType = void();
   PDK_DEFINE_SIGNAL_ENUMS(CheckPoint);
   PDK_DEFINE_SIGNAL_BINDER(CheckPoint)
   PDK_DEFINE_SIGNAL_EMITTER(CheckPoint)
   public:
      std::mutex m_mutex;
   std::condition_variable m_cond;
   volatile int m_result1;
   volatile int m_result2;
   MultipleExecThread()
      : m_result1(0xdead),
        m_result2(0xbeef)
   {}
   
   void run()
   {
      std::unique_lock<std::mutex> locker(m_mutex);
      // this exec should work
      
      m_cond.notify_one();
      m_cond.wait(locker);
      
      Timer timer;
      timer.connectTimeoutSignal(this, &MultipleExecThread::quit, pdk::ConnectionType::DirectConnection);
      timer.setInterval(1000);
      timer.start();
      m_result1 = exec();
      
      // this should return immediately, since exit() has been called
      m_cond.notify_one();
      m_cond.wait(locker);
      EventLoop eventLoop;
      m_result2 = eventLoop.exec();
   }
};

class StartStopEvent: public Event
{
public:
   explicit StartStopEvent(Event::Type type, EventLoop *loop = nullptr)
      : Event(Type(type)),
        m_el(loop)
   { }
   
   EventLoop *m_el;
};

class EventLoopExecutor : public Object
{
   EventLoop *m_eventLoop;
public:
   int m_returnCode;
   EventLoopExecutor(EventLoop *eventLoop)
      : Object(),
        m_eventLoop(eventLoop),
        m_returnCode(-42)
   {
   }
public:
   void exec()
   {
      Timer::singleShot(100, m_eventLoop, &EventLoop::quit);
      // this should return immediately, and the timer event should be delivered to
      // EventLoopTest::exec() test, letting the test complete
      m_returnCode = m_eventLoop->exec();
   }
};

class EventDispatcherSignalUnlock
{
public:
   EventDispatcherSignalUnlock(AbstractEventDispatcher *dispatcher)
      :m_dispatcher(dispatcher)
   {}
   ~EventDispatcherSignalUnlock()
   {
      m_dispatcher->disconnectAboutToBlockSignal();
      m_dispatcher->disconnectAwakeSignal();
   }
private:
   AbstractEventDispatcher *m_dispatcher;
};

PDKTEST_DECLARE_APP_STARTUP_ARGS();

//TEST(EventLoopTest, testProcessEvents)
//{
//   PDKTEST_BEGIN_APP_CONTEXT();
//   int aboutToBlockCount = 0;
//   int awakeCount = 0;
//   AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance();
//   //EventDispatcherSignalUnlock signalLocker(eventDispatcher);
//   eventDispatcher->connectAboutToBlockSignal([&aboutToBlockCount](){
//      aboutToBlockCount++;
//   });
//   eventDispatcher->connectAwakeSignal([&awakeCount](){
//      awakeCount++;
//   });
//   Object object;
//   EventLoop eventLoop;
//   CoreApplication::postEvent(&eventLoop, new Event(Event::Type::User));
//   // process posted events, EventLoop::processEvents() should return
//   // true
//   ASSERT_TRUE(eventLoop.processEvents());
//   ASSERT_EQ(aboutToBlockCount, 0);
//   ASSERT_EQ(awakeCount, 1);
   
//   // allow any session manager to complete its handshake, so that
//   // there are no pending events left. This tests that we are able
//   // to process all events from the queue, otherwise it will hang.
//   while (eventLoop.processEvents())
//      ;
//   // make sure the test doesn't block forever
//   int timerId = object.startTimer(100);
//   aboutToBlockCount = 0;
//   awakeCount = 0;
//   ASSERT_TRUE(eventLoop.processEvents(EventLoop::WaitForMoreEvents));
//   ASSERT_TRUE(awakeCount > 0);
//   ASSERT_TRUE(awakeCount >= aboutToBlockCount);
//   object.killTimer(timerId);
//   PDKTEST_END_APP_CONTEXT();
//}

#define EXEC_TIMEOUT 100

//TEST(EventLoopTest, testExec)
//{
//   PDKTEST_BEGIN_APP_CONTEXT();
//   {
//      EventLoop eventLoop;
//      EventLoopExiter exiter(&eventLoop);
//      int returnCode;
      
//      Timer::singleShot(EXEC_TIMEOUT, &exiter, &EventLoopExiter::exit);
//      returnCode = eventLoop.exec();
//      ASSERT_EQ(returnCode, 0);
      
//      Timer::singleShot(EXEC_TIMEOUT, &exiter, &EventLoopExiter::exit1);
//      returnCode = eventLoop.exec();
//      ASSERT_EQ(returnCode, 1);
      
//      Timer::singleShot(EXEC_TIMEOUT, &exiter, &EventLoopExiter::exit2);
//      returnCode = eventLoop.exec();
//      ASSERT_EQ(returnCode, 2);
//   }
   
//   {
//      // calling EventLoop::exec() after a thread loop has exit()ed should return immediately
//      // Note: this behaviour differs from CoreApplication and EventLoop
//      // see CoreApplicationTest::testEventLoopExecAfterExit, EventLoopTest::testReexec
//      MultipleExecThread thread;
      
//      // start thread and wait for checkpoint
//      std::unique_lock<std::mutex> locker(thread.m_mutex);
//      thread.start();
//      thread.m_cond.wait(locker);
      
//      // make sure the eventloop runs
//      AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance(&thread);
//      int awakeCount = 0;
//      eventDispatcher->connectAwakeSignal([&awakeCount]() {
//         ++awakeCount;
//      });
      
//      thread.m_cond.notify_one();
//      thread.m_cond.wait(locker);
//      ASSERT_TRUE(awakeCount > 0);
//      int v = thread.m_result1;
//      ASSERT_EQ(v, 0);
      
//      // exec should return immediately
//      awakeCount = 0;
//      thread.m_cond.notify_one();
//      thread.m_mutex.unlock();
//      thread.wait();
//      ASSERT_EQ(awakeCount, 0);
//      v = thread.m_result2;
//      ASSERT_EQ(v, -1);
//   }
   
//   {
//      // a single instance of EventLoop should not be allowed to recurse into exec()
//      EventLoop eventLoop;
//      EventLoopExecutor executor(&eventLoop);
      
//      Timer::singleShot(EXEC_TIMEOUT, &executor, &EventLoopExecutor::exec);
//      int returnCode = eventLoop.exec();
//      ASSERT_EQ(returnCode, 0);
//      ASSERT_EQ(executor.m_returnCode, -1);
//   }
//   PDKTEST_END_APP_CONTEXT();
//}

//TEST(EventLoopTest, testReexec)
//{
//   PDKTEST_BEGIN_APP_CONTEXT();
//   EventLoop loop;
//   CallableInvoker::invokeAsync([&loop](){
//      loop.quit();
//   }, &loop);
//   // exec once
//   ASSERT_EQ(loop.exec(), 0);
   
//   // and again
//   CallableInvoker::invokeAsync([&loop](){
//      loop.quit();
//   }, &loop);
   
//   // exec once
//   ASSERT_EQ(loop.exec(), 0);
//   PDKTEST_END_APP_CONTEXT();
//}

//TEST(EventLoopTest, testWakeUp)
//{
//   PDKTEST_BEGIN_APP_CONTEXT();
//   int awakeCount = 0;
   
//   EventLoopThread thread;
//   EventLoop eventLoop;
//   thread.connectCheckPointSignal(&eventLoop, &EventLoop::quit);
//   thread.connectFinishedSignal(&eventLoop, &EventLoop::quit);
   
//   thread.start();
//   (void) eventLoop.exec();
//   AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance();
//   EventDispatcherSignalUnlock signalLocker(eventDispatcher);
//   eventDispatcher->connectAwakeSignal([&awakeCount](){
//      ++awakeCount;
//   });
//   thread.m_eventLoop->wakeUp();
//   // give the thread time to wake up
//   Timer::singleShot(1000, &eventLoop, &EventLoop::quit);
//   (void) eventLoop.exec();
//   ASSERT_TRUE(awakeCount);
//   thread.quit();
//   (void) eventLoop.exec();
//   PDKTEST_END_APP_CONTEXT();
//}

//TEST(EventLoopTest, testQuit)
//{
//   PDKTEST_BEGIN_APP_CONTEXT();
//   EventLoop eventLoop;
//   int returnCode;
//   Timer::singleShot(100, &eventLoop, &EventLoop::quit);
//   returnCode = eventLoop.exec();
//   ASSERT_EQ(returnCode, 0);
//   PDKTEST_END_APP_CONTEXT();
//}

class EventLoopObject: public Object
{
public:
   void customEvent(Event *event)
   {
      if (event->getType() == Event::Type::User) {
         EventLoop loop;
         CoreApplication::postEvent(this, new StartStopEvent(Event::Type::MaxUser, &loop));
         loop.exec();
      } else {
         static_cast<StartStopEvent *>(event)->m_el->exit();
      }
   }
};

//TEST(EventLoopTest, testNestedLoops)
//{
//   PDKTEST_BEGIN_APP_CONTEXT();
//   EventLoopObject obj;
//   CoreApplication::postEvent(&obj, new StartStopEvent(Event::Type::User));
//   CoreApplication::postEvent(&obj, new StartStopEvent(Event::Type::User));
//   CoreApplication::postEvent(&obj, new StartStopEvent(Event::Type::User));
   
//   // without the fix, this will *wedge* and never return
//   pdktest::wait(1000);
//   PDKTEST_END_APP_CONTEXT();
//}

class TimerReceiver : public Object
{
public:
   int m_gotTimerEvent;
   
   TimerReceiver()
      :m_gotTimerEvent(-1)
   {}
   
   void timerEvent(TimerEvent *event)
   {
      m_gotTimerEvent = event->getTimerId();
   }
};

//TEST(EventLoopTest, testProcessEventsExcludeTimers)
//{
//   PDKTEST_BEGIN_APP_CONTEXT();
//   TimerReceiver timerReceiver;
//   int timerId = timerReceiver.startTimer(0);
   
//   EventLoop eventLoop;
   
//   // normal process events will send timers
//   eventLoop.processEvents();
//   ASSERT_EQ(timerReceiver.m_gotTimerEvent, timerId);
//   timerReceiver.m_gotTimerEvent = -1;
//   // but not if we exclude timers
//   eventLoop.processEvents(EventLoop::X11ExcludeTimers);
   
//#if defined(PDK_OS_UNIX)
//   AbstractEventDispatcher *eventDispatcher = CoreApplication::getEventDispatcher();
//   if (!dynamic_cast<EventDispatcherUNIX *>(eventDispatcher)
//       )
//#endif
//      std::cerr << "X11ExcludeTimers only supported in the UNIX/Glib dispatchers" << std::endl;
   
//   ASSERT_EQ(timerReceiver.m_gotTimerEvent, -1);
//   timerReceiver.m_gotTimerEvent = -1;
   
//   // resume timer processing
//   eventLoop.processEvents();
//   ASSERT_EQ(timerReceiver.m_gotTimerEvent, timerId);
//   timerReceiver.m_gotTimerEvent = -1;
//   PDKTEST_END_APP_CONTEXT();
//}

namespace DeliverInDefinedOrder {

enum { NbThread = 3,  NbObject = 500, NbEventQueue = 5, NbEvent = 50 };

struct CustomEvent : public Event
{
   CustomEvent(int q, int v) 
      : Event(Type(pdk::as_integer<Type>(Type::User) + q)),
        m_value(v)
   {}
   int m_value;
};

struct MyObject : public Object {
public:
   MyObject() 
      : m_count(0)
   {
      for (int i = 0; i < NbEventQueue;  i++) {
         m_lastReceived[i] = -1;
      }
   }
   int m_lastReceived[NbEventQueue];
   int m_count;
   virtual void customEvent(Event* event)
   {
      ASSERT_TRUE(pdk::as_integer<Event::Type>(event->getType()) >= pdk::as_integer<Event::Type>(Event::Type::User));
      ASSERT_TRUE(pdk::as_integer<Event::Type>(event->getType()) < pdk::as_integer<Event::Type>(Event::Type::User) + 5);
      uint idx = pdk::as_integer<Event::Type>(event->getType()) - pdk::as_integer<Event::Type>(Event::Type::User);
      int value = static_cast<CustomEvent *>(event)->m_value;
      ASSERT_TRUE(m_lastReceived[idx] < value);
      m_lastReceived[idx] = value;
      m_count++;
   }
   
public:
   void moveToThread(Thread *t) {
      Object::moveToThread(t);
   }
};

}

//TEST(EventLoopTest, testDeliverInDefinedOrder)
//{
//   PDKTEST_BEGIN_APP_CONTEXT();
//   using namespace DeliverInDefinedOrder;
//   Thread threads[NbThread];
//   MyObject objects[NbObject];
//   for (int t = 0; t < NbThread; t++) {
//      threads[t].start();
//   }
   
//   int event = 0;
   
//   for (int o = 0; o < NbObject; o++) {
//      objects[o].moveToThread(&threads[o % NbThread]);
//      for (int e = 0; e < NbEvent; e++) {
//         int q = e % NbEventQueue;
//         CoreApplication::postEvent(&objects[o], new CustomEvent(q, ++event) , pdk::EventPriority(q));
//         if (e % 7) {
//            CallableInvoker::invokeAsync([&objects, o](Thread *thread){
//               objects[o].moveToThread(thread);
//            }, &objects[o], &threads[(e+o)%NbThread]);
//         }
//      }
//   }
//   pdktest::wait(30);
//   for (int o = 0; o < NbObject; o++) {
//      PDK_TRY_COMPARE(objects[o].m_count, int(NbEvent));
//   }
   
//   for (int t = 0; t < NbThread; t++) {
//      threads[t].quit();
//      threads[t].wait();
//   }
//   PDKTEST_END_APP_CONTEXT();
//}

class JobObject : public Object
{
public:
   using DoneHandlerType = void(int);
   PDK_DEFINE_SIGNAL_ENUMS(Done);
   PDK_DEFINE_SIGNAL_BINDER(Done)
   PDK_DEFINE_SIGNAL_EMITTER(Done)
   explicit JobObject(EventLoop *loop, Object *parent = nullptr)
      : Object(parent),
        m_locker(loop)
   {
   }
   
   explicit JobObject(Object *parent = nullptr)
      : Object(parent)
   {
   }
   
public:
   void start(int timeout = 200)
   {
      Timer::singleShot(timeout, this, &JobObject::timeout);
   }
   
private:
   void timeout()
   {
      emitDoneSignal(200);
      deleteLater();
   }
   
private:
   EventLoopLocker m_locker;
};

TEST(EventLoopTest, testQuitLock)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   EventLoop eventLoop;
   EventLoopPrivate* privateClass = static_cast<EventLoopPrivate*>(ObjectPrivate::get(&eventLoop));
//   ASSERT_EQ(privateClass->m_quitLockRef.load(), 0);
   JobObject *job1 = new JobObject(&eventLoop, PDK_RETRIEVE_APP_INSTANCE());
   job1->start(500);
//   ASSERT_EQ(privateClass->m_quitLockRef.load(), 1);
   eventLoop.exec();
//   ASSERT_EQ(privateClass->m_quitLockRef.load(), 0);
   job1 = new JobObject(&eventLoop, PDK_RETRIEVE_APP_INSTANCE());
   job1->start(200);
   JobObject *previousJob = job1;
   for (int i = 0; i < 9; ++i) {
      JobObject *subJob = new JobObject(&eventLoop, PDK_RETRIEVE_APP_INSTANCE());
      previousJob->connectDoneSignal(subJob, &JobObject::start);
      previousJob = subJob;
   }
   eventLoop.exec();
   std::cout << "event loop exit" << std::endl;
   PDKTEST_END_APP_CONTEXT();
}
