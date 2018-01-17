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

#ifndef PDK_STDEXT_PREPROCESSOR_ARRAY_REPLACE_H
#define PDK_STDEXT_PREPROCESSOR_ARRAY_REPLACE_H

#include "pdk/stdext/preprocessor/arithmetic/Inc.h"
#include "pdk/stdext/preprocessor/array/Element.h"
#include "pdk/stdext/preprocessor/array/PushBack.h"
#include "pdk/stdext/preprocessor/comparison/NotEqual.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/Iif.h"
#include "pdk/stdext/preprocessor/control/While.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"

// PDK_PP_ARRAY_REPLACE
# define PDK_PP_ARRAY_REPLACE(array, i, elem) PDK_PP_ARRAY_REPLACE_I(PDK_PP_DEDUCE_D(), array, i, elem)
# define PDK_PP_ARRAY_REPLACE_I(d, array, i, elem) PDK_PP_ARRAY_REPLACE_D(d, array, i, elem)

// PDK_PP_ARRAY_REPLACE_D
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_ARRAY_REPLACE_D(d, array, i, elem) PDK_PP_TUPLE_ELEM(5, 3, PDK_PP_WHILE_ ## d(PDK_PP_ARRAY_REPLACE_P, PDK_PP_ARRAY_REPLACE_O, (0, i, elem, (0, ()), array)))
# else
#    define PDK_PP_ARRAY_REPLACE_D(d, array, i, elem) PDK_PP_ARRAY_REPLACE_D_I(d, array, i, elem)
#    define PDK_PP_ARRAY_REPLACE_D_I(d, array, i, elem) PDK_PP_TUPLE_ELEM(5, 3, PDK_PP_WHILE_ ## d(PDK_PP_ARRAY_REPLACE_P, PDK_PP_ARRAY_REPLACE_O, (0, i, elem, (0, ()), array)))
# endif

# define PDK_PP_ARRAY_REPLACE_P(d, state) PDK_PP_NOT_EQUAL(PDK_PP_TUPLE_ELEM(5, 0, state), PDK_PP_ARRAY_SIZE(PDK_PP_TUPLE_ELEM(5, 4, state)))
#
# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_ARRAY_REPLACE_O(d, state) PDK_PP_ARRAY_REPLACE_O_I state
# else
#    define PDK_PP_ARRAY_REPLACE_O(d, state) PDK_PP_ARRAY_REPLACE_O_I(PDK_PP_TUPLE_ELEM(5, 0, state), PDK_PP_TUPLE_ELEM(5, 1, state), PDK_PP_TUPLE_ELEM(5, 2, state), PDK_PP_TUPLE_ELEM(5, 3, state), PDK_PP_TUPLE_ELEM(5, 4, state))
# endif

# define PDK_PP_ARRAY_REPLACE_O_I(n, i, elem, res, arr) (PDK_PP_INC(n), i, elem, PDK_PP_ARRAY_PUSH_BACK(res, PDK_PP_IIF(PDK_PP_NOT_EQUAL(n, i), PDK_PP_ARRAY_ELEM(n, arr), elem)), arr)

#endif // PDK_STDEXT_PREPROCESSOR_ARRAY_REPLACE_H
