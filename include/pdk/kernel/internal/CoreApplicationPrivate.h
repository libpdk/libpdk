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

#ifndef PDK_KERNEL_INTERNAL_CORE_APPLICATION_PRIVATE_H
#define PDK_KERNEL_INTERNAL_CORE_APPLICATION_PRIVATE_H

#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/base/os/thread/ReadWriteLock.h"

namespace pdk {

// forward declare class with namespace
namespace os {
namespace thread {
namespace internal {
class ThreadData;
} // internal
} // thread
} // os

namespace utils {
class Translator;
} // utils

namespace kernel {

using pdk::utils::Translator;
using TranslatorList = std::list<Translator *>;

class AbstractEventDispatcher;

namespace internal {

using pdk::os::thread::AtomicInt;
using pdk::os::thread::BasicAtomicPointer;
using pdk::os::thread::internal::ThreadData;
using pdk::os::thread::ReadWriteLock;

class PDK_CORE_EXPORT CoreApplicationPrivate : public ObjectPrivate
{
   PDK_DECLARE_PUBLIC(CoreApplication);
public:
   enum class Type {
      Tty,
      Gui
   };
   static void initLocale();
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
   static void sendPostedEvents(Object *receiver, Event::Type eventType, ThreadData *data);
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
   String getAppName() const;
   String getAppVersion() const;
#ifdef PDK_OS_DARWIN
   static String infoDictionaryStringProperty(const String &propertyName);
#endif
   bool sendThroughApplicationEventFilters(Object *, Event *);
#ifdef PDK_OS_WIN
   static void removePostedTimerEvent(Object *object, int timerId);
#endif
   AtomicInt m_quitLockRef;
   void ref();
   void deref();
   void maybeQuit();
   void cleanupThreadData();
   void appendAppPathToLibPaths(void);
   
#ifndef PDK_NO_TRANSLATION
   TranslatorList m_translators;
   ReadWriteLock m_translateMutex;
   static bool isTranslatorInstalled(Translator *translator);
#endif
   
   static void setAppFilePath(const String &path);
   static inline void clearAppFilePath()
   {
      delete sm_cachedAppFilePath;
      sm_cachedAppFilePath = nullptr;
   }
   void execCleanup();
   void processCommandLineArguments();
   
   static AbstractEventDispatcher *sm_eventDispatcher;
   static bool sm_isAppRunning;
   static bool sm_isAppClosing;
   static bool sm_setuidAllowed;
   static uint sm_attribs;
   static BasicAtomicPointer<Thread> sm_theMainThread;
   static String *sm_cachedAppFilePath;
   static uint sm_appCompileVersion;
   
   CoreApplicationPrivate::Type m_appType;
   String m_cachedAppDirPath;
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

#endif // PDK_KERNEL_INTERNAL_CORE_APPLICATION_PRIVATE_H
