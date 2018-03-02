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
// Created by softboy on 2018/03/1.

#include "pdk/kernel/EventDispatcherCf.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/TextStream.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/base/lang/String.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/internal/CoreUnixPrivate.h"
#include "pdk/global/Logging.h"
#include "pdk/stdext/utility/Algorithms.h"
#include <limits>

#ifdef PDK_OS_OSX
#  include <AppKit/NSApplication.h>
#else
#  include <UIKit/UIApplication.h>
#endif

@interface PdkRunLoopModeTracker : NSObject
{
   std::stack<CFStringRef> m_runLoopModes;
}
@end

PDK_NAMESPACE_ALIAS_OBJC_CLASS(PdkRunLoopModeTracker)

using pdk::lang::String;
using pdk::MessageLogger;

@implementation PdkRunLoopModeTracker

- (id) init
{
   if (self = [super init]) {
      m_runLoopModes.push(kCFRunLoopDefaultMode);
      [[NSNotificationCenter defaultCenter]
            addObserver:self
                        selector:@selector(receivedNotification:)
       name:nil
#ifdef PDK_OS_OSX
       object:[NSApplication sharedApplication]];
#else
       // Use performSelector so this can work in an App Extension
       object:[[UIApplication class] performSelector:@selector(sharedApplication)]];
#endif
   }
   return self;
}

- (void) dealloc
{
   [[NSNotificationCenter defaultCenter] removeObserver:self];
   [super dealloc];
}

namespace {

CFStringRef runLoopMode(NSDictionary *dictionary)
{
   for (NSString *key in dictionary) {
      if (CFStringHasSuffix((CFStringRef)key, CFSTR("RunLoopMode"))) {
         return (CFStringRef)[dictionary objectForKey: key];
      } 
   }
   return nil;
}

} // anonymous namespace

- (void) receivedNotification:(NSNotification *) notification
{
   if (CFStringHasSuffix((CFStringRef)notification.name, CFSTR("RunLoopModePushNotification"))) {
      if (CFStringRef mode = runLoopMode(notification.userInfo)) {
         m_runLoopModes.push(mode);
      } else {
         warning_stream("Encountered run loop push notification without run loop mode!");
      }
   } else if (CFStringHasSuffix((CFStringRef)notification.name, CFSTR("RunLoopModePopNotification"))) {
      CFStringRef mode = runLoopMode(notification.userInfo);
      if (CFStringCompare(mode, [self currentMode], 0) == kCFCompareEqualTo) {
         m_runLoopModes.pop();
      } else {
         warning_stream("Tried to pop run loop mode '%s' that was never pushed!", pdk_printable(String::fromCFString(mode)));
      }
      PDK_ASSERT(m_runLoopModes.size() >= 1);
   }
}

- (CFStringRef) currentMode
{
   return m_runLoopModes.top();
}
@end

namespace pdk {
namespace kernel {

using pdk::os::thread::internal::ThreadData;
using pdk::os::thread::Thread;
using pdk::io::set_field_width;
using pdk::io::set_pad_char;
using pdk::io::reset;
using pdk::lang::Character;
using pdk::lang::String;
using pdk::kernel::CoreApplication;

class RunLoopDebugger : public Object
{
public:
#define PDK_MIRROR_ENUM(name) name = name
   enum Activity
   {
      PDK_MIRROR_ENUM(kCFRunLoopEntry),
      PDK_MIRROR_ENUM(kCFRunLoopBeforeTimers),
      PDK_MIRROR_ENUM(kCFRunLoopBeforeSources),
      PDK_MIRROR_ENUM(kCFRunLoopBeforeWaiting),
      PDK_MIRROR_ENUM(kCFRunLoopAfterWaiting),
      PDK_MIRROR_ENUM(kCFRunLoopExit)
   };
   
