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
#include <tuple>

namespace pdk {
namespace stdext {
namespace internal {

template <int...>
struct IndexesList {};

template <typename IndexList, int Right>
struct IndexesAppend;

template <int... Left, int Right>
struct IndexesAppend<IndexesList<Left...>, Right>
{
   typedef IndexesList<Left..., Right> Value;
};

template <int N> struct Indexes
{
   typedef typename IndexesAppend<typename Indexes<N - 1>::Value, N - 1>::Value Value;
};

template <>
struct Indexes<0>
{
   typedef IndexesList<> Value;
};

/*
      trick to set the return value of a slot that works even if the signal or the slot returns void
      to be used like     function(), ApplyReturnValue<ReturnType>(&return_value)
      if function() returns a value, the operator,(T, ApplyReturnValue<ReturnType>) is called, but if it
      returns void, the builtin one is used without an error.
   */
template <typename T>
struct ApplyReturnValue {
   void *m_data;
   explicit ApplyReturnValue(void *data) 
      : m_data(data)
   {}
};

template<typename T, typename U>
void operator,(T &&value, const ApplyReturnValue<U> &container)
{
   if (container.m_data) {
      *reinterpret_cast<U *>(container.m_data) = std::forward<T>(value);
   }
}

template<typename T>
void operator,(T, const ApplyReturnValue<void> &)
{}

template <typename, typename, typename, typename> struct FunctorCall;
template <int... ArgIndex, typename... SignalArgs, typename R, typename Function>
struct FunctorCall<IndexesList<ArgIndex...>, std::tuple<SignalArgs...>, R, Function>
{
   static void call(Function &func, void **args)
   {
      func((*reinterpret_cast<typename std::remove_reference<SignalArgs>::type *>(args[ArgIndex + 1]))...), ApplyReturnValue<R>(args[0]);
   }
};

template <int... ArgIndex, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
struct FunctorCall<IndexesList<ArgIndex...>, std::tuple<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...)>
{
   static void call(SlotRet (Obj::*func)(SlotArgs...), Obj *object, void **args) {
      (object->*func)((*reinterpret_cast<typename std::remove_reference<SignalArgs>::type *>(args[ArgIndex + 1]))...), ApplyReturnValue<R>(args[0]);
   }
};

template <int... ArgIndex, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
struct FunctorCall<IndexesList<ArgIndex...>, std::tuple<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...) const>
{
   static void call(SlotRet (Obj::*func)(SlotArgs...) const, Obj *object, void **args) {
      (object->*func)((*reinterpret_cast<typename std::remove_reference<SignalArgs>::type *>(args[ArgIndex + 1]))...), ApplyReturnValue<R>(args[0]);
   }
};

template <int... ArgIndex, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
struct FunctorCall<IndexesList<ArgIndex...>, std::tuple<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...) noexcept>
{
   static void call(SlotRet (Obj::*func)(SlotArgs...) noexcept, Obj *object, void **args) {
      (object->*func)((*reinterpret_cast<typename std::remove_reference<SignalArgs>::type *>(args[ArgIndex + 1]))...), ApplyReturnValue<R>(args[0]);
   }
};

template <int... ArgIndex, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
struct FunctorCall<IndexesList<ArgIndex...>, std::tuple<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...) const noexcept>
{
   static void call(SlotRet (Obj::*func)(SlotArgs...) const noexcept, Obj *object, void **args) {
      (object->*func)((*reinterpret_cast<typename std::remove_reference<SignalArgs>::type *>(args[ArgIndex + 1]))...), ApplyReturnValue<R>(args[0]);
   }
};

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

using internal::Indexes;
using internal::FunctorCall;

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

template<class Obj, typename Ret, typename... Args>
struct FunctionPointer<Ret (Obj::*) (Args...)>
{
   using ObjectType = Obj;
   using ReturnType = Ret;
   using Function = Ret (Obj::*) (Args...);
   using ArgTypes = std::tuple<Args...>;
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, ArgTypes>::type;
   };
   template <typename SignalArgs, typename R>
   static void call(Function func, Obj *object, void **args)
   {
      FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(func, object, args);
   }
};

template<class Obj, typename Ret, typename... Args>
struct FunctionPointer<Ret (Obj::*) (Args...) const>
{
   using ObjectType = Obj;
   using ReturnType = Ret;
   using Function = Ret (Obj::*) (Args...) const;
   using ArgTypes = std::tuple<Args...>;
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, ArgTypes>::type;
   };
   
   template <typename SignalArgs, typename R>
   static void call(Function func, Obj *object, void **args)
   {
      FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(func, object, args);
   }
};

template<typename Ret, typename... Args>
struct FunctionPointer<Ret (*) (Args...)>
{
   using ReturnType = Ret;
   using Function = Ret (*) (Args...);
   using ArgTypes = std::tuple<Args...>;
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = false};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, ArgTypes>::type;
   };
   
   template <typename SignalArgs, typename R>
   static void call(Function func, void *, void **args)
   {
      FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(func, args);
   }
};

template<class Obj, typename Ret, typename... Args>
struct FunctionPointer<Ret (Obj::*) (Args...) noexcept>
{
   using ObjectType = Obj;
   using ReturnType = Ret;
   using Function = Ret (Obj::*) (Args...) noexcept;
   using ArgTypes = std::tuple<Args...>;
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, ArgTypes>::type;
   };
   
   template <typename SignalArgs, typename R>
   static void call(Function func, Obj *object, void **args)
   {
      FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(func, object, args);
   }
};

template<class Obj, typename Ret, typename... Args>
struct FunctionPointer<Ret (Obj::*) (Args...) const noexcept>
{
   using ObjectType = Obj;
   using ReturnType = Ret;
   using ArgTypes = std::tuple<Args...>;
   using Function = Ret (Obj::*) (Args...) const noexcept;
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, ArgTypes>::type;
   };
   
   template <typename SignalArgs, typename R>
   static void call(Function func, Obj *object, void **args)
   {
      FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(func, object, args);
   }
};

template<typename Ret, typename... Args>
struct FunctionPointer<Ret (*) (Args...) noexcept>
{
   using ReturnType = Ret;
   using Function = Ret (*) (Args...) noexcept;
   using ArgTypes = std::tuple<Args...>;
   enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = false};
   template <size_t index>
   struct arg
   {
      using type = typename std::tuple_element<index, ArgTypes>::type;
   };
   
   template <typename SignalArgs, typename R>
   static void call(Function func, void *, void **args)
   {
      FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(func, args);
   }
};

template<typename Function, int N> struct Functor
{
   template <typename SignalArgs, typename R>
   static void call(Function &func, void *, void **args)
   {
      FunctorCall<typename Indexes<N>::Value, SignalArgs, R, Function>::call(func, args);
   }
};

} // stdext
} // pdk

#endif // PDK_STDEXT_TYPE_TRAITS_FUNCTION_TRAITS_H
