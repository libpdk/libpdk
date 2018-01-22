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

// Copyright Douglas Gregor 2001-2004.
// Copyright Frank Mori Hess 2007. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef PDK_KERNEL_SIGNAL_INTERNAL_RESULT_TYPE_WRAPPER_H
#define PDK_KERNEL_SIGNAL_INTERNAL_RESULT_TYPE_WRAPPER_H

namespace pdk {
namespace kernel {
namespace signal {
namespace internal {

// A placeholder for void on compilers that don't support void returns
struct VoidType
{};

// Replaces void with VoidType
template<typename R>
struct NonVoid
{
   using type = R;
};

template<>
struct NonVoid<void> 
{
   using type = VoidType;
};

// Replaces void with void_type only if compiler doesn't support void returns
template<typename R>
struct ResultTypeWrapper
{
   using type = R;
};

template<>
struct ResultTypeWrapper<void>
{
   using type = VoidType;
};

// specialization deals with possible void return from combiners
template<typename R> 
class CombinerInvoker
{
public:
   using result_type = R;
   using ResultType = result_type;
   template<typename Combiner, typename InputIterator>
   ResultType operator()(Combiner &combiner,
                         InputIterator first, InputIterator last) const
   {
      return combiner(first, last);
   }
};

template<>
class CombinerInvoker<void>
{
public:
   using result_type = ResultTypeWrapper<void>::type;
   using ResultType = result_type;
   
   template<typename Combiner, typename InputIterator>
   ResultType operator()(Combiner &combiner,
                         InputIterator first, InputIterator last) const
   {
      combiner(first, last);
      return ResultType();
   }
};

} // internal
} // signal
} // kernel
} // pdk

#endif // PDK_KERNEL_SIGNAL_INTERNAL_RESULT_TYPE_WRAPPER_H
