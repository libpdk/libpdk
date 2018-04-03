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

#ifndef PDK_KERNEL_EVENT_DISPATCHER_CF_H
#define PDK_KERNEL_EVENT_DISPATCHER_CF_H

#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/internal/TimerInfoUnixPrivate.h"
#include "pdk/kernel/internal/CfSocketNotifierPrivate.h"
#include "pdk/kernel/internal/CoreMacPrivate.h"
#include "pdk/global/Logging.h"

PDK_FORWARD_DECLARE_OBJC_CLASS(PdkRunLoopModeTracker);

namespace pdk {
namespace kernel {

class EventDispatcherCoreFoundation;

template <class T = EventDispatcherCoreFoundation>
class RunLoopSource
{
public:
   using CallbackFunction = bool (T::*)();
   
   enum { kHighestPriority = 0 } RunLoopSourcePriority;
   
   RunLoopSource(T *delegate, CallbackFunction callback)
      : m_delegate(delegate), 
        m_callback(callback)
   {
      CFRunLoopSourceContext context = {};
      context.info = this;
      context.perform = RunLoopSource::process;
      
      m_source = CFRunLoopSourceCreate(kCFAllocatorDefault, kHighestPriority, &context);
      PDK_ASSERT(m_source);
   }
   
   ~RunLoopSource()
   {
      CFRunLoopSourceInvalidate(m_source);
      CFRelease(m_source);
   }
   
   void addToMode(CFStringRef mode, CFRunLoopRef runLoop = nullptr)
   {
      if (!runLoop) {
         runLoop = CFRunLoopGetCurrent();
      }
      CFRunLoopAddSource(runLoop, m_source, mode);
   }
   
   void signal()
   {
      CFRunLoopSourceSignal(m_source);
   }
   
private:
   static void process(void *info)
   {
      RunLoopSource *self = static_cast<RunLoopSource *>(info);
      ((self->m_delegate)->*(self->m_callback))();
   }
   
   T *m_delegate;
   CallbackFunction m_callback;
   CFRunLoopSourceRef m_source;
};

template <class T = EventDispatcherCoreFoundation>
class RunLoopObserver
{
public:
   using CallbackFunction = void (T::*) (CFRunLoopActivity activity);
   
   RunLoopObserver(T *delegate, CallbackFunction callback, CFOptionFlags activities)
      : m_delegate(delegate),
        m_callback(callback)
   {
      CFRunLoopObserverContext context = {};
      context.info = this;
      m_observer = CFRunLoopObserverCreate(kCFAllocatorDefault, activities, true, 0, process, &context);
      PDK_ASSERT(m_observer);
   }
   
   ~RunLoopObserver()
   {
      CFRunLoopObserverInvalidate(m_observer);
      CFRelease(m_observer);
   }
   
   void addToMode(CFStringRef mode, CFRunLoopRef runLoop = nullptr)
   {
      if (!runLoop) {
         runLoop = CFRunLoopGetCurrent();
      }
      if (!CFRunLoopContainsObserver(runLoop, m_observer, mode)) {
         CFRunLoopAddObserver(runLoop, m_observer, mode);
      }
   }
   
   void removeFromMode(CFStringRef mode, CFRunLoopRef runLoop = nullptr)
   {
      if (!runLoop) {
         runLoop = CFRunLoopGetCurrent();
      }
      if (CFRunLoopContainsObserver(runLoop, m_observer, mode)) {
         CFRunLoopRemoveObserver(runLoop, m_observer, mode);
      }
   }
   
private:
   static void process(CFRunLoopObserverRef, CFRunLoopActivity activity, void *info)
   {
      RunLoopObserver *self = static_cast<RunLoopObserver *>(info);
      ((self->m_delegate)->*(self->m_callback))(activity);
   }
   
   T *m_delegate;
   CallbackFunction m_callback;
   CFRunLoopObserverRef m_observer;
};

class PDK_CORE_EXPORT EventDispatcherCoreFoundation : public AbstractEventDispatcher
{
public:
   explicit EventDispatcherCoreFoundation(Object *parent = 0);
   ~EventDispatcherCoreFoundation();
   
   bool processEvents(EventLoop::ProcessEventsFlags flags) override;
   bool hasPendingEvents() override;
   
   void registerSocketNotifier(SocketNotifier *notifier) override;
   void unregisterSocketNotifier(SocketNotifier *notifier) override;
   
   void registerTimer(int timerId, int interval, pdk::TimerType timerType, Object *object) override;
   bool unregisterTimer(int timerId) override;
   bool unregisterTimers(Object *object) override;
   std::list<AbstractEventDispatcher::TimerInfo> getRegisteredTimers(Object *object) const override;
   
   int getRemainingTime(int timerId) override;
   
   void wakeUp() override;
   void interrupt() override;
   void flush() override;
   
protected:
   EventLoop *currentEventLoop() const;
   
   virtual bool processPostedEvents();
   
   struct ProcessEventsState
   {
      ProcessEventsState(EventLoop::ProcessEventsFlags f)
         : m_flags(f),
           m_wasInterrupted(false),
           m_processedPostedEvents(false), 
           m_processedTimers(false), 
           m_deferredWakeUp(false), 
           m_deferredUpdateTimers(false)
      {}
      
      EventLoop::ProcessEventsFlags m_flags;
      bool m_wasInterrupted;
      bool m_processedPostedEvents;
      bool m_processedTimers;
      bool m_deferredWakeUp;
      bool m_deferredUpdateTimers;
   };
   
   ProcessEventsState m_processEvents;
   
private:
   RunLoopSource<> m_postedEventsRunLoopSource;
   RunLoopObserver<> m_runLoopActivityObserver;
   
   PdkRunLoopModeTracker *m_runLoopModeTracker;
   
   internal::TimerInfoList m_timerInfoList;
   CFRunLoopTimerRef m_runLoopTimer;
   CFRunLoopTimerRef m_blockedRunLoopTimer;
   bool m_overdueTimerScheduled;
   
   internal::CFSocketNotifier m_cfSocketNotifier;
   
   void processTimers(CFRunLoopTimerRef);
   
   void handleRunLoopActivity(CFRunLoopActivity activity);
   
   void updateTimers();
   void invalidateTimer();
};

} // kernel
} // pdk

#if DEBUG_EVENT_DISPATCHER
extern uint sg_eventDispatcherIndentationLevel;
#define event_dispatcher_debug_stream() debug_stream().nospace() \
            << pdk_printable(String(Latin1String("| ")).repeated(sg_eventDispatcherIndentationLevel)) \
            << __FUNCTION__ << "(): "
#define pdk_indent() ++sg_eventDispatcherIndentationLevel
#define pdk_unindent() --sg_eventDispatcherIndentationLevel
#else
#define event_dispatcher_debug_stream() PDK_NO_DEBUG_MACRO()
#define pdk_indent()
#define pdk_unindent()
#endif

#endif // PDK_KERNEL_EVENT_DISPATCHER_CF_H
