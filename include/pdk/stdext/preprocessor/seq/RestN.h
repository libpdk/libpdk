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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_RESTN_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_RESTN_H

#include "pdk/stdext/preprocessor/arithmetic/Inc.h"
#include "pdk/stdext/preprocessor/comparison/NotEqual.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/ExprIif.h"
#include "pdk/stdext/preprocessor/facilities/Identity.h"
#include "pdk/stdext/preprocessor/logical/BitAnd.h"
#include "pdk/stdext/preprocessor/seq/internal/IsEmpty.h"
#include "pdk/stdext/preprocessor/seq/internal/Split.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"

// PDK_PP_SEQ_REST_N
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SEQ_REST_N(n, seq) PDK_PP_SEQ_REST_N_DETAIL_EXEC(n, seq, PDK_PP_SEQ_DETAIL_EMPTY_SIZE(seq))
# else
#    define PDK_PP_SEQ_REST_N(n, seq) PDK_PP_SEQ_REST_N_I(n, seq)
#    define PDK_PP_SEQ_REST_N_I(n, seq) PDK_PP_SEQ_REST_N_DETAIL_EXEC(n, seq, PDK_PP_SEQ_DETAIL_EMPTY_SIZE(seq))
# endif
#
#    define PDK_PP_SEQ_REST_N_DETAIL_EXEC(n, seq, size) \
		PDK_PP_EXPR_IIF \
			( \
			PDK_PP_BITAND \
				( \
				PDK_PP_SEQ_DETAIL_IS_NOT_EMPTY_SIZE(size), \
				PDK_PP_NOT_EQUAL(n,size) \
				), \
			PDK_PP_TUPLE_ELEM(2, 1, PDK_PP_SEQ_SPLIT(PDK_PP_INC(n), PDK_PP_IDENTITY( (nil) seq )))() \
			) \
/**/

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_RESTN_H
