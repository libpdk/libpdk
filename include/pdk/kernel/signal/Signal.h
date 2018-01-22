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

//  A thread-safe version of Boost.Signals.

// Copyright Frank Mori Hess 2007-2009
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_SIGNAL_H
#define PDK_KERNEL_SIGNAL_SIGNAL_H

#include <algorithm>
#include <functional>
#include <mutex>

#include "pdk/kernel/signal/internal/SignalCommon.h"
#include "pdk/kernel/signal/internal/SlotGroup.h"
#include "pdk/kernel/signal/internal/SlotCallIterator.h"
#include "pdk/kernel/signal/internal/SignalPrivate.h"
#include "pdk/kernel/signal/internal/VariadicArgType.h"
#include "pdk/kernel/signal/internal/VariadicSlotInvoker.h"
#include "pdk/kernel/signal/internal/ResultTypeWrapper.h"
#include "pdk/kernel/signal/internal/ReplaceSlotFunction.h"

#include "pdk/kernel/signal/Connection.h"
#include "pdk/kernel/signal/OptionalLastValue.h"
#include "pdk/kernel/signal/Slot.h"
#include "pdk/stdext/typetraits/FunctionTraits.h"

namespace pdk {
namespace kernel {
namespace signal {

template <typename Signature,
          typename Combiner = OptionalLastValue<typename pdk::stdext::FunctionTraits<Signature>::result_type>,
          typename Group = int,
          typename GroupCompare = std::less<Group>,
          typename SlotFunction = std::function<Signature>,
          typename ExtendedSlotFunction = typename internal::VariadicExtendedSignature<Signature>::function_type,
          typename Mutex = std::mutex>
class Signal;

template <typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename ExtendedSlotFunction,
          typename Mutex,
          typename R,
          typename ... Args>
class Signal <R (Args...), Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex> 
   : public SignalBase, public internal::StdFuncBase<Args...>
{
   using ImplClass = internal::SignalImpl<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
                                          ExtendedSlotFunction, Mutex>;
public:
   using WeakSignalType = internal::WeakSignal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
                                               ExtendedSlotFunction, Mutex>;
   using SlotFunctionType = SlotFunction;
   using SlotType = typename ImplClass::SlotType;
   using ExtendedSlotFunctionType = typename ImplClass::ExtendedSlotFunctionType;
   using ExtendedSlotType = typename ImplClass::ExtendedSlotType;
   using SlotResultType = typename SlotFunctionType::ResultType;
   using CombinerType = Combiner;
   using ResultType = typename ImplClass::ResultType;
   using GroupType = Group;
   using GroupCompareType = GroupCompare;
   using SlotCallIterator = typename ImplClass::SlotCallIterator;
   using SignatureType = R (Args...);

   friend class internal::WeakSignal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
                                     ExtendedSlotFunction, Mutex>;

   template<unsigned N> class Arg
   {
   public:
      typedef typename internal::VariadicArgType<N, Args...>::type type;
   };

   static const int arity = sizeof...(Args);
   
   Signal(const CombinerType &combinerArg = CombinerType(),
          const GroupCompareType &groupCompare = GroupCompareType())
      : m_pimpl(new ImplClass(combinerArg, groupCompare))
   {}
   
   virtual ~Signal()
   {}
   
   Signal(Signal &&other)
   {
      using std::swap;
      swap(m_pimpl, other.m_pimpl);
   }
   
   Signal &operator=(Signal &&other)
   {
      if(this == &other)
      {
         return *this;
      }
      m_pimpl.reset();
      using std::swap;
      swap(m_pimpl, other.m_pimpl);
      return *this;
   }
   
   Connection connect(const SlotType &slot, ConnectPosition position = ConnectPosition::AtBack)
   {
      return (*m_pimpl).connect(slot, position);
   }
   
   Connection connect(const GroupType &group,
                      const SlotType &slot, ConnectPosition position = ConnectPosition::AtBack)
   {
      return (*m_pimpl).connect(group, slot, position);
   }
   
   Connection connectExtended(const ExtendedSlotType &slot, ConnectPosition position = ConnectPosition::AtBack)
   {
      return (*m_pimpl).connectExtended(slot, position);
   }
   
   Connection connectExtended(const GroupType &group,
                              const ExtendedSlotType &slot, ConnectPosition position = ConnectPosition::AtBack)
   {
      return (*m_pimpl).connectExtended(group, slot, position);
   }
   
   void disconnectAllSlots()
   {
      (*m_pimpl).disconnectAllSlots();
   }
   
   void disconnect(const GroupType &group)
   {
      (*m_pimpl).disconnect(group);
   }
   
   template <typename T>
   void disconnect(const T &slot)
   {
      (*m_pimpl).disconnect(slot);
   }
   
   ResultType operator ()(Args ... args)
   {
      return (*m_pimpl)(args...);
   }
   
   ResultType operator ()(Args ... args) const
   {
      return (*m_pimpl)(args...);
   }
   
   std::size_t numSlots() const
   {
      return (*m_pimpl).numSlots();
   }
   
   bool empty() const
   {
      return (*m_pimpl).empty();
   }
   
   CombinerType combiner() const
   {
      return (*m_pimpl).combiner();
   }
   
   void setCombiner(const CombinerType &combinerArg)
   {
      return (*m_pimpl).setCombiner(combinerArg);
   }
   
   void swap(Signal &other)
   {
      using std::swap;
      swap(m_pimpl, other.m_pimpl);
   }

protected:
   virtual std::shared_ptr<void> getLockPimpl() const
   {
      return m_pimpl;
   }

private:
   std::shared_ptr<ImplClass> m_pimpl;
};

// free swap function, findable by ADL
template<typename Signature,
         typename Combiner,
         typename Group,
         typename GroupCompare,
         typename SlotFunction,
         typename ExtendedSlotFunction,
         typename Mutex>
void swap(
      Signal<Signature, Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex> &sig1,
      Signal<Signature, Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex> &sig2)
{
   sig1.swap(sig2);
}

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_SIGNAL_H
