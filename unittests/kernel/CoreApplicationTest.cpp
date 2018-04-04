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
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/CallableInvoker.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/kernel/Object.h"
#include "pdk/kernel/Timer.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/base/lang/String.h"
#include "pdk/kernel/Pointer.h"
#include <list>
#include <string>
#include <tuple>
#include <iostream>

using pdk::kernel::CoreApplication;
using pdk::kernel::CallableInvoker;
using pdk::kernel::Object;
using pdk::kernel::Event;
using pdk::kernel::Timer;
using pdk::kernel::internal::ObjectPrivate;
using pdk::kernel::EventLoop;
using pdk::kernel::EventLoopLocker;
using pdk::kernel::SocketNotifier;
using pdk::kernel::internal::CoreApplicationPrivate;
using pdk::kernel::AbstractEventDispatcher;
using pdk::os::thread::internal::DaemonThread;
using pdk::os::thread::Thread;
using pdk::time::Time;
using pdk::os::thread::internal::ThreadData;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::kernel::Pointer;
using pdk::utils::ScopedPointer;

int sg_argc;
char **sg_argv;

class EventSpy : public Object
{
public:
   std::list<Event::Type> m_recordedEvents;
   bool eventFilter(Object *, Event *event)
   {
      m_recordedEvents.push_back(event->getType());
      return false;
   }
};

class ThreadedEventReceiver : public Object
{
public:
   std::list<Event::Type> m_recordedEvents;
   bool event(Event *event) override
   {
      if (event->getType() != Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)) {
         return Object::event(event);
      }
      m_recordedEvents.push_back(event->getType());
      Thread::getCurrentThread()->quit();
      CoreApplication::quit();
      moveToThread(nullptr);
      return true;
   }
};

class MyThread : public DaemonThread
{
   void run() override
   {
      ThreadData *data = ThreadData::current();
      ASSERT_TRUE(!data->m_requiresCoreApplication);        // daemon thread
      data->m_requiresCoreApplication = m_requiresCoreApplication;
      Thread::run();
   }
   
public:
   MyThread() : m_requiresCoreApplication(true) {}
   bool m_requiresCoreApplication;
};

TEST(CoreApplicationTest, testSendEventsOnProcessEvents)
{
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   
   EventSpy spy;
   app.installEventFilter(&spy);
   
   CoreApplication::postEvent(&app,  new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)));
   CoreApplication::processEvents();
   ASSERT_TRUE(spy.m_recordedEvents.end() != std::find(spy.m_recordedEvents.begin(),
                                                       spy.m_recordedEvents.end(),
                                                       Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)));
}

TEST(CoreApplicationTest, testGetSetCheck)
{
   String version = CoreApplication::getAppVersion();
   version = Latin1String("1.0.0 alpha");
   CoreApplication::setAppVersion(version);
   ASSERT_EQ(CoreApplication::getAppVersion(), version);
   version = String();
   CoreApplication::setAppVersion(version);
   ASSERT_EQ(CoreApplication::getAppVersion(), version);
}

TEST(CoreApplicationTest, testAppName)
{
   const char* appName = "CoreAplicationTest";
   {
      int argc = 1;
      char *argv[] = { const_cast<char *>(appName) };
      CoreApplication app(argc, argv);
      ASSERT_EQ(pdk::kernel::retrieve_app_name(), String::fromLatin1(appName));
      ASSERT_EQ(CoreApplication::getAppName(), String::fromLatin1(appName));
   }
   // The application name should still be available after destruction;
   // global statics often rely on this.
   ASSERT_EQ(CoreApplication::getAppName(), String::fromLatin1(appName));
   // Setting the appname before creating the application should work (QTBUG-45283)
   const String wantedAppName(Latin1String("YetAnotherApp"));
   {
      int argc = 1;
      char *argv[] = { const_cast<char*>(appName) };
      CoreApplication::setAppName(wantedAppName);
      CoreApplication app(argc, argv);
      ASSERT_EQ(pdk::kernel::retrieve_app_name(), String::fromLatin1(appName));
      ASSERT_EQ(CoreApplication::getAppName(), wantedAppName);
   }
   ASSERT_EQ(CoreApplication::getAppName(), wantedAppName);
   
   // Restore to initial value
   CoreApplication::setAppName(String());
   ASSERT_EQ(CoreApplication::getAppName(), String());
}

