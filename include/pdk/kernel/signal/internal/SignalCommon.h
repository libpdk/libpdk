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

// Boost.Signals library

// Copyright Douglas Gregor 2001-2004.
// Copyright Frank Mori Hess 2007. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_COMMON_H
#define PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_COMMON_H

#include "pdk/kernel/signal/SignalBase.h"

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

// Determine if the given type T is a signal
template<typename T>
struct IsSignal: public std::integral_constant<bool, std::is_base_of<SignalBase, T>>
{};

// A slot can be a signal, a reference to a function object, or a
// function object.
struct SignalTag {};
struct ReferenceTag {};
struct ValueTag {};

// Classify the given slot as a signal, a reference-to-slot, or a
// standard slot
template<typename S>
class GetSlotTag {
   using SignalOrValue = typename std::conditional<IsSignal<S>::value,
   SignalTag, ValueTag>::type;
public:
   typedef typename mpl::if_<is_reference_wrapper<S>,
   reference_tag,
   signal_or_value>::type type;
   using type = typename std::conditional<>
};

} // internal
} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_SIGNAL_COMMON_H
