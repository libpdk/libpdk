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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_FILTER_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_FILTER_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/ExprIf.h"
#include "pdk/stdext/preprocessor/facilities/Empty.h"
#include "pdk/stdext/preprocessor/seq/FoldLeft.h"
#include "pdk/stdext/preprocessor/seq/Seq.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_SEQ_FILTER
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SEQ_FILTER(pred, data, seq) PDK_PP_SEQ_TAIL(PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_SEQ_FOLD_LEFT(PDK_PP_SEQ_FILTER_O, (pred, data, (nil)), seq)))
# else
#    define PDK_PP_SEQ_FILTER(pred, data, seq) PDK_PP_SEQ_FILTER_I(pred, data, seq)
#    define PDK_PP_SEQ_FILTER_I(pred, data, seq) PDK_PP_SEQ_TAIL(PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_SEQ_FOLD_LEFT(PDK_PP_SEQ_FILTER_O, (pred, data, (nil)), seq)))
# endif
#
# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_SEQ_FILTER_O(s, st, elem) PDK_PP_SEQ_FILTER_O_IM(s, PDK_PP_TUPLE_REM_3 st, elem)
#    define PDK_PP_SEQ_FILTER_O_IM(s, im, elem) PDK_PP_SEQ_FILTER_O_I(s, im, elem)
# else
#    define PDK_PP_SEQ_FILTER_O(s, st, elem) PDK_PP_SEQ_FILTER_O_I(s, PDK_PP_TUPLE_ELEM(3, 0, st), PDK_PP_TUPLE_ELEM(3, 1, st), PDK_PP_TUPLE_ELEM(3, 2, st), elem)
# endif
#
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_DMC()
#   define PDK_PP_SEQ_FILTER_O_I(s, pred, data, res, elem) (pred, data, res PDK_PP_EXPR_IF(pred(s, data, elem), (elem)))
# else
#   define PDK_PP_SEQ_FILTER_O_I(s, pred, data, res, elem) (pred, data, res PDK_PP_EXPR_IF(pred##(s, data, elem), (elem)))
# endif

// PDK_PP_SEQ_FILTER_S
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SEQ_FILTER_S(s, pred, data, seq) PDK_PP_SEQ_TAIL(PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_SEQ_FOLD_LEFT_ ## s(PDK_PP_SEQ_FILTER_O, (pred, data, (nil)), seq)))
# else
#    define PDK_PP_SEQ_FILTER_S(s, pred, data, seq) PDK_PP_SEQ_FILTER_S_I(s, pred, data, seq)
#    define PDK_PP_SEQ_FILTER_S_I(s, pred, data, seq) PDK_PP_SEQ_TAIL(PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_SEQ_FOLD_LEFT_ ## s(PDK_PP_SEQ_FILTER_O, (pred, data, (nil)), seq)))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_FILTER_H
