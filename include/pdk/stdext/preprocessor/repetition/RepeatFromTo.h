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

#ifndef PDK_STDEXT_PREPROCESSOR_REPETITION_REPEAT_FROM_TO_H
#define PDK_STDEXT_PREPROCESSOR_REPETITION_REPEAT_FROM_TO_H

#include "pdk/stdext/preprocessor/arithmetic/Add.h"
#include "pdk/stdext/preprocessor/arithmetic/Sub.h"
#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/debug/Error.h"
#include "pdk/stdext/preprocessor/internal/AutoRec.h"
#include "pdk/stdext/preprocessor/repetition/Repeat.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_REPEAT_FROM_TO
# if 0
#    define PDK_PP_REPEAT_FROM_TO(first, last, macro, data)
# endif

# define PDK_PP_REPEAT_FROM_TO PDK_PP_CAT(PDK_PP_REPEAT_FROM_TO_, PDK_PP_AUTO_REC(PDK_PP_REPEAT_P, 4))

# define PDK_PP_REPEAT_FROM_TO_1(f, l, m, dt) PDK_PP_REPEAT_FROM_TO_D_1(PDK_PP_AUTO_REC(PDK_PP_WHILE_P, 256), f, l, m, dt)
# define PDK_PP_REPEAT_FROM_TO_2(f, l, m, dt) PDK_PP_REPEAT_FROM_TO_D_2(PDK_PP_AUTO_REC(PDK_PP_WHILE_P, 256), f, l, m, dt)
# define PDK_PP_REPEAT_FROM_TO_3(f, l, m, dt) PDK_PP_REPEAT_FROM_TO_D_3(PDK_PP_AUTO_REC(PDK_PP_WHILE_P, 256), f, l, m, dt)
# define PDK_PP_REPEAT_FROM_TO_4(f, l, m, dt) PDK_PP_ERROR(0x0003)
#
# define PDK_PP_REPEAT_FROM_TO_1ST PDK_PP_REPEAT_FROM_TO_1
# define PDK_PP_REPEAT_FROM_TO_2ND PDK_PP_REPEAT_FROM_TO_2
# define PDK_PP_REPEAT_FROM_TO_3RD PDK_PP_REPEAT_FROM_TO_3

// PDK_PP_REPEAT_FROM_TO_D
# if 0
#    define PDK_PP_REPEAT_FROM_TO_D(d, first, last, macro, data)
# endif

# define PDK_PP_REPEAT_FROM_TO_D PDK_PP_CAT(PDK_PP_REPEAT_FROM_TO_D_, PDK_PP_AUTO_REC(PDK_PP_REPEAT_P, 4))

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_REPEAT_FROM_TO_D_1(d, f, l, m, dt) PDK_PP_REPEAT_1(PDK_PP_SUB_D(d, l, f), PDK_PP_REPEAT_FROM_TO_M_1, (d, f, m, dt))
#    define PDK_PP_REPEAT_FROM_TO_D_2(d, f, l, m, dt) PDK_PP_REPEAT_2(PDK_PP_SUB_D(d, l, f), PDK_PP_REPEAT_FROM_TO_M_2, (d, f, m, dt))
#    define PDK_PP_REPEAT_FROM_TO_D_3(d, f, l, m, dt) PDK_PP_REPEAT_3(PDK_PP_SUB_D(d, l, f), PDK_PP_REPEAT_FROM_TO_M_3, (d, f, m, dt))
# else
#    define PDK_PP_REPEAT_FROM_TO_D_1(d, f, l, m, dt) PDK_PP_REPEAT_FROM_TO_D_1_I(d, f, l, m, dt)
#    define PDK_PP_REPEAT_FROM_TO_D_2(d, f, l, m, dt) PDK_PP_REPEAT_FROM_TO_D_2_I(d, f, l, m, dt)
#    define PDK_PP_REPEAT_FROM_TO_D_3(d, f, l, m, dt) PDK_PP_REPEAT_FROM_TO_D_3_I(d, f, l, m, dt)
#    define PDK_PP_REPEAT_FROM_TO_D_1_I(d, f, l, m, dt) PDK_PP_REPEAT_1(PDK_PP_SUB_D(d, l, f), PDK_PP_REPEAT_FROM_TO_M_1, (d, f, m, dt))
#    define PDK_PP_REPEAT_FROM_TO_D_2_I(d, f, l, m, dt) PDK_PP_REPEAT_2(PDK_PP_SUB_D(d, l, f), PDK_PP_REPEAT_FROM_TO_M_2, (d, f, m, dt))
#    define PDK_PP_REPEAT_FROM_TO_D_3_I(d, f, l, m, dt) PDK_PP_REPEAT_3(PDK_PP_SUB_D(d, l, f), PDK_PP_REPEAT_FROM_TO_M_3, (d, f, m, dt))
# endif

# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_REPEAT_FROM_TO_M_1(z, n, dfmd) PDK_PP_REPEAT_FROM_TO_M_1_IM(z, n, PDK_PP_TUPLE_REM_4 dfmd)
#    define PDK_PP_REPEAT_FROM_TO_M_2(z, n, dfmd) PDK_PP_REPEAT_FROM_TO_M_2_IM(z, n, PDK_PP_TUPLE_REM_4 dfmd)
#    define PDK_PP_REPEAT_FROM_TO_M_3(z, n, dfmd) PDK_PP_REPEAT_FROM_TO_M_3_IM(z, n, PDK_PP_TUPLE_REM_4 dfmd)
#    define PDK_PP_REPEAT_FROM_TO_M_1_IM(z, n, im) PDK_PP_REPEAT_FROM_TO_M_1_I(z, n, im)
#    define PDK_PP_REPEAT_FROM_TO_M_2_IM(z, n, im) PDK_PP_REPEAT_FROM_TO_M_2_I(z, n, im)
#    define PDK_PP_REPEAT_FROM_TO_M_3_IM(z, n, im) PDK_PP_REPEAT_FROM_TO_M_3_I(z, n, im)
# else
#    define PDK_PP_REPEAT_FROM_TO_M_1(z, n, dfmd) PDK_PP_REPEAT_FROM_TO_M_1_I(z, n, PDK_PP_TUPLE_ELEM(4, 0, dfmd), PDK_PP_TUPLE_ELEM(4, 1, dfmd), PDK_PP_TUPLE_ELEM(4, 2, dfmd), PDK_PP_TUPLE_ELEM(4, 3, dfmd))
#    define PDK_PP_REPEAT_FROM_TO_M_2(z, n, dfmd) PDK_PP_REPEAT_FROM_TO_M_2_I(z, n, PDK_PP_TUPLE_ELEM(4, 0, dfmd), PDK_PP_TUPLE_ELEM(4, 1, dfmd), PDK_PP_TUPLE_ELEM(4, 2, dfmd), PDK_PP_TUPLE_ELEM(4, 3, dfmd))
#    define PDK_PP_REPEAT_FROM_TO_M_3(z, n, dfmd) PDK_PP_REPEAT_FROM_TO_M_3_I(z, n, PDK_PP_TUPLE_ELEM(4, 0, dfmd), PDK_PP_TUPLE_ELEM(4, 1, dfmd), PDK_PP_TUPLE_ELEM(4, 2, dfmd), PDK_PP_TUPLE_ELEM(4, 3, dfmd))
# endif

# define PDK_PP_REPEAT_FROM_TO_M_1_I(z, n, d, f, m, dt) PDK_PP_REPEAT_FROM_TO_M_1_II(z, PDK_PP_ADD_D(d, n, f), m, dt)
# define PDK_PP_REPEAT_FROM_TO_M_2_I(z, n, d, f, m, dt) PDK_PP_REPEAT_FROM_TO_M_2_II(z, PDK_PP_ADD_D(d, n, f), m, dt)
# define PDK_PP_REPEAT_FROM_TO_M_3_I(z, n, d, f, m, dt) PDK_PP_REPEAT_FROM_TO_M_3_II(z, PDK_PP_ADD_D(d, n, f), m, dt)

# define PDK_PP_REPEAT_FROM_TO_M_1_II(z, n, m, dt) m(z, n, dt)
# define PDK_PP_REPEAT_FROM_TO_M_2_II(z, n, m, dt) m(z, n, dt)
# define PDK_PP_REPEAT_FROM_TO_M_3_II(z, n, m, dt) m(z, n, dt)

#endif // PDK_STDEXT_PREPROCESSOR_REPETITION_REPEAT_FROM_TO_H
