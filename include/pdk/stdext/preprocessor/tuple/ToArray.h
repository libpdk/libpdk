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
// Created by softboy on 2018/01/15.

// ****************************************************************************
// *                                                                          *
// *     (C) Copyright Paul Mensonides 2002-2011.                             *
// *     (C) Copyright Edward Diener 2011.                                    *
// *     Distributed under the Boost Software License, Version 1.0. (See      *
// *     accompanying file LICENSE_1_0.txt or copy at                         *
// *     http://www.boost.org/LICENSE_1_0.txt)                                *
// *                                                                          *
// ****************************************************************************
// 
// See http://www.boost.org for most recent version.

#ifndef PDK_STDEXT_PREPROCESSOR_TUPLE_TO_ARRAY_H
#define PDK_STDEXT_PREPROCESSOR_TUPLE_TO_ARRAY_H

#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/facilities/Overload.h"
#include "pdk/stdext/preprocessor/variadic/Size.h"
#include "pdk/stdext/preprocessor/tuple/Size.h"
#include "pdk/stdext/preprocessor/variadic/Size.h"

// PDK_PP_TUPLE_TO_ARRAY
# if PDK_PP_VARIADICS
#    if PDK_PP_VARIADICS_MSVC
#        define PDK_PP_TUPLE_TO_ARRAY(...) PDK_PP_TUPLE_TO_ARRAY_I(PDK_PP_OVERLOAD(PDK_PP_TUPLE_TO_ARRAY_, __VA_ARGS__), (__VA_ARGS__))
#        define PDK_PP_TUPLE_TO_ARRAY_I(m, args) PDK_PP_TUPLE_TO_ARRAY_II(m, args)
#        define PDK_PP_TUPLE_TO_ARRAY_II(m, args) PDK_PP_CAT(m ## args,)
#        define PDK_PP_TUPLE_TO_ARRAY_1(tuple) (PDK_PP_TUPLE_SIZE(tuple), tuple)
#    else
#        define PDK_PP_TUPLE_TO_ARRAY(...) PDK_PP_OVERLOAD(PDK_PP_TUPLE_TO_ARRAY_, __VA_ARGS__)(__VA_ARGS__)
#        define PDK_PP_TUPLE_TO_ARRAY_1(tuple) (PDK_PP_VARIADIC_SIZE tuple, tuple)
#    endif
#    define PDK_PP_TUPLE_TO_ARRAY_2(size, tuple) (size, tuple)
# else
#    define PDK_PP_TUPLE_TO_ARRAY(size, tuple) (size, tuple)
# endif

#endif // PDK_STDEXT_PREPROCESSOR_TUPLE_TO_ARRAY_H
