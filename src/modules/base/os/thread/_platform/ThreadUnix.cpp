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
// Created by softboy on 2017/01/25.

#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/EventDispatcherUnix.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/base/io/Debug.h"
#include "pdk/global/PlatformDefs.h"
#include "pdk/base/os/thread/ThreadStorage.h"
#include <iostream>
#if defined(PDK_OS_DARWIN)
#  include "pdk/kernel/EventDispatcherCf.h"
#endif

#include <thread>

#include <sched.h>
#include <errno.h>

#ifdef PDK_OS_BSD4
#include <sys/sysctl.h>
#endif
#ifdef PDK_OS_VXWORKS
#  if (_WRS_VXWORKS_MAJOR > 6) || ((_WRS_VXWORKS_MAJOR == 6) && (_WRS_VXWORKS_MINOR >= 6))
#    include <vxCpuLib.h>
#    include <cpuset.h>
#    define PDK_VXWORKS_HAS_CPUSET
#  endif
#endif // PDK_OS_VXWORKS

#ifdef __GLIBCXX__
#include <cxxabi.h>
#endif

#ifdef PDK_OS_HPUX
#include <sys/pstat.h>
#endif // PDK_OS_HPUX

#if defined(PDK_OS_MAC)
# ifdef warning_stream
#   define old_warning_stream warning_stream
#   undef warning_stream
# endif

# ifdef old_warning_stream
#   undef warning_stream
#   define warning_stream PDK_NO_DEBUG_MACRO
#   undef old_warning_stream
# endif
#endif

#if defined(PDK_OS_LINUX) && !defined(PDK_LINUXBASE)
#include <sys/prctl.h>
#endif

#if defined(PDK_OS_LINUX) && !defined(SCHED_IDLE)
// from linux/sched.h
# define SCHED_IDLE    5
#endif

#if defined(PDK_OS_DARWIN) || !defined(PDK_OS_OPENBSD) && defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && (_POSIX_THREAD_PRIORITY_SCHEDULING-0 >= 0)
#define PDK_HAS_THREAD_PRIORITY_SCHEDULING
#endif

