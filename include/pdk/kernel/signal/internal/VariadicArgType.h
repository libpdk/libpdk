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

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_VARIADIC_ARG_TYPE_H
#define PDK_KERNEL_SIGNAL_INTERNAL_VARIADIC_ARG_TYPE_H

#include <functional>

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

template<unsigned, typename ... Args> 
class VariadicArgType;

template<typename T, typename ... Args>
class VariadicArgType<0, T, Args...>
{
public:
   using type = T;
};

template<unsigned n, typename T, typename ... Args> 
class VariadicArgType<n, T, Args...>
{
public:
   using type = typename VariadicArgType<n - 1, Args...>::type;
};

template <typename ... Args>
struct StdFuncBase
{};

template <typename T1>
struct StdFuncBase<T1>
{
   using ArgType = T1;
};

template <typename T1, typename T2>
struct StdFuncBase<T1, T2>
{
   using FirstArgType = T1;
   using SecondArgType = T2;
};

} // internal
} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_VARIADIC_ARG_TYPE_H
