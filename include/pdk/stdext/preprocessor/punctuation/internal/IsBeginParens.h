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

#ifndef PDK_STDEXT_PREPROCESSOR_PUNCTUATION_INTERNAL_IS_BEGIN_PARENS_H
#define PDK_STDEXT_PREPROCESSOR_PUNCTUATION_INTERNAL_IS_BEGIN_PARENS_H

#if PDK_PP_VARIADICS_MSVC

#include "pdk/stdext/preprocessor/facilities/Empty.h"

#define PDK_PP_DETAIL_VD_IBP_CAT(a, b) PDK_PP_DETAIL_VD_IBP_CAT_I(a, b)
#define PDK_PP_DETAIL_VD_IBP_CAT_I(a, b) PDK_PP_DETAIL_VD_IBP_CAT_II(a ## b)
#define PDK_PP_DETAIL_VD_IBP_CAT_II(res) res

#define PDK_PP_DETAIL_IBP_SPLIT(i, ...) \
   PDK_PP_DETAIL_VD_IBP_CAT(PDK_PP_DETAIL_IBP_PRIMITIVE_CAT(PDK_PP_DETAIL_IBP_SPLIT_,i)(__VA_ARGS__),PDK_PP_EMPTY()) \
   /**/

#define PDK_PP_DETAIL_IBP_IS_VARIADIC_C(...) 1 1

#else

#define PDK_PP_DETAIL_IBP_SPLIT(i, ...) \
   PDK_PP_DETAIL_IBP_PRIMITIVE_CAT(PDK_PP_DETAIL_IBP_SPLIT_,i)(__VA_ARGS__) \
   /**/

#define PDK_PP_DETAIL_IBP_IS_VARIADIC_C(...) 1

#endif /* PDK_PP_VARIADICS_MSVC */

#define PDK_PP_DETAIL_IBP_SPLIT_0(a, ...) a
#define PDK_PP_DETAIL_IBP_SPLIT_1(a, ...) __VA_ARGS__

#define PDK_PP_DETAIL_IBP_CAT(a, ...) PDK_PP_DETAIL_IBP_PRIMITIVE_CAT(a, __VA_ARGS__)
#define PDK_PP_DETAIL_IBP_PRIMITIVE_CAT(a, ...) a ## __VA_ARGS__

#define PDK_PP_DETAIL_IBP_IS_VARIADIC_R_1 1,
#define PDK_PP_DETAIL_IBP_IS_VARIADIC_R_PDK_PP_DETAIL_IBP_IS_VARIADIC_C 0,

#endif // PDK_STDEXT_PREPROCESSOR_PUNCTUATION_INTERNAL_IS_BEGIN_PARENS_H