TEST(CoreApplicationTest, testAppVersion)
{
#if defined(PDK_OS_WIN)
   const char appVersion[] = "1.2.3.4";
#elif defined(PDK_OS_DARWIN)
   const char appVersion[] = "1.2.3";
#else
   const char appVersion[] = "";
#endif
   {
      int argc = 0;
      char *argv[] = { nullptr };
      CoreApplication app(argc, argv);
      CoreApplication::setAppVersion(String::fromLatin1(appVersion));
      ASSERT_EQ(CoreApplication::getAppVersion(), String::fromLatin1(appVersion));
   }
   // The application version should still be available after destruction
   ASSERT_EQ(CoreApplication::getAppVersion(), String::fromLatin1(appVersion));
   // Setting the appversion before creating the application should work
   const String wantedAppVersion(Latin1String("0.0.1"));
   {
      int argc = 0;
      char *argv[] = { nullptr };
      CoreApplication::setAppVersion(wantedAppVersion);
      CoreApplication app(argc, argv);
      ASSERT_EQ(CoreApplication::getAppVersion(), wantedAppVersion);
   }
   ASSERT_EQ(CoreApplication::getAppVersion(), wantedAppVersion);
   
   // Restore to initial value
   CoreApplication::setAppVersion(String());
   ASSERT_EQ(CoreApplication::getAppVersion(), String());
}

TEST(CoreApplicationTest, testArgc)
{
   std::string str("CoreAplicationTest");
   {
      int argc = 1;
      char *argv[] = {const_cast<char *>(str.c_str())};
      CoreApplication app(argc, argv);
      ASSERT_EQ(argc, 1);
      ASSERT_EQ(app.getArguments().size(), 1ul);
   }
   
   {
      int argc = 4;
      char *argv[] = { const_cast<char *>(str.c_str()),
                       const_cast<char *>("arg1"),
                       const_cast<char *>("arg2"),
                       const_cast<char *>("arg3") };
      CoreApplication app(argc, argv);
      ASSERT_EQ(argc, 4);
      ASSERT_EQ(app.getArguments().size(), 4ul);
   }
   
   {
      int argc = 0;
      char **argv = 0;
      CoreApplication app(argc, argv);
      ASSERT_EQ(argc, 0);
      ASSERT_EQ(app.getArguments().size(), 0ul);
   }
   
   {
      int argc = 2;
      char *argv[] = { const_cast<char*>(str.c_str()),
                       const_cast<char*>("-jsdebugger=port:3768,block") };
      CoreApplication app(argc, argv);
      ASSERT_EQ(argc, 2);
      ASSERT_EQ(app.getArguments().size(), 2ul);
   }
}

class EventGenerator : public Object
{  
public:
   Object *m_other;
   bool event(Event *event)
   {
      if (event->getType() == Event::Type::MaxUser) {
         CoreApplication::sendPostedEvents(m_other, Event::Type::None);
      } else if (pdk::as_integer<Event::Type>(event->getType()) <= pdk::as_integer<Event::Type>(Event::Type::User) + 999) {
         // post a new event in response to this posted event
         int offset = pdk::as_integer<Event::Type>(event->getType()) - pdk::as_integer<Event::Type>(Event::Type::User);
         offset = (offset * 10 + offset % 10);
         CoreApplication::postEvent(this, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + offset)), 
                                    pdk::EventPriority(offset));
      }
      return Object::event(event);
   }
};

