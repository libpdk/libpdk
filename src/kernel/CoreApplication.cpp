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

#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/CoreEvent.h"
#include "pdk/kernel/EventLoop.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
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

namespace pdk {
namespace kernel {

using pdk::utils::ScopedPointer;
using pdk::os::thread::Thread;
using pdk::os::thread::internal::ThreadData;

extern "C" void PDK_CORE_EXPORT startup_hook()
{
}

using StartupFuncList = std::list<StartUpFunction>;
PDK_GLOBAL_STATIC(StartupFuncList, sg_preRList);
using ShutdownFuncList = std::list<CleanUpFunction>;
PDK_GLOBAL_STATIC(ShutdownFuncList, sg_postRList);

static std::mutex sg_preRoutinesMutex;

namespace {

void call_pre_routines()
{
   StartupFuncList *list = sg_preRList();
   if (!list) {
      return;
   }
   std::lock_guard<std::mutex> locker(sg_preRoutinesMutex);
   // Unlike pdk::kernel::call_post_routines, we don't empty the list, because
   // PDK_COREAPP_STARTUP_FUNCTION is a macro, so the user expects
   // the function to be executed every time CoreApplication is created.
   auto iter = list->cbegin();
   auto end = list->cend();
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

bool do_notify(Object *, Event *)
{
   
}
}

namespace internal {

bool CoreApplicationPrivate::sm_setuidAllowed = false;

std::string CoreApplicationPrivate::getAppName() const
{
   std::string appName;
   if (m_argv[0]) {
      char *p = std::strrchr(m_argv[0], '/');
      appName = p ? (p + 1) : m_argv[0];
   }
   return appName;
}

std::string *CoreApplicationPrivate::m_cachedApplicationFilePath = nullptr;

bool CoreApplicationPrivate::checkInstance(const char *method)
{
   bool flag = (CoreApplication::sm_self != nullptr);
   if (!flag) {
      // qWarning("CoreApplication::%s: Please instantiate the CoreApplication object first", method);
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
   list->push_front(func);
}

void remove_post_routine(CleanUpFunction func)
{
   ShutdownFuncList *list = sg_postRList();
   if (!list) {
      return;
   }
   list->remove(func);
}

void call_post_routines()
{
   ShutdownFuncList *list = nullptr;
   try {
      list = sg_postRList();
   } catch (const std::bad_alloc &) {
      // ignore - if we can't allocate a post routine list,
      // there's a high probability that there's no post
      // routine to be executed :)
   }
   if (!list) {
      return;
   }
   while (!list->empty()) {
      (list->front())();
      list->pop_front();
   }
}

static bool pdk_locale_initialized = false;

#ifdef PDK_OS_UNIX
pdk::HANDLE pdk_application_thread_id = 0;
#endif

namespace internal {
bool CoreApplicationPrivate::sm_isAppRunning = false;
bool CoreApplicationPrivate::sm_isAppClosing = false;
AbstractEventDispatcher *CoreApplicationPrivate::sm_eventDispatcher = nullptr;

CoreApplicationPrivate::CoreApplicationPrivate(int &aargc, char **aargv, uint flags)
   : ObjectPrivate(),
     m_argc(aargc),
     m_argv(aargv),
     #if defined(PDK_OS_WIN)
     m_origArgc(0),
     m_origArgv(Q_NULLPTR),
     #endif
     m_applicationType(CoreApplicationPrivate::Type::Tty),
     m_inExec(false),
     m_aboutToQuitEmitted(false),
     m_threadDataClean(false)
{
}

CoreApplicationPrivate::~CoreApplicationPrivate()
{
}

void CoreApplicationPrivate::cleanupThreadData()
{
}

void CoreApplicationPrivate::createEventDispatcher()
{
}

void CoreApplicationPrivate::eventDispatcherReady()
{
}

BasicAtomicPointer<Thread> CoreApplicationPrivate::sm_theMainThread = PDK_BASIC_ATOMIC_INITIALIZER(0);

Thread *CoreApplicationPrivate::getMainThread()
{
}

bool CoreApplicationPrivate::threadRequiresCoreApplication()
{
}

void CoreApplicationPrivate::checkReceiverThread(Object *receiver)
{
}

void CoreApplicationPrivate::appendApplicationPathToLibraryPaths()
{
}

void CoreApplicationPrivate::init()
{
}

bool CoreApplicationPrivate::sendThroughApplicationEventFilters(Object *receiver, Event *event)
{
}

bool CoreApplicationPrivate::sendThroughObjectEventFilters(Object *receiver, Event *event)
{
   
}

bool CoreApplicationPrivate::notifyHelper(Object *receiver, Event *event)
{
}

// Cleanup after eventLoop is done executing in CoreApplication::exec().
// This is for use cases in which CoreApplication is instantiated by a
// library and not by an application executable, for example, Active X
// servers.

void CoreApplicationPrivate::execCleanup()
{
}


} // internal

uint global_posted_events_count()
{
   ThreadData *currentThreadData = ThreadData::current();
   return currentThreadData->m_postEventList.size() - currentThreadData->m_postEventList.m_startOffset;
}

CoreApplication *CoreApplication::sm_self = nullptr;

struct CoreApplicationData {
   CoreApplicationData() noexcept
   {
      m_appNameSet = false;
   }
   ~CoreApplicationData()
   {
      // cleanup the QAdoptedThread created for the main() thread
      if (CoreApplicationPrivate::sm_theMainThread) {
         internal::ThreadData *data = internal::ThreadData::get(CoreApplicationPrivate::sm_theMainThread);
         data->deref(); // deletes the data and the adopted thread
      }
   }
   
   std::string m_orgName;
   std::string m_orgDomain;
   std::string m_appName; // application name, initially from argv[0], can then be modified.
   std::string m_appVersion;
   bool m_appNameSet; // true if setApplicationName was called
   
#ifndef PDK_NO_LIBRARY
   ScopedPointer<StringList> m_appLibpaths;
   ScopedPointer<StringList> m_manualLibpaths;
#endif
};

PDK_GLOBAL_STATIC(CoreApplicationData, sg_coreAppData);

static bool sg_quitLockRefEnabled = true;

std::string retrieve_app_name()
{
   
}

CoreApplication::CoreApplication(CoreApplicationPrivate &p)
   : Object(p, 0)
{
}

CoreApplication::CoreApplication(int &argc, char **argv, int internal)
   : Object(*new CoreApplicationPrivate(argc, argv, internal))
{
   
}

void CoreApplication::flush()
{
}

CoreApplication::~CoreApplication()
{
}

void CoreApplication::setSetuidAllowed(bool allow)
{
}

bool CoreApplication::isSetuidAllowed()
{
}

bool CoreApplication::isQuitLockEnabled()
{
}

void CoreApplication::setQuitLockEnabled(bool enabled)
{
}

bool CoreApplication::notifyInternal(Object *receiver, Event *event)
{
}

bool CoreApplication::notify(Object *receiver, Event *event)
{
}

bool CoreApplication::startingUp()
{
}

bool CoreApplication::closingDown()
{
}

void CoreApplication::processEvents(EventLoop::ProcessEventsFlags flags)
{
}

void CoreApplication::processEvents(EventLoop::ProcessEventsFlags flags, int maxtime)
{
}

int CoreApplication::exec()
{
   
}

void CoreApplication::exit(int returnCode)
{
}

void CoreApplication::postEvent(Object *receiver, Event *event, pdk::EventPriority priority)
{
}

bool CoreApplication::compressEvent(Event *event, Object *receiver, PostEventList *postedEvents)
{
}

void CoreApplication::sendPostedEvents(Object *receiver, Event::Type eventType)
{
}

namespace internal {

void CoreApplicationPrivate::sendPostedEvents(Object *receiver, int eventType,
                                              ThreadData *data)
{
}

void CoreApplicationPrivate::removePostedEvent(Event * event)
{
}

void CoreApplicationPrivate::ref()
{
}

void CoreApplicationPrivate::deref()
{
}

void CoreApplicationPrivate::maybeQuit()
{
}

// Makes it possible to point QCoreApplication to a custom location to ensure
// the directory is added to the patch, and qt.conf and deployed plugins are
// found from there. This is for use cases in which QGuiApplication is
// instantiated by a library and not by an application executable, for example,
// Active X servers.

void CoreApplicationPrivate::setApplicationFilePath(const std::string &path)
{
}


}

void CoreApplication::removePostedEvents(Object *receiver, int eventType)
{
}

bool CoreApplication::event(Event *e)
{
}

void CoreApplication::quit()
{
}

std::string CoreApplication::getAppDirPath()
{
}

std::string CoreApplication::getAppFilePath()
{
}

pdk::pint64 CoreApplication::getAppPid()
{
}

StringList CoreApplication::getArguments()
{
}

void CoreApplication::setOrgName(const std::string &orgName)
{
}

std::string CoreApplication::getOrgName()
{
}

void CoreApplication::setOrgDomain(const std::string &orgDomain)
{
}

std::string CoreApplication::getOrgDomain()
{
}

void CoreApplication::setAppName(const std::string &application)
{
}

std::string CoreApplication::getAppName()
{
}

void CoreApplication::setAppVersion(const std::string &version)
{
}

std::string CoreApplication::getAppVersion()
{

}

PDK_GLOBAL_STATIC(std::recursive_mutex, sg_libraryPathMutex);

StringList CoreApplication::getLibraryPaths()
{
}

void CoreApplication::setLibraryPaths(const StringList &paths)
{
}

void CoreApplication::addLibraryPath(const std::string &path)
{
}

void CoreApplication::removeLibraryPath(const std::string &path)
{
}

void CoreApplication::installNativeEventFilter(AbstractNativeEventFilter *filterObj)
{
}

void CoreApplication::removeNativeEventFilter(AbstractNativeEventFilter *filterObject)
{
}

AbstractEventDispatcher *CoreApplication::eventDispatcher()
{
}

void CoreApplication::setEventDispatcher(AbstractEventDispatcher *eventDispatcher)
{
}

} // kernel
} // pdk
