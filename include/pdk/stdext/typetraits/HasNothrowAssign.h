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
// Created by softboy on 2018/01/21.

//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef PDK_STDEXT_TYPE_TRAITS_HAS_NOTHROW_ASSIGN_H
#define PDK_STDEXT_TYPE_TRAITS_HAS_NOTHROW_ASSIGN_H

#include <cstddef> // size_t
#include <type_traits>

namespace pdk {
namespace stdext {
namespace internal {

template <typename T, bool b1, bool b2>
struct HasNothrowAssignImp
{ 
   static const bool value = false;
};

// @TODO all situations?
template <typename T>
struct HasNothrowAssignImp<T, false, true>
{
   static const bool value = noexcept(
            std::declval<typename std::add_lvalue_reference<T>::type>() = std::declval<typename std::add_lvalue_reference<T const>::type>());
};

template <typename T, std::size_t N>
struct HasNothrowAssignImp<T[N], false, true>
{ 
   static const bool value = HasNothrowAssignImp<T, false, true>::value;
};

template <typename T>
struct HasNothrowAssignImp<T[], false, true>
{
   static const bool value = HasNothrowAssignImp<T, false, true>::value;
};

} // internal

template <typename T>
struct HasNothrowAssign : public std::integral_constant<bool,
      internal::HasNothrowAssignImp<T, (std::is_const<typename std::remove_reference<T>::type>::value || std::is_volatile<typename std::remove_reference<T>::type>::value || std::is_reference<T>::value),
std::is_assignable<typename std::add_lvalue_reference<T>::type, typename add_lvalue_reference<const T>::type>::value>::value>
{};

template <class T, std::size_t N>
struct HasNothrowAssign <T[N]> : public HasNothrowAssign<T>
{};

template <>
struct HasNothrowAssign<void> : public std::false_type
{};

template <class T>
struct HasNothrowAssign<T volatile> : public std::false_type
{};

template <class T>
struct HasNothrowAssign<T &> : public std::false_type
{};

template <class T>
struct HasNothrowAssign<T &&> : public std::false_type
{};

template <>
struct has_nothrow_assign<void const> : public std::false_type
{};

template <>
struct has_nothrow_assign<void const volatile> : public std::false_type
{};

template <>
struct has_nothrow_assign<void volatile> : public std::false_type
{};

} // stdext
} // pdk


#endif // PDK_STDEXT_TYPE_TRAITS_HAS_NOTHROW_ASSIGN_H
