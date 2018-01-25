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
// Created by softboy on 2018/01/25.

#ifndef PDK_KERNEL_OBJECT_PRIVATE_H
#define PDK_KERNEL_OBJECT_PRIVATE_H

#include <tuple>

namespace pdk {
namespace kernel {
namespace internal {

template<typename Func> struct FunctionPointer
{ 
   static constexpr int ArgumentCount = -1;
   static constexpr bool IsPointerToMemberFunction = false;
};

template<class ObjType, typename Ret, typename... Args>
struct FunctionPointer<Ret (ObjType::*) (Args...)>
{
   using ObjectType = ObjType;
   using Arguments = std::tuple<Args...>;
   using ReturnType = Ret;
   using Function = Ret (ObjType::*) (Args...);
   template <std::size_t N>
   using ArgType = typename std::tuple_element<N, std::tuple<Args...>>::type;
   static constexpr int ArgumentCount = sizeof...(Args);
   static constexpr bool IsPointerToMemberFunction = true;
};

template<class ObjType, typename Ret, typename... Args>
struct FunctionPointer<Ret (ObjType::*) (Args...) const>
{
   using ObjectType = ObjType;
   using Arguments = std::tuple<Args...>;
   using ReturnType = Ret;
   using Function = Ret (ObjType::*) (Args...) const;
   template <std::size_t N>
   using ArgType = typename std::tuple_element<N, std::tuple<Args...>>::type;
   static constexpr int ArgumentCount = sizeof...(Args);
   static constexpr bool IsPointerToMemberFunction = true;
};

template<typename Ret, typename... Args>
struct FunctionPointer<Ret (*) (Args...)>
{
   using Arguments = std::tuple<Args...>;
   using ReturnType = Ret;
   using Function = Ret (*) (Args...);
   template <std::size_t N>
   using ArgType = typename std::tuple_element<N, std::tuple<Args...>>::type;
   static constexpr int ArgumentCount = sizeof...(Args);
   static constexpr bool IsPointerToMemberFunction = false;
};

template<class ObjType, typename Ret, typename... Args>
struct FunctionPointer<Ret (ObjType::*) (Args...) noexcept>
{
   using ObjectType = ObjType;
   using Arguments = std::tuple<Args...>;
   using ReturnType = Ret;
   using Function = Ret (ObjType::*) (Args...) noexcept;
   template <class Base>
   struct ChangeClass
   {
      using Type = Ret (Base:: *)(Args...) noexcept;
   };
   template <std::size_t N>
   using ArgType = typename std::tuple_element<N, std::tuple<Args...>>::type;
   static constexpr int ArgumentCount = sizeof...(Args);
   static constexpr bool IsPointerToMemberFunction = true;
};

template<class ObjType, typename Ret, typename... Args>
struct FunctionPointer<Ret (ObjType::*) (Args...) const noexcept>
{
   using ObjectType = ObjType;
   using Arguments = std::tuple<Args...>;
   using ReturnType = Ret;
   using Function = Ret (ObjType::*) (Args...) const noexcept;
   template <class Base>
   struct ChangeClass
   {
      using Type = Ret (Base:: *)(Args...) const noexcept;
   };
   template <std::size_t N>
   using ArgType = typename std::tuple_element<N, std::tuple<Args...>>::type;
   static constexpr int ArgumentCount = sizeof...(Args);
   static constexpr bool IsPointerToMemberFunction = true;
};

template<typename Ret, typename... Args>
struct FunctionPointer<Ret (*) (Args...) noexcept>
{
   using Arguments = std::tuple<Args...>;
   using ReturnType = Ret;
   using Function = Ret (*) (Args...) noexcept;
   template <std::size_t N>
   using ArgType = typename std::tuple_element<N, std::tuple<Args...>>::type;
   static constexpr int ArgumentCount = sizeof...(Args);
   static constexpr bool IsPointerToMemberFunction = false;
};

} // internal
} // kernel
} // pdk

#endif // PDK_KERNEL_FUNCTION_TRAITS_H
