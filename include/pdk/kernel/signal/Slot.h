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
// Created by softboy on 2018/01/20.

// Boost.Signals2 library

// Copyright Frank Mori Hess 2009.
//
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_SLOT_H
#define PDK_KERNEL_SIGNAL_SLOT_H

#include "pdk/kernel/signal/internal/SignalCommon.h"
#include "pdk/kernel/signal/internal/VariadicArgType.h"
#include "pdk/kernel/signal/internal/TrackedObjectsVisitor.h"
#include "pdk/kernel/signal/SlotBase.h"
#include "pdk/stdext/VisitEach.h"

namespace pdk {
namespace kernel {
namespace signal {

template <typename Signature, typename SlotFunction = std::function<Signature>>
class Slot;

template <typename SlotFunction, typename R, typename ... Args>
class Slot<R (Args...), SlotFunction> : public SlotBase, public internal::StdFuncBase<Args...>
{
public:
   template <typename prefixSignature, typename OtherSlotFunction>
   friend class Slot;
   using SlotFunctionType = SlotFunction;
   using ResultType = R;
   using SignatureType = R(Args...);
   using slot_function_type = SlotFunctionType;
   using result_type = ResultType;
   using signature_type = SignatureType;
   template<unsigned n> 
   class Arg
   {
   public:
      using type = typename internal::VariadicArgType<n, Args...>::type;
   };
   static constexpr int arity = sizeof...(Args);
   
   template<typename F>
   Slot(const F &f)
   {
      initSlotFunc(f);
   }
   
   template<typename Signature, typename OtherSlotFunction>
   Slot(const Slot<Signature, OtherSlotFunction> &otherSlot)
      : SlotBase(otherSlot), 
        m_slotFunc(otherSlot.m_slotFunc)
   {}
   
   template<typename A1, typename A2, typename ... BindArgs>
   Slot(const A1 &arg1, const A2 &arg2, const BindArgs & ... args)
   {
      initSlotFunc(std::bind(arg1, arg2, args...));
   }
   
   // invocation
   R operator()(Args ... args)
   {
      LockedContainerType lockedObjects = lock();
      return m_slotFunc(args...);
   }
   
   R operator()(Args ... args) const
   {
      LockedContainerType lockedObjects = lock();
      return m_slotFunc(args...);
   }
   
   // tracking
   Slot &track(const std::weak_ptr<void> &tracked)
   {
      m_trackedObjects.push_back(tracked);
      return *this;
   }
   
   Slot &track(const SignalBase &signal)
   {
      trackSignal(signal);
      return *this;
   }
   
   Slot &track(const SlotBase &slot)
   {
      TrackedContainerType::const_iterator it;
      for(it = slot.getTrackedObjects().begin(); it != slot.getTrackedObjects().end(); ++it)
      {
         m_trackedObjects.push_back(*it);
      }
      return *this;
   }
   
   const SlotFunctionType &slotFunc() const
   {
      return m_slotFunc;
   }

   SlotFunctionType &slotFunc()
   {
      return m_slotFunc;
   }
   
private:
   template<typename F>
   void initSlotFunc(const F& f)
   {
      m_slotFunc = internal::get_invocable_slot(f, internal::tag_type(f));
      internal::TrackedObjectsVisitor visitor(this);
      pdk::stdext::visit_each(visitor, f);
   }
  
   SlotFunction m_slotFunc;
};

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_SLOT_H
