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
// Created by softboy on 2018/01/19.

/*
  Author: Frank Mori Hess <fmhess@users.sourceforge.net>
  Begin: 2007-01-23
*/
// Copyright Frank Mori Hess 2007-2008
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_COMMON_MACROS_H
#define PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_COMMON_MACROS_H

#define PDK_SIGNAL_SIGNAL_CLASS_NAME(arity) Signal
#define PDK_SIGNAL_WEAK_CLASS_NAME(arity) WeakSignal
#define PDK_SIGNAL_IMPL_CLASS_NAME(arity) SignalImpl
#define PDK_SIGNAL_SIGNATURE_TEMPLATE_DECL(arity) typename Signature
#define PDK_SIGNAL_ARGS_TEMPLATE_INSTANTIATION(arity) Args...
#define PDK_SIGNAL_SIGNATURE_TEMPLATE_INSTANTIATION(arity) R (Args...)
#define PDK_SIGNAL_SIGNATURE_FUNCTION_TYPE(arity) R (Args...)
#define PDK_SIGNAL_ARGS_TEMPLATE_DECL(arity) typename ... Args
#define PDK_SIGNAL_FULL_FORWARD_ARGS(arity) Args && ... args
#define PDK_SIGNAL_FORWARDED_ARGS(arity) std::forward<Args>(args)...
#define PDK_SIGNAL_SLOT_CLASS_NAME(arity) Slot
#define PDK_SIGNAL_EXTENDED_SLOT_TYPE(arity) Slot<R (const connection &, Args...), ExtendedSlotFunctionType>
#define PDK_SIGNAL_BOUND_EXTENDED_SLOT_FUNCTION_N(arity) BoundExtendedSlotFunction
#define PDK_SIGNAL_BOUND_EXTENDED_SLOT_FUNCTION_INVOKER_N(arity) BoundExtendedSlotFunctionInvoker
#define PDK_SIGNAL_FUNCTION_N_DECL(arity) std::function<Signature>
#define PDK_SIGNAL_PREFIXED_SIGNATURE_TEMPLATE_DECL(arity, prefix) typename prefixSignature
#define PDK_SIGNAL_PREFIXED_SIGNATURE_TEMPLATE_INSTANTIATION(arity, prefix) prefixSignature
#define PDK_SIGNAL_SIGNATURE_FULL_ARGS(arity) Args ... args
#define PDK_SIGNAL_SIGNATURE_ARG_NAMES(arity) args...
#define PDK_SIGNAL_PORTABLE_SIGNATURE(arity, Signature) Signature

#define PDK_SIGNAL_SLOT_TEMPLATE_SPECIALIZATION_DECL(arity) \
  typename SlotFunction, \
  typename R, \
  typename ... Args
#define PDK_SIGNAL_SLOT_TEMPLATE_SPECIALIZATION \
   <R (Args...), SlotFunction>

#define PDK_SIGNAL_SIGNAL_TEMPLATE_DECL(arity) \
  typename Signature, \
  typename Combiner, \
  typename Group, \
  typename GroupCompare, \
  typename SlotFunction, \
  typename ExtendedSlotFunction, \
  typename Mutex

#define PDK_SIGNAL_SIGNAL_TEMPLATE_DEFAULTED_DECL(arity) \
  typename Signature, \
  typename Combiner = OptionalLastValue<typename pdk::stdext::FunctionTraits<Signature>::ResultType>, \
  typename Group = int, \
  typename GroupCompare = std::less<Group>, \
  typename SlotFunction = boost::function<Signature>, \
  typename ExtendedSlotFunction = typename internal::VariadicExtendedSignature<Signature>::FunctionType, \
  typename Mutex = pdk::kernel::signal::Mutex

#define PDK_SIGNAL_SIGNAL_TEMPLATE_SPECIALIZATION_DECL(arity) \
  typename Combiner, \
  typename Group, \
  typename GroupCompare, \
  typename SlotFunction, \
  typename ExtendedSlotFunction, \
  typename Mutex, \
  typename R, \
  typename ... Args

#define PDK_SIGNAL_SIGNAL_TEMPLATE_SPECIALIZATION <\
  R (Args...), \
  Combiner, \
  Group, \
  GroupCompare, \
  SlotFunction, \
  ExtendedSlotFunction, \
  Mutex>

#define PDK_SIGNAL_STD_FUNCTIONAL_BASE \
  StdFunctionalBase<Args...>

#define PDK_SIGNAL_PP_COMMA_IF(arity) ,

#endif // PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_COMMON_MACROS_H
