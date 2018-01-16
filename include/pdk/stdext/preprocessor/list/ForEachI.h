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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_FOR_EACH_I_H
#define PDK_STDEXT_PREPROCESSOR_LIST_FOR_EACH_I_H

#include "pdk/stdext/preprocessor/arithmetic/Inc.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/list/Adt.h"
#include "pdk/stdext/preprocessor/repetition/For.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_LIST_FOR_EACH_I
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG() && ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()
#    define PDK_PP_LIST_FOR_EACH_I(macro, data, list) PDK_PP_FOR((macro, data, list, 0), PDK_PP_LIST_FOR_EACH_I_P, PDK_PP_LIST_FOR_EACH_I_O, PDK_PP_LIST_FOR_EACH_I_M)
# else
#    define PDK_PP_LIST_FOR_EACH_I(macro, data, list) PDK_PP_LIST_FOR_EACH_I_I(macro, data, list)
#    define PDK_PP_LIST_FOR_EACH_I_I(macro, data, list) PDK_PP_FOR((macro, data, list, 0), PDK_PP_LIST_FOR_EACH_I_P, PDK_PP_LIST_FOR_EACH_I_O, PDK_PP_LIST_FOR_EACH_I_M)
# endif

# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_LIST_FOR_EACH_I_P(r, x) PDK_PP_LIST_FOR_EACH_I_P_D x
#    define PDK_PP_LIST_FOR_EACH_I_P_D(m, d, l, i) PDK_PP_LIST_IS_CONS(l)
# else
#    define PDK_PP_LIST_FOR_EACH_I_P(r, x) PDK_PP_LIST_IS_CONS(PDK_PP_TUPLE_ELEM(4, 2, x))
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MWCC()
#    define PDK_PP_LIST_FOR_EACH_I_O(r, x) PDK_PP_LIST_FOR_EACH_I_O_D x
#    define PDK_PP_LIST_FOR_EACH_I_O_D(m, d, l, i) (m, d, PDK_PP_LIST_REST(l), PDK_PP_INC(i))
# else
#    define PDK_PP_LIST_FOR_EACH_I_O(r, x) (PDK_PP_TUPLE_ELEM(4, 0, x), PDK_PP_TUPLE_ELEM(4, 1, x), PDK_PP_LIST_REST(PDK_PP_TUPLE_ELEM(4, 2, x)), PDK_PP_INC(PDK_PP_TUPLE_ELEM(4, 3, x)))
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FOR_EACH_I_M(r, x) PDK_PP_LIST_FOR_EACH_I_M_D(r, PDK_PP_TUPLE_ELEM(4, 0, x), PDK_PP_TUPLE_ELEM(4, 1, x), PDK_PP_TUPLE_ELEM(4, 2, x), PDK_PP_TUPLE_ELEM(4, 3, x))
# else
#    define PDK_PP_LIST_FOR_EACH_I_M(r, x) PDK_PP_LIST_FOR_EACH_I_M_I(r, PDK_PP_TUPLE_REM_4 x)
#    define PDK_PP_LIST_FOR_EACH_I_M_I(r, x_e) PDK_PP_LIST_FOR_EACH_I_M_D(r, x_e)
# endif

# define PDK_PP_LIST_FOR_EACH_I_M_D(r, m, d, l, i) m(r, d, i, PDK_PP_LIST_FIRST(l))

// PDK_PP_LIST_FOR_EACH_I_R
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FOR_EACH_I_R(r, macro, data, list) PDK_PP_FOR_ ## r((macro, data, list, 0), PDK_PP_LIST_FOR_EACH_I_P, PDK_PP_LIST_FOR_EACH_I_O, PDK_PP_LIST_FOR_EACH_I_M)
# else
#    define PDK_PP_LIST_FOR_EACH_I_R(r, macro, data, list) PDK_PP_LIST_FOR_EACH_I_R_I(r, macro, data, list)
#    define PDK_PP_LIST_FOR_EACH_I_R_I(r, macro, data, list) PDK_PP_FOR_ ## r((macro, data, list, 0), PDK_PP_LIST_FOR_EACH_I_P, PDK_PP_LIST_FOR_EACH_I_O, PDK_PP_LIST_FOR_EACH_I_M)
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_FOR_EACH_I_H
