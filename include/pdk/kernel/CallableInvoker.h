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
// Created by softboy on 2018/03/16.

#ifndef PDK_KERNEL_CALLABLE_INVOKER_H
#define PDK_KERNEL_CALLABLE_INVOKER_H

#include "pdk/global/Global.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/kernel/CoreApplication.h"
#include <any>
#include <tuple>

namespace pdk {
namespace kernel {

class Object;

class PDK_CORE_EXPORT CallableInvoker
{
public:
   template <typename CallableType, typename ...ArgTypes>
   static void invoke(CallableType callable, ArgTypes&&... args)
   {
      callable(std::forward<ArgTypes>(args)...);
   }
   
   template <typename CallableType, typename ...ArgTypes>
   static void invokeAsync(CallableType callable, Object *receiver, ArgTypes&&... args)
   {
      using TupleType = std::tuple<ArgTypes...>;
      CoreApplication::postEvent(receiver, new internal::MetaCallEvent([&](std::any &arg) {
                                    std::apply(callable, std::any_cast<TupleType>(arg));
                                 }, std::any(std::make_tuple(std::forward<ArgTypes>(args)...))));
   }
};

} // kernel
} // pdk

#endif // PDK_KERNEL_CALLABLE_INVOKER_H
