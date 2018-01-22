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

namespace pdk {
namespace kernel {
namespace signal {

template <typename Signature,
          typename Combiner = OptionalLastValue<typename std::function_traits<Signature>::result_type>,
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
               using impl_class = internal::SignalImpl<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
               ExtendedSlotFunction, Mutex>;
public:
using weak_signal_type = internal::WeakSignal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
ExtendedSlotFunction, Mutex>;
friend class internal::WeakSignal<R (Args...), Combiner, Group, GroupCompare, SlotFunction, 
ExtendedSlotFunction, Mutex>;
using slot_function_type = SlotFunction;
using slot_type = typename impl_class::slot_type;
using extended_slot_function_type = typename impl_class::extended_slot_function_type;
using extended_slot_type = typename impl_class::extended_slot_type;
using slot_result_type = typename slot_function_type::result_type;
using combiner_type = Combiner;
using result_type = typename impl_class::result_type;
using group_type = Group;
using group_compare_type = GroupCompare;
using slot_call_iterator = typename impl_class::slot_call_iterator;
using signature_type = typename mpl::identity<R (Args...)>::type;

template<unsigned n> class arg
{
public:
   typedef typename detail::variadic_arg_type<n, Args...>::type type;
};
static const int arity = sizeof...(Args);

Signal(const combiner_type &combiner_arg = combiner_type(),
       const group_compare_type &group_compare = group_compare_type())
   : m_impl(new impl_class(combiner_arg, group_compare))
{
   
}

virtual ~Signal()
{
}

Signal(Signal &&other)
{
   using std::swap;
   swap(m_pimpl, other.m_pimpl);
}

Signal &operator=(Signal &&other)
{
   if(this == &rhs)
   {
      return *this;
   }
   m_pimpl.reset();
   using std::swap;
   swap(m_pimpl, rhs.m_pimpl);
   return *this;
}

Connection connect(const slot_type &slot, connect_position position = at_back)
{
   return (*m_pimpl).connect(slot, position);
}

connection connect(const group_type &group,
                   const slot_type &slot, connect_position position = at_back)
{
   return (*m_pimpl).connect(group, slot, position);
}

connection connect_extended(const extended_slot_type &slot, connect_position position = at_back)
{
   return (*m_pimpl).connect_extended(slot, position);
}

connection connect_extended(const group_type &group,
                            const extended_slot_type &slot, connect_position position = at_back)
{
   return (*m_pimpl).connect_extended(group, slot, position);
}

void disconnect_all_slots()
{
   (*m_pimpl).disconnect_all_slots();
}

void disconnect(const group_type &group)
{
   (*m_pimpl).disconnect(group);
}

template <typename T>
void disconnect(const T &slot)
{
   (*m_pimpl).disconnect(slot);
}

result_type operator ()(Args ... args)
{
   return (*m_pimpl)(args...);
}

result_type operator ()(Args ... args) const
{
   return (*m_pimpl)(args...);
}

std::size_t num_slots() const
{
   return (*m_pimpl).num_slots();
}

bool empty() const
{
   return (*m_pimpl).empty();
}

combiner_type combiner() const
{
   return (*m_pimpl).combiner();
}

void set_combiner(const combiner_type &combiner_arg)
{
   return (*_pimpl).set_combiner(combiner_arg);
}

void swap(Signal &other)
{
   using std::swap;
   swap(m_pimpl, other.m_pimpl);
}

protected:
virtual std::shared_ptr<void> lock_pimpl() const
{
   return m_pimpl;
}

private:
std::shared_ptr<impl_class> m_pimpl;
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
