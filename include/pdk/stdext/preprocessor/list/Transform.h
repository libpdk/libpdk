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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_TRANSFORM_H
#define PDK_STDEXT_PREPROCESSOR_LIST_TRANSFORM_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/list/FoldRight.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_LIST_TRANSFORM
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_TRANSFORM(op, data, list) PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_LIST_FOLD_RIGHT(PDK_PP_LIST_TRANSFORM_O, (op, data, PDK_PP_NIL), list))
# else
#    define PDK_PP_LIST_TRANSFORM(op, data, list) PDK_PP_LIST_TRANSFORM_I(op, data, list)
#    define PDK_PP_LIST_TRANSFORM_I(op, data, list) PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_LIST_FOLD_RIGHT(PDK_PP_LIST_TRANSFORM_O, (op, data, PDK_PP_NIL), list))
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_TRANSFORM_O(d, odr, elem) PDK_PP_LIST_TRANSFORM_O_D(d, PDK_PP_TUPLE_ELEM(3, 0, odr), PDK_PP_TUPLE_ELEM(3, 1, odr), PDK_PP_TUPLE_ELEM(3, 2, odr), elem)
# else
#    define PDK_PP_LIST_TRANSFORM_O(d, odr, elem) PDK_PP_LIST_TRANSFORM_O_I(d, PDK_PP_TUPLE_REM_3 odr, elem)
#    define PDK_PP_LIST_TRANSFORM_O_I(d, im, elem) PDK_PP_LIST_TRANSFORM_O_D(d, im, elem)
# endif

# define PDK_PP_LIST_TRANSFORM_O_D(d, op, data, res, elem) (op, data, (op(d, data, elem), res))

// PDK_PP_LIST_TRANSFORM_D
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_TRANSFORM_D(d, op, data, list) PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_LIST_FOLD_RIGHT_ ## d(PDK_PP_LIST_TRANSFORM_O, (op, data, PDK_PP_NIL), list))
# else
#    define PDK_PP_LIST_TRANSFORM_D(d, op, data, list) PDK_PP_LIST_TRANSFORM_D_I(d, op, data, list)
#    define PDK_PP_LIST_TRANSFORM_D_I(d, op, data, list) PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_LIST_FOLD_RIGHT_ ## d(PDK_PP_LIST_TRANSFORM_O, (op, data, PDK_PP_NIL), list))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_TO_TUPLE_H
