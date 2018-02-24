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
// Created by softboy on 2018/01/22.

//  Copyright 2000 John Maddock (john@johnmaddock.co.uk)
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef PDK_STDEXT_TYPE_TRAITS_FUNCTION_TRAITS_H
#define PDK_STDEXT_TYPE_TRAITS_FUNCTION_TRAITS_H

#include <type_traits>

namespace pdk {
namespace stdext {
namespace internal {

template<typename Function>
struct FunctionTraitsHelper;

template<typename R>
struct FunctionTraitsHelper<R (*)(void)>
{
   static const unsigned arity = 0;
   using ResultType = R;
};

template<typename R, typename T1>
struct FunctionTraitsHelper<R (*)(T1)>
{
   static const unsigned arity = 1;
   using ResultType = R;
   using ArgType1 = T1;
};

template<typename R, typename T1, typename T2>
struct FunctionTraitsHelper<R (*)(T1, T2)>
{
   static const unsigned arity = 2;
   using ResultType = R;
   using ArgType1 = T1;
   using ArgType2 = T2;
};

template<typename R, typename T1, typename T2, typename T3>
struct FunctionTraitsHelper<R (*)(T1, T2, T3)>
{
   static const unsigned arity = 3;
   using ResultType = R;
   using ArgType1 = T1;
   using ArgType2 = T2;
   using ArgType3 = T3;
};

template<typename R, typename T1, typename T2, typename T3, typename T4>
struct FunctionTraitsHelper<R (*)(T1, T2, T3, T4)>
{
   static const unsigned arity = 4;
   using ResultType = R;
   using ArgType1 = T1;
   using ArgType2 = T2;
   using ArgType3 = T3;
   using ArgType4 = T4;
};

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
struct FunctionTraitsHelper<R (*)(T1, T2, T3, T4, T5)>
{
   static const unsigned arity = 5;
   using ResultType = R;
   using ArgType1 = T1;
   using ArgType2 = T2;
   using ArgType3 = T3;
   using ArgType4 = T4;
   using ArgType5 = T5;
};

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct FunctionTraitsHelper<R (*)(T1, T2, T3, T4, T5, T6)>
{
   static const unsigned arity = 6;
   using ResultType = R;
   using ArgType1 = T1;
   using ArgType2 = T2;
   using ArgType3 = T3;
   using ArgType4 = T4;
   using ArgType5 = T5;
   using ArgType6 = T6;
};

template<typename R, typename T1, typename T2, typename T3, typename T4, 
         typename T5, typename T6, typename T7>
struct FunctionTraitsHelper<R (*)(T1, T2, T3, T4, T5, T6, T7)>
{
   static const unsigned arity = 7;
   using ResultType = R;
   using ArgType1 = T1;
   using ArgType2 = T2;
   using ArgType3 = T3;
   using ArgType4 = T4;
   using ArgType5 = T5;
   using ArgType6 = T6;
   using ArgType7 = T7;
};

template<typename R, typename T1, typename T2, typename T3, typename T4, 
         typename T5, typename T6, typename T7, typename T8>
struct FunctionTraitsHelper<R (*)(T1, T2, T3, T4, T5, T6, T7, T8)>
{
   static const unsigned arity = 8;
   using ResultType = R;
   using ArgType1 = T1;
   using ArgType2 = T2;
   using ArgType3 = T3;
   using ArgType4 = T4;
   using ArgType5 = T5;
   using ArgType6 = T6;
   using ArgType7 = T7;
   using ArgType8 = T8;
};

template<typename R, typename T1, typename T2, typename T3, typename T4, 
         typename T5, typename T6, typename T7, typename T8, typename T9>
struct FunctionTraitsHelper<R (*)(T1, T2, T3, T4, T5, T6, T7, T8, T9)>
{
   static const unsigned arity = 9;
   using ResultType = R;
   using ArgType1 = T1;
   using ArgType2 = T2;
   using ArgType3 = T3;
   using ArgType4 = T4;
   using ArgType5 = T5;
   using ArgType6 = T6;
   using ArgType7 = T7;
   using ArgType8 = T8;
   using ArgType9 = T9;
};

template<typename R, typename T1, typename T2, typename T3, typename T4, 
         typename T5, typename T6, typename T7, typename T8, typename T9,
         typename T10>
struct FunctionTraitsHelper<R (*)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)>
{
   static const unsigned arity = 10;
   using ResultType = R;
   using ArgType1 = T1;
   using ArgType2 = T2;
   using ArgType3 = T3;
   using ArgType4 = T4;
   using ArgType5 = T5;
   using ArgType6 = T6;
   using ArgType7 = T7;
   using ArgType8 = T8;
   using ArgType9 = T9;
   using ArgType10 = T10;
};

} // internal

template<typename Function>
struct FunctionTraits : 
      public internal::FunctionTraitsHelper<typename std::add_pointer<Function>::type>
{};

template<typename Func> 
struct FunctionPointer 
{ 
   enum {
      ArgumentCount = -1, 
      IsPointerToMemberFunction = false
   }; 
};

template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...)>
{
   typedef Obj Object;
   typedef Ret ReturnType;
   typedef Ret (Obj::*Function) (Args...);
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<Args...>>::type;
   };
};

template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...) const>
{
   typedef Obj Object;
   typedef Ret ReturnType;
   typedef Ret (Obj::*Function) (Args...) const;
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<Args...>>::type;
   };
};

template<typename Ret, typename... Args> struct FunctionPointer<Ret (*) (Args...)>
{
   typedef Ret ReturnType;
   typedef Ret (*Function) (Args...);
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = false};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<Args...>>::type;
   };
};

template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...) noexcept>
{
   typedef Obj Object;
   typedef Ret ReturnType;
   typedef Ret (Obj::*Function) (Args...) noexcept;
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<Args...>>::type;
   };
};
template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...) const noexcept>
{
   typedef Obj Object;
   typedef Ret ReturnType;
   typedef Ret (Obj::*Function) (Args...) const noexcept;
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<Args...>>::type;
   };
};

template<typename Ret, typename... Args> struct FunctionPointer<Ret (*) (Args...) noexcept>
{
   typedef Ret ReturnType;
   typedef Ret (*Function) (Args...) noexcept;
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = false};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, std::tuple<Args...>>::type;
   };
};

} // stdext
} // pdk

#endif // PDK_STDEXT_TYPE_TRAITS_FUNCTION_TRAITS_H