TEST(CoreApplicationTest, testPostEvent)
{
   std::string str("CoreAplicationTest");
   int argc = 1;
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   EventSpy spy;
   EventGenerator odd;
   EventGenerator even;
   odd.m_other = &even;
   odd.installEventFilter(&spy);
   even.m_other = &odd;
   even.installEventFilter(&spy);
   
   CoreApplication::postEvent(&odd,  new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)));
   CoreApplication::postEvent(&even, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 2)));
   
   CoreApplication::postEvent(&odd,  new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 3)), pdk::EventPriority(1));
   CoreApplication::postEvent(&even, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 4)), pdk::EventPriority(2));
   
   CoreApplication::postEvent(&odd,  new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 5)), pdk::EventPriority(-2));
   CoreApplication::postEvent(&even, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 6)), pdk::EventPriority(-1));
   
   std::list<Event::Type> expected;
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 4));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 3));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 2));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 6));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 5));
   
   CoreApplication::sendPostedEvents();
   // live lock protection ensures that we only send the initial events
   ASSERT_EQ(spy.m_recordedEvents, expected);
   
   expected.clear();
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 66));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 55));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 44));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 33));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 22));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 11));
   
   spy.m_recordedEvents.clear();
   CoreApplication::sendPostedEvents();
   // expect next sequence events
   ASSERT_EQ(spy.m_recordedEvents, expected);
   
   // have the generators call sendPostedEvents() on each other in
   // response to an event
   CoreApplication::postEvent(&odd, new Event(Event::Type::MaxUser), pdk::EventPriority(INT_MAX));
   CoreApplication::postEvent(&even, new Event(Event::Type::MaxUser), pdk::EventPriority(INT_MAX));
   
   expected.clear();
   expected.push_back(Event::Type::MaxUser);
   expected.push_back(Event::Type::MaxUser);
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 555));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 333));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 111));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 666));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 444));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 222));
   
   spy.m_recordedEvents.clear();
   CoreApplication::sendPostedEvents();
   ASSERT_EQ(spy.m_recordedEvents, expected);
   
   expected.clear();
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 6666));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 5555));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 4444));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 3333));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 2222));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1111));
   
   spy.m_recordedEvents.clear();
   CoreApplication::sendPostedEvents();
   ASSERT_EQ(spy.m_recordedEvents, expected);
   
   // no more events
   expected.clear();
   spy.m_recordedEvents.clear();
   CoreApplication::sendPostedEvents();
   ASSERT_EQ(spy.m_recordedEvents, expected);
}

TEST(CoreApplicationTest, testRemovePostedEvents)
{
   std::string str("CoreAplicationTest");
   int argc = 1;
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   
   EventSpy spy;
   Object one;
   Object two;
   one.installEventFilter(&spy);
   two.installEventFilter(&spy);
   
   std::list<Event::Type> expected;
   
   // remove all events for one object
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)));
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 2)));
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 3)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 4)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 5)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 6)));
   CoreApplication::removePostedEvents(&one);
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 4));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 5));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 6));
   CoreApplication::sendPostedEvents();
   ASSERT_EQ(spy.m_recordedEvents, expected);
   spy.m_recordedEvents.clear();
   expected.clear();
   
   // remove all events for all objects
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 7)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 8)));
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 9)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 10)));
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 11)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 12)));
   CoreApplication::removePostedEvents(0);
   CoreApplication::sendPostedEvents();
   ASSERT_TRUE(spy.m_recordedEvents.empty());
   
   // remove a specific type of event for one object
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 13)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 14)));
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 15)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 16)));
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 17)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 18)));
   CoreApplication::removePostedEvents(&one, Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 13));
   CoreApplication::removePostedEvents(&two, Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 18));
   CoreApplication::sendPostedEvents();
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 14));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 15));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 16));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 17));
   ASSERT_EQ(spy.m_recordedEvents, expected);
   spy.m_recordedEvents.clear();
   expected.clear();
   
   // remove a specific type of event for all objects
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 19)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 19)));
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 20)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 20)));
   CoreApplication::postEvent(&one, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 21)));
   CoreApplication::postEvent(&two, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 21)));
   CoreApplication::removePostedEvents(nullptr, Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 20));
   CoreApplication::sendPostedEvents();
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 19));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 19));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 21));
   expected.push_back(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 21));
   ASSERT_EQ(spy.m_recordedEvents, expected);
   spy.m_recordedEvents.clear();
   expected.clear();
}

