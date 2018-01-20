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
// Created by softboy on 2017/01/20.

//
//  ref.hpp - ref/cref, useful helper functions
//
//  Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//  Copyright (C) 2001, 2002 Peter Dimov
//  Copyright (C) 2002 David Abrahams
//
//  Copyright (C) 2014 Glen Joseph Fernandes
//  glenfe at live dot com
//  Copyright (C) 2014 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/core/doc/html/core/ref.html for documentation.
//
#ifndef PDK_STDEXT_CORE_REF_H
#define PDK_STDEXT_CORE_REF_H

#include <functional>

namespace pdk {
namespace stdext {

template<typename T>
struct IsReferenceWrapper
{
   static const bool value = false;
};

template<typename T>
struct IsReferenceWrapper<std::reference_wrapper<T>>
{
   static const bool value = true;
};

template<typename T>
struct IsReferenceWrapper<std::reference_wrapper<T> const>
{
   static const bool value = true;
};

template<typename T>
struct IsReferenceWrapper<std::reference_wrapper<T> volatile>
{
   static const bool value = true;
};

template<typename T>
struct IsReferenceWrapper<std::reference_wrapper<T> const volatile>
{
   static const bool value = true;
};

} // stdext
} // pdk

#endif // PDK_STDEXT_CORE_REF_H

