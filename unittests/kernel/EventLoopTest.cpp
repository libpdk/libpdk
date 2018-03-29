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
using pdk::kernel::Timer;
using pdk::kernel::AbstractEventDispatcher;
using pdk::kernel::CoreApplication;
using pdk::kernel::CallableInvoker;

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

TEST(EventLoopTest, testProcessEvents)
{
   int aboutToBlockCount = 0;
   int awakeCount = 0;
   AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance();
   EventDispatcherSignalUnlock signalLocker(eventDispatcher);
   eventDispatcher->connectAboutToBlockSignal([&aboutToBlockCount](){
      aboutToBlockCount++;
   });
   eventDispatcher->connectAwakeSignal([&awakeCount](){
      awakeCount++;
   });
   Object object;
   EventLoop eventLoop;
   CoreApplication::postEvent(&eventLoop, new Event(Event::Type::User));
   // process posted events, EventLoop::processEvents() should return
   // true
   ASSERT_TRUE(eventLoop.processEvents());
   ASSERT_EQ(aboutToBlockCount, 0);
   ASSERT_EQ(awakeCount, 1);
   
   // allow any session manager to complete its handshake, so that
   // there are no pending events left. This tests that we are able
   // to process all events from the queue, otherwise it will hang.
   while (eventLoop.processEvents())
      ;
   // make sure the test doesn't block forever
   int timerId = object.startTimer(100);
   aboutToBlockCount = 0;
   awakeCount = 0;
   ASSERT_TRUE(eventLoop.processEvents(EventLoop::WaitForMoreEvents));
   ASSERT_TRUE(awakeCount > 0);
   ASSERT_TRUE(awakeCount >= aboutToBlockCount);
   object.killTimer(timerId);
}

#define EXEC_TIMEOUT 100

TEST(EventLoopTest, testExec)
{
   {
      EventLoop eventLoop;
      EventLoopExiter exiter(&eventLoop);
      int returnCode;
      
      Timer::singleShot(EXEC_TIMEOUT, &exiter, &EventLoopExiter::exit);
      returnCode = eventLoop.exec();
      ASSERT_EQ(returnCode, 0);
      
      Timer::singleShot(EXEC_TIMEOUT, &exiter, &EventLoopExiter::exit1);
      returnCode = eventLoop.exec();
      ASSERT_EQ(returnCode, 1);
      
      Timer::singleShot(EXEC_TIMEOUT, &exiter, &EventLoopExiter::exit2);
      returnCode = eventLoop.exec();
      ASSERT_EQ(returnCode, 2);
   }
   
   {
      // calling EventLoop::exec() after a thread loop has exit()ed should return immediately
      // Note: this behaviour differs from CoreApplication and EventLoop
      // see CoreApplicationTest::testEventLoopExecAfterExit, EventLoopTest::testReexec
      MultipleExecThread thread;
      
      // start thread and wait for checkpoint
      std::unique_lock<std::mutex> locker(thread.m_mutex);
      thread.start();
      thread.m_cond.wait(locker);
      
      // make sure the eventloop runs
      AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance(&thread);
      int awakeCount = 0;
      eventDispatcher->connectAwakeSignal([&awakeCount]() {
         ++awakeCount;
      });
      
      thread.m_cond.notify_one();
      thread.m_cond.wait(locker);
      ASSERT_TRUE(awakeCount > 0);
      int v = thread.m_result1;
      ASSERT_EQ(v, 0);
      
      // exec should return immediately
      awakeCount = 0;
      thread.m_cond.notify_one();
      thread.m_mutex.unlock();
      thread.wait();
      ASSERT_EQ(awakeCount, 0);
      v = thread.m_result2;
      ASSERT_EQ(v, -1);
   }
   
   {
      // a single instance of EventLoop should not be allowed to recurse into exec()
      EventLoop eventLoop;
      EventLoopExecutor executor(&eventLoop);
      
      Timer::singleShot(EXEC_TIMEOUT, &executor, &EventLoopExecutor::exec);
      int returnCode = eventLoop.exec();
      ASSERT_EQ(returnCode, 0);
      ASSERT_EQ(executor.m_returnCode, -1);
   }
}

TEST(EventLoopTest, testReexec)
{
   EventLoop loop;
   CallableInvoker::invokeAsync([&loop](){
      loop.quit();
   }, &loop);
   // exec once
   ASSERT_EQ(loop.exec(), 0);
   
   // and again
   CallableInvoker::invokeAsync([&loop](){
      loop.quit();
   }, &loop);
   
   // exec once
   ASSERT_EQ(loop.exec(), 0);
}

TEST(EventLoopTest, testWakeUp)
{
   int awakeCount = 0;
   
   EventLoopThread thread;
   EventLoop eventLoop;
   thread.connectCheckPointSignal(&eventLoop, &EventLoop::quit);
   thread.connectFinishedSignal(&eventLoop, &EventLoop::quit);
   
   thread.start();
   (void) eventLoop.exec();
   AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance();
   EventDispatcherSignalUnlock signalLocker(eventDispatcher);
   eventDispatcher->connectAwakeSignal([&awakeCount](){
      ++awakeCount;
   });
   thread.m_eventLoop->wakeUp();
   // give the thread time to wake up
   Timer::singleShot(1000, &eventLoop, &EventLoop::quit);
   (void) eventLoop.exec();
   ASSERT_TRUE(awakeCount);
   thread.quit();
   (void) eventLoop.exec();
}

TEST(EventLoopTest, testQuit)
{
   EventLoop eventLoop;
   int returnCode;
   Timer::singleShot(100, &eventLoop, &EventLoop::quit);
   returnCode = eventLoop.exec();
   ASSERT_EQ(returnCode, 0);
}

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

TEST(EventLoopTest, testNestedLoops)
{
   EventLoopObject obj;
   CoreApplication::postEvent(&obj, new StartStopEvent(Event::Type::User));
   CoreApplication::postEvent(&obj, new StartStopEvent(Event::Type::User));
   CoreApplication::postEvent(&obj, new StartStopEvent(Event::Type::User));
   
   // without the fix, this will *wedge* and never return
   pdktest::wait(1000);
}
