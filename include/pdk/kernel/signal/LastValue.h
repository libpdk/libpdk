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

// last_value function object (documented as part of Boost.Signals)

// Copyright Frank Mori Hess 2007.
// Copyright Douglas Gregor 2001-2003. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_LAST_VALUE_H
#define PDK_KERNEL_SIGNAL_LAST_VALUE_H

#include <stdexcept>
#include <optional>
#include "pdk/kernel/signal/ExpiredSlot.h"

namespace pdk {
namespace kernel {
namespace signal {

class NoSlotException : public std::exception
{
public:
   virtual const char* what() const noexcept
   {
      return "pdk::kernel::signal::NoSlotError";
   }
};

template <typename T>
class LastValue
{
public:
   using ResultType = T;
   using result_type = ResultType;
private:
   using OptionalType = typename std::conditional<std::is_lvalue_reference<ResultType>::value, 
   std::reference_wrapper<typename std::remove_reference<ResultType>::type>, ResultType>::type;
public:
   template <typename InputIterator>
   T operator()(InputIterator first, InputIterator last) const
   {
      if (first == last) {
         throw NoSlotException();
      }
      std::optional<OptionalType> value;
      while (first != last) {
         try{
            value = *first;
         } catch(const ExpiredSlot &) {}
         ++first;
      }
      if (value) {
         return value.value();
      }
      throw NoSlotException();
   }
};

template <>
class LastValue<void>
{
public:
   using ResultType = void;
   using result_type = ResultType;
   
public:
   template <typename InputIterator>
   ResultType operator()(InputIterator first, InputIterator last) const
   {
      while (first != last) {
         try {
            *first;
         } catch(const ExpiredSlot &) {}
         ++first;
      }
      return;
   }
};

} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_LAST_VALUE_H
