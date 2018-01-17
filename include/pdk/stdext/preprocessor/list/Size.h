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
// Created by softboy on 2018/01/17.

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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_SIZE_H
#define PDK_STDEXT_PREPROCESSOR_LIST_SIZE_H

#include "pdk/stdext/preprocessor/arithmetic/Inc.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/While.h"
#include "pdk/stdext/preprocessor/list/Adt.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_LIST_SIZE
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_SIZE(list) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE(PDK_PP_LIST_SIZE_P, PDK_PP_LIST_SIZE_O, (0, list)))
# else
#    define PDK_PP_LIST_SIZE(list) PDK_PP_LIST_SIZE_I(list)
#    define PDK_PP_LIST_SIZE_I(list) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE(PDK_PP_LIST_SIZE_P, PDK_PP_LIST_SIZE_O, (0, list)))
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_SIZE_P(d, rl) PDK_PP_LIST_IS_CONS(PDK_PP_TUPLE_ELEM(2, 1, rl))
# else
#    define PDK_PP_LIST_SIZE_P(d, rl) PDK_PP_LIST_SIZE_P_I(PDK_PP_TUPLE_REM_2 rl)
#    define PDK_PP_LIST_SIZE_P_I(im) PDK_PP_LIST_SIZE_P_II(im)
#    define PDK_PP_LIST_SIZE_P_II(r, l) PDK_PP_LIST_IS_CONS(l)
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_SIZE_O(d, rl) (PDK_PP_INC(PDK_PP_TUPLE_ELEM(2, 0, rl)), PDK_PP_LIST_REST(PDK_PP_TUPLE_ELEM(2, 1, rl)))
# else
#    define PDK_PP_LIST_SIZE_O(d, rl) PDK_PP_LIST_SIZE_O_I(PDK_PP_TUPLE_REM_2 rl)
#    define PDK_PP_LIST_SIZE_O_I(im) PDK_PP_LIST_SIZE_O_II(im)
#    define PDK_PP_LIST_SIZE_O_II(r, l) (PDK_PP_INC(r), PDK_PP_LIST_REST(l))
# endif

// PDK_PP_LIST_SIZE_D
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_SIZE_D(d, list) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE_ ## d(PDK_PP_LIST_SIZE_P, PDK_PP_LIST_SIZE_O, (0, list)))
# else
#    define PDK_PP_LIST_SIZE_D(d, list) PDK_PP_LIST_SIZE_D_I(d, list)
#    define PDK_PP_LIST_SIZE_D_I(d, list) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE_ ## d(PDK_PP_LIST_SIZE_P, PDK_PP_LIST_SIZE_O, (0, list)))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_SIZE_H
