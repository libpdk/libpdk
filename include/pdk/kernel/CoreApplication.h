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
// Created by softboy on 2018/01/23.

#ifndef PDK_KERNEL_ABSTRACT_CORE_APPLICATION_H
#define PDK_KERNEL_ABSTRACT_CORE_APPLICATION_H

#include "pdk/global/Global.h"
#include "pdk/kernel/Object.h"
#include "pdk/kernel/CoreEvent.h"
#include "pdk/kernel/EventLoop.h"
#include "pdk/base/ds/StringList.h"
#include <string>

#define PDK_RETRIEVE_APP_INSTANCE() pdk::kernel::CoreApplication::getInstance()
#define PDK_DECLARE_TR_FUNCTIONS(context) \
   public: \
   static inline String tr(const char *sourceText, const char *disambiguation = nullptr, int n = -1) \
{ return CoreApplication::translate(#context, sourceText, disambiguation, n); } \
   
namespace pdk {

// forward declare class with namespace
namespace utils {
class Translator;
} // utils

// forward declare class with namespace
namespace os {
namespace thread {
namespace internal {
class PostEventList;
} // internal
} // thread
} // os

namespace kernel {

// forward declare class with namespace
namespace internal {
class CoreApplicationPrivate;
class EventDispatcherUNIXPrivate;
} // internal

class AbstractEventDispatcher;
class AbstractNativeEventFilter;

using internal::CoreApplicationPrivate;
using pdk::ds::StringList;
using pdk::lang::String;
using pdk::os::thread::internal::PostEventList;
using pdk::utils::Translator;

class PDK_CORE_EXPORT CoreApplication : public Object
{
public:
   using InfoChangeHandlerType = void();
   using AboutToQuitHandlerType = InfoChangeHandlerType;
   using OrgNameChangedHandlerType = InfoChangeHandlerType;
   using OrgDomainChangedHandlerType = InfoChangeHandlerType;
   using AppNameChangedHandlerType = InfoChangeHandlerType;
   using AppVersionChangedHandlerType = InfoChangeHandlerType;
   
   PDK_DEFINE_SIGNAL_ENUMS(AboutToQuit, OrgNameChanged, OrgDomainChanged,
                           AppNameChanged, AppVersionChanged);
   enum { 
      ApplicationFlags = 0x000001// PDK_VERSION
   };
   
   CoreApplication(int &argc, char **argv, int = ApplicationFlags);
   ~CoreApplication();
   
   static void setAttribute(pdk::AppAttribute attribute, bool on = true);
   static bool testAttribute(pdk::AppAttribute attribute);
   
   static StringList getArguments();
   static void setOrgDomain(const String &domain);
   static String getOrgDomain();
   static void setOrgName(const String &name);
   static String getOrgName();
   static void setAppName(const String &name);
   static String getAppName();
   static void setAppVersion(const String &version);
   static String getAppVersion();
   static void setSetuidAllowed(bool allow);
   static bool isSetuidAllowed();
   static CoreApplication *getInstance()
   {
      return sm_self;
   }
   static int exec();
   static void processEvents(EventLoop::ProcessEventsFlags flags = EventLoop::AllEvents);
   static void processEvents(EventLoop::ProcessEventsFlags flags, int maxtime);
   static void exit(int retcode=0);
   
   static bool sendEvent(Object *receiver, Event *event);
   static void postEvent(Object *receiver, Event *event, pdk::EventPriority priority = pdk::EventPriority::NormalEventPriority);
   static void sendPostedEvents(Object *receiver = nullptr, Event::Type eventType = Event::Type::None);
   static void removePostedEvents(Object *receiver, Event::Type eventType = Event::Type::None);
   static AbstractEventDispatcher *getEventDispatcher();
   static void setEventDispatcher(AbstractEventDispatcher *eventDispatcher);
   virtual bool notify(Object *, Event *);
   static bool startingUp();
   static bool closingDown();
   
   static String getAppDirPath();
   static String getAppFilePath();
   static pdk::pint64 getAppPid();
   
   static void setLibraryPaths(const StringList &);
   static StringList getLibraryPaths();
   static void addLibraryPath(const String &);
   static void removeLibraryPath(const String &);
   
#ifndef PDK_NO_TRANSLATION
   static bool installTranslator(Translator *messageFile);
   static bool removeTranslator(Translator *messageFile);
#endif   
   
   static String translate(const char * context,
                           const char * key,
                           const char * disambiguation = nullptr,
                           int n = -1);
   
   void installNativeEventFilter(AbstractNativeEventFilter *filterObj);
   void removeNativeEventFilter(AbstractNativeEventFilter *filterObj);
   static bool isQuitLockEnabled();
   static void setQuitLockEnabled(bool enabled);
   static void quit();
   
   PDK_DEFINE_SIGNAL_BINDER(AboutToQuit)
   PDK_DEFINE_SIGNAL_BINDER(OrgNameChanged)
   PDK_DEFINE_SIGNAL_BINDER(OrgDomainChanged)
   PDK_DEFINE_SIGNAL_BINDER(AppNameChanged)
   PDK_DEFINE_SIGNAL_BINDER(AppVersionChanged)
   
   PDK_DEFINE_SIGNAL_EMITTER(AboutToQuit)
   PDK_DEFINE_SIGNAL_EMITTER(OrgNameChanged)
   PDK_DEFINE_SIGNAL_EMITTER(OrgDomainChanged)
   PDK_DEFINE_SIGNAL_EMITTER(AppNameChanged)
   PDK_DEFINE_SIGNAL_EMITTER(AppVersionChanged)
protected:
   bool event(Event *) override;
   virtual bool compressEvent(Event *, Object *receiver, PostEventList *);
   CoreApplication(CoreApplicationPrivate &p);
   
private:
   static bool sendSpontaneousEvent(Object *receiver, Event *event);
   static bool notifyInternal(Object *receiver, Event *);
   static bool forwardEvent(Object *receiver, Event *event, Event *originatingEvent = nullptr);
   friend class internal::EventDispatcherUNIXPrivate;
   friend PDK_CORE_EXPORT String retrieve_app_name();
   friend class ClassFactory;
private:
   PDK_DISABLE_COPY(CoreApplication);
   PDK_DECLARE_PRIVATE(CoreApplication);
   
private:
   static CoreApplication *sm_self;
};

inline bool CoreApplication::sendEvent(Object *receiver, Event *event)
{
   if (event) {
      event->m_spont = false; 
   }
   return notifyInternal(receiver, event);
}

inline bool CoreApplication::sendSpontaneousEvent(Object *receiver, Event *event)
{
   if (event) {
      event->m_spont = true; 
   }
   return notifyInternal(receiver, event); 
}

using StartUpFunction = void (*)();
using CleanUpFunction = void (*)();

PDK_CORE_EXPORT void add_pre_routine(StartUpFunction);
PDK_CORE_EXPORT void add_post_routine(CleanUpFunction);
PDK_CORE_EXPORT void remove_post_routine(CleanUpFunction);
PDK_CORE_EXPORT String retrieve_app_name();                // get application name

#define PDK_COREAPP_STARTUP_FUNCTION(AFUNC) \
   static void AFUNC ## _ctor_function() {  \
   add_pre_routine(AFUNC);        \
}                                 \
   PDK_CONSTRUCTOR_FUNCTION(AFUNC ## _ctor_function)

PDK_CORE_EXPORT uint global_posted_events_count();
void PDK_CORE_EXPORT call_post_routines();

} // kernel
} // pdk

#endif // PDK_KERNEL_ABSTRACT_CORE_APPLICATION_H
