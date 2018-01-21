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

#ifndef PDK_STDEXT_HAS_NOTHROW_CONSTRUCTOR_H
#define PDK_STDEXT_HAS_NOTHROW_CONSTRUCTOR_H

#include <cstddef> // size_t
#include <type_traits>

#ifdef PDK_CC_MSVC
#pragma warning(push)
#pragma warning(disable:4197) // top-level volatile in cast is ignored
#endif

namespace pdk {
namespace stdext {
namespace internal {

template <typename T, bool b>
struct HasNothrowConstructorImp : public std::integral_constant<bool, false>
{};

template <typename T>
struct HasNothrowConstructorImp<T, true> : public std::integral_constant<bool, noexcept(T())>
{};

template <typename T, std::size_t N>
struct HasNothrowConstructorImp<T[N], true> : public HasNothrowConstructorImp<T, true>
{};

} // internal

template <class T>
struct HasNothrowConstructor : public internal::HasNothrowConstructorImp<T, std::is_default_constructible<T>::value>
{};

} // stdext
} // pdk

#ifdef PDK_CC_MSVC
#pragma warning(pop)
#endif

#endif // PDK_STDEXT_HAS_NOTHROW_CONSTRUCTOR_H
