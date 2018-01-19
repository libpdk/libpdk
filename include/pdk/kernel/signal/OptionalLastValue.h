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

// optional_last_value function object (documented as part of Boost.Signals2)

// Copyright Frank Mori Hess 2007-2008.
// Copyright Douglas Gregor 2001-2003.
// Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/signals2 for library home page.

#ifndef PDK_KERNEL_SIGNAL_OPTIONAL_LAST_VALUE_H
#define PDK_KERNEL_SIGNAL_OPTIONAL_LAST_VALUE_H

#include <optional>
#include "pdk/kernel/signal/ExpiredSlot.h"

namespace pdk {
namespace kernel {
namespace signal {

template <typename T>
class OptionalLastValue
{
public:
   using ResultType = std::optional<T>;
   using result_type = ResultType;
   
public:
   template <typename InputIterator>
   std::optional<T> operator ()(InputIterator first, InputIterator last) const
   {
      std::optional<T> value;
      while (first != last) {
         try {
            value = *first;
         } catch (const ExpiredSlot &)
         {}
         ++first;
      }
      return value;
   }
};

template <>
class OptionalLastValue<void>
{
public:
   using ResultType = void;
   using result_type = ResultType;
   
public:
   template <typename InputIterator>
   ResultType operator ()(InputIterator first, InputIterator last) const
   {
      while (first != last) {
         try {
            *first;
         } catch (const ExpiredSlot &) {}
         ++first;
      }
      return;
   }
};

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_OPTIONAL_LAST_VALUE_H