   enum Result
   {
      PDK_MIRROR_ENUM(kCFRunLoopRunFinished),
      PDK_MIRROR_ENUM(kCFRunLoopRunStopped),
      PDK_MIRROR_ENUM(kCFRunLoopRunTimedOut),
      PDK_MIRROR_ENUM(kCFRunLoopRunHandledSource)
   };
};

Debug operator<<(Debug s, timespec tv)
{
   s << tv.tv_sec << "." << set_field_width(9) << set_pad_char(Character(48)) << tv.tv_nsec << reset;
   return s;
}

#if DEBUG_EVENT_DISPATCHER
uint g_eventDispatcherIndentationLevel = 0;
#endif

static const CFTimeInterval kCFTimeIntervalMinimum = 0;
static const CFTimeInterval kCFTimeIntervalDistantFuture = std::numeric_limits<CFTimeInterval>::max();

#pragma mark - Class definition

EventDispatcherCoreFoundation::EventDispatcherCoreFoundation(Object *parent)
    : AbstractEventDispatcher(parent), 
      m_processEvents(EventLoop::EventLoopExec),
      m_postedEventsRunLoopSource(this, &EventDispatcherCoreFoundation::processPostedEvents),
      m_runLoopActivityObserver(this, &EventDispatcherCoreFoundation::handleRunLoopActivity,
#if DEBUG_EVENT_DISPATCHER
        kCFRunLoopAllActivities
#else
        kCFRunLoopBeforeWaiting | kCFRunLoopAfterWaiting
#endif
    ),
      m_runLoopModeTracker([[PdkRunLoopModeTracker alloc] init]),
      m_runLoopTimer(nullptr),
      m_blockedRunLoopTimer(nullptr),
      m_overdueTimerScheduled(false)
{
   m_cfSocketNotifier.setHostEventDispatcher(this);
   
   m_postedEventsRunLoopSource.addToMode(kCFRunLoopCommonModes);
   m_runLoopActivityObserver.addToMode(kCFRunLoopCommonModes);
}

EventDispatcherCoreFoundation::~EventDispatcherCoreFoundation()
{
   invalidateTimer();
   pdk::stdext::delete_all(m_timerInfoList);
   m_cfSocketNotifier.removeSocketNotifiers();
}

EventLoop *EventDispatcherCoreFoundation::currentEventLoop() const
{
   EventLoop *eventLoop = ThreadData::current()->m_eventLoops.back();
   PDK_ASSERT(eventLoop);
   return eventLoop;
}

bool EventDispatcherCoreFoundation::processEvents(EventLoop::ProcessEventsFlags flags)
{
   bool eventsProcessed = false;
   if (flags & (EventLoop::ExcludeUserInputEvents | EventLoop::ExcludeSocketNotifiers)) {
      //warning_stream() << "processEvents() flags" << flags << "not supported on iOS";       
   }
   event_dispatcher_debug_stream() << "Entering with " << flags;
   pdk_indent();
   if (m_blockedRunLoopTimer) {
      PDK_ASSERT(m_blockedRunLoopTimer == m_runLoopTimer);
      event_dispatcher_debug_stream() << "Recursing from blocked timer " << m_blockedRunLoopTimer;
      m_runLoopTimer = 0; // Unset current timer to force creation of new timer
      updateTimers();
   }
   
   if (m_processEvents.m_deferredWakeUp) {
      // We may be processing events recursivly as a result of processing a posted event,
      // in which case we need to signal the run-loop source so that this iteration of
      // processEvents will take care of the newly posted events.
      m_postedEventsRunLoopSource.signal();
      m_processEvents.m_deferredWakeUp = false;
      event_dispatcher_debug_stream() << "Processed deferred wake-up";
   }
   
   // The documentation states that this signal is emitted after the event
   // loop returns from a function that could block, which is not the case
   // here, but all the other event dispatchers emit awake at the start of
   // processEvents, and the EventLoop auto-test has an explicit check for
   // this behavior, so we assume it's for a good reason and do it as well.
   // @TODO emit signal
   //    emit awake();
   
   ProcessEventsState previousState = m_processEvents;
   m_processEvents = ProcessEventsState(flags);
   
   bool returnAfterSingleSourceHandled = !(m_processEvents.m_flags & EventLoop::EventLoopExec);
   
   while(true) {
      CFStringRef mode = [m_runLoopModeTracker currentMode];
      
      CFTimeInterval duration = (m_processEvents.m_flags & EventLoop::WaitForMoreEvents) ?
               kCFTimeIntervalDistantFuture : kCFTimeIntervalMinimum;
      
      event_dispatcher_debug_stream() << "Calling CFRunLoopRunInMode = " << pdk_printable(String::fromCFString(mode))
                                      << " for " << duration << " ms, processing single source = " << returnAfterSingleSourceHandled;
      pdk_indent();
      
      SInt32 result = CFRunLoopRunInMode(mode, duration, returnAfterSingleSourceHandled);
      
      pdk_unindent();
      event_dispatcher_debug_stream() << "result = " << result;
      
      eventsProcessed |= (result == kCFRunLoopRunHandledSource
                          || m_processEvents.m_processedPostedEvents
                          || m_processEvents.m_processedTimers);
      
      if (result == kCFRunLoopRunFinished) {
         // This should only happen at application shutdown, as the main runloop
         // will presumably always have sources registered.
         break;
      } else if (m_processEvents.m_wasInterrupted) {
         if (m_processEvents.m_flags & EventLoop::EventLoopExec) {
            PDK_ASSERT(result == kCFRunLoopRunStopped);
            // The runloop was potentially stopped (interrupted) by us, as a response to
            // a Qt event loop being asked to exit. We check that the topmost eventloop
            // is still supposed to keep going and return if not. Note that the runloop
            // might get stopped as a result of a non-top eventloop being asked to exit,
            // in which case we continue running the top event loop until that is asked
            // to exit, and then unwind back to the previous event loop which will break
            // immediately, since it has already been exited.
            
            if (!currentEventLoop()->isRunning()) {
               event_dispatcher_debug_stream() << "Top level event loop was exited";
               break;
            } else {
               event_dispatcher_debug_stream() << "Top level event loop still running, making another pass";
            }
         } else {
            // We were called manually, through processEvents(), and should stop processing
            // events, even if we didn't finish processing all the queued events.
            event_dispatcher_debug_stream() << "Top level processEvents was interrupted";
            break;
         }
      }
      
      if (m_processEvents.m_flags & EventLoop::EventLoopExec) {
         // We were called from EventLoop's exec(), which blocks until the event
         // loop is asked to exit by calling processEvents repeatedly. Instead of
         // re-entering this method again and again from EventLoop, we can block
         // here,  one lever closer to CFRunLoopRunInMode, by running the native
         // event loop again and again until we're interrupted by EventLoop.
         continue;
      } else {
         // We were called 'manually', through processEvents()
         
         if (result == kCFRunLoopRunHandledSource) {
            // We processed one or more sources, but there might still be other
            // sources that did not get a chance to process events, so we need
            // to do another pass.
            
            // But we should only wait for more events the first time
            m_processEvents.m_flags &= ~EventLoop::WaitForMoreEvents;
            continue;
            
         } else if (m_overdueTimerScheduled && !m_processEvents.m_processedTimers) {
            // CFRunLoopRunInMode does not guarantee that a scheduled timer with a fire
            // date in the past (overdue) will fire on the next run loop pass. The Qt
            // APIs on the other hand document eg. zero-interval timers to always be
            // handled after processing all available window-system events.
            event_dispatcher_debug_stream() << "Manually processing timers due to overdue timer";
            processTimers(0);
            eventsProcessed = true;
         }
      }
      
      break;
   }
   
   if (m_blockedRunLoopTimer) {
      invalidateTimer();
      m_runLoopTimer = m_blockedRunLoopTimer;
   }
   if (m_processEvents.m_deferredUpdateTimers) {
      updateTimers();
   }
   if (m_processEvents.m_deferredWakeUp) {
      m_postedEventsRunLoopSource.signal();
      event_dispatcher_debug_stream() << "Processed deferred wake-up";
   }
   bool wasInterrupted = m_processEvents.m_wasInterrupted;
   // Restore state of previous processEvents() call
   m_processEvents = previousState;
   if (wasInterrupted) {
      // The current processEvents run has been interrupted, but there may still be
      // others below it (eg, in the case of nested event loops). We need to trigger
      // another interrupt so that the parent processEvents call has a chance to check
      // if it should continue.
      event_dispatcher_debug_stream() << "Forwarding interrupt in case of nested processEvents";
      interrupt();
   }
   event_dispatcher_debug_stream() << "Returning with eventsProcessed = " << eventsProcessed;
   pdk_unindent();
   return eventsProcessed;
}

bool EventDispatcherCoreFoundation::processPostedEvents()
{
   if (m_processEvents.m_processedPostedEvents && !(m_processEvents.m_flags & EventLoop::EventLoopExec)) {
      event_dispatcher_debug_stream() << "Already processed events this pass";
      return false;
   }
   m_processEvents.m_processedPostedEvents = true;
   event_dispatcher_debug_stream() << "Sending posted events for " << m_processEvents.m_flags;
   pdk_indent();
   CoreApplication::sendPostedEvents();
   pdk_unindent();
   return true;
}

void EventDispatcherCoreFoundation::processTimers(CFRunLoopTimerRef timer)
{
   if (m_processEvents.m_processedTimers && !(m_processEvents.m_flags & EventLoop::EventLoopExec)) {
      event_dispatcher_debug_stream() << "Already processed timers this pass";
      m_processEvents.m_deferredUpdateTimers = true;
      return;
   }
   event_dispatcher_debug_stream() << "CFRunLoopTimer " << timer << " fired, activating pdk timers";
   pdk_indent();
   // Activating pdk timers might recurse into processEvents() if a timer-callback
   // brings up a new event-loop or tries to processes events manually. Although
   // a CFRunLoop can recurse inside its callbacks, a single CFRunLoopTimer can
   // not. So, for each recursion into processEvents() from a timer-callback we
   // need to set up a new timer-source. Instead of doing it preemtivly each
   // time we activate pdk timers, we set a flag here, and let processEvents()
   // decide whether or not it needs to bring up a new timer source.
   
   // We may have multiple recused timers, so keep track of the previous blocked timer
   CFRunLoopTimerRef previouslyBlockedRunLoopTimer = m_blockedRunLoopTimer;
   m_blockedRunLoopTimer = timer;
   m_timerInfoList.getActivateTimers();
   m_blockedRunLoopTimer = previouslyBlockedRunLoopTimer;
   m_processEvents.m_processedTimers = true;
   
   pdk_unindent();
   // Now that the timer source is unblocked we may need to schedule it again
   updateTimers();
}

void EventDispatcherCoreFoundation::handleRunLoopActivity(CFRunLoopActivity activity)
{
   event_dispatcher_debug_stream() << "handleRunLoopActivity(CFRunLoopActivity activity)";
   switch (activity) {
   case kCFRunLoopBeforeWaiting:
      if (m_processEvents.m_processedTimers
          && !(m_processEvents.m_flags & EventLoop::EventLoopExec)
          && m_processEvents.m_flags & EventLoop::WaitForMoreEvents) {
         // CoreFoundation does not treat a timer as a reason to exit CFRunLoopRunInMode
         // when asked to only process a single source, so we risk waiting a long time for
         // a 'proper' source to fire (typically a system source that we don't control).
         // To fix this we do an explicit interrupt after processing our timer, so that
         // processEvents() gets a chance to re-evaluate the state of things.
         interrupt();
      }
      // @TODO emit signal
      //        emit aboutToBlock();
      break;
   case kCFRunLoopAfterWaiting:
      // @TODO emit signal
      //        emit awake();
      break;
#if DEBUG_EVENT_DISPATCHER
   case kCFRunLoopEntry:
   case kCFRunLoopBeforeTimers:
   case kCFRunLoopBeforeSources:
   case kCFRunLoopExit:
      break;
#endif
   default:
      PDK_UNREACHABLE();
   }
}

extern uint global_posted_events_count();

bool EventDispatcherCoreFoundation::hasPendingEvents()
{
   // There doesn't seem to be any API on iOS to peek into the other sources
   // to figure out if there are pending non-Qt events. As a workaround, we
   // assume that if the run-loop is currently blocking and waiting for a
   // source to signal then there are no system-events pending. If this
   // function is called from the main thread then the second clause
   // of the condition will always be true, as the run loop is
   // never waiting in that case. The function would be more aptly named
   // 'maybeHasPendingEvents' in our case.
   return global_posted_events_count() || !CFRunLoopIsWaiting(CFRunLoopGetMain());
}

void EventDispatcherCoreFoundation::wakeUp()
{
   if (m_processEvents.m_processedPostedEvents && !(m_processEvents.m_flags & EventLoop::EventLoopExec)) {
      // A manual processEvents call should only result in processing the events posted
      // up until then. Any newly posted events as result of processing existing posted
      // events should be handled in the next call to processEvents(). Since we're using
      // a run-loop source to process our posted events we need to prevent it from being
      // signaled as a result of posting new events, otherwise we end up in an infinite
      // loop. We do however need to signal the source at some point, so that the newly
      // posted event gets processed on the next processEvents() call, so we flag the
      // need to do a deferred wake-up.
      m_processEvents.m_deferredWakeUp = true;
      event_dispatcher_debug_stream() << "Already processed posted events, deferring wakeUp";
      return;
   }
   
   m_postedEventsRunLoopSource.signal();
   CFRunLoopWakeUp(CFRunLoopGetMain());
   event_dispatcher_debug_stream() << "Signaled posted event run-loop source";
}

void EventDispatcherCoreFoundation::interrupt()
{
   event_dispatcher_debug_stream() << "Marking current processEvent as interrupted";
   m_processEvents.m_wasInterrupted = true;
   CFRunLoopStop(CFRunLoopGetMain());
}

void EventDispatcherCoreFoundation::flush()
{
   // X11 only.
}

#pragma mark - Socket notifiers

void EventDispatcherCoreFoundation::registerSocketNotifier(SocketNotifier *notifier)
{
   m_cfSocketNotifier.registerSocketNotifier(notifier);
}

void EventDispatcherCoreFoundation::unregisterSocketNotifier(SocketNotifier *notifier)
{
   m_cfSocketNotifier.unregisterSocketNotifier(notifier);
}

#pragma mark - Timers

void EventDispatcherCoreFoundation::registerTimer(int timerId, int interval, pdk::TimerType timerType, Object *object)
{
   event_dispatcher_debug_stream() << "id = " << timerId << ", interval = " << interval
                                   << ", type = " << timerType << ", object = " << object;
   
   PDK_ASSERT(timerId > 0 && interval >= 0 && object);
   PDK_ASSERT(object->getThread() == getThread() && getThread() == Thread::getCurrentThread());
   
   m_timerInfoList.registerTimer(timerId, interval, timerType, object);
   updateTimers();
}

bool EventDispatcherCoreFoundation::unregisterTimer(int timerId)
{
   PDK_ASSERT(timerId > 0);
   PDK_ASSERT(getThread() == Thread::getCurrentThread());
   bool returnValue = m_timerInfoList.unregisterTimer(timerId);
   event_dispatcher_debug_stream() << "id = " << timerId << ", timers left: " << m_timerInfoList.size();
   updateTimers();
   return returnValue;
}

bool EventDispatcherCoreFoundation::unregisterTimers(Object *object)
{
   PDK_ASSERT(object && object->getThread() == getThread() && getThread() == Thread::getCurrentThread());
   bool returnValue = m_timerInfoList.unregisterTimers(object);
   event_dispatcher_debug_stream() << "object = " << object << ", timers left: " << m_timerInfoList.size();
   updateTimers();
   return returnValue;
}

std::list<AbstractEventDispatcher::TimerInfo> EventDispatcherCoreFoundation::getRegisteredTimers(Object *object) const
{
   PDK_ASSERT(object);
   return m_timerInfoList.getRegisteredTimers(object);
}

int EventDispatcherCoreFoundation::remainingTime(int timerId)
{
   PDK_ASSERT(timerId > 0);
   return m_timerInfoList.timerRemainingTime(timerId);
}

static double timespecToSeconds(const timespec &spec)
{
   static double nanosecondsPerSecond = 1.0 * 1000 * 1000 * 1000;
   return spec.tv_sec + (spec.tv_nsec / nanosecondsPerSecond);
}

void EventDispatcherCoreFoundation::updateTimers()
{
   if (m_timerInfoList.size() > 0) {
      // We have Qt timers registered, so create or reschedule CF timer to match
      timespec tv = { -1, -1 };
      CFAbsoluteTime timeToFire = m_timerInfoList.timerWait(tv) ?
               // We have a timer ready to fire right now, or some time in the future
               CFAbsoluteTimeGetCurrent() + timespecToSeconds(tv)
               // We have timers, but they are all currently blocked by callbacks
             : kCFTimeIntervalDistantFuture;
      if (!m_runLoopTimer) {
         m_runLoopTimer = CFRunLoopTimerCreateWithHandler(kCFAllocatorDefault,
                                                          timeToFire, kCFTimeIntervalDistantFuture, 0, 0, ^(CFRunLoopTimerRef timer) {
                                                             processTimers(timer);
                                                          });
         CFRunLoopAddTimer(CFRunLoopGetMain(), m_runLoopTimer, kCFRunLoopCommonModes);
         event_dispatcher_debug_stream() << "Created new CFRunLoopTimer " << m_runLoopTimer;
      } else {
         CFRunLoopTimerSetNextFireDate(m_runLoopTimer, timeToFire);
         event_dispatcher_debug_stream() << "Re-scheduled CFRunLoopTimer " << m_runLoopTimer;
      }
      m_overdueTimerScheduled = !timespecToSeconds(tv);
      event_dispatcher_debug_stream() << "Next timeout in " << tv << " seconds";
      
   } else {
      // No pdk timers are registered, so make sure we're not running any CF timers
      invalidateTimer();
      m_overdueTimerScheduled = false;
   }
}

void EventDispatcherCoreFoundation::invalidateTimer()
{
   if (!m_runLoopTimer || (m_runLoopTimer == m_blockedRunLoopTimer)) {
      return;
   }
   CFRunLoopTimerInvalidate(m_runLoopTimer);
   event_dispatcher_debug_stream() << "Invalidated CFRunLoopTimer " << m_runLoopTimer;
   CFRelease(m_runLoopTimer);
   m_runLoopTimer = nullptr;
}

} // kernel
} // pdk
