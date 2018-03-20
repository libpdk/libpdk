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
#include "pdk/stdext/typetraits/CallableInfoTrait.h"

template <typename FuncType, typename... ArgTypes>
decltype(auto) generate_slot_wrapper(FuncType func, ArgTypes...args1)
{
   return [=](int n){
      func(1);
   };
}
//inline decltype(auto) generate_slot_wrapper1() noexcept
//{
//   return [](int) -> void{
      
//   };
//}

template <typename... ArgTypes>
inline int generate_slot_wrapper1(ArgTypes&&... args)
{
   return 1;
}



//template <typename FuncType>
//struct AyncSlotGenerator
//{
//   decltype(auto) operator ()(std::function<FuncType> func)
//   {
//      using ArgTypes = typename pdk::stdext::CallableInfoTrait<FuncType>::ArgTypes;
//      return std::apply(generate_slot_wrapper1<std::function<FuncType>>, std::tuple_cat(std::make_tuple(func), std::make_tuple(1)));
//   }
//};

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

#define PDK_DECLARE_SIGNAL_BINDER(signalName) public:\
   pdk::kernel::signal::Connection\
   connect##signalName##Signal(const std::function<signalName##HandlerType> &callable,\
   pdk::kernel::Object *receiver = nullptr,\
   pdk::ConnectionType connectionType = pdk::ConnectionType::AutoConnection);\
   protected: \
   std::shared_ptr<pdk::kernel::signal::Signal<signalName##HandlerType>> m_##signalName##Signal

#define PDK_DEFINE_SIGNAL_EMITTER(signalName) \
   template <typename ...ArgTypes>\
   void emit##signalName##Signal(ArgTypes&& ...args)\
{\
   if (m_##signalName##Signal) {\
   (*m_##signalName##Signal)(std::forward<ArgTypes>(args)...);\
}\
}\
   
#define PDK_DEFINE_SIGNAL_CLS_NAME(clsname) clsname
#define PDK_DEFINE_SIGNAL_BINDER(clsname, signalName) \
   pdk::kernel::signal::Connection PDK_DEFINE_SIGNAL_CLS_NAME(clsname)::connect##signalName##Signal(\
   const std::function<signalName##HandlerType> &callable, \
   pdk::kernel::Object *receiver,\
   pdk::ConnectionType connectionType)\
{\
   using ArgTypes = typename pdk::stdext::CallableInfoTrait<signalName##HandlerType>::ArgTypes;\
   using ReturnType = typename pdk::stdext::CallableInfoTrait<signalName##HandlerType>::ReturnType;\
   if (!m_##signalName##Signal) {\
      m_##signalName##Signal.reset(new pdk::kernel::signal::Signal<signalName##HandlerType>);\
   }\
   if (nullptr == receiver) {\
      receiver = this;\
   }\
   if (connectionType == pdk::ConnectionType::DirectConnection) {\
      return m_##signalName##Signal->connect(callable);\
   } else if (connectionType == pdk::ConnectionType::QueuedConnection) {\
      auto wrapper = std::apply([&callable, this, receiver](auto&&...args) {\
         return [&callable,this, receiver](decltype(args)... args1) -> ReturnType{\
            pdk::kernel::CoreApplication::postEvent(receiver, new pdk::kernel::internal::MetaCallEvent([&callable](std::any &arg) {\
               std::apply(callable, std::any_cast<ArgTypes>(arg));\
            }, std::any(std::make_tuple(std::forward<decltype(args1)>(args1)...))));\
         };\
      }, ArgTypes());\
      return m_##signalName##Signal->connect(wrapper);\
   } else if (connectionType == pdk::ConnectionType::AutoConnection) {\
      if (getThread() == receiver->getThread()) {\
         return m_##signalName##Signal->connect(callable);\
      } else { \
         auto wrapper = std::apply([&](auto&&...args) {\
            return [&](decltype(args)... args1) -> ReturnType{\
               pdk::kernel::CoreApplication::postEvent(receiver, new pdk::kernel::internal::MetaCallEvent([&](std::any &arg) {\
                  std::apply(callable, std::any_cast<ArgTypes>(arg));\
               }, std::any(std::make_tuple(std::forward<decltype(args)>(args1)...))));\
            };\
         }, ArgTypes());\
         return m_##signalName##Signal->connect(wrapper);\
      }\
   }\
}

class PDK_CORE_EXPORT Thread : public pdk::kernel::Object
{
public:
   using FinishedHandlerType = void(int);
   using StartedHandlerType = void(int);
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
   
   PDK_DEFINE_SIGNAL_EMITTER(Started)
   PDK_DEFINE_SIGNAL_EMITTER(Finished)
   
   PDK_DECLARE_SIGNAL_BINDER(Finished);
   PDK_DECLARE_SIGNAL_BINDER(Started);
   
protected:
   virtual void run();
   int exec();
   static void setTerminationEnabled(bool enabled = true);
   
protected:
   Thread(ThreadPrivate &dd, Object *parent = nullptr);
   
private:
   static Thread *createThreadImpl(std::future<void> &&future);
   PDK_DECLARE_PRIVATE(Thread);
   friend class CoreApplication;
   friend class ThreadData;
};

} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_THREAD_H
