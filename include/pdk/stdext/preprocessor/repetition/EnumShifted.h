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

#ifndef PDK_STDEXT_PREPROCESSOR_REPETITION_ENUM_SHIFTED_H
#define PDK_STDEXT_PREPROCESSOR_REPETITION_ENUM_SHIFTED_H

#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/arithmetic/Inc.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/debug/Error.h"
#include "pdk/stdext/preprocessor/internal/AutoRec.h"
#include "pdk/stdext/preprocessor/punctuation/CommaIf.h"
#include "pdk/stdext/preprocessor/repetition/Repeat.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_ENUM_SHIFTED
# if 0
#    define PDK_PP_ENUM_SHIFTED(count, macro, data)
# endif
#
# define PDK_PP_ENUM_SHIFTED PDK_PP_CAT(PDK_PP_ENUM_SHIFTED_, PDK_PP_AUTO_REC(PDK_PP_REPEAT_P, 4))
#
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_ENUM_SHIFTED_1(c, m, d) PDK_PP_REPEAT_1(PDK_PP_DEC(c), PDK_PP_ENUM_SHIFTED_M_1, (m, d))
#    define PDK_PP_ENUM_SHIFTED_2(c, m, d) PDK_PP_REPEAT_2(PDK_PP_DEC(c), PDK_PP_ENUM_SHIFTED_M_2, (m, d))
#    define PDK_PP_ENUM_SHIFTED_3(c, m, d) PDK_PP_REPEAT_3(PDK_PP_DEC(c), PDK_PP_ENUM_SHIFTED_M_3, (m, d))
# else
#    define PDK_PP_ENUM_SHIFTED_1(c, m, d) PDK_PP_ENUM_SHIFTED_1_I(c, m, d)
#    define PDK_PP_ENUM_SHIFTED_2(c, m, d) PDK_PP_ENUM_SHIFTED_1_2(c, m, d)
#    define PDK_PP_ENUM_SHIFTED_3(c, m, d) PDK_PP_ENUM_SHIFTED_1_3(c, m, d)
#    define PDK_PP_ENUM_SHIFTED_1_I(c, m, d) PDK_PP_REPEAT_1(PDK_PP_DEC(c), PDK_PP_ENUM_SHIFTED_M_1, (m, d))
#    define PDK_PP_ENUM_SHIFTED_2_I(c, m, d) PDK_PP_REPEAT_2(PDK_PP_DEC(c), PDK_PP_ENUM_SHIFTED_M_2, (m, d))
#    define PDK_PP_ENUM_SHIFTED_3_I(c, m, d) PDK_PP_REPEAT_3(PDK_PP_DEC(c), PDK_PP_ENUM_SHIFTED_M_3, (m, d))
# endif

# define PDK_PP_ENUM_SHIFTED_4(c, m, d) PDK_PP_ERROR(0x0003)

# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_ENUM_SHIFTED_M_1(z, n, md) PDK_PP_ENUM_SHIFTED_M_1_IM(z, n, PDK_PP_TUPLE_REM_2 md)
#    define PDK_PP_ENUM_SHIFTED_M_2(z, n, md) PDK_PP_ENUM_SHIFTED_M_2_IM(z, n, PDK_PP_TUPLE_REM_2 md)
#    define PDK_PP_ENUM_SHIFTED_M_3(z, n, md) PDK_PP_ENUM_SHIFTED_M_3_IM(z, n, PDK_PP_TUPLE_REM_2 md)
#    define PDK_PP_ENUM_SHIFTED_M_1_IM(z, n, im) PDK_PP_ENUM_SHIFTED_M_1_I(z, n, im)
#    define PDK_PP_ENUM_SHIFTED_M_2_IM(z, n, im) PDK_PP_ENUM_SHIFTED_M_2_I(z, n, im)
#    define PDK_PP_ENUM_SHIFTED_M_3_IM(z, n, im) PDK_PP_ENUM_SHIFTED_M_3_I(z, n, im)
# else
#    define PDK_PP_ENUM_SHIFTED_M_1(z, n, md) PDK_PP_ENUM_SHIFTED_M_1_I(z, n, PDK_PP_TUPLE_ELEM(2, 0, md), PDK_PP_TUPLE_ELEM(2, 1, md))
#    define PDK_PP_ENUM_SHIFTED_M_2(z, n, md) PDK_PP_ENUM_SHIFTED_M_2_I(z, n, PDK_PP_TUPLE_ELEM(2, 0, md), PDK_PP_TUPLE_ELEM(2, 1, md))
#    define PDK_PP_ENUM_SHIFTED_M_3(z, n, md) PDK_PP_ENUM_SHIFTED_M_3_I(z, n, PDK_PP_TUPLE_ELEM(2, 0, md), PDK_PP_TUPLE_ELEM(2, 1, md))
# endif

# define PDK_PP_ENUM_SHIFTED_M_1_I(z, n, m, d) PDK_PP_COMMA_IF(n) m(z, PDK_PP_INC(n), d)
# define PDK_PP_ENUM_SHIFTED_M_2_I(z, n, m, d) PDK_PP_COMMA_IF(n) m(z, PDK_PP_INC(n), d)
# define PDK_PP_ENUM_SHIFTED_M_3_I(z, n, m, d) PDK_PP_COMMA_IF(n) m(z, PDK_PP_INC(n), d)

#endif // PDK_STDEXT_PREPROCESSOR_REPETITION_ENUM_SHIFTED_H
