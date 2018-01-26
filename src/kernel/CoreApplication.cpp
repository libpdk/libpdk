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
} // internal

CoreApplication *CoreApplication::sm_self = nullptr;

struct CoreApplicationData {
   CoreApplicationData() noexcept
   {
      m_applicationNameSet = false;
   }
   ~CoreApplicationData()
   {
#ifndef PDK_NO_QOBJECT
      // cleanup the QAdoptedThread created for the main() thread
      if (CoreApplicationPrivate::sm_theMainThread) {
         ThreadData *data = ThreadData::get(CoreApplicationPrivate::sm_theMainThread);
         data->deref(); // deletes the data and the adopted thread
      }
#endif
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

} // kernel
} // pdk
