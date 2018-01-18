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
// Created by softboy on 2018/01/18.

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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_TRANSFORM_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_TRANSFORM_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/seq/FoldLeft.h"
#include "pdk/stdext/preprocessor/seq/Seq.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_SEQ_TRANSFORM
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SEQ_TRANSFORM(op, data, seq) PDK_PP_SEQ_TAIL(PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_SEQ_FOLD_LEFT(PDK_PP_SEQ_TRANSFORM_O, (op, data, (nil)), seq)))
# else
#    define PDK_PP_SEQ_TRANSFORM(op, data, seq) PDK_PP_SEQ_TRANSFORM_I(op, data, seq)
#    define PDK_PP_SEQ_TRANSFORM_I(op, data, seq) PDK_PP_SEQ_TAIL(PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_SEQ_FOLD_LEFT(PDK_PP_SEQ_TRANSFORM_O, (op, data, (nil)), seq)))
# endif
#
# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_SEQ_TRANSFORM_O(s, state, elem) PDK_PP_SEQ_TRANSFORM_O_IM(s, PDK_PP_TUPLE_REM_3 state, elem)
#    define PDK_PP_SEQ_TRANSFORM_O_IM(s, im, elem) PDK_PP_SEQ_TRANSFORM_O_I(s, im, elem)
# else
#    define PDK_PP_SEQ_TRANSFORM_O(s, state, elem) PDK_PP_SEQ_TRANSFORM_O_I(s, PDK_PP_TUPLE_ELEM(3, 0, state), PDK_PP_TUPLE_ELEM(3, 1, state), PDK_PP_TUPLE_ELEM(3, 2, state), elem)
# endif
#
# define PDK_PP_SEQ_TRANSFORM_O_I(s, op, data, res, elem) (op, data, res (op(s, data, elem)))
#
# /* PDK_PP_SEQ_TRANSFORM_S */
#
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SEQ_TRANSFORM_S(s, op, data, seq) PDK_PP_SEQ_TAIL(PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_SEQ_FOLD_LEFT_ ## s(PDK_PP_SEQ_TRANSFORM_O, (op, data, (nil)), seq)))
# else
#    define PDK_PP_SEQ_TRANSFORM_S(s, op, data, seq) PDK_PP_SEQ_TRANSFORM_S_I(s, op, data, seq)
#    define PDK_PP_SEQ_TRANSFORM_S_I(s, op, data, seq) PDK_PP_SEQ_TAIL(PDK_PP_TUPLE_ELEM(3, 2, PDK_PP_SEQ_FOLD_LEFT_ ## s(PDK_PP_SEQ_TRANSFORM_O, (op, data, (nil)), seq)))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_TRANSFORM_H
