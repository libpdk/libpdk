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
// Created by softboy on 2018/01/16.

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

#ifndef PDK_STDEXT_PREPROCESSOR_REPETITION_ENUM_BINARY_PARAMS_H
#define PDK_STDEXT_PREPROCESSOR_REPETITION_ENUM_BINARY_PARAMS_H

#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/punctuation/CommaIf.h"
#include "pdk/stdext/preprocessor/repetition/Repeat.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_ENUM_BINARY_PARAMS
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_ENUM_BINARY_PARAMS(count, p1, p2) PDK_PP_REPEAT(count, PDK_PP_ENUM_BINARY_PARAMS_M, (p1, p2))
# else
#    define PDK_PP_ENUM_BINARY_PARAMS(count, p1, p2) PDK_PP_ENUM_BINARY_PARAMS_I(count, p1, p2)
#    define PDK_PP_ENUM_BINARY_PARAMS_I(count, p1, p2) PDK_PP_REPEAT(count, PDK_PP_ENUM_BINARY_PARAMS_M, (p1, p2))
# endif

# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_ENUM_BINARY_PARAMS_M(z, n, pp) PDK_PP_ENUM_BINARY_PARAMS_M_IM(z, n, PDK_PP_TUPLE_REM_2 pp)
#    define PDK_PP_ENUM_BINARY_PARAMS_M_IM(z, n, im) PDK_PP_ENUM_BINARY_PARAMS_M_I(z, n, im)
# else
#    define PDK_PP_ENUM_BINARY_PARAMS_M(z, n, pp) PDK_PP_ENUM_BINARY_PARAMS_M_I(z, n, PDK_PP_TUPLE_ELEM(2, 0, pp), PDK_PP_TUPLE_ELEM(2, 1, pp))
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()
#    define PDK_PP_ENUM_BINARY_PARAMS_M_I(z, n, p1, p2) PDK_PP_ENUM_BINARY_PARAMS_M_II(z, n, p1, p2)
#    define PDK_PP_ENUM_BINARY_PARAMS_M_II(z, n, p1, p2) PDK_PP_COMMA_IF(n) p1 ## n p2 ## n
# else
#    define PDK_PP_ENUM_BINARY_PARAMS_M_I(z, n, p1, p2) PDK_PP_COMMA_IF(n) PDK_PP_CAT(p1, n) PDK_PP_CAT(p2, n)
# endif

// PDK_PP_ENUM_BINARY_PARAMS_Z
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_ENUM_BINARY_PARAMS_Z(z, count, p1, p2) PDK_PP_REPEAT_ ## z(count, PDK_PP_ENUM_BINARY_PARAMS_M, (p1, p2))
# else
#    define PDK_PP_ENUM_BINARY_PARAMS_Z(z, count, p1, p2) PDK_PP_ENUM_BINARY_PARAMS_Z_I(z, count, p1, p2)
#    define PDK_PP_ENUM_BINARY_PARAMS_Z_I(z, count, p1, p2) PDK_PP_REPEAT_ ## z(count, PDK_PP_ENUM_BINARY_PARAMS_M, (p1, p2))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_REPETITION_ENUM_BINARY_PARAMS_H