namespace pdk {
namespace os {
namespace thread {

using internal::ThreadData;
using internal::ThreadPrivate;
using pdk::kernel::internal::ObjectPrivate;
using pdk::kernel::internal::CoreApplicationPrivate;
using pdk::kernel::EventDispatcherUNIX;
#ifdef PDK_OS_DARWIN
using pdk::kernel::EventDispatcherCoreFoundation;
#endif

PDK_STATIC_ASSERT(sizeof(pthread_t) <= sizeof(pdk::HANDLE));
constexpr int THREAD_PRIORITY_RESET_FLAG = 0x80000000;
#if defined(PDK_OS_LINUX) && defined(__GLIBC__) && (defined(PDK_CC_GNU) || defined(PDK_CC_INTEL)) && !defined(PDK_LINUXBASE)
/* LSB doesn't have __thread, https://lsbbugs.linuxfoundation.org/show_bug.cgi?id=993 */
#define HAVE_TLS
#endif

#define PDK_HAVE_TLS
static thread_local ThreadData *sg_currentThreadData = nullptr;
static pthread_once_t sg_currentThreadDataOnce = PTHREAD_ONCE_INIT;
static pthread_key_t sg_currentThreadDataKey;

namespace {

void destroy_current_thread_data(void *p)
{
#if defined(PDK_OS_VXWORKS)
   // Calling setspecific(..., 0) sets the value to 0 for ALL threads.
   // The 'set to 1' workaround adds a bit of an overhead though,
   // since this function is called twice now.
   if (p == (void *)1) {
      return;
   }
#endif
   // POSIX says the value in our key is set to zero before calling
   // this destructor function, so we need to set it back to the
   // right value...
   pthread_setspecific(sg_currentThreadDataKey, p);
   ThreadData *data = static_cast<ThreadData *>(p);
   if (data->m_isAdopted) {
      Thread *thread = data->m_thread;
      PDK_ASSERT(thread);
      ThreadPrivate *threadPrivate = static_cast<ThreadPrivate *>(ObjectPrivate::get(thread));
      PDK_ASSERT(!threadPrivate->m_finished);
      threadPrivate->finish(thread);
   }
   data->deref();
   // ... but we must reset it to zero before returning so we aren't
   // called again (POSIX allows implementations to call destructor
   // functions repeatedly until all values are zero)
   pthread_setspecific(sg_currentThreadDataKey,
#if defined(PDK_OS_VXWORKS)
   (void *)1);
#else
   nullptr);
#endif
}

void create_current_thread_data_key()
{
   pthread_key_create(&sg_currentThreadDataKey, destroy_current_thread_data);
}

void destroy_current_thread_data_key()
{
   pthread_once(&sg_currentThreadDataOnce, create_current_thread_data_key);
   pthread_key_delete(sg_currentThreadDataKey);
   // Reset sg_currentThreadDataOnce in case we end up recreating
   // the thread-data in the rare case of Object construction
   // after destroying the ThreadData.
   pthread_once_t pthreadOnceInit = PTHREAD_ONCE_INIT;
   sg_currentThreadDataOnce = pthreadOnceInit;
}

PDK_DESTRUCTOR_FUNCTION(destroy_current_thread_data_key)

ThreadData *get_thread_data()
{
#ifdef PDK_HAVE_TLS
   return sg_currentThreadData;
#else
   pthread_once(&sg_currentThreadDataOnce, sg_currentThreadDataKey);
   return reinterpret_cast<ThreadData *>(pthread_getspecific(sg_currentThreadDataKey));
#endif
}

void set_thread_data(ThreadData *data)
{
#ifdef PDK_HAVE_TLS
   sg_currentThreadData = data;
#else
   pthread_once(&sg_currentThreadDataOnce, sg_currentThreadDataKey);
   pthread_setspecific(sg_currentThreadDataKey, data);
#endif
}

static void clear_thread_data()
{
#ifdef PDK_HAVE_TLS
   sg_currentThreadData = nullptr;
#else
   pthread_once(&sg_currentThreadDataOnce, sg_currentThreadDataKey);
   pthread_setspecific(sg_currentThreadDataKey, nullptr);
#endif
}

template <typename T>
static typename std::enable_if<std::is_integral<T>::value, pdk::HANDLE>::type
to_pdk_handle_type(T id)
{
   return reinterpret_cast<pdk::HANDLE>(static_cast<intptr_t>(id));
}

template <typename T>
static typename std::enable_if<std::is_integral<T>::value, T>::type
from_pdk_handle_type(pdk::HANDLE id)
{
   return static_cast<T>(reinterpret_cast<intptr_t>(id));
}

template <typename T>
static typename std::enable_if<std::is_pointer<T>::value, pdk::HANDLE>::type
to_pdk_handle_type(T id)
{
   return id;
}

template <typename T>
static typename std::enable_if<std::is_pointer<T>::value, T>::type
from_pdk_handle_type(pdk::HANDLE id)
{
   return static_cast<T>(id);
}

#if (defined(PDK_OS_LINUX) || defined(PDK_OS_MAC))
void set_current_thread_name(pthread_t threadId, const char *name)
{
#  if defined(PDK_OS_LINUX) && !defined(PDK_LINUXBASE)
   PDK_UNUSED(threadId);
   prctl(PR_SET_NAME, (unsigned long)name, 0, 0, 0);
#  elif defined(PDK_OS_MAC)
   PDK_UNUSED(threadId);
   pthread_setname_np(name);
#  endif
}
#endif

timespec make_timespec(time_t secs, long nsecs)
{
   struct timespec ts;
   ts.tv_sec = secs;
   ts.tv_nsec = nsecs;
   return ts;
}

#ifdef PDK_HAS_THREAD_PRIORITY_SCHEDULING
// Does some magic and calculate the Unix scheduler priorities
// sched_policy is IN/OUT: it must be set to a valid policy before calling this function
// sched_priority is OUT only
static bool calculate_unix_priority(int priority, int *schedPolicy, int *schedPriority)
{
#ifdef SCHED_IDLE
   if (priority == static_cast<int>(Thread::IdlePriority)) {
      *schedPolicy = SCHED_IDLE;
      *schedPriority = 0;
      return true;
   }
   const int lowestPriority = Thread::LowestPriority;
#else
   const int lowestPriority = Thread::IdlePriority;
#endif
   const int highestPriority = Thread::TimeCriticalPriority;
   int priorityMin;
   int priorityMax;
#if defined(PDK_OS_VXWORKS) && defined(VXWORKS_DKM)
   // for other scheduling policies than SCHED_RR or SCHED_FIFO
   priorityMin = SCHED_FIFO_LOW_PRI;
   priorityMax = SCHED_FIFO_HIGH_PRI;
   
   if ((*schedPolicy == SCHED_RR) || (*schedPolicy == SCHED_FIFO))
#endif
   {
      priorityMin = sched_get_priority_min(*schedPolicy);
      priorityMax = sched_get_priority_max(*schedPolicy);
   }
   if (priorityMin == -1 || priorityMax == -1) {
      return false;
   }
   int calculatedPriority;
   calculatedPriority = ((priority - lowestPriority) * (priorityMax - priorityMin) / highestPriority) + priorityMin;
   calculatedPriority = std::max(priorityMin, std::min(priorityMax, calculatedPriority));
   *schedPriority = calculatedPriority;
   return true;
}
#endif

}

namespace internal {

void ThreadData::clearCurrentThreadData()
{
   clear_thread_data();
}

ThreadData *ThreadData::current(bool createIfNecessary)
{
   ThreadData *data = get_thread_data();
   if (!data && createIfNecessary) {
      data = new ThreadData;
      try {
         set_thread_data(data);
         data->m_thread = new AdoptedThread(data);
      } catch (...) {
         clear_thread_data();
         data->deref();
         data = nullptr;
         throw;
      }
      data->deref();
      data->m_isAdopted = true;
      data->m_threadId.store(to_pdk_handle_type(pthread_self()));
      if (!CoreApplicationPrivate::sm_theMainThread) {
         CoreApplicationPrivate::sm_theMainThread = data->m_thread.load();
      }
   }
   return data;
}

void AdoptedThread::init()
{}

extern "C" {
using PdkThreadCallback = void* (*)(void *);
}

void ThreadPrivate::createEventDispatcher(ThreadData *data)
{
#if defined(PDK_OS_DARWIN)
   bool ok = false;
   int value = pdk::env_var_intval("PDK_EVENT_DISPATCHER_CORE_FOUNDATION", &ok);
   if (ok && value > 0) {
      data->m_eventDispatcher.storeRelease(new EventDispatcherCoreFoundation);
   } else {
      data->m_eventDispatcher.storeRelease(new EventDispatcherUNIX);
   }
#else
   data->m_eventDispatcher.storeRelease(new EventDispatcherUNIX);
#endif
}

void *ThreadPrivate::start(void *arg)
{
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   pthread_cleanup_push(ThreadPrivate::finish, arg);
   try {
      Thread *thread = reinterpret_cast<Thread *>(arg);
      ThreadData *data = ThreadData::get(thread);
      {
         std::scoped_lock locker(thread->getImplPtr()->m_mutex);
         // do we need to reset the thread priority?
         if (static_cast<int>(thread->getImplPtr()->m_priority) & THREAD_PRIORITY_RESET_FLAG) {
            thread->getImplPtr()->setPriority(Thread::Priority(thread->getImplPtr()->m_priority & ~THREAD_PRIORITY_RESET_FLAG));
         }
         data->m_threadId.store(to_pdk_handle_type(pthread_self()));
         set_thread_data(data);
         data->ref();
         data->m_quitNow = thread->getImplPtr()->m_exited;
      }
      if (data->m_eventDispatcher.load()) {
         data->m_eventDispatcher.load()->startingUp();
      } else {
         createEventDispatcher(data);
      }
#if (defined(PDK_OS_LINUX) || defined(PDK_OS_MAC))
      {
         // sets the name of the current thread.
         String objectName = thread->getObjectName();
         pthread_t threadId = from_pdk_handle_type<pthread_t>(data->m_threadId.load());
         if (PDK_LIKELY(objectName.isEmpty())) {
            set_current_thread_name(threadId, typeid(thread).name());
         } else {
            set_current_thread_name(threadId, objectName.toLocal8Bit());
         }
      }
#endif
      thread->emitStartedSignal();
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
      pthread_testcancel();
      thread->run();
   }
#ifdef __GLIBCXX__
   // POSIX thread cancellation under glibc is implemented by throwing an exception
   // of this type. Do what libstdc++ is doing and handle it specially in order not to
   // abort the application if user's code calls a cancellation function.
   catch (const abi::__forced_unwind &) {
      throw;
   }
#endif // __GLIBCXX__
   catch (...) {
      std::terminate();
   }
   pthread_cleanup_pop(1);
   return 0;
}

void ThreadPrivate::finish(void *arg)
{
   try {
      Thread *thread = reinterpret_cast<Thread *>(arg);
      ThreadPrivate *implPtr = thread->getImplPtr();
      std::unique_lock locker(implPtr->m_mutex);
      implPtr->m_isInFinish = true;
      implPtr->m_priority = Thread::InheritPriority;
      void *data = &implPtr->m_data->m_tls;
      locker.unlock();
      thread->emitFinishedSignal();
      CoreApplication::sendPostedEvents(0, Event::Type::DeferredDelete);
      ThreadStorageData::finish((void **)data);
      locker.lock();
      AbstractEventDispatcher *eventDispatcher = implPtr->m_data->m_eventDispatcher.load();
      if (eventDispatcher) {
         implPtr->m_data->m_eventDispatcher = nullptr;
         locker.unlock();
         eventDispatcher->closingDown();
         delete eventDispatcher;
         locker.lock();
      }
      implPtr->m_running = false;
      implPtr->m_finished = true;
      implPtr->m_interruptionRequested = false;
      implPtr->m_isInFinish = false;
      implPtr->m_threadDone.notify_all();
   }
#ifdef __GLIBCXX__
   // POSIX thread cancellation under glibc is implemented by throwing an exception
   // of this type. Do what libstdc++ is doing and handle it specially in order not to
   // abort the application if user's code calls a cancellation function.
   catch (const abi::__forced_unwind &) {
      throw;
   }
#endif // __GLIBCXX__
   catch (...) {
      std::terminate();
   }
}

// Caller must lock the mutex
void ThreadPrivate::setPriority(Thread::Priority priority)
{
   m_priority = priority;
#ifdef PDK_HAS_THREAD_PRIORITY_SCHEDULING
   int schedPolicy;
   sched_param param;
   if (pthread_getschedparam(from_pdk_handle_type<pthread_t>(m_data->m_threadId), &schedPolicy, &param) != 0) {
      // failed to get the scheduling policy, don't bother setting
      // the priority
      // warning_stream("Thread::setPriority: Cannot get scheduler parameters");
      return;
   }
#endif
   int calculatedPriority;
   if (!calculate_unix_priority(priority, &schedPolicy, &calculatedPriority)) {
      // failed to get the scheduling parameters, don't
      // bother setting the priority
      // warning_stream("Thread::setPriority: Cannot determine scheduler priority range");
      return;
   }
   param.sched_priority = calculatedPriority;
   int status = pthread_setschedparam(from_pdk_handle_type<pthread_t>(m_data->m_threadId), schedPolicy, &param);
# ifdef SCHED_IDLE
   // were we trying to set to idle priority and failed?
   if (status == -1 && schedPolicy == SCHED_IDLE && errno == EINVAL) {
      // reset to lowest priority possible
      pthread_getschedparam(from_pdk_handle_type<pthread_t>(m_data->m_threadId), &schedPolicy, &param);
      param.sched_priority = sched_get_priority_min(schedPolicy);
      pthread_setschedparam(from_pdk_handle_type<pthread_t>(m_data->m_threadId), schedPolicy, &param);
   }
# else
   PDK_UNUSED(status);
# endif // SCHED_IDLE
}

} // internal

pdk::HANDLE Thread::getCurrentThreadId() noexcept
{
   return to_pdk_handle_type(pthread_self());
}

#if defined(PDK_LINUXBASE) && !defined(_SC_NPROCESSORS_ONLN)
// LSB doesn't define _SC_NPROCESSORS_ONLN.
#  define _SC_NPROCESSORS_ONLN 84
#endif

int Thread::getIdealThreadCount() noexcept
{
   return std::thread::hardware_concurrency();
}

void Thread::yieldCurrentThread()
{
   sched_yield();
}

void Thread::sleep(unsigned long secs)
{
   pdk::kernel::nanosleep(make_timespec(secs, 0));
}

void Thread::msleep(unsigned long msecs)
{
   pdk::kernel::nanosleep(make_timespec(msecs / 1000, msecs % 1000 * 1000 * 1000));
}

void Thread::usleep(unsigned long usecs)
{
   pdk::kernel::nanosleep(make_timespec(usecs / 1000 / 1000, usecs % (1000 * 1000) * 1000));
}

//PDK_DEFINE_SIGNAL_BINDER(Thread, Started)
//PDK_DEFINE_SIGNAL_BINDER(Thread, Finished)

void Thread::start(Priority priority)
{
   PDK_D(Thread);
   std::unique_lock locker(implPtr->m_mutex);
   if (implPtr->m_isInFinish) {
      implPtr->m_threadDone.wait(locker);
   }
   if (implPtr->m_running) {
      return;
   }
   implPtr->m_running = true;
   implPtr->m_finished = false;
   implPtr->m_returnCode = 0;
   implPtr->m_exited = false;
   implPtr->m_interruptionRequested = false;
   
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   implPtr->m_priority = priority;
   
#if defined(PDK_HAS_THREAD_PRIORITY_SCHEDULING)
   switch (priority) {
   case InheritPriority:
   {
      pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
      break;
   }
   default:
   {
      int schedPolicy;
      if (pthread_attr_getschedpolicy(&attr, &schedPolicy) != 0) {
         // failed to get the scheduling policy, don't bother
         // setting the priority
         warning_stream("Thread::start: Cannot determine default scheduler policy");
         break;
      }
      int calculatedPriority;
      if (!calculate_unix_priority(priority, &schedPolicy, &calculatedPriority)) {
         // failed to get the scheduling parameters, don't
         // bother setting the priority
         warning_stream("Thread::start: Cannot determine scheduler priority range");
         break;
      }
      sched_param sp;
      sp.sched_priority = calculatedPriority;
      if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0
          || pthread_attr_setschedpolicy(&attr, schedPolicy) != 0
          || pthread_attr_setschedparam(&attr, &sp) != 0) {
         // could not set scheduling hints, fallback to inheriting them
         // we'll try again from inside the thread
         pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
         implPtr->m_priority = Priority(priority | THREAD_PRIORITY_RESET_FLAG);
      }
      break;
   }
   }
#endif
   if (implPtr->m_stackSize > 0) {
#if defined(_POSIX_THREAD_ATTR_STACKSIZE) && (_POSIX_THREAD_ATTR_STACKSIZE-0 > 0)
      int code = pthread_attr_setstacksize(&attr, implPtr->m_stackSize);
#else
      int code = ENOSYS; // stack size not supported, automatically fail
#endif // _POSIX_THREAD_ATTR_STACKSIZE
      if (code) {
         warning_stream("Thread::start: Thread stack size error: %s",
                        pdk_printable(pdk::error_string(code)));
         // we failed to set the stacksize, and as the documentation states,
         // the thread will fail to run...
         implPtr->m_running = false;
         implPtr->m_finished = false;
         return;
      }
   }
   pthread_t threadId;
   int code = pthread_create(&threadId, &attr, ThreadPrivate::start, this);
   if (code == EPERM) {
      // caller does not have permission to set the scheduling
      // parameters/policy
#if defined(PDK_HAS_THREAD_PRIORITY_SCHEDULING)
      pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
#endif
      code = pthread_create(&threadId, &attr, ThreadPrivate::start, this); 
   }
   implPtr->m_data->m_threadId.store(to_pdk_handle_type(threadId));
   pthread_attr_destroy(&attr);
   if (code) {
      warning_stream("Thread::start: Thread creation error: %s", pdk_printable(pdk::error_string(code)));
      implPtr->m_running = false;
      implPtr->m_finished = false;
      implPtr->m_data->m_threadId.store(nullptr);
   }
}

void Thread::terminate()
{
   PDK_D(Thread);
   std::scoped_lock locker(implPtr->m_mutex);
   if (!implPtr->m_data->m_threadId) {
      return;
   }
   int code = pthread_cancel(from_pdk_handle_type<pthread_t>(implPtr->m_data->m_threadId));
   if (code) {
      warning_stream("Thread::start: Thread termination error: %s",
                     pdk_printable(pdk::error_string((code))));
   }
}

bool Thread::wait(unsigned long time)
{
   PDK_D(Thread);
   std::unique_lock locker(implPtr->m_mutex);
   if (from_pdk_handle_type<pthread_t>(implPtr->m_data->m_threadId) == pthread_self()) {
      warning_stream("Thread::wait: Thread tried to wait on itself");
      return false;
   }
   if (implPtr->m_finished || !implPtr->m_running) {
      return true;
   }
   while (implPtr->m_running) {
      if (time == ULONG_MAX) {
         implPtr->m_threadDone.wait(locker);
      } else {
         if (std::cv_status::timeout == implPtr->m_threadDone.wait_for(locker, std::chrono::milliseconds(time))) {
            return false;
         }
      }
   }
   return true;
}

void Thread::setTerminationEnabled(bool enabled)
{
   Thread *thread = getCurrentThread();
   PDK_ASSERT_X(thread != nullptr, "Thread::setTerminationEnabled()",
                "Current thread was not started with Thread.");
   PDK_UNUSED(thread);
   pthread_setcancelstate(enabled ? PTHREAD_CANCEL_ENABLE : PTHREAD_CANCEL_DISABLE, NULL);
   if (enabled) {
      pthread_testcancel();
   }
}

} // thread
} // os
} // pdk