class DeliverInDefinedOrderThread : public Thread
{
public:
   using ProgressHandlerType = void(int);
   PDK_DEFINE_SIGNAL_ENUMS(Progress);
   PDK_DEFINE_SIGNAL_BINDER(Progress)
   PDK_DEFINE_SIGNAL_EMITTER(Progress)
   DeliverInDefinedOrderThread()
   {}
   
protected:
   void run()
   {
      emitProgressSignal(1);
      emitProgressSignal(2);
      emitProgressSignal(3);
      emitProgressSignal(4);
      emitProgressSignal(5);
      emitProgressSignal(6);
      emitProgressSignal(7);
   }
};

class DeliverInDefinedOrderObject : public Object
{
   Pointer<DeliverInDefinedOrderThread> m_thread;
   int m_count;
   int m_startCount;
   int m_loopLevel;
   
public:
   using DoneHandlerType = void();
   PDK_DEFINE_SIGNAL_ENUMS(Done);
   PDK_DEFINE_SIGNAL_EMITTER(Done)
   PDK_DEFINE_SIGNAL_BINDER(Done)
   DeliverInDefinedOrderObject(Object *parent)
      : Object(parent),
        m_thread(0),
        m_count(0),
        m_startCount(0),
        m_loopLevel(0)
   {}
   
public:
   void startThread()
   {
      ASSERT_TRUE(!m_thread);
      m_thread = new DeliverInDefinedOrderThread();
      m_thread->connectProgressSignal(this, &DeliverInDefinedOrderObject::threadProgress);
      m_thread->connectFinishedSignal(this, &DeliverInDefinedOrderObject::threadFinished);
      m_thread->connectDestroyedSignal(this, &DeliverInDefinedOrderObject::threadDestroyed);
      m_thread->start();
      CoreApplication::postEvent(this, new Event(Event::Type::MaxUser), pdk::EventPriority(-1));
   }
   
   void threadProgress(int v)
   {
      ++m_count;
      ASSERT_EQ(v, m_count);
      CoreApplication::postEvent(this, new Event(Event::Type::MaxUser), pdk::EventPriority(-1));
   }
   
   void threadFinished()
   {
      ASSERT_EQ(m_count, 7);
      m_count = 0;
      m_thread->deleteLater();
      CoreApplication::postEvent(this, new Event(Event::Type::MaxUser), pdk::EventPriority(-1));
   }
   
   void threadDestroyed()
   {
      if (++m_startCount < 20) {
         startThread();
      } else {
         emitDoneSignal();
      }
   }
   
public:
   bool event(Event *event)
   {
      switch (event->getType()) {
      case Event::Type::User:
      {
         ++m_loopLevel;
         if (m_loopLevel == 2) {
            // Ready. Starts a thread that emits (queued) signals, which should be handled in order
            startThread();
         }
         CoreApplication::postEvent(this, new Event(Event::Type::MaxUser), pdk::EventPriority(-1));
         (void) EventLoop().exec();
         break;
      }
      default:
         break;
      }
      return Object::event(event);
   }
};

TEST(CoreApplicationTest, testDeliverInDefinedOrder)
{
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   
   DeliverInDefinedOrderObject obj(&app);
   // causes sendPostedEvents() to recurse twice
   CoreApplication::postEvent(&obj, new Event(Event::Type::User));
   CoreApplication::postEvent(&obj, new Event(Event::Type::User));
   obj.connectDoneSignal(&CoreApplication::quit, static_cast<Object *>(&app));
   app.exec();
}

TEST(CoreApplicationTest, testGetAppPid)
{
   ASSERT_TRUE(CoreApplication::getAppPid() > 0);
}

namespace pdk {
namespace kernel {
PDK_CORE_EXPORT uint global_posted_events_count();
} // kernel
} // pdk

class GlobalPostedEventsCountObject : public Object
{
public:
   std::list<int> m_globalPostedEventsCount;
   bool event(Event *event)
   {
      if (event->getType() == Event::Type::User) {
         m_globalPostedEventsCount.push_back(pdk::kernel::global_posted_events_count());
      }
      return Object::event(event);
   }
};

