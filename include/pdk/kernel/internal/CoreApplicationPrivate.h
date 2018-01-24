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

#ifndef PDK_KERNEL_CORE_APPLICATION_PRIVATE_H
#define PDK_KERNEL_CORE_APPLICATION_PRIVATE_H

#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/base/os/thread/Atomic.h"

namespace pdk {
namespace kernel {

class AbstractEventDispatcher;

namespace internal {

using pdk::os::thread::AtomicInt;
using pdk::os::thread::BasicAtomicPointer;

class PDK_CORE_EXPORT CoreApplicationPrivate : public ObjectPrivate
{
   PDK_DECLARE_PUBLIC(CoreApplication);
public:
   enum class Type {
      Tty,
      Gui
   };
   static bool checkInstance(const char *method);
   static bool sendThroughObjectEventFilters(Object *, Event *);
   static bool notifyHelper(Object *, Event *);
   static inline void setEventSpontaneous(Event *event, bool spontaneous) 
   { 
      event->m_spont = spontaneous;
   }
   static void removePostedEvent(Event *);
   static Thread *getMainThread();
   static bool threadRequiresCoreApplication();
   static void sendPostedEvents(Object *receiver, int eventType, ThreadData *data);
   static void checkReceiverThread(Object *receiver);
   
   virtual void createEventDispatcher();
   virtual void eventDispatcherReady();
   virtual bool shouldQuit()
   {
      return true;
   }
   
   CoreApplicationPrivate(int &aargc,  char **aargv, uint flags);
   ~CoreApplicationPrivate();
   void init();
   std::string getAppName() const;
   bool sendThroughApplicationEventFilters(Object *, Event *);
#ifdef PDK_OS_WIN
   static void removePostedTimerEvent(Object *object, int timerId);
#endif
   AtomicInt m_quitLockRef;
   void ref();
   void deref();
   void maybeQuit();
   void cleanupThreadData();
   void appendApplicationPathToLibraryPaths(void);
   
   static std::string *m_cachedApplicationFilePath;
   static void setApplicationFilePath(const std::string &path);
   static inline void clearApplicationFilePath()
   {
      delete m_cachedApplicationFilePath;
      m_cachedApplicationFilePath = nullptr;
   }
   void execCleanup();
   void processCommandLineArguments();

   static AbstractEventDispatcher *sm_eventDispatcher;
   static bool sm_isAppRunning;
   static bool sm_isAppClosing;
   static bool sm_setuidAllowed;
   static int sm_appCompileVersion;
   static BasicAtomicPointer<Thread> sm_theMainThread;
   
   CoreApplicationPrivate::Type m_applicationType;
   std::string m_cachedApplicationDirPath;
   bool m_inExec;
   bool m_aboutToQuitEmitted;
   bool m_threadDataClean;
   int &m_argc;
   char **m_argv;
#if defined(PDK_OS_WIN)
   int m_origArgc;
   char **m_origArgv; // store unmodified arguments for CoreApplication::arguments()
#endif
   CoreApplication *m_apiPtr;
};

} // internal
} // kernel
} // pdk

#endif // PDK_KERNEL_CORE_APPLICATION_PRIVATE_H
