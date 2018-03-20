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
// Created by softboy on 2018/03/08.

#ifndef PDK_KERNEL_INTERNAL_OBJECT_DEFS_PRIVATE_H
#define PDK_KERNEL_INTERNAL_OBJECT_DEFS_PRIVATE_H

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
   pdk::kernel::signal::Connection PDK_DEFINE_SIGNAL_CLS_NAME(clsname)::connect## signalName ##Signal(\
         const std::function<signalName ## HandlerType> &callable,\
         pdk::kernel::Object *receiver,\
         pdk::ConnectionType connectionType)\
   {\
      using ReturnType = typename pdk::stdext::CallableInfoTrait<signalName ## HandlerType>::ReturnType;\
      if (!m_## signalName ##Signal) {\
         m_## signalName ##Signal.reset(new pdk::kernel::signal::Signal<signalName ## HandlerType>);\
      }\
      if (nullptr == receiver) {\
         receiver = this;\
      }\
      switch(connectionType) {\
      case pdk::ConnectionType::DirectConnection:\
         return m_## signalName ##Signal->connect(callable);\
      case pdk::ConnectionType::QueuedConnection: {\
         auto wrapper = [callable, receiver](auto&&... args) -> ReturnType{\
            pdk::kernel::CoreApplication::postEvent(receiver, new pdk::kernel::internal::MetaCallEvent([=](){\
               callable(args...);\
            }));\
         };\
         return m_## signalName ##Signal->connect(wrapper);\
      }\
      case pdk::ConnectionType::AutoConnection:{\
         if (getThread() == receiver->getThread()) {\
            return m_StartedSignal->connect(callable);\
         } else {\
            auto wrapper = [callable, receiver](auto&&... args) -> ReturnType{\
               CoreApplication::postEvent(receiver, new pdk::kernel::internal::MetaCallEvent([=](){\
                  callable(args...);\
               }));\
            };\
            return m_## signalName ##Signal->connect(wrapper);\
         }\
      }\
      }\
   }


#endif // PDK_KERNEL_INTERNAL_OBJECT_DEFS_PRIVATE_H
