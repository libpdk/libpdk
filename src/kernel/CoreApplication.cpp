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

namespace internal {

bool CoreApplicationPrivate::sm_setuidAllowed = false;

std::string CoreApplicationPrivate::getAppName() const
{
}

std::string *CoreApplicationPrivate::m_cachedApplicationFilePath = nullptr;

bool CoreApplicationPrivate::checkInstance(const char *method)
{
   
}

} // internal

namespace {

void call_pre_routines()
{
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

void CoreApplicationPrivate::processCommandLineArguments()
{}

extern "C" void PDK_CORE_EXPORT startup_hook()
{
}

using StartupFuncList = std::list<StartUpFunction>;
PDK_GLOBAL_STATIC(StartupFuncList, sg_preRList);
using ShutdownFuncList = std::list<CleanUpFunction>;
PDK_GLOBAL_STATIC(ShutdownFuncList, sg_postRList);

static std::mutex sg_preRoutinesMutex;

void add_pre_routine(StartUpFunction func)
{
   
}

void add_post_routine(CleanUpFunction func)
{
   
}

void remove_post_routine(CleanUpFunction func)
{
   
}

void PDK_CORE_EXPORT call_post_routines()
{
}

static bool pdk_locale_initialized = false;

PDK_CORE_EXPORT uint global_posted_events_count()
{
}

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

CoreApplication *CoreApplication::sm_self = nullptr;

struct CoreApplicationData {
   CoreApplicationData() noexcept
   {
      m_applicationNameSet = false;
   }
   ~CoreApplicationData()
   {
      // cleanup the QAdoptedThread created for the main() thread
      if (CoreApplicationPrivate::sm_theMainThread) {
         ThreadData *data = ThreadData::get(CoreApplicationPrivate::sm_theMainThread);
         data->deref(); // deletes the data and the adopted thread
      }
   }
   
   std::string m_orgName;
   std::string m_orgDomain;
   std::string m_application; // application name, initially from argv[0], can then be modified.
   std::string m_applicationVersion;
   bool m_applicationNameSet; // true if setApplicationName was called
   
#ifndef PDK_NO_LIBRARY
   ScopedPointer<StringList> m_appLibpaths;
   ScopedPointer<StringList> m_manualLibpaths;
#endif
};

PDK_GLOBAL_STATIC(CoreApplicationData, sg_coreappdata);

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

void CoreApplication::postEvent(Object *receiver, Event *event, int priority)
{
}

bool CoreApplication::compressEvent(Event *event, Object *receiver, PostEventList *postedEvents)
{
}

void CoreApplication::sendPostedEvents(Object *receiver, int eventType)
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

std::string CoreApplication::getApplicationDirPath()
{
}

std::string CoreApplication::getApplicationFilePath()
{
}

pdk::pint64 CoreApplication::getApplicationPid()
{
}

StringList CoreApplication::getArguments()
{
}

void CoreApplication::setOrganizationName(const std::string &orgName)
{
}

std::string CoreApplication::getOrganizationName()
{
}

void CoreApplication::setOrganizationDomain(const std::string &orgDomain)
{
}

std::string CoreApplication::getOrganizationDomain()
{
}

void CoreApplication::setApplicationName(const std::string &application)
{
}

std::string CoreApplication::getApplicationName()
{
}

void CoreApplication::setApplicationVersion(const std::string &version)
{
}

std::string CoreApplication::getApplicationVersion()
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
