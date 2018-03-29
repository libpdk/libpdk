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
// Created by softboy on 2018/01/26.

#include "pdk/global/GlobalStatic.h"
#include "pdk/global/internal/HooksPrivate.h"
#include "pdk/global/internal/PdkInternal.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/CoreEvent.h"
#include "pdk/kernel/EventLoop.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/kernel/internal/CoreCommandLineArgsPrivate.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/utils/Translator.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/ThreadPool.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/base/os/thread/ThreadStorage.h"
#include "pdk/base/os/thread/ReadWriteLock.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/StandardPaths.h"
#include "pdk/global/LibraryInfo.h"
#include "pdk/dll/internal/FactoryLoaderPrivate.h"

#include <cstdlib>
#include <list>
#include <mutex>
#include <string>
#include <cstring>

#ifdef PDK_OS_UNIX
#  include <locale.h>
#  include <unistd.h>
#  include <sys/types.h>
#endif

#include <algorithm>

#if defined(PDK_OS_UNIX)
# if defined(PDK_OS_DARWIN)
#  include "pdk/kernel/EventDispatcherCf.h"
# endif
# include "pdk/kernel/EventDispatcherUnix.h"
#endif

namespace pdk {
namespace kernel {

using pdk::utils::ScopedPointer;
using pdk::utils::Translator;
using pdk::os::thread::Thread;
using pdk::os::thread::internal::ThreadData;
using pdk::os::thread::internal::PostEvent;
using pdk::os::thread::internal::PostEventList;
using pdk::os::thread::ThreadStorageData;
using pdk::os::thread::ThreadPool;
using pdk::os::thread::internal::ScopedScopeLevelCounter;
using pdk::kernel::internal::CoreApplicationPrivate;
using pdk::os::thread::ReadLocker;
using pdk::os::thread::WriteLocker;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::lang::StringList;
using pdk::io::fs::File;
using pdk::io::fs::Dir;
using pdk::io::fs::FileInfo;
using pdk::io::fs::StandardPaths;
using pdk::LibraryInfo;
using pdk::ds::VarLengthArray;
using pdk::dll::internal::FactoryLoader;

#if defined(PDK_OS_WIN) || defined(PDK_OS_MAC)
extern String retrieve_app_filename();
#endif

#if defined(PDK_OS_DARWIN)
using pdk::kernel::EventDispatcherCoreFoundation;
#endif

extern "C" void PDK_CORE_EXPORT startup_hook()
{}

using StartupFuncList = std::list<StartUpFunction>;
PDK_GLOBAL_STATIC(StartupFuncList, sg_preRList);
using ShutdownFuncList = std::list<CleanUpFunction>;
PDK_GLOBAL_STATIC(ShutdownFuncList, sg_postRList);

static std::mutex sg_preRoutinesMutex;

class MutexUnlocker
{
public:
   inline explicit MutexUnlocker(std::mutex *mutex)
      : m_mutex(mutex)
   { }
   inline ~MutexUnlocker()
   {
      unlock();
   }
   inline void unlock()
   {
      if (m_mutex) {
         m_mutex->unlock();
      }
      m_mutex = 0;
   }
   
private:
   PDK_DISABLE_COPY(MutexUnlocker);
   std::mutex *m_mutex;
};

namespace {

void call_pre_routines()
{
   if (!sg_preRList.exists()) {
      return;
   }
   StartupFuncList list;
   {
      std::lock_guard<std::mutex> locker(sg_preRoutinesMutex);
      // Unlike pdk::kernel::call_post_routines, we don't empty the list, because
      // PDK_COREAPP_STARTUP_FUNCTION is a macro, so the user expects
      // the function to be executed every time CoreApplication is created.
      list = *sg_preRList;
   }
   auto iter = list.cbegin();
   auto end = list.cend();
   while (iter != end) {
      (*iter)();
      ++iter;
   }
}

#if defined(PDK_OS_WIN)
// Check whether the command line arguments match those passed to main()
// by comparing to the global __argv/__argc (MS extension).
// Deep comparison is required since argv/argc is rebuilt by WinMain for
// GUI apps or when using MinGW due to its globbing.
inline bool is_argv_modified(int argc, char **argv)
{
   if (__argc != argc || !__argv /* wmain() */)
      return true;
   if (__argv == argv)
      return false;
   for (int a = 0; a < argc; ++a) {
      if (argv[a] != __argv[a] && strcmp(argv[a], __argv[a]))
         return true;
   }
   return false;
}

inline bool contains(int argc, char **argv, const char *needle)
{
   for (int a = 0; a < argc; ++a) {
      if (!strcmp(argv[a], needle)) {
         return true;
      }
   }
   return false;
}
#endif // PDK_OS_WIN

bool do_notify(Object *receiver, Event *event)
{
   if (receiver == nullptr) {                        // serious error
      warning_stream("CoreApplication::notify: Unexpected null receiver");
      return true;
   }
#ifndef PDK_NO_DEBUG
   CoreApplicationPrivate::checkReceiverThread(receiver);
#endif
   return receiver->isWidgetType() 
         ? false 
         : CoreApplicationPrivate::notifyHelper(receiver, event);
}

}

namespace internal {

uint CoreApplicationPrivate::sm_appCompileVersion = 0x010000;
bool CoreApplicationPrivate::sm_setuidAllowed = false;

#ifdef PDK_OS_DARWIN
String CoreApplicationPrivate::infoDictionaryStringProperty(const String &propertyName)
{
   String bundleName;
   CFString cfPropertyName = propertyName.toCFString();
   CFTypeRef string = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
                                                           cfPropertyName);
   if (string) {
      bundleName = String::fromCFString(static_cast<CFStringRef>(string));
   }
   return bundleName;
}
#endif

String CoreApplicationPrivate::getAppName() const
{
   String appName;
#ifdef PDK_OS_DARWIN
   appName = infoDictionaryStringProperty(StringLiteral("CFBundleName"));
#endif
   if (m_argv[0]) {
      char *p = std::strrchr(m_argv[0], '/');
      appName = String::fromLocal8Bit(p ? (p + 1) : m_argv[0]);
   }
   return appName;
}

String CoreApplicationPrivate::getAppVersion() const
{
   String applicationVersion;
#ifdef PDK_OS_DARWIN
   applicationVersion = infoDictionaryStringProperty(StringLiteral("CFBundleVersion"));
#endif
   return applicationVersion;
}

String *CoreApplicationPrivate::sm_cachedAppFilePath = nullptr;

bool CoreApplicationPrivate::checkInstance(const char *method)
{
   bool flag = (CoreApplication::sm_self != nullptr);
   if (!flag) {
      warning_stream("CoreApplication::%s: Please instantiate the CoreApplication object first", method);
   }
   return flag;
}

void CoreApplicationPrivate::processCommandLineArguments()
{
   int j = m_argc ? 1 : 0;
   for (int i = 1; i < m_argc; ++i) {
      if (!m_argv[i]) {
         continue;
      }
      if (*m_argv[i] != '-') {
         m_argv[j++] = m_argv[i];
         continue;
      }
      const char *arg = m_argv[i];
      if (arg[1] == '-') { // startsWith("--")
         ++arg;
      }
      m_argv[j++] = m_argv[i];
   }
   
   if (j < m_argc) {
      m_argv[j] = 0;
      m_argc = j;
   }
}

} // anonymous namespace

void add_pre_routine(StartUpFunction func)
{
   StartupFuncList *list = sg_preRList();
   if (!list) {
      return;
   }
   // Due to C++11 parallel dynamic initialization, this can be called
   // from multiple threads.
   std::lock_guard<std::mutex> locker(sg_preRoutinesMutex);
   if (CoreApplication::getInstance()) {
      func();
   }
   list->push_back(func);
}

void add_post_routine(CleanUpFunction func)
{
   ShutdownFuncList *list = sg_postRList();
   if (!list) {
      return;
   }
   std::lock_guard<std::mutex> locker(sg_preRoutinesMutex);
   list->push_front(func);
}

void remove_post_routine(CleanUpFunction func)
{
   ShutdownFuncList *list = sg_postRList();
   if (!list) {
      return;
   }
   std::lock_guard<std::mutex> locker(sg_preRoutinesMutex);
   list->remove(func);
}

void call_post_routines()
{
   if (!sg_postRList.exists()) {
      return;
   }
   while (true) {
      ShutdownFuncList list;
      {
         std::lock_guard<std::mutex> locker(sg_preRoutinesMutex);
         std::swap(*sg_postRList, list);
      }
      if (list.empty()) {
         break;
      }
      
      for (CleanUpFunction func : std::as_const(list)) {
         func();
      }
   }
}

static bool sg_localeInitialized = false;

#ifdef PDK_OS_UNIX
pdk::HANDLE pdk_application_thread_id = 0;
#endif

uint global_posted_events_count()
{
   ThreadData *currentThreadData = ThreadData::current();
   return currentThreadData->m_postEventList.size() - currentThreadData->m_postEventList.m_startOffset;
}

CoreApplication *CoreApplication::sm_self = nullptr;

// current pdk does not support GUI programming
uint CoreApplicationPrivate::sm_attribs = 0;

struct CoreApplicationData {
   CoreApplicationData() noexcept
   {
      m_appNameSet = false;
      m_appVersionSet = false;
   }
   ~CoreApplicationData()
   {
      // cleanup the QAdoptedThread created for the main() thread
      if (CoreApplicationPrivate::sm_theMainThread) {
         internal::ThreadData *data = internal::ThreadData::get(CoreApplicationPrivate::sm_theMainThread);
         data->deref(); // deletes the data and the adopted thread
      }
   }
   
