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

#include "pdk/global/Global.h"
#include "pdk/stdext/typetraits/CallableInfoTrait.h"
#include "pdk/stdext/typetraits/Sequence.h"
#include "pdk/kernel/Pointer.h"
#include <functional>
#include <tuple>

namespace pdk {
namespace kernel {

// forward declare class
class Object;

namespace internal {

// forward declare class
class MetaCallEvent;

void post_app_event_helper(Object *, MetaCallEvent *);
bool is_in_current_thread(const Object *);

template <bool isObject, typename SignalType, typename CallableType>
struct SlotArgInfoImpl
{
   using CallableInfo = pdk::stdext::CallableInfoTrait<decltype(&CallableType::operator())>;
   using ArgTypes = typename CallableInfo::ArgTypes;
   constexpr static bool detectCanPassSenderInfo()
   {
      constexpr size_t argNum = CallableInfo::argNum;
      if constexpr (argNum < 2) {
         return false;
      } else {
         return std::is_same<typename CallableInfo::template arg<argNum - 2>::type, SignalType>::value &&
                std::is_same<std::remove_pointer_t<std::remove_reference_t<std::remove_cv_t<typename CallableInfo::template arg<argNum - 1>::type>>>, Object>::value;
      }
   }
   constexpr static size_t argNum = CallableInfo::argNum;
   constexpr static bool canPassSenderInfo = detectCanPassSenderInfo();
};

template <typename SignalType, typename CallableType>
struct SlotArgInfoImpl<false, SignalType, CallableType>
{
   using CallableInfo = pdk::stdext::CallableInfoTrait<CallableType>;
   using ArgTypes = typename CallableInfo::ArgTypes;
   constexpr static bool detectCanPassSenderInfo()
   {
      constexpr size_t argNum = CallableInfo::argNum;
      if constexpr (argNum < 2) {
         return false;
      } else {
         return std::is_same<typename CallableInfo::template arg<argNum - 2>::type, SignalType>::value &&
                std::is_same<std::remove_pointer_t<std::remove_reference_t<std::remove_cv_t<typename CallableInfo::template arg<argNum - 1>::type>>>, Object>::value;
      }
   }
   constexpr static size_t argNum = CallableInfo::argNum;
   constexpr static bool canPassSenderInfo = detectCanPassSenderInfo();
};

template <typename SignalType, typename CallableType>
struct SlotArgInfo : SlotArgInfoImpl<std::is_class<std::remove_reference_t<CallableType>>::value, SignalType, CallableType>
{};

template <typename SignalType, typename MemberFuncSlot>
struct IsMemberFuncSlotCanPassSenderInfo
{
   constexpr static bool canPassSenderInfo()
   {
      using CallableInfo = pdk::stdext::CallableInfoTrait<MemberFuncSlot>;
      constexpr size_t argNum = CallableInfo::argNum;
      if constexpr (argNum < 2) {
         return false;
      } else {
         return std::is_same<typename CallableInfo::template arg<argNum - 2>::type, SignalType>::value &&
                std::is_same<std::remove_pointer_t<std::remove_reference_t<std::remove_cv_t<typename CallableInfo::template arg<argNum - 1>::type>>>, Object>::value;
      }
   }
   constexpr static bool value = canPassSenderInfo();
};

template <size_t slotArgNum, bool useSenderInfo,
          typename SignalArgTypes, typename SignalType, 
          typename Class, std::size_t... ArgIndex, typename... ArgTypes>
constexpr inline decltype(auto) get_slot_args(Class receiver, SignalType signal, Object *sender, 
                                              const std::index_sequence<ArgIndex...> &, ArgTypes&& ...args)
{
   
   if constexpr(useSenderInfo) {
      return std::tuple_cat(std::tuple_cat(std::tuple<Class>(receiver),
                                           pdk::stdext::extract_first_n_items<slotArgNum - 2>(
                                              std::make_tuple((typename std::tuple_element<ArgIndex..., SignalArgTypes>::type)args...))
                                           ), 
                            std::make_tuple(signal, sender));
   } else {
      return std::tuple_cat(std::tuple<Class>(receiver),
                            pdk::stdext::extract_first_n_items<slotArgNum>(
                               std::make_tuple((typename std::tuple_element<ArgIndex..., SignalArgTypes>::type)args...)
                               ));
   }
}

template <size_t slotArgNum, bool useSenderInfo,
          typename SignalArgTypes, typename SignalType,
          std::size_t...ArgIndex, typename... ArgTypes>
constexpr inline decltype(auto) get_slot_args(SignalType signal, Object *sender, const std::index_sequence<ArgIndex...> &, ArgTypes&& ...args)
{
   if constexpr(useSenderInfo) {
      auto base = pdk::stdext::extract_first_n_items<slotArgNum - 2>(
               std::make_tuple((typename std::tuple_element<ArgIndex..., SignalArgTypes>::type)args...));
      return std::tuple_cat(base, std::make_tuple(signal, sender));
   } else {
      return pdk::stdext::extract_first_n_items<slotArgNum>(
               std::make_tuple((typename std::tuple_element<ArgIndex..., SignalArgTypes>::type)args...));
   }
}

} // internal
} // kernel
} // pdk

