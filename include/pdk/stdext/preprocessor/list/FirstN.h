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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_FIRST_N_H
#define PDK_STDEXT_PREPROCESSOR_LIST_FIRST_N_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/control/While.h"
#include "pdk/stdext/preprocessor/list/Adt.h"
#include "pdk/stdext/preprocessor/list/Reverse.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"

// PDK_PP_LIST_FIRST_N
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FIRST_N(count, list) PDK_PP_LIST_REVERSE(PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_WHILE(PDK_PP_LIST_FIRST_N_P, PDK_PP_LIST_FIRST_N_O, (count, list, PDK_PP_NIL))))
# else
#    define PDK_PP_LIST_FIRST_N(count, list) PDK_PP_LIST_FIRST_N_I(count, list)
#    define PDK_PP_LIST_FIRST_N_I(count, list) PDK_PP_LIST_REVERSE(PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_WHILE(PDK_PP_LIST_FIRST_N_P, PDK_PP_LIST_FIRST_N_O, (count, list, PDK_PP_NIL))))
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FIRST_N_P(d, data) PDK_PP_TUPLE_ELEM(3, 0, data)
# else
#    define PDK_PP_LIST_FIRST_N_P(d, data) PDK_PP_LIST_FIRST_N_P_I data
#    define PDK_PP_LIST_FIRST_N_P_I(c, l, nl) c
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MWCC()
#    define PDK_PP_LIST_FIRST_N_O(d, data) PDK_PP_LIST_FIRST_N_O_D data
# else
#    define PDK_PP_LIST_FIRST_N_O(d, data) PDK_PP_LIST_FIRST_N_O_D(PDK_PP_TUPLE_ELEM(3, 0, data), PDK_PP_TUPLE_ELEM(3, 1, data), PDK_PP_TUPLE_ELEM(3, 2, data))
# endif

# define PDK_PP_LIST_FIRST_N_O_D(c, l, nl) (PDK_PP_DEC(c), PDK_PP_LIST_REST(l), (PDK_PP_LIST_FIRST(l), nl))

// PDK_PP_LIST_FIRST_N_D
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FIRST_N_D(d, count, list) PDK_PP_LIST_REVERSE_D(d, PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_WHILE_ ## d(PDK_PP_LIST_FIRST_N_P, PDK_PP_LIST_FIRST_N_O, (count, list, PDK_PP_NIL))))
# else
#    define PDK_PP_LIST_FIRST_N_D(d, count, list) PDK_PP_LIST_FIRST_N_D_I(d, count, list)
#    define PDK_PP_LIST_FIRST_N_D_I(d, count, list) PDK_PP_LIST_REVERSE_D(d, PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_WHILE_ ## d(PDK_PP_LIST_FIRST_N_P, PDK_PP_LIST_FIRST_N_O, (count, list, PDK_PP_NIL))))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_FIRST_N_H