TEST(CoreApplicationTest, testGlobalPostedEventsCount)
{
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   
   CoreApplication::sendPostedEvents();
   ASSERT_EQ(pdk::kernel::global_posted_events_count(), 0u);
   
   GlobalPostedEventsCountObject x;
   CoreApplication::postEvent(&x, new Event(Event::Type::User));
   CoreApplication::postEvent(&x, new Event(Event::Type::User));
   CoreApplication::postEvent(&x, new Event(Event::Type::User));
   CoreApplication::postEvent(&x, new Event(Event::Type::User));
   CoreApplication::postEvent(&x, new Event(Event::Type::User));
   ASSERT_EQ(pdk::kernel::global_posted_events_count(), 5u);
   
   CoreApplication::sendPostedEvents();
   ASSERT_EQ(pdk::kernel::global_posted_events_count(), 0u);
   
   std::list<int> expected{4, 3, 2, 1, 0};
   ASSERT_EQ(x.m_globalPostedEventsCount, expected);
}

class ProcessEventsAlwaysSendsPostedEventsObject : public Object
{
public:
   int m_counter;
   
   inline ProcessEventsAlwaysSendsPostedEventsObject()
      : m_counter(0)
   {}
   
   bool event(Event *event)
   {
      if (event->getType() == Event::Type::User) {
         ++m_counter;
      }
      return Object::event(event);
   }
};

TEST(CoreApplicationTest, testProcessEventsAlwaysSendsPostedEvents)
{
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   
   ProcessEventsAlwaysSendsPostedEventsObject object;
   Time t;
   t.start();
   int i = 1;
   do {
      CoreApplication::postEvent(&object, new Event(Event::Type::User));
      CoreApplication::processEvents();
      ASSERT_EQ(object.m_counter, i);
      ++i;
   } while (t.elapsed() < 1000);
}

TEST(CoreApplicationTest, testReexec)
{
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   CallableInvoker::invokeAsync(&CoreApplication::quit, &app);
   // exec once
   ASSERT_EQ(app.exec(), 0);
   // and again
   CallableInvoker::invokeAsync(&CoreApplication::quit, &app);
   ASSERT_EQ(app.exec(), 0);
}

TEST(CoreApplicationTest, testExecAfterExit)
{
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   
   app.exit(1);
   CallableInvoker::invokeAsync(&CoreApplication::quit, &app);
   ASSERT_EQ(app.exec(), 0);
}

TEST(CoreApplicationTest, testEventLoopExecAfterExit)
{
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   
   // exec once and exit
   CallableInvoker::invokeAsync(&CoreApplication::quit, &app);
   ASSERT_EQ(app.exec(), 0);
   // and again, but this time using a EventLoop
   EventLoop loop;
   CallableInvoker::invokeAsync(&loop, &EventLoop::quit);
   ASSERT_EQ(loop.exec(), 0);
}

class DummyEventDispatcher : public AbstractEventDispatcher
{
public:
   DummyEventDispatcher()
      : m_visited(false)
   {}
   
   bool processEvents(EventLoop::ProcessEventsFlags) override
   {
      m_visited = true;
      emitAwakeSignal();
      CoreApplication::sendPostedEvents();
      return false;
   }
   
   bool hasPendingEvents() override
   {
      return pdk::kernel::global_posted_events_count();
   }
   
   void registerSocketNotifier(SocketNotifier *) override
   {}
   
   void unregisterSocketNotifier(SocketNotifier *) override
   {}
   
   void registerTimer(int , int , pdk::TimerType, Object *) override
   {}
   
   bool unregisterTimer(int ) override
   {
      return false;
   }
   
   bool unregisterTimers(Object *) override
   {
      return false;
   }
   
   std::list<TimerInfo> getRegisteredTimers(Object *) const override
   {
      return std::list<TimerInfo>();
   }
   
   int getRemainingTime(int) override
   {
      return 0;
   }
   
   void wakeUp() override
   {}
   
   void interrupt() override
   {}
   
   void flush() override
   {}
   
#ifdef PDK_OS_WIN
   bool registerEventNotifier(WinEventNotifier *) { return false; }
   void unregisterEventNotifier(WinEventNotifier *) { }
#endif
   
   bool m_visited;
};

