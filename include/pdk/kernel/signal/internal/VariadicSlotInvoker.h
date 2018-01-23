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

// Copyright Frank Mori Hess 2009
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_VARIADIC_SLOT_INVOKER_H
#define PDK_KERNEL_SIGNAL_INTERNAL_VARIADIC_SLOT_INVOKER_H

#include "pdk/global/Global.h"
#include "pdk/kernel/signal/internal/VariadicArgType.h"
#include "pdk/stdext/utility/DisableIf.h"
#include <tuple>

#ifdef PDK_CC_MSVC
#pragma warning(push)
#if  PDK_CC_MSVC >= 1800
#pragma warning(disable:4100)
#endif
#endif

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

template<unsigned ... values> 
class UnsignedMetaArray
{};

template<typename UnsignedMetaArray, unsigned n>
class UnsignedMetaArrayAppender;

template<unsigned n, unsigned ... Args>
class UnsignedMetaArrayAppender<UnsignedMetaArray<Args...>, n>
{
public:
   using type = UnsignedMetaArray<Args..., n>;
};

template<unsigned n> 
class MakeUnsignedMetaArray;

template<> 
class MakeUnsignedMetaArray<0>
{
public:
   using type = UnsignedMetaArray<>;
};

template<> 
class MakeUnsignedMetaArray<1>
{
public:
   using type = UnsignedMetaArray<0>;
};

template<unsigned n>
class MakeUnsignedMetaArray
{
public:
   using type = typename UnsignedMetaArrayAppender<typename MakeUnsignedMetaArray<n - 1>::type, n - 1>::type;
};

template<typename R>
class CallWithTupleArgs
{
public:
   using ResultType = R;
   using result_type = ResultType;
   
   template<typename Func, typename ... Args, std::size_t N>
   R operator()(Func &func, const std::tuple<Args...> & args, std::integral_constant<std::size_t, N>) const
   {
      typedef typename MakeUnsignedMetaArray<N>::type indices_type;
      return invoke<Func>(func, indices_type(), args);
   }
private:
   template<typename Func, unsigned ... indices, typename ... Args>
   R invoke(Func &func, UnsignedMetaArray<indices...>, const std::tuple<Args...> &args,
            typename pdk::stdext::DisableIf<std::is_void<typename Func::result_type>::value>::type * = nullptr
         ) const
   {
      return func(std::get<indices>(args)...);
   }
   
   template<typename Func, unsigned ... indices, typename ... Args>
   R invoke(Func &func, UnsignedMetaArray<indices...>, const std::tuple<Args...> &args,
            typename std::enable_if<std::is_void<typename Func::result_type>::value>::type * = nullptr
         ) const
   {
      func(std::get<indices>(args)...);
      return R();
   }
   
   // This overload is redundant, as it is the same as the previous variadic method when
   // it has zero "indices" or "Args" variadic template parameters.  This overload
   // only exists to quiet some unused parameter warnings
   // on certain compilers (some versions of gcc and msvc)
   template<typename Func>
   R invoke(Func &func, UnsignedMetaArray<>, const std::tuple<> &, 
            typename std::enable_if<std::is_void<typename Func::ResultType>::value>::type * = 0
         ) const
   {
      func();
      return R();
   }
};

template<typename R, typename ... Args>
class VariadicSlotInvoker
{
public:
   using ResultType = R;
   using result_type = ResultType;
   
   VariadicSlotInvoker(Args & ... args)
      : m_args(args...)
   {}
   
   template<typename ConnectionBodyType>
   ResultType operator ()(const ConnectionBodyType &connectionBody) const
   {
      return CallWithTupleArgs<ResultType>()(connectionBody->slot().slotFunc(), 
                                              m_args, std::integral_constant<size_t, sizeof...(Args)>());
   }
private:
   std::tuple<Args& ...> m_args;
};

} // internal
} // signal
} // kernel
} // pdk

#ifdef PDK_CC_MSVC
#pragma warning(pop)
#endif

#endif // PDK_KERNEL_SIGNAL_INTERNAL_VARIADIC_SLOT_INVOKER_H
