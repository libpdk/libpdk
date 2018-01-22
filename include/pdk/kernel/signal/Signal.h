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

#include "pdk/kernel/signal/internal/SignalPrivate.h"
#include "pdk/kernel/signal/LastValue.h"
#include "pdk/kernel/signal/DummyMutex.h"
#include "pdk/kernel/signal/SharedConnectionBlock.h"

namespace pdk {
namespace kernel {
namespace signal {

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
