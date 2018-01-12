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
// Created by softboy on 2017/01/11.

// Boost enable_if library

// Copyright 2003 (c) The Trustees of Indiana University.

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//    Authors: Jaakko Jarvi (jajarvi at osl.iu.edu)
//             Jeremiah Willcock (jewillco at osl.iu.edu)
//             Andrew Lumsdaine (lums at osl.iu.edu)

#ifndef PDK_STDEXT_UTILITY_DISABLE_IF_H
#define PDK_STDEXT_UTILITY_DISABLE_IF_H

namespace pdk {
namespace stdext {

template <bool B, typename T = void>
struct DisableIfCond
{
   using type = T;
};

template <typename T>
struct DisableIfCond<true, T>
{};

template <bool Cond, typename T = void>
struct DisableIf : public DisableIfCond<Cond, T>
{};

template <bool B, typename T>
struct LazyDisableIfCond
{
   using type = typename T::type; 
};

template <typename T>
struct LazyDisableIfCond<true, T>
{};

template <bool Cond, typename T>
struct LazyDisableIf : public LazyDisableIfCond<Cond, T>
{};

} // stdext
} // pdk

#endif // PDK_STDEXT_UTILITY_DISABLE_IF_H
