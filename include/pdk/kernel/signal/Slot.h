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
#include "pdk/kernel/signal/internal/TrackedObjectsVisitor.h"
#include "pdk/kernel/signal/internal/VariadicArgType.h"
#include "pdk/kernel/signal/SlotBase.h"

namespace pdk {
namespace kernel {
namespace signal {

template <typename Signature, typename SlotFunction std::function<Signature>>
class Slot;

template <typename SlotFunction, typename R, typename ... Args>
class Slot<R (Args...), SlotFunction> : public SlotBase, public internal::StdFuncBase<Args...>
{
public:
   template <typename prefixSignature, typename OtherSlotFunction>
   friend class Slot;
   using slot_function_type = SlotFunction;
   using result_type = R;
   using signature_type = R(Args...);

   template<unsigned n> 
   class arg
   {
   public:
      using type = typename internal::VariadicArgType<n, Args...>::type;
   };
   static const int arity = sizeof...(Args);
   
   template<typename F>
   Slot(const F &f)
   {
      initSlotFunction(f);
   }
   
   template<typename Signature, typename OtherSlotFunction>
   Slot(const Slot<Signature, OtherSlotFunction> &otherSlot)
      : m_slotBase(otherSlot), m_slotFunction(otherSlot.m_slotFunction)
   {}
   
   template<typename A1, typename A2, typename ... BindArgs>
   Slot(const A1 &arg1, const A2 &arg2, const BindArgs & ... args)
   {
      initSlotFunction(std::bind(arg1, arg2, args...));
   }
   
   // invocation
   R operator()(Args ... args)
   {
      LockedContainerType lockedObjects = lock();
      return slotFunction(args...);
   }
   
   R operator()(Args ... args) const
   {
      LockedContainerType lockedObjects = lock();
      return slotFunction(args...);
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
      tracked_container_type::const_iterator it;
      for(it = slot.trackedObjects().begin(); it != slot.trackedObjects().end(); ++it)
      {
         m_trackedObjects.push_back(*it);
      }
      return *this;
   }
   
   template<typename ForeignWeakPtr>
   Slot &trackForeign(const ForeignWeakPtr &tracked,
                      typename weak_ptr_traits<ForeignWeakPtr>::shared_type * /*SFINAE*/ = 0)
   {
      _tracked_objects.push_back(internel::foreign_void_weak_ptr(tracked));
      return *this;
   }
   
   template<typename ForeignSharedPtr>
   Slot &trackForeign(const ForeignSharedPtr &tracked,
                      typename shared_ptr_traits<ForeignSharedPtr>::weak_type * /*SFINAE*/ = 0)
   {
      m_tracked_objects.push_back(
             detail::foreign_void_weak_ptr
             (
               typename shared_ptr_traits<ForeignSharedPtr>::weak_type(tracked)
             )
      );
      return *this;
   }
   
   const slot_function_type &slotFunction() const
   {
      return _slot_function;
   }

   slot_function_type& slot_function()
   {
      return _slot_function;
   }
   
private:
   template<typename F>
   void init_slot_function(const F& f)
   {
      _slot_function = detail::get_invocable_slot(f, detail::tag_type(f));
      signals2::detail::tracked_objects_visitor visitor(this);
      boost::visit_each(visitor, f);
   }
  
   SlotFunction _slot_function;
};

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_SLOT_H
