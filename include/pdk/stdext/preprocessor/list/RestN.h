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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_REST_N_H
#define PDK_STDEXT_PREPROCESSOR_LIST_REST_N_H

#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/While.h"
#include "pdk/stdext/preprocessor/list/Adt.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"

// PDK_PP_LIST_REST_N
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_REST_N(count, list) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE(PDK_PP_LIST_REST_N_P, PDK_PP_LIST_REST_N_O, (list, count)))
# else
#    define PDK_PP_LIST_REST_N(count, list) PDK_PP_LIST_REST_N_I(count, list)
#    define PDK_PP_LIST_REST_N_I(count, list) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE(PDK_PP_LIST_REST_N_P, PDK_PP_LIST_REST_N_O, (list, count)))
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_REST_N_P(d, lc) PDK_PP_TUPLE_ELEM(2, 1, lc)
# else
#    define PDK_PP_LIST_REST_N_P(d, lc) PDK_PP_LIST_REST_N_P_I lc
#    define PDK_PP_LIST_REST_N_P_I(list, count) count
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_REST_N_O(d, lc) (PDK_PP_LIST_REST(PDK_PP_TUPLE_ELEM(2, 0, lc)), PDK_PP_DEC(PDK_PP_TUPLE_ELEM(2, 1, lc)))
# else
#    define PDK_PP_LIST_REST_N_O(d, lc) PDK_PP_LIST_REST_N_O_I lc
#    define PDK_PP_LIST_REST_N_O_I(list, count) (PDK_PP_LIST_REST(list), PDK_PP_DEC(count))
# endif

// PDK_PP_LIST_REST_N_D
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_REST_N_D(d, count, list) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE_ ## d(PDK_PP_LIST_REST_N_P, PDK_PP_LIST_REST_N_O, (list, count)))
# else
#    define PDK_PP_LIST_REST_N_D(d, count, list) PDK_PP_LIST_REST_N_D_I(d, count, list)
#    define PDK_PP_LIST_REST_N_D_I(d, count, list) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE_ ## d(PDK_PP_LIST_REST_N_P, PDK_PP_LIST_REST_N_O, (list, count)))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_REST_N_H