   String m_orgName;
   String m_orgDomain;
   String m_appName; // application name, initially from argv[0], can then be modified.
   String m_appVersion;
   bool m_appNameSet; // true if setAppName was called
   bool m_appVersionSet; // true if setAppVersion was called
#if PDK_CONFIG(library)
   ScopedPointer<StringList> m_appLibPaths;
   ScopedPointer<StringList> m_manualLibPaths;
#endif
};

PDK_GLOBAL_STATIC(CoreApplicationData, sg_coreAppData);
static bool sg_quitLockRefEnabled = true;

namespace internal {
bool CoreApplicationPrivate::sm_isAppRunning = false;
bool CoreApplicationPrivate::sm_isAppClosing = false;
AbstractEventDispatcher *CoreApplicationPrivate::sm_eventDispatcher = nullptr;

CoreApplicationPrivate::CoreApplicationPrivate(int &argc, char **argv, uint flags)
   : ObjectPrivate(),
     #if defined(PDK_OS_WIN)
     m_origArgc(0),
     m_origArgv(nullptr),
     #endif
     m_appType(CoreApplicationPrivate::Type::Tty),
     m_inExec(false),
     m_aboutToQuitEmitted(false),
     m_threadDataClean(false),
     m_argc(argc),
     m_argv(argv)
{
#if defined(PDK_OS_DARWIN)
   apple_check_os_version();
#endif
   sm_appCompileVersion = flags & 0xffffff;
   static const char *const empty = "";
   if (m_argc == 0 || m_argv == nullptr) {
      m_argc = 0;
      m_argv = const_cast<char **>(&empty);
   }
#if defined(PDK_OS_WIN)
   if (!is_argv_modified(argc, argv)) {
      m_origArgc = argc;
      m_origArgv = new char *[argc];
      std::copy(argv, argv + argc, PDK_MAKE_CHECKED_ARRAY_ITERATOR(origArgv, argc));
   }
#endif // PDK_OS_WIN
   CoreApplicationPrivate::sm_isAppClosing = false;
   
#if defined(PDK_OS_UNIX)
   if (PDK_UNLIKELY(!sm_setuidAllowed && (geteuid() != getuid())))
      fatal_stream("FATAL: The application binary appears to be running setuid, this is a security hole.");
#endif // PDK_OS_UNIX
   Thread *cur = Thread::getCurrentThread(); // note: this may end up setting theMainThread!
   if (cur != sm_theMainThread) {
      warning_stream("WARNING: Application was not created in the main() thread.");
   }
}

CoreApplicationPrivate::~CoreApplicationPrivate()
{
   cleanupThreadData();
#if defined(PDK_OS_WIN)
   delete [] m_origArgv;
#endif
   CoreApplicationPrivate::clearAppFilePath();
}

void CoreApplicationPrivate::cleanupThreadData()
{
   if (m_threadData && !m_threadDataClean) {
      void *data = &m_threadData->m_tls;
      ThreadStorageData::finish(reinterpret_cast<void **>(data));
   }
   std::lock_guard<std::mutex> locker(m_threadData->m_postEventList.m_mutex);
   for (size_t i = 0; i < m_threadData->m_postEventList.size(); ++i) {
      const PostEvent &pe = m_threadData->m_postEventList.at(i);
      if (pe.m_event) {
         --pe.m_receiver->getImplPtr()->m_postedEvents;
         pe.m_event->m_posted = false;
         delete pe.m_event;
      }
   }
   m_threadData->m_postEventList.clear();
   m_threadData->m_postEventList.m_recursion = 0;
   m_threadData->m_quitNow = false;
   m_threadDataClean = true;
}

void CoreApplicationPrivate::createEventDispatcher()
{
   PDK_Q(CoreApplication);
#if defined(PDK_OS_UNIX)
#  if defined(PDK_OS_DARWIN)
   bool ok = false;
   int value = pdk::env_var_intval("PDK_EVENT_DISPATCHER_CORE_FOUNDATION", &ok);
   if (ok && value > 0) {
      sm_eventDispatcher = new EventDispatcherCoreFoundation(apiPtr);
   } else {
      sm_eventDispatcher = new EventDispatcherUNIX(apiPtr);
   }
#  endif
#else
#  error "pdk::kernel::EventDispatcher not yet ported to this platform"
#endif
}

void CoreApplicationPrivate::eventDispatcherReady()
{}

BasicAtomicPointer<Thread> CoreApplicationPrivate::sm_theMainThread = PDK_BASIC_ATOMIC_INITIALIZER(0);

Thread *CoreApplicationPrivate::getMainThread()
{
   PDK_ASSERT(sm_theMainThread.load() != nullptr);
   return sm_theMainThread.load();
}

bool CoreApplicationPrivate::threadRequiresCoreApplication()
{
   ThreadData *data = ThreadData::current(false);
   if (!data) {
      return true;
   }
   return data->m_requiresCoreApplication;
}

void CoreApplicationPrivate::checkReceiverThread(Object *receiver)
{
   Thread *currentThread = Thread::getCurrentThread();
   Thread *thread = receiver->getThread();
   PDK_ASSERT_X(currentThread == thread || !thread,
                "CoreApplication::sendEvent",
                String::fromLatin1("Cannot send events to objects owned by a different thread. "
                                   "Current thread %1. Receiver '%2' (of type '%3') was created in thread %4")
                .arg(String::number((pdk::uintptr) currentThread, 16))
                .arg(receiver->getObjectName())
                .arg(Latin1String(typeid(receiver).name()))
                .arg(String::number((pdk::uintptr) thread, 16))
                .toLocal8Bit().getRawData());
   PDK_ASSERT_X(currentThread == thread || !thread,
                "CoreApplication::sendEvent",
                "Cannot send events to objects owned by a different thread.");
   PDK_UNUSED(currentThread);
   PDK_UNUSED(thread);
}

void CoreApplicationPrivate::appendAppPathToLibPaths()
{
#if PDK_CONFIG(library)
   StringList *appLibPaths = sg_coreAppData()->m_appLibPaths.getData();
   if (!appLibPaths) {
      sg_coreAppData->m_appLibPaths.reset(appLibPaths = new StringList);
   }
   String appLocation = CoreApplication::getAppFilePath();
   appLocation.truncate(appLocation.lastIndexOf(Latin1Character('/')));
   // TODO we need check exist
   appLocation = Dir(appLocation).getCanonicalPath();
   if (File::exists(appLocation) && !appLibPaths->contains(appLocation)) {
      appLibPaths->push_back(appLocation);
   }  
#endif
}

void CoreApplicationPrivate::initLocale()
{
   if (sg_localeInitialized) {
      return;
   }
   sg_localeInitialized = true;
#ifdef PDK_OS_UNIX
   setlocale(LC_ALL, "");
#endif
}

void CoreApplicationPrivate::init()
{
#if defined(PDK_OS_MACOS)
   MacAutoReleasePool pool;
#endif
   PDK_Q(CoreApplication);
   initLocale();
   PDK_ASSERT_X(!CoreApplication::sm_self, "CoreApplication", "there should be only one application object");
   CoreApplication::sm_self = apiPtr;
   // Store app name/version (so they're still available after CoreApplication is destroyed)
   if (!sg_coreAppData()->m_appNameSet) {
      sg_coreAppData()->m_appName = retrieve_app_name();
   }
   if (!sg_coreAppData()->m_appVersionSet) {
      sg_coreAppData()->m_appVersion = getAppVersion();
   }
#if PDK_CONFIG(library)
   // Reset the lib paths, so that they will be recomputed, taking the availability of argv[0]
   // into account. If necessary, recompute right away and replay the manual changes on top of the
   // new lib paths.
   StringList *appPaths = sg_coreAppData()->m_appLibPaths.take();
   StringList *manualPaths = sg_coreAppData()->m_manualLibPaths.take();
   if (appPaths) {
      if (manualPaths) {
         // Replay the delta. As paths can only be prepended to the front or removed from
         // anywhere in the list, we can just linearly scan the lists and find the items that
         // have been removed. Once the original list is exhausted we know all the remaining
         // items have been added.
         StringList newPaths(apiPtr->getLibraryPaths());
         for (int i = manualPaths->size(), j = appPaths->size(); i > 0 || j > 0; pdk_noop()) {
            if (--j < 0) {
               newPaths.insert(newPaths.begin(), (*manualPaths)[--i]);
            } else if (--i < 0) {
               newPaths.remove((*appPaths)[j]);
            } else if ((*manualPaths)[i] != (*appPaths)[j]) {
               newPaths.remove((*appPaths)[j]);
               ++i; // try again with next item.
            }
         }
         delete manualPaths;
         sg_coreAppData()->m_manualLibPaths.reset(new StringList(newPaths));
      }
      delete appPaths;
   }
#endif
   // use the event dispatcher created by the app programmer (if any)
   if (!sm_eventDispatcher) {
      sm_eventDispatcher = m_threadData->m_eventDispatcher.load();
   }
   // otherwise we create one
   if (!sm_eventDispatcher) {
      createEventDispatcher();
   }
   PDK_ASSERT(sm_eventDispatcher);
   if (!sm_eventDispatcher->getParent()) {
      sm_eventDispatcher->moveToThread(m_threadData->m_thread);
      sm_eventDispatcher->setParent(apiPtr);
   }
   m_threadData->m_eventDispatcher = sm_eventDispatcher;
   eventDispatcherReady();
   processCommandLineArguments();
   call_pre_routines();
   startup_hook();
   if (PDK_UNLIKELY(pdk::sg_pdkHookData[pdk::hooks::Startup])) {
      reinterpret_cast<pdk::hooks::StartupCallback>(pdk::sg_pdkHookData[pdk::hooks::Startup])();
   }
   sm_isAppRunning = true; // No longer starting up.
}

bool CoreApplicationPrivate::sendThroughApplicationEventFilters(Object *receiver, Event *event)
{
   // We can't access the application event filters outside of the main thread (race conditions)
   PDK_ASSERT(receiver->getImplPtr()->m_threadData->m_thread == getMainThread());
   if (m_extraData) {
      // application event filters are only called for objects in the GUI thread
      for (size_t i = 0; i < m_extraData->m_eventFilters.size(); ++i) {
         auto iter = m_extraData->m_eventFilters.begin();
         std::advance(iter, i);
         Object *obj = *iter;
         if (!obj) {
            continue;
         }
         if (obj->getImplPtr()->m_threadData != m_threadData) {
            warning_stream("CoreApplication: Application event filter cannot be in a different thread.");
            continue;
         }
         if (obj->eventFilter(receiver, event)) {
            return true;
         }
      }
   }
   return false;
}

bool CoreApplicationPrivate::sendThroughObjectEventFilters(Object *receiver, Event *event)
{
   if (receiver != CoreApplication::getInstance() && receiver->getImplPtr()->m_extraData) {
      for (size_t i = 0; i < receiver->getImplPtr()->m_extraData->m_eventFilters.size(); ++i) {
         auto iter = receiver->getImplPtr()->m_extraData->m_eventFilters.begin();
         std::advance(iter, i);
         Object *obj = *iter;
         if (!obj) {
            continue;
         }
         if (obj->getImplPtr()->m_threadData != receiver->getImplPtr()->m_threadData) {
            warning_stream("CoreApplication: Object event filter cannot be in a different thread.");
            continue;
         }
         if (obj->eventFilter(receiver, event)) {
            return true;
         }
      }
   }
   return false;
}

bool CoreApplicationPrivate::notifyHelper(Object *receiver, Event *event)
{
   // send to all application event filters (only does anything in the main thread)
   if (CoreApplication::sm_self
       && receiver->getImplPtr()->m_threadData->m_thread == getMainThread()
       && CoreApplication::sm_self->getImplPtr()->sendThroughApplicationEventFilters(receiver, event))
      return true;
   // send to all receiver event filters
   if (sendThroughObjectEventFilters(receiver, event)) {
      return true;
   }
   // deliver the event
   return receiver->event(event);
}

// Cleanup after eventLoop is done executing in CoreApplication::exec().
// This is for use cases in which CoreApplication is instantiated by a
// library and not by an application executable, for example, Active X
// servers.

void CoreApplicationPrivate::execCleanup()
{
   m_threadData->m_quitNow = false;
   m_inExec = false;
   if (!m_aboutToQuitEmitted) {
      getApiPtr()->emitAboutToQuitSignal();
   }
   m_aboutToQuitEmitted = true;
   CoreApplication::sendPostedEvents(0, Event::Type::DeferredDelete);
}

} // internal

String retrieve_app_name()
{
   if (!CoreApplicationPrivate::checkInstance("retrieve_app_name")) {
      return String();
   }
   return CoreApplication::getInstance()->getImplPtr()->getAppName();
}

CoreApplication::CoreApplication(CoreApplicationPrivate &p)
   : Object(p, nullptr)
{
   getImplPtr()->m_apiPtr = this;
}

CoreApplication::CoreApplication(int &argc, char **argv, int internal)
   : Object(*new CoreApplicationPrivate(argc, argv, internal))
{
   getImplPtr()->m_apiPtr = this;
   getImplPtr()->init();
   CoreApplicationPrivate::sm_eventDispatcher->startingUp();
}

#ifndef PDK_NO_TRANSLATION
bool CoreApplication::installTranslator(Translator *translationFile)
{
   if (!translationFile) {
      return false;
   }
   if (!CoreApplicationPrivate::checkInstance("installTranslator")) {
      return false;
   }
   CoreApplicationPrivate *implPtr = sm_self->getImplPtr();
   {
      WriteLocker locker(&implPtr->m_translateMutex);
      implPtr->m_translators.insert(implPtr->m_translators.begin(), translationFile);
   }
   
#ifndef PDK_NO_TRANSLATION_BUILDER
   if (translationFile->isEmpty()) {
      return false;
   }
#endif
   
   Event event(Event::Type::LanguageChange);
   CoreApplication::sendEvent(sm_self, &event);
   return true;
}

bool CoreApplication::removeTranslator(Translator *translationFile)
{
   if (!translationFile) {
      return false;
   }
   if (!CoreApplicationPrivate::checkInstance("removeTranslator")) {
      return false;
   }
   CoreApplicationPrivate *d = sm_self->getImplPtr();
   WriteLocker locker(&d->m_translateMutex);
   int removedCount = 0;
   d->m_translators.remove_if(
            [&removedCount, translationFile](const Translator *item)->bool{
      if (translationFile == item) {
         ++removedCount;
         return true;
      }
      return false;
   });
   if (removedCount) {
#ifndef PDK_NO_Object
      locker.unlock();
      if (!sm_self->closingDown()) {
         Event ev(Event::Type::LanguageChange);
         CoreApplication::sendEvent(sm_self, &ev);
      }
#endif
      return true;
   }
   return false;
}

namespace {

void replace_percent_n(String *result, int n)
{
   if (n >= 0) {
      int percentPos = 0;
      int len = 0;
      while ((percentPos = result->indexOf(Latin1Character('%'), percentPos + len)) != -1) {
         len = 1;
         String fmt;
         if (result->at(percentPos + len) == Latin1Character('L')) {
            ++len;
            fmt = Latin1String("%L1");
         } else {
            fmt = Latin1String("%1");
         }
         if (result->at(percentPos + len) == Latin1Character('n')) {
            fmt = fmt.arg(n);
            ++len;
            result->replace(percentPos, len, fmt);
            len = fmt.length();
         }
      }
   }
}


} // anonymous namespace

String CoreApplication::translate(const char *context, const char *sourceText,
                                  const char *disambiguation, int n)
{
   String result;
   
   if (!sourceText)
      return result;
   
   if (sm_self) {
      CoreApplicationPrivate *d = sm_self->getImplPtr();
      ReadLocker locker(&d->m_translateMutex);
      if (!d->m_translators.empty()) {
         std::list<Translator *>::const_iterator iter;
         Translator *translationFile;
         for (iter = d->m_translators.cbegin(); iter != d->m_translators.cend(); ++iter) {
            translationFile = *iter;
            result = translationFile->translate(context, sourceText, disambiguation, n);
            if (!result.isNull()) {
               break;
            }   
         }
      }
   }
   
   if (result.isNull()) {
      result = String::fromUtf8(sourceText);
   }
   replace_percent_n(&result, n);
   return result;
}

bool CoreApplicationPrivate::isTranslatorInstalled(Translator *translator)
{
   if (!CoreApplication::sm_self) {
      return false;
   }
   
   CoreApplicationPrivate *d = CoreApplication::sm_self->getImplPtr();
   ReadLocker locker(&d->m_translateMutex);
   return std::find(d->m_translators.begin(), d->m_translators.end(), translator) != d->m_translators.end();
}

#else
String CoreApplication::translate(const char *context, const char *sourceText,
                                  const char *disambiguation, int n)
{
   PDK_UNUSED(context);
   PDK_UNUSED(disambiguation);
   String ret = String::fromUtf8(sourceText);
   if (n >= 0) {
      ret.replace(Latin1String("%n"), String::number(n));
   }
   return ret;
}
#endif


CoreApplication::~CoreApplication()
{
   call_post_routines();
   sm_self = nullptr;
   CoreApplicationPrivate::sm_isAppClosing = true;
   CoreApplicationPrivate::sm_isAppRunning = false;
   ThreadPool *globalThreadPool = nullptr;
   try {
      globalThreadPool = ThreadPool::getGlobalInstance();
   } catch (...){
      // swallow the exception, since destructors shouldn't throw
   }
   if (globalThreadPool) {
      globalThreadPool->waitForDone();
   }
   getImplPtr()->m_threadData->m_eventDispatcher = 0;
   if (CoreApplicationPrivate::sm_eventDispatcher) {
      CoreApplicationPrivate::sm_eventDispatcher->closingDown();
   }
   CoreApplicationPrivate::sm_eventDispatcher = 0;
#if PDK_CONFIG(library)
   sg_coreAppData()->m_appLibPaths.reset();
   sg_coreAppData()->m_manualLibPaths.reset();
#endif
}

void CoreApplication::setAttribute(pdk::AppAttribute attribute, bool on)
{
   if (on) {
      CoreApplicationPrivate::sm_attribs |= 1 << pdk::as_integer<pdk::AppAttribute>(attribute);
   } else {
      CoreApplicationPrivate::sm_attribs &= ~(1 << pdk::as_integer<pdk::AppAttribute>(attribute));
   }
}

bool CoreApplication::testAttribute(pdk::AppAttribute attribute)
{
   return CoreApplicationPrivate::testAttribute(attribute);
}

void CoreApplication::setSetuidAllowed(bool allow)
{
   CoreApplicationPrivate::sm_setuidAllowed = allow;
}

bool CoreApplication::isSetuidAllowed()
{
   return CoreApplicationPrivate::sm_setuidAllowed;
}

bool CoreApplication::isQuitLockEnabled()
{
   return sg_quitLockRefEnabled;
}

void CoreApplication::setQuitLockEnabled(bool enabled)
{
   sg_quitLockRefEnabled = enabled;
}

bool CoreApplication::notifyInternal(Object *receiver, Event *event)
{
   bool selfRequired = CoreApplicationPrivate::threadRequiresCoreApplication();
   if (!sm_self && selfRequired) {
      return false;
   }
   // Make it possible for pdk Script to hook into events even
   // though Application is subclassed...
   bool result = false;
   void *cbdata[] = { receiver, event, &result };
   if (Internal::activateCallbacks(Internal::Callback::EventNotifyCallback, cbdata)) {
      return result;
   }
   // pdk enforces the rule that events can only be sent to objects in
   // the current thread, so receiver->d_func()->threadData is
   // equivalent to ThreadData::current(), just without the function
   // call overhead.
   ObjectPrivate *d = receiver->getImplPtr();
   ThreadData *threadData = d->m_threadData;
   ScopedScopeLevelCounter scopeLevelCounter(threadData);
   if (!selfRequired) {
      return do_notify(receiver, event);
   }
   return sm_self->notify(receiver, event);
}

bool CoreApplication::forwardEvent(Object *receiver, Event *event, Event *originatingEvent)
{
   if (event && originatingEvent) {
      event->m_spont = originatingEvent->m_spont;
   }
   return notifyInternal(receiver, event);
}

bool CoreApplication::notify(Object *receiver, Event *event)
{
   // no events are delivered after ~CoreApplication() has started
   if (CoreApplicationPrivate::sm_isAppClosing) {
      return true;
   }
   return do_notify(receiver, event);
}

bool CoreApplication::startingUp()
{
   return !CoreApplicationPrivate::sm_isAppRunning;
}

bool CoreApplication::closingDown()
{
   return CoreApplicationPrivate::sm_isAppClosing;
}

void CoreApplication::processEvents(EventLoop::ProcessEventsFlags flags)
{
   ThreadData *data = ThreadData::current();
   if (!data->hasEventDispatcher()) {
      return;
   }
   data->m_eventDispatcher.load()->processEvents(flags);
}

void CoreApplication::processEvents(EventLoop::ProcessEventsFlags flags, int maxtime)
{
   ThreadData *data = ThreadData::current();
   if (!data->hasEventDispatcher()) {
      return;
   }
   ElapsedTimer start;
   start.start();
   while (data->m_eventDispatcher.load()->processEvents(flags & ~EventLoop::WaitForMoreEvents)) {
      if (start.getElapsed() > maxtime) {
         break;
      }
   }
}

int CoreApplication::exec()
{
   if (!CoreApplicationPrivate::checkInstance("exec")) {
      return -1;
   }
   ThreadData *threadData = sm_self->getImplPtr()->m_threadData;
   if (threadData != ThreadData::current()) {
      warning_stream("%s::exec: Must be called from the main thread", typeid(sm_self).name());
      return -1;
   }
   if (!threadData->m_eventLoops.empty()) {
      warning_stream("CoreApplication::exec: The event loop is already running");
      return -1;
   }
   threadData->m_quitNow = false;
   EventLoop eventLoop;
   sm_self->getImplPtr()->m_inExec = true;
   sm_self->getImplPtr()->m_aboutToQuitEmitted = false;
   int returnCode = eventLoop.exec();
   threadData->m_quitNow = false;
   if (sm_self) {
      sm_self->getImplPtr()->execCleanup();
   }
   return returnCode;
}

void CoreApplication::exit(int returnCode)
{
   if (!sm_self) {
      return;
   }
   ThreadData *data = sm_self->getImplPtr()->m_threadData;
   data->m_quitNow = true;
   for (size_t i = 0; i < data->m_eventLoops.size(); ++i) {
      EventLoop *eventLoop = data->m_eventLoops.at(i);
      eventLoop->exit(returnCode);
   }
}

void CoreApplication::postEvent(Object *receiver, Event *event, pdk::EventPriority priority)
{
   if (receiver == nullptr) {
      warning_stream("CoreApplication::postEvent: Unexpected null receiver");
      delete event;
      return;
   }
   ThreadData * volatile * pdata = &receiver->getImplPtr()->m_threadData;
   ThreadData *data = *pdata;
   if (!data) {
      // posting during destruction? just delete the event to prevent a leak
      delete event;
      return;
   }
   // lock the post event mutex
   data->m_postEventList.m_mutex.lock();
   // if object has moved to another thread, follow it
   while (data != *pdata) {
      data->m_postEventList.m_mutex.unlock();
      data = *pdata;
      if (!data) {
         // posting during destruction? just delete the event to prevent a leak
         delete event;
         return;
      }
      data->m_postEventList.m_mutex.lock();
   }
   MutexUnlocker locker(&data->m_postEventList.m_mutex);
   // if this is one of the compressible events, do compression
   if (receiver->getImplPtr()->m_postedEvents
       && sm_self && sm_self->compressEvent(event, receiver, &data->m_postEventList)) {
      return;
   }
   if (event->getType() == Event::Type::DeferredDelete) {
      receiver->m_implPtr->m_deleteLaterCalled = true;
   }
   if (event->getType() == Event::Type::DeferredDelete && data == ThreadData::current()) {
      // remember the current running eventloop for DeferredDelete
      // events posted in the receiver's thread.
      
      // Events sent by non-pdk event handlers (such as glib) may not
      // have the scopeLevel set correctly. The scope level makes sure that
      // code like this:
      //     foo->deleteLater();
      //     qApp->processEvents(); // without passing Event::Type::DeferredDelete
      // will not cause "foo" to be deleted before returning to the event loop.
      
      // If the scope level is 0 while loopLevel != 0, we are called from a
      // non-conformant code path, and our best guess is that the scope level
      // should be 1. (Loop level 0 is special: it means that no event loops
      // are running.)
      int loopLevel = data->m_loopLevel;
      int scopeLevel = data->m_scopeLevel;
      if (scopeLevel == 0 && loopLevel != 0) {
         scopeLevel = 1;
      }
      static_cast<DeferredDeleteEvent *>(event)->m_level = loopLevel + scopeLevel;
   }
   
   // delete the event on exceptions to protect against memory leaks till the event is
   // properly owned in the postEventList
   pdk::utils::ScopedPointer<Event> eventDeleter(event);
   data->m_postEventList.addEvent(PostEvent(receiver, event, pdk::as_integer<pdk::EventPriority>(priority)));
   eventDeleter.take();
   event->m_posted = true;
   ++receiver->getImplPtr()->m_postedEvents;
   data->m_canWait = false;
   locker.unlock();
   AbstractEventDispatcher* dispatcher = data->m_eventDispatcher.loadAcquire();
   if (dispatcher) {
      dispatcher->wakeUp();
   }
}

bool CoreApplication::compressEvent(Event *event, Object *receiver, PostEventList *postedEvents)
{
#ifdef PDK_OS_WIN
   PDK_ASSERT(event);
   PDK_ASSERT(receiver);
   PDK_ASSERT(postedEvents);
   
   // compress posted timers to this object.
   if (event->getType() == Event::Type::Timer && receiver->getImplPtr()->m_postedEvents > 0) {
      int timerId = ((TimerEvent *) event)->getTimerId();
      for (size_t i = 0; i < postedEvents->size(); ++i) {
         const PostEvent &e = postedEvents->at(i);
         if (e.m_receiver == receiver && e.m_event && e.m_event->getType() == Event::Type::Timer
             && ((TimerEvent *) e.m_event)->getTimerId() == timerId) {
            delete event;
            return true;
         }
      }
      return false;
   }
#endif
   if (event->getType() == Event::Type::DeferredDelete) {
      if (receiver->m_implPtr->m_deleteLaterCalled) {
         // there was a previous DeferredDelete event, so we can drop the new one
         delete event;
         return true;
      }
      // deleteLaterCalled is set to true in postedEvents when queueing the very first
      // deferred deletion event.
      return false;
   }
   if (event->getType() == Event::Type::Quit && receiver->getImplPtr()->m_postedEvents > 0) {
      for (size_t i = 0; i < postedEvents->size(); ++i) {
         const PostEvent &cur = postedEvents->at(i);
         if (cur.m_receiver != receiver
             || cur.m_event == 0
             || cur.m_event->getType() != event->getType())
            continue;
         // found an event for this receiver
         delete event;
         return true;
      }
   }
   return false;
}

void CoreApplication::sendPostedEvents(Object *receiver, Event::Type eventType)
{
   // ### @todo: consider splitting this method into a public and a private
   //           one, so that a user-invoked sendPostedEvents can be detected
   //           and handled properly.
   ThreadData *data = ThreadData::current();
   CoreApplicationPrivate::sendPostedEvents(receiver, eventType, data);
}

namespace internal {

void CoreApplicationPrivate::sendPostedEvents(Object *receiver, Event::Type eventType,
                                              ThreadData *data)
{
   if (receiver && receiver->getImplPtr()->m_threadData != data) {
      warning_stream("CoreApplication::sendPostedEvents: Cannot send "
                     "posted events for objects in another thread");
      return;
   }
   ++data->m_postEventList.m_recursion;
   std::unique_lock<std::mutex> locker(data->m_postEventList.m_mutex);
   // by default, we assume that the event dispatcher can go to sleep after
   // processing all events. if any new events are posted while we send
   // events, canWait will be set to false.
   data->m_canWait = (data->m_postEventList.size() == 0);
   if (data->m_postEventList.size() == 0 || (receiver && !receiver->getImplPtr()->m_postedEvents)) {
      --data->m_postEventList.m_recursion;
      return;
   }
   data->m_canWait = true;
   // okay. here is the tricky loop. be careful about optimizing
   // this, it looks the way it does for good reasons.
   int startOffset = data->m_postEventList.m_startOffset;
   int &i = (eventType == Event::Type::None && !receiver) ? data->m_postEventList.m_startOffset : startOffset;
   data->m_postEventList.m_insertionOffset = data->m_postEventList.size();
   // Exception-safe cleaning up without the need for a try/catch block
   struct CleanUp {
      Object *m_receiver;
      Event::Type m_eventType;
      ThreadData *m_data;
      bool m_exceptionCaught;
      
      inline CleanUp(Object *receiver, Event::Type eventType, ThreadData *data) 
         : m_receiver(receiver),
           m_eventType(eventType),
           m_data(data),
           m_exceptionCaught(true)
      {}
      inline ~CleanUp()
      {
         if (m_exceptionCaught) {
            // since we were interrupted, we need another pass to make sure we clean everything up
            m_data->m_canWait = false;
         }
         --m_data->m_postEventList.m_recursion;
         if (!m_data->m_postEventList.m_recursion && !m_data->m_canWait && m_data->hasEventDispatcher()) {
            m_data->m_eventDispatcher.load()->wakeUp();
         }
         // clear the global list, i.e. remove everything that was
         // delivered.
         if (Event::Type::None == m_eventType && !m_receiver &&
             m_data->m_postEventList.m_startOffset >= 0) {
            const PostEventList::iterator iter = m_data->m_postEventList.begin();
            m_data->m_postEventList.erase(iter, iter + m_data->m_postEventList.m_startOffset);
            m_data->m_postEventList.m_insertionOffset -= m_data->m_postEventList.m_startOffset;
            PDK_ASSERT(m_data->m_postEventList.m_insertionOffset >= 0);
            m_data->m_postEventList.m_startOffset = 0;
         }
      }
   };
   CleanUp cleanup(receiver, eventType, data);
   while ((size_t)i < data->m_postEventList.size()) {
      // avoid live-lock
      if (i >= data->m_postEventList.m_insertionOffset) {
         break;
      }
      const PostEvent &pe = data->m_postEventList.at(i);
      ++i;
      
      if (!pe.m_event) {
         continue;
      }
      if ((receiver && receiver != pe.m_receiver) ||
          (eventType != Event::Type::None && eventType != pe.m_event->getType())) {
         data->m_canWait = false;
         continue;
      }
      if (pe.m_event->getType() == Event::Type::DeferredDelete) {
         // DeferredDelete events are sent either
         // 1) when the event loop that posted the event has returned; or
         // 2) if explicitly requested (with Event::Type::DeferredDelete) for
         //    events posted by the current event loop; or
         // 3) if the event was posted before the outermost event loop.
         
         int eventLevel = static_cast<DeferredDeleteEvent *>(pe.m_event)->getLoopLevel();
         int loopLevel = data->m_loopLevel + data->m_scopeLevel;
         const bool allowDeferredDelete =
               (eventLevel > loopLevel
                || (!eventLevel && loopLevel > 0)
                || (eventType == Event::Type::DeferredDelete
                    && eventLevel == loopLevel));
         if (!allowDeferredDelete) {
            // cannot send deferred delete
            if (eventType == Event::Type::None && !receiver) {
               // we must copy it first; we want to re-post the event
               // with the event pointer intact, but we can't delay
               // nulling the event ptr until after re-posting, as
               // addEvent may invalidate pe.
               PostEvent pecopy = pe;
               // null out the event so if sendPostedEvents recurses, it
               // will ignore this one, as it's been re-posted.
               const_cast<PostEvent &>(pe).m_event = nullptr;
               // re-post the copied event so it isn't lost
               data->m_postEventList.addEvent(pecopy);
            }
            continue;
         }
      }
      // first, we diddle the event so that we can deliver
      // it, and that no one will try to touch it later.
      pe.m_event->m_posted = false;
      Event *e = pe.m_event;
      Object * r = pe.m_receiver;
      
      --r->getImplPtr()->m_postedEvents;
      PDK_ASSERT(r->getImplPtr()->m_postedEvents >= 0);
      // next, update the data structure so that we're ready
      // for the next event.
      const_cast<PostEvent &>(pe).m_event = 0;
      struct LocalMutexUnlocker
      {
         std::unique_lock<std::mutex> &m_locker;
         LocalMutexUnlocker(std::unique_lock<std::mutex> &m) 
            : m_locker(m)
         {
            m_locker.unlock(); 
         }
         ~LocalMutexUnlocker() { m_locker.lock(); }
      };
      LocalMutexUnlocker unlocker(locker);
      
      ScopedPointer<Event> eventDeleter(e); // will delete the event (with the mutex unlocked)
      // after all that work, it's time to deliver the event.
      CoreApplication::sendEvent(r, e);
      // careful when adding anything below this point - the
      // sendEvent() call might invalidate any invariants this
      // function depends on.
   }
   cleanup.m_exceptionCaught = false;
}

void CoreApplicationPrivate::removePostedEvent(Event * event)
{
   if (!event || !event->m_posted) {
      return;
   }
   ThreadData *data = ThreadData::current();
   std::lock_guard<std::mutex> locker(data->m_postEventList.m_mutex);
   if (data->m_postEventList.size() == 0) {
#if defined(PDK_DEBUG)
      debug_stream("CoreApplication::removePostedEvent: Internal error: %p %d is posted",
                   (void*)event, event->getType());
      return;
#endif
   }
   
   for (size_t i = 0; i < data->m_postEventList.size(); ++i) {
      const PostEvent & pe = data->m_postEventList.at(i);
      if (pe.m_event == event) {
#ifndef PDK_NO_DEBUG
         warning_stream("CoreApplication::removePostedEvent: Event of type %d deleted while posted to %s %s",
                        event->getType(),
                        typeid(pe.m_receiver).name(),
                        pe.m_receiver->getObjectName().toLocal8Bit().getRawData());
#endif
         --pe.m_receiver->getImplPtr()->m_postedEvents;
         pe.m_event->m_posted = false;
         delete pe.m_event;
         const_cast<PostEvent &>(pe).m_event = nullptr;
         return;
      }
   }
}

void CoreApplicationPrivate::ref()
{
   m_quitLockRef.ref();
}

void CoreApplicationPrivate::deref()
{
   if (!m_quitLockRef.deref()) {
      maybeQuit();
   }    
}

void CoreApplicationPrivate::maybeQuit()
{
   if (m_quitLockRef.load() == 0 && m_inExec && sg_quitLockRefEnabled && shouldQuit()) {
      CoreApplication::postEvent(CoreApplication::getInstance(), new Event(Event::Type::Quit));
   }
}

// Makes it possible to point CoreApplication to a custom location to ensure
// the directory is added to the patch, and pdk.conf and deployed plugins are
// found from there. This is for use cases in which GuiApplication is
// instantiated by a library and not by an application executable, for example,
// Active X servers.

void CoreApplicationPrivate::setAppFilePath(const String &path)
{
   if (CoreApplicationPrivate::sm_cachedAppFilePath) {
      *CoreApplicationPrivate::sm_cachedAppFilePath = path;
   } else {
      CoreApplicationPrivate::sm_cachedAppFilePath = new String(path);
   }
}

}

void CoreApplication::removePostedEvents(Object *receiver, Event::Type eventType)
{
   ThreadData *data = receiver ? receiver->getImplPtr()->m_threadData : ThreadData::current();
   std::unique_lock<std::mutex> locker(data->m_postEventList.m_mutex);
   // the Object destructor calls this function directly.  this can
   // happen while the event loop is in the middle of posting events,
   // and when we get here, we may not have any more posted events
   // for this object.
   if (receiver && !receiver->getImplPtr()->m_postedEvents) {
      return;
   }
   //we will collect all the posted events for the Object
   //and we'll delete after the mutex was unlocked
   VarLengthArray<Event*> events;
   int n = data->m_postEventList.size();
   int j = 0;
   for (int i = 0; i < n; ++i) {
      const PostEvent &pe = data->m_postEventList.at(i);
      if ((!receiver || pe.m_receiver == receiver)
          && (pe.m_event && (eventType == Event::Type::None || pe.m_event->getType() == eventType))) {
         --pe.m_receiver->getImplPtr()->m_postedEvents;
         pe.m_event->m_posted = false;
         events.append(pe.m_event);
         const_cast<PostEvent &>(pe).m_event = 0;
      } else if (!data->m_postEventList.m_recursion) {
         if (i != j) {
            std::swap(data->m_postEventList[i], data->m_postEventList[j]);
         }
         ++j;
      }
   }
   
#ifdef PDK_DEBUG
   if (receiver && eventType == Event::Type::None) {
      PDK_ASSERT(!receiver->getImplPtr()->m_postedEvents);
   }
#endif
   if (!data->m_postEventList.m_recursion) {
      // truncate list
      data->m_postEventList.erase(data->m_postEventList.begin() + j, data->m_postEventList.end());
   }
   locker.unlock();
   for (int i = 0; i < events.count(); ++i) {
      delete events[i];
   }
}

bool CoreApplication::event(Event *event)
{
   if (event->getType() == Event::Type::Quit) {
      quit();
      return true;
   }
   return Object::event(event);
}

void CoreApplication::quit()
{
   exit(0);
}

String CoreApplication::getAppDirPath()
{
   if (!sm_self) {
      warning_stream("CoreApplication::applicationDirPath: Please instantiate the Application object first");
      return String();
   }
   CoreApplicationPrivate *implPtr = sm_self->getImplPtr();
   if (implPtr->m_cachedAppDirPath.isNull()) {
      implPtr->m_cachedAppDirPath = FileInfo(getAppFilePath()).getPath();
   }
   return implPtr->m_cachedAppDirPath;
}

String CoreApplication::getAppFilePath()
{
   if (!sm_self) {
      warning_stream("CoreApplication::appFilePath: Please instantiate the Application object first");
      return String();
   }
   CoreApplicationPrivate *implPtr = sm_self->getImplPtr();
   if (implPtr->m_argc) {
      static ByteArray procName = ByteArray(implPtr->m_argv[0]);
      if (procName != implPtr->m_argv[0]) {
         // clear the cache if the procname changes, so we reprocess it.
         CoreApplicationPrivate::clearAppFilePath();
         procName = ByteArray(implPtr->m_argv[0]);
      }
   }
   
   if (CoreApplicationPrivate::sm_cachedAppFilePath) {
      return *CoreApplicationPrivate::sm_cachedAppFilePath;
   }
#if defined(PDK_OS_WIN)
   CoreApplicationPrivate::setAppFilePath(FileInfo(retrieve_app_filename()).getFilePath());
   return *CoreApplicationPrivate::sm_cachedAppFilePath;
#elif defined(PDK_OS_MAC)
   String appFilename = retrieve_app_filename();
   if(!appFilename.isEmpty()) {
      FileInfo fi(appFilename);
      if (fi.exists()) {
         CoreApplicationPrivate::setAppFilePath(fi.getCanonicalFilePath());
         return *CoreApplicationPrivate::sm_cachedAppFilePath;
      }
   }
#endif
#if defined( PDK_OS_UNIX )
#  if defined(PDK_OS_LINUX)
   // Try looking for a /proc/<pid>/exe symlink first which points to
   // the absolute path of the executable
   FileInfo pfi(String::fromLatin1("/proc/%1/exe").arg(getpid()));
   if (pfi.exists() && pfi.isSymLink()) {
      CoreApplicationPrivate::setAppFilePath(pfi.getCanonicalFilePath());
      return *CoreApplicationPrivate::sm_cachedAppFilePath;
   }
#  endif
   if (!getArguments().empty()) {
      String argv0 = File::decodeName(getArguments().at(0).toLocal8Bit());
      String absPath;
      
      if (!argv0.isEmpty() && argv0.at(0) == Latin1Character('/')) {
         // If argv0 starts with a slash, it is already an absolute
         // file path.
         absPath = argv0;
      } else if (argv0.contains(Latin1Character('/'))) {
         // If argv0 contains one or more slashes, it is a file path
         // relative to the current directory.
         absPath = Dir::getCurrent().getAbsoluteFilePath(argv0);
      } else {
         // Otherwise, the file path has to be determined using the
         // PATH environment variable.
         absPath = StandardPaths::findExecutable(argv0);
      }
      absPath = Dir::cleanPath(absPath);
      FileInfo fi(absPath);
      if (fi.exists()) {
         CoreApplicationPrivate::setAppFilePath(fi.getCanonicalFilePath());
         return *CoreApplicationPrivate::sm_cachedAppFilePath;
      }
   }
   
#endif
   return String();
}

pdk::pint64 CoreApplication::getAppPid()
{
#if defined(PDK_OS_WIN)
   return GetCurrentProcessId();
#elif defined(PDK_OS_VXWORKS)
   return (pid_t) taskIdCurrent;
#else
   return getpid();
#endif
}

StringList CoreApplication::getArguments()
{
   StringList list;
   if (!sm_self) {
      warning_stream("CoreApplication::arguments: Please instantiate the Application object first");
      return list;
   }
   const int ac = sm_self->getImplPtr()->m_argc;
   char ** const av = sm_self->getImplPtr()->m_argv;
   list.resize(ac);
   
#if defined(PDK_OS_WIN)
   // On Windows, it is possible to pass Unicode arguments on
   // the command line. To restore those, we split the command line
   // and filter out arguments that were deleted by derived application
   // classes by index.
   String cmdline = String::fromWCharArray(GetCommandLine());
   const CoreApplicationPrivate *d = sm_self->getImplPtr();
   if (d->m_origArgv) {
      const StringList allArguments = pdk::kernel::internal::win_cmd_args(cmdline);
      PDK_ASSERT(allArguments.size() == d->origArgc);
      for (int i = 0; i < d->origArgc; ++i) {
         if (contains(ac, av, d->origArgv[i])) {
            list.append(allArguments.at(i));
         }
      }
      return list;
   } // Fall back to rebuilding from argv/argc when a modified argv was passed.
#endif // defined(PDK_OS_WIN)
   
   for (int a = 0; a < ac; ++a) {
      list << String::fromLocal8Bit(av[a]);
   }
   return list;
}

void CoreApplication::setOrgName(const String &orgName)
{
   if (sg_coreAppData()->m_orgName == orgName) {
      return;
   }
   sg_coreAppData()->m_orgName = orgName;
#ifndef PDK_NO_Object
   if (CoreApplication::sm_self) {
      CoreApplication::sm_self->emitOrgNameChangedSignal();
   }      
#endif
}

String CoreApplication::getOrgName()
{
   return sg_coreAppData()->m_orgName;
}

void CoreApplication::setOrgDomain(const String &orgDomain)
{
   if (sg_coreAppData()->m_orgDomain == orgDomain) {
      return;
   }
   sg_coreAppData()->m_orgDomain = orgDomain;
#ifndef PDK_NO_Object
   if (CoreApplication::sm_self) {
      CoreApplication::sm_self->emitOrgDomainChangedSignal();
   }
#endif
}

String CoreApplication::getOrgDomain()
{
   return sg_coreAppData()->m_orgDomain;
}

void CoreApplication::setAppName(const String &application)
{
   sg_coreAppData()->m_appNameSet = !application.isEmpty();
   String newAppName = application;
   if (newAppName.isEmpty() && CoreApplication::sm_self) {
      newAppName = CoreApplication::sm_self->getImplPtr()->getAppName();
   }
   if (sg_coreAppData()->m_appName == newAppName) {
      return;
   }
   sg_coreAppData()->m_appName = newAppName;
#ifndef PDK_NO_Object
   if (CoreApplication::sm_self) {
      CoreApplication::sm_self->emitAppNameChangedSignal();
   }
#endif
}

String CoreApplication::getAppName()
{
   return sg_coreAppData() ? sg_coreAppData()->m_appName : String();
}

void CoreApplication::setAppVersion(const String &version)
{
   sg_coreAppData()->m_appVersionSet = !version.isEmpty();
   String newVersion = version;
   if (newVersion.isEmpty() && CoreApplication::sm_self) {
      newVersion = CoreApplication::sm_self->getImplPtr()->getAppVersion();
   }
   if (sg_coreAppData()->m_appVersion == newVersion) {
      return;
   }
   sg_coreAppData()->m_appVersion = newVersion;
#ifndef PDK_NO_Object
   if (CoreApplication::sm_self) {
      CoreApplication::sm_self->emitAppVersionChangedSignal();
   }
#endif
}

String CoreApplication::getAppVersion()
{
   return sg_coreAppData() ? sg_coreAppData()->m_appVersion : String();
}

#if PDK_CONFIG(library)

PDK_GLOBAL_STATIC(std::recursive_mutex, sg_libraryPathMutex);

StringList CoreApplication::getLibraryPaths()
{
   std::lock_guard<std::recursive_mutex> locker(*sg_libraryPathMutex());
   if (sg_coreAppData()->m_manualLibPaths) {
      return *(sg_coreAppData()->m_manualLibPaths);
   }
   if (!sg_coreAppData()->m_appLibPaths) {
      StringList *appLibPaths = new StringList;
      sg_coreAppData()->m_appLibPaths.reset(appLibPaths);
      const ByteArray libPathEnv = pdk::pdk_getenv("PDK_PLUGIN_PATH");
      if (!libPathEnv.isEmpty()) {
         StringList paths = File::decodeName(libPathEnv).split(Dir::getListSeparator(), String::SplitBehavior::SkipEmptyParts);
         for (StringList::const_iterator iter = paths.cbegin(); iter != paths.cend(); ++iter) {
            String canonicalPath = Dir(*iter).getCanonicalPath();
            if (!canonicalPath.isEmpty()
                && !appLibPaths->contains(canonicalPath)) {
               appLibPaths->push_back(canonicalPath);
            }
         }
      }
      
#ifdef PDK_OS_DARWIN
      // Check the main bundle's PlugIns directory as this is a standard location for Apple OSes.
      // Note that the LibraryInfo::PluginsPath below will coincidentally be the same as this value
      // but with a different casing, so it can't be relied upon when the underlying filesystem
      // is case sensitive (and this is always the case on newer OSes like iOS).
      if (CFBundleRef bundleRef = CFBundleGetMainBundle()) {
         if (CFType<CFURLRef> urlRef = CFBundleCopyBuiltInPlugInsURL(bundleRef)) {
            if (CFType<CFURLRef> absoluteUrlRef = CFURLCopyAbsoluteURL(urlRef)) {
               if (CFString path = CFURLCopyFileSystemPath(absoluteUrlRef, kCFURLPOSIXPathStyle)) {
                  if (File::exists(path)) {
                     path = Dir(path).getCanonicalPath();
                     if (!appLibPaths->contains(path)) {
                        appLibPaths->push_back(path);
                     }
                  }
               }
            }
         }
      }
#endif // PDK_OS_DARWIN
      String installPathPlugins =  LibraryInfo::getPath(LibraryInfo::LibraryLocation::PluginsPath);
      if (File::exists(installPathPlugins)) {
         // Make sure we convert from backslashes to slashes.
         installPathPlugins = Dir(installPathPlugins).getCanonicalPath();
         if (!appLibPaths->contains(installPathPlugins))
            appLibPaths->push_back(installPathPlugins);
      }
      // If CoreApplication is not yet instantiated,
      // make sure we add the application path when we construct the CoreApplication
      if (sm_self) {
         sm_self->getImplPtr()->appendAppPathToLibPaths();
      }
   }
   return *(sg_coreAppData()->m_appLibPaths);
}

void CoreApplication::setLibraryPaths(const StringList &paths)
{
   // setLibraryPaths() is considered a "remove everything and then add some new ones" operation.
   // When the application is constructed it should still amend the paths. So we keep the originals
   // around, and even create them if they don't exist, yet.
   std::unique_lock<std::recursive_mutex> locker(*sg_libraryPathMutex());
   if (!sg_coreAppData()->m_appLibPaths) {
      getLibraryPaths();
   }
   if (sg_coreAppData()->m_manualLibPaths) {
      *(sg_coreAppData()->m_manualLibPaths) = paths;
   } else {
      sg_coreAppData()->m_manualLibPaths.reset(new StringList(paths));
   }
   locker.unlock();
   FactoryLoader::refreshAll();
}

void CoreApplication::addLibraryPath(const String &path)
{
   if (path.isEmpty()) {
      return;
   }
   String canonicalPath = Dir(path).getCanonicalPath();
   if (canonicalPath.isEmpty()) {
      return;
   }
   std::unique_lock<std::recursive_mutex> locker(*sg_libraryPathMutex());
   StringList *libpaths = sg_coreAppData()->m_manualLibPaths.getData();
   if (libpaths) {
      if (libpaths->contains(canonicalPath)) {
         return;
      }
   } else {
      // make sure that library paths are initialized
      getLibraryPaths();
      StringList *appLibPaths = sg_coreAppData()->m_appLibPaths.getData();
      if (appLibPaths->contains(canonicalPath)) {
         return;
      }
      sg_coreAppData()->m_manualLibPaths.reset(libpaths = new StringList(*appLibPaths));
   }
   libpaths->push_front(canonicalPath);
   locker.unlock();
   FactoryLoader::refreshAll();
}

void CoreApplication::removeLibraryPath(const String &path)
{
   if (path.isEmpty()) {
      return;
   }
   String canonicalPath = Dir(path).getCanonicalPath();
   if (canonicalPath.isEmpty()) {
      return;
   }
   std::unique_lock<std::recursive_mutex> locker(*sg_libraryPathMutex());
   StringList *libPaths = sg_coreAppData()->m_manualLibPaths.getData();
   if (libPaths) {
      int oldSize = libPaths->size();
      libPaths->remove(canonicalPath);
      if (libPaths - oldSize == 0) {
         return;
      }
   } else {
      // make sure that library paths is initialized
      getLibraryPaths();
      StringList *appLibPaths = sg_coreAppData()->m_appLibPaths.getData();
      if (!appLibPaths->contains(canonicalPath)) {
         return;
      }
      sg_coreAppData()->m_manualLibPaths.reset(libPaths = new StringList(*appLibPaths));
      libPaths->remove(canonicalPath);
   }
   locker.unlock();
   FactoryLoader::refreshAll();
}

#endif

void CoreApplication::installNativeEventFilter(AbstractNativeEventFilter *filterObj)
{
   if (CoreApplication::testAttribute(pdk::AppAttribute::AA_PluginApplication)) {
      warning_stream("Native event filters are not applied when the pdk::AppAttribute::AA_PluginApplication attribute is set");
      return;
   }
   AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance(CoreApplicationPrivate::sm_theMainThread);
   if (!filterObj || !eventDispatcher) {
      return;
   }
   eventDispatcher->installNativeEventFilter(filterObj);
}

void CoreApplication::removeNativeEventFilter(AbstractNativeEventFilter *filterObject)
{
   AbstractEventDispatcher *eventDispatcher = AbstractEventDispatcher::getInstance();
   if (!filterObject || !eventDispatcher) {
      return;
   }
   eventDispatcher->removeNativeEventFilter(filterObject);
}

AbstractEventDispatcher *CoreApplication::getEventDispatcher()
{
   if (CoreApplicationPrivate::sm_theMainThread) {
      return CoreApplicationPrivate::sm_theMainThread.load()->getEventDispatcher();
   }
   return nullptr;
}

void CoreApplication::setEventDispatcher(AbstractEventDispatcher *eventDispatcher)
{
   Thread *mainThread = CoreApplicationPrivate::sm_theMainThread;
   if (!mainThread) {
      mainThread = Thread::getCurrentThread(); // will also setup theMainThread
   }
   mainThread->setEventDispatcher(eventDispatcher);
}

} // kernel
} // pdk