#define PDK_DEFINE_SIGNAL_ENUMS(...) \
   enum class SignalType { __VA_ARGS__ }

#define PDK_SIGNAL_NAME(signalName) signalName

#define PDK_DEFINE_SIGNAL_BINDER(signalName) \
   protected: \
   std::shared_ptr<pdk::kernel::signal::Signal<signalName##HandlerType>> m_##signalName##Signal;\
   public:\
   pdk::kernel::signal::Connection\
   connect##signalName##Signal(const std::function<signalName##HandlerType> &callable,\
   const pdk::kernel::Object *context = nullptr,\
   pdk::ConnectionType connectionType = pdk::ConnectionType::AutoConnection);\
   template <typename RetType, typename Class> \
   pdk::kernel::signal::Connection\
   connect##signalName##Signal(const pdk::kernel::Object *receiver, RetType Class::* memberFunc,\
   pdk::ConnectionType connectionType = pdk::ConnectionType::AutoConnection)\
   {\
      using SignalHandlerTypeInfo = typename pdk::stdext::CallableInfoTrait<signalName ## HandlerType>;\
      using ReturnType = typename SignalHandlerTypeInfo::ReturnType;\
      using SignalArgTypes = typename SignalHandlerTypeInfo::ArgTypes;\
      constexpr bool canPassSenderInfo = pdk::kernel::internal::IsMemberFuncSlotCanPassSenderInfo<SignalType, decltype(memberFunc)>::value;\
      constexpr size_t signalArgNum = SignalHandlerTypeInfo::argNum;\
      constexpr size_t slotArgNum = pdk::stdext::CallableInfoTrait<decltype(memberFunc)>::argNum;\
      pdk::kernel::Pointer<pdk::kernel::Object> receiverPtr(const_cast<pdk::kernel::Object *>(receiver));\
      if constexpr(canPassSenderInfo) {\
         PDK_STATIC_ASSERT_X(signalArgNum >= (slotArgNum - 2), "slot handler argument number must less or equal than signal");\
      } else {\
         PDK_STATIC_ASSERT_X(signalArgNum >= slotArgNum, "slot handler argument number must less or equal than signal");\
      }\
      if (!m_## signalName ##Signal) {\
         m_## signalName ##Signal.reset(new pdk::kernel::signal::Signal<signalName ## HandlerType>);\
      }\
      Object *sender = this;\
      if (connectionType == pdk::ConnectionType::DirectConnection) {\
         return m_## signalName ##Signal->connect([memberFunc, receiverPtr, sender](auto&&... args) -> ReturnType{\
            if (receiverPtr) {\
               return std::apply(std::mem_fn(memberFunc),\
                           pdk::kernel::internal::get_slot_args<slotArgNum, canPassSenderInfo, SignalArgTypes>(dynamic_cast<Class *>(receiverPtr.getData()), \
                           SignalType::PDK_SIGNAL_NAME(signalName), sender, std::make_index_sequence<signalArgNum>(), args...));\
            }\
         });\
      } else if (connectionType == pdk::ConnectionType::QueuedConnection) {\
         auto wrapper = [memberFunc, sender, receiverPtr](auto&&... args) -> ReturnType{\
            if (receiverPtr) {\
               pdk::kernel::internal::post_app_event_helper(receiverPtr, new pdk::kernel::internal::MetaCallEvent(\
                  [memberFunc, sender, receiverPtr, args...](){\
                  std::apply(std::mem_fn(memberFunc),\
                  pdk::kernel::internal::get_slot_args<slotArgNum, canPassSenderInfo, SignalArgTypes>(dynamic_cast<Class *>(receiverPtr.getData()), \
                       SignalType::PDK_SIGNAL_NAME(signalName), sender, std::make_index_sequence<signalArgNum>(), args...));\
               }));\
            }\
         };\
         return m_## signalName ##Signal->connect(wrapper);\
      }else {\
         auto wrapper = [memberFunc, sender, receiverPtr](auto&&... args) -> ReturnType{\
            if (receiverPtr) {\
               if (pdk::kernel::internal::is_in_current_thread(receiverPtr)) {\
                  return std::apply(std::mem_fn(memberFunc),\
                     pdk::kernel::internal::get_slot_args<slotArgNum, canPassSenderInfo, SignalArgTypes>(dynamic_cast<Class *>(receiverPtr.getData()), \
                          SignalType::PDK_SIGNAL_NAME(signalName), sender, std::make_index_sequence<signalArgNum>(), args...));\
               }\
               pdk::kernel::internal::post_app_event_helper(receiverPtr, new pdk::kernel::internal::MetaCallEvent(\
                  [memberFunc, sender, receiverPtr, args...](){\
                  std::apply(std::mem_fn(memberFunc),\
                     pdk::kernel::internal::get_slot_args<slotArgNum, canPassSenderInfo, SignalArgTypes>(dynamic_cast<Class *>(receiverPtr.getData()), \
                          SignalType::PDK_SIGNAL_NAME(signalName), sender, std::make_index_sequence<signalArgNum>(), args...));\
               }));\
            }\
         };\
         return m_## signalName ##Signal->connect(wrapper);\
      }\
   }\
   template <typename SlotFuncType>\
   pdk::kernel::signal::Connection connect## signalName ##Signal(\
         SlotFuncType &&callable,\
         const pdk::kernel::Object *context = nullptr,\
         pdk::ConnectionType connectionType = pdk::ConnectionType::AutoConnection)\
   {\
      PDK_STATIC_ASSERT_X(pdk::stdext::IsCallable<SlotFuncType>::value, "Slot must be callable type");\
      using CallableInfo = pdk::stdext::CallableInfoTrait<signalName ## HandlerType>;\
      using ReturnType = typename CallableInfo::ReturnType;\
      using SlotArgInfo = pdk::kernel::internal::SlotArgInfo<SignalType, SlotFuncType>;\
      using SignalArgTypes = typename CallableInfo::ArgTypes;\
      using DecayedSlotFuncType = typename std::decay<SlotFuncType>::type;\
      if (nullptr == context) {\
         context = this;\
      }\
      pdk::kernel::Pointer<pdk::kernel::Object> contextPtr(const_cast<pdk::kernel::Object *>(context));\
      constexpr bool canPassSenderInfo = SlotArgInfo::canPassSenderInfo;\
      constexpr size_t signalArgNum = CallableInfo::argNum;\
      constexpr size_t slotArgNum = SlotArgInfo::argNum;\
      if constexpr(canPassSenderInfo) {\
         PDK_STATIC_ASSERT_X(signalArgNum >= (slotArgNum - 2), "slot handler argument number must less or equal than signal");\
      } else {\
         PDK_STATIC_ASSERT_X(signalArgNum >= slotArgNum, "slot handler argument number must less or equal than signal");\
      }\
      if (!m_## signalName ##Signal) {\
         m_## signalName ##Signal.reset(new pdk::kernel::signal::Signal<signalName ## HandlerType>);\
      }\
      Object *sender = this;\
      if (connectionType == pdk::ConnectionType::DirectConnection) {\
         return m_## signalName ##Signal->connect([sender, callable = static_cast<DecayedSlotFuncType>(std::forward<SlotFuncType>(callable))]\
            (auto&&... args) -> ReturnType{\
               return std::apply(std::move(callable),\
                                 pdk::kernel::internal::get_slot_args<slotArgNum, canPassSenderInfo, SignalArgTypes>(SignalType::PDK_SIGNAL_NAME(signalName),\
                                    sender, std::make_index_sequence<signalArgNum>(), args...));\
         });\
      } else if (connectionType == pdk::ConnectionType::QueuedConnection) {\
         auto wrapper = [sender, contextPtr, callable = static_cast<DecayedSlotFuncType>(std::forward<SlotFuncType>(callable))](auto&&... args) -> ReturnType{\
            if (contextPtr) {\
               pdk::kernel::internal::post_app_event_helper(contextPtr.getData(),\
                  new pdk::kernel::internal::MetaCallEvent([sender, callable = std::move(callable), args...](){\
                     std::apply(std::move(callable),\
                             pdk::kernel::internal::get_slot_args<slotArgNum, canPassSenderInfo, SignalArgTypes>(SignalType::PDK_SIGNAL_NAME(signalName),\
                                 sender, std::make_index_sequence<signalArgNum>(), args...));\
               }));\
            }\
         };\
         return m_## signalName ##Signal->connect(wrapper);\
      } else {\
         auto wrapper = [sender, contextPtr, callable = static_cast<DecayedSlotFuncType>(std::forward<SlotFuncType>(callable))](auto&&... args) -> ReturnType{\
            if (contextPtr) {\
               if (pdk::kernel::internal::is_in_current_thread(contextPtr.getData())) {\
                  return std::apply(std::move(callable),\
                                    pdk::kernel::internal::get_slot_args<slotArgNum, canPassSenderInfo, SignalArgTypes>(SignalType::PDK_SIGNAL_NAME(signalName),\
                                       sender, std::make_index_sequence<signalArgNum>(), args...));\
               } else {\
                  pdk::kernel::internal::post_app_event_helper(contextPtr.getData(),\
                     new pdk::kernel::internal::MetaCallEvent([sender, callable = std::move(callable), args...](){\
                        std::apply(std::move(callable),\
                                pdk::kernel::internal::get_slot_args<slotArgNum, canPassSenderInfo, SignalArgTypes>(SignalType::PDK_SIGNAL_NAME(signalName),\
                                    sender, std::make_index_sequence<signalArgNum>(), args...));\
                  }));\
               }\
            }\
         };\
         return m_## signalName ##Signal->connect(wrapper);\
      }\
   }\
   void disconnect## signalName ##Signal(const pdk::kernel::signal::Connection &connection)\
   {\
      if (m_## signalName ##Signal) {\
         m_## signalName ##Signal->disconnect(connection);\
      }\
   }

#define PDK_DEFINE_SIGNAL_EMITTER(signalName) \
   public:\
   template <typename ...ArgTypes>\
   void emit##signalName##Signal(ArgTypes&& ...args)\
   {\
      if (m_##signalName##Signal) {\
         (*m_##signalName##Signal)(std::forward<ArgTypes>(args)...);\
      }\
   }\

#endif // PDK_KERNEL_INTERNAL_OBJECT_DEFS_PRIVATE_H
