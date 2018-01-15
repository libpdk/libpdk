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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_FILTER_H
#define PDK_STDEXT_PREPROCESSOR_LIST_FILTER_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/If.h"
#include "pdk/stdext/preprocessor/list/FoldRight.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"

// PDK_PP_LIST_FILTER
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FILTER(pred, data, list) PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_LIST_FOLD_RIGHT(PDK_PP_LIST_FILTER_O, (pred, data, PDK_PP_NIL), list))
# else
#    define PDK_PP_LIST_FILTER(pred, data, list) PDK_PP_LIST_FILTER_I(pred, data, list)
#    define PDK_PP_LIST_FILTER_I(pred, data, list) PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_LIST_FOLD_RIGHT(PDK_PP_LIST_FILTER_O, (pred, data, PDK_PP_NIL), list))
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FILTER_O(d, pdr, elem) PDK_PP_LIST_FILTER_O_D(d, PDK_PP_TUPLE_ELEM(3, 0, pdr), PDK_PP_TUPLE_ELEM(3, 1, pdr), PDK_PP_TUPLE_ELEM(3, 2, pdr), elem)
# else
#    define PDK_PP_LIST_FILTER_O(d, pdr, elem) PDK_PP_LIST_FILTER_O_I(d, PDK_PP_TUPLE_REM_3 pdr, elem)
#    define PDK_PP_LIST_FILTER_O_I(d, im, elem) PDK_PP_LIST_FILTER_O_D(d, im, elem)
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_DMC()
#    define PDK_PP_LIST_FILTER_O_D(d, pred, data, res, elem) (pred, data, PDK_PP_IF(pred(d, data, elem), (elem, res), res))
# else
#    define PDK_PP_LIST_FILTER_O_D(d, pred, data, res, elem) (pred, data, PDK_PP_IF(pred##(d, data, elem), (elem, res), res))
# endif

// PDK_PP_LIST_FILTER_D
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FILTER_D(d, pred, data, list) PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_LIST_FOLD_RIGHT_ ## d(PDK_PP_LIST_FILTER_O, (pred, data, PDK_PP_NIL), list))
# else
#    define PDK_PP_LIST_FILTER_D(d, pred, data, list) PDK_PP_LIST_FILTER_D_I(d, pred, data, list)
#    define PDK_PP_LIST_FILTER_D_I(d, pred, data, list) PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_LIST_FOLD_RIGHT_ ## d(PDK_PP_LIST_FILTER_O, (pred, data, PDK_PP_NIL), list))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_FILTER_H