TEST(CoreApplicationTest, testCustomEventDispatcher)
{
   PDK_RETRIEVE_APP_INSTANCE()->quit();
   // there should be no ED yet
   ASSERT_TRUE(!CoreApplication::getEventDispatcher());
   DummyEventDispatcher *ed = new DummyEventDispatcher;
   CoreApplication::setEventDispatcher(ed);
   // the new ED should be set
   ASSERT_EQ(CoreApplication::getEventDispatcher(), ed);
   // test the alternative API of AbstractEventDispatcher
   ASSERT_EQ(AbstractEventDispatcher::getInstance(), ed);
   Pointer<DummyEventDispatcher> weak_ed(ed);
   ASSERT_TRUE(!weak_ed.isNull());
   {
      int argc = 1;
      std::string str("CoreAplicationTest");
      char *argv[] = {const_cast<char *>(str.c_str())};
      CoreApplication app(argc, argv);
      // instantiating app should not overwrite the ED
      ASSERT_EQ(CoreApplication::getEventDispatcher(), ed);
      CallableInvoker::invokeAsync(&CoreApplication::quit, &app);
      app.exec();
      // the custom ED has really been used?
      ASSERT_TRUE(ed->m_visited);
   }
   // ED has been deleted?
   ASSERT_TRUE(weak_ed.isNull());
}

class JobObject : public Object
{
public:
   using DoneHandlerType = void();
   PDK_DEFINE_SIGNAL_ENUMS(Done);
   PDK_DEFINE_SIGNAL_EMITTER(Done)
   PDK_DEFINE_SIGNAL_BINDER(Done)
   explicit JobObject(EventLoop *loop, Object *parent = nullptr)
      : Object(parent),
        m_locker(loop)
   {
      Timer::singleShot(1000, this, &JobObject::timeout);
   }
   
   explicit JobObject(Object *parent = nullptr)
      : Object(parent)
   {
      Timer::singleShot(1000, this, &JobObject::timeout);
   }
   
public:
   void startSecondaryJob()
   {
      new JobObject();
   }
   
private:
   void timeout()
   {
      emitDoneSignal();
      deleteLater();
   }
private:
   EventLoopLocker m_locker;
};

class QuitTester : public Object
{
public:
   QuitTester(Object *parent = nullptr)
      : Object(parent)
   {
      Timer::singleShot(0, this, &QuitTester::doTest);
   }
   
private:
   void doTest()
   {
      CoreApplicationPrivate *privateClass = static_cast<CoreApplicationPrivate*>(ObjectPrivate::get(PDK_RETRIEVE_APP_INSTANCE()));
      
      {
         ASSERT_EQ(privateClass->m_quitLockRef.load(), 0);
         // Test with a lock active so that the refcount doesn't drop to zero during these tests, causing a quit.
         // (until we exit the scope)
         EventLoopLocker locker;
         ASSERT_EQ(privateClass->m_quitLockRef.load(), 1);
         JobObject *job1 = new JobObject(this);
         ASSERT_EQ(privateClass->m_quitLockRef.load(), 2);
         delete job1;
         ASSERT_EQ(privateClass->m_quitLockRef.load(), 1);
         job1 = new JobObject(this);
         ASSERT_EQ(privateClass->m_quitLockRef.load(), 2);
         JobObject *job2 = new JobObject(this);
         ASSERT_EQ(privateClass->m_quitLockRef.load(), 3);
         delete job1;
         ASSERT_EQ(privateClass->m_quitLockRef.load(), 2);
         JobObject *job3 = new JobObject(job2);
         PDK_UNUSED(job3);
         ASSERT_EQ(privateClass->m_quitLockRef.load(), 3);
         JobObject *job4 = new JobObject(job2);
         PDK_UNUSED(job4);
         ASSERT_EQ(privateClass->m_quitLockRef.load(), 4);
         delete job2;
         ASSERT_EQ(privateClass->m_quitLockRef.load(), 1);
         
      }
      ASSERT_EQ(privateClass->m_quitLockRef.load(), 0);
   }
};

TEST(CoreApplicationTest, testQuitLock)
{
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   QuitTester tester;
   app.exec();
}

