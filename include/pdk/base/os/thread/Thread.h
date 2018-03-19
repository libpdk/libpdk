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
// Created by softboy on 2017/01/24.

#ifndef PDK_M_BASE_OS_THREAD_THREAD_H
#define PDK_M_BASE_OS_THREAD_THREAD_H

#include <limits.h>
#include <future>
#include "pdk/kernel/Object.h"

namespace pdk {

// forward declare with namespace
namespace kernel {
class AbstractEventDispatcher;
class Event;
class CoreApplication;
} // kernel

namespace os {
namespace thread {

// forward declare with namespace
namespace internal {
class ThreadPrivate;
class ThreadData;
} // internal

using internal::ThreadPrivate;
using internal::ThreadData;
using pdk::kernel::AbstractEventDispatcher;
using pdk::kernel::Event;
using pdk::kernel::CoreApplication;
using pdk::kernel::signal::Signal;
using pdk::kernel::signal::Connection;

class PDK_CORE_EXPORT Thread : public pdk::kernel::Object
{
public:
   using FinishedHandlerType = void();
   using StartedHandlerType = void();
   static pdk::HANDLE getCurrentThreadId() noexcept PDK_DECL_PURE_FUNCTION;
   static Thread *getCurrentThread();
   static int getIdealThreadCount() noexcept;
   static void yieldCurrentThread();
   
   explicit Thread(Object *parent = nullptr);
   ~Thread();
   enum Priority {
      IdlePriority,
      LowestPriority,
      LowPriority,
      NormalPriority,
      HighPriority,
      HighestPriority,
      TimeCriticalPriority,
      InheritPriority
   };
   void setPriority(Priority priority);
   Priority getPriority() const;
   
   bool isFinished() const;
   bool isRunning() const;
   
   void requestInterruption();
   bool isInterruptionRequested() const;
   
   void setStackSize(uint stackSize);
   uint getStackSize() const;
   
   void exit(int retcode = 0);
   
   AbstractEventDispatcher *getEventDispatcher() const;
   void setEventDispatcher(AbstractEventDispatcher *eventDispatcher);
   
   bool event(Event *event) override;
   int getLoopLevel() const;
   
public:
   void start(Priority = Priority::InheritPriority);
   void terminate();
   void quit();
   
public:
   // default argument causes thread to block indefinetely
   bool wait(unsigned long time = ULONG_MAX);
   static void sleep(unsigned long);
   static void msleep(unsigned long);
   static void usleep(unsigned long);
   // signals
   // void started(PrivateSignal);
   // void finished(PrivateSignal);
   template <typename ...ArgTypes>
   void emitStartedSignal(ArgTypes&& ...args);
   template <typename ...ArgTypes>
   void emitFinishedSignal(ArgTypes&& ...args);
   
   Connection connectStartedSignal(const std::function<StartedHandlerType> &callable);
   Connection connectFinishedSignal(const std::function<FinishedHandlerType> &callable);
   
protected:
   virtual void run();
   int exec();
   static void setTerminationEnabled(bool enabled = true);
   
protected:
   Thread(ThreadPrivate &dd, Object *parent = nullptr);
protected:
   std::shared_ptr<Signal<FinishedHandlerType>> m_finishedSignal;
   std::shared_ptr<Signal<StartedHandlerType>> m_startedSignal;
   
private:
   static Thread *createThreadImpl(std::future<void> &&future);
   PDK_DECLARE_PRIVATE(Thread);
   friend class CoreApplication;
   friend class ThreadData;
};

template <typename ...ArgTypes>
void Thread::emitStartedSignal(ArgTypes&& ...args)
{
   if (m_startedSignal) {
      (*m_startedSignal)(std::forward<ArgTypes>(args)...);
   }
}

template <typename ...ArgTypes>
void Thread::emitFinishedSignal(ArgTypes&& ...args)
{
   if (m_finishedSignal) {
      (*m_finishedSignal)(std::forward<ArgTypes>(args)...);
   }
}

} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_THREAD_H