TEST(CoreApplicationTest, testEventDestructorDeadLock)
{
   class MyEvent : public Event
   { public:
      MyEvent() : Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)) {}
      ~MyEvent()
      {
         CoreApplication::postEvent(PDK_RETRIEVE_APP_INSTANCE(), new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 2)));
      }
   };
   
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   
   EventSpy spy;
   app.installEventFilter(&spy);
   
   CoreApplication::postEvent(&app, new MyEvent);
   CoreApplication::processEvents();
   ASSERT_TRUE(std::find(spy.m_recordedEvents.begin(),
                         spy.m_recordedEvents.end(),
                         Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)) != spy.m_recordedEvents.end());
   ASSERT_TRUE(std::find(spy.m_recordedEvents.begin(),
                         spy.m_recordedEvents.end(),
                         Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 2)) == spy.m_recordedEvents.end());
   CoreApplication::processEvents();
   ASSERT_TRUE(std::find(spy.m_recordedEvents.begin(),
                         spy.m_recordedEvents.end(),
                         Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 2)) != spy.m_recordedEvents.end());
}

// this is almost identical to sendEventsOnProcessEvents
TEST(CoreApplicationTest, testAppEventFiltersMainThread)
{
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   
   EventSpy spy;
   app.installEventFilter(&spy);
   
   CoreApplication::postEvent(&app,  new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)));
   Timer::singleShot(10, &app, &CoreApplication::quit);
   app.exec();
   ASSERT_TRUE(std::find(spy.m_recordedEvents.begin(),
                         spy.m_recordedEvents.end(), 
                         Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)) != spy.m_recordedEvents.end());
}

TEST(CoreApplicationTest, testAppEventFiltersAuxThread)
{
   int argc = 1;
   std::string str("CoreAplicationTest");
   char *argv[] = {const_cast<char *>(str.c_str())};
   CoreApplication app(argc, argv);
   Thread thread;
   ThreadedEventReceiver receiver;
   receiver.moveToThread(&thread);
   
   EventSpy spy;
   app.installEventFilter(&spy);
   
   // this is very similar to sendEventsOnProcessEvents
   CoreApplication::postEvent(&receiver,  new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)));
   Timer::singleShot(1000, &app, &CoreApplication::quit);
   thread.start();
   app.exec();
   ASSERT_TRUE(thread.wait(1000));
   ASSERT_TRUE(std::find(receiver.m_recordedEvents.begin(),
                         receiver.m_recordedEvents.end(),
                         Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)) != receiver.m_recordedEvents.end());
   ASSERT_TRUE(std::find(spy.m_recordedEvents.begin(),
                         spy.m_recordedEvents.end(),
                         Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)) == spy.m_recordedEvents.end());
}

void threaded_event_delivery_data(std::list<std::tuple<bool, bool, bool>> &data)
{
   data.push_back(std::make_tuple(true, true, true));
   data.push_back(std::make_tuple(false, false, true));
   data.push_back(std::make_tuple(false, true, true));
}

// posts the event before the CoreApplication is destroyed, starts thread after
TEST(CoreApplicationTest, testThreadedEventDelivery)
{
   std::list<std::tuple<bool, bool, bool>> data;
   threaded_event_delivery_data(data);
   for (auto item: data) {
      bool requiresCoreApplication = std::get<0>(item);
      bool createCoreApplication = std::get<1>(item);
      bool eventsReceived = std::get<2>(item);
      
      int argc = 1;
      std::string str("CoreAplicationTest");
      char *argv[] = {const_cast<char *>(str.c_str())};
      
      ScopedPointer<CoreApplication> app(createCoreApplication ? new CoreApplication(argc, argv) : nullptr);
      
      MyThread thread;
      thread.m_requiresCoreApplication = requiresCoreApplication;
      ThreadedEventReceiver receiver;
      receiver.moveToThread(&thread);
      CoreApplication::postEvent(&receiver, new Event(Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)));
      
      thread.start();
      ASSERT_TRUE(thread.wait(1000000000));
      ASSERT_EQ(std::find(receiver.m_recordedEvents.begin(),
                          receiver.m_recordedEvents.end(),
                          Event::Type(pdk::as_integer<Event::Type>(Event::Type::User) + 1)) != receiver.m_recordedEvents.end(), eventsReceived);
   }
}

int main(int argc, char **argv)
{
   ::testing::InitGoogleTest(&argc, argv);
   sg_argc = argc;
   sg_argv = argv;
   return RUN_ALL_TESTS();;
}
