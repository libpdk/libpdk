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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_FOREACH_I_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_FOREACH_I_H

#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/arithmetic/Inc.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/If.h"
#include "pdk/stdext/preprocessor/control/Iif.h"
#include "pdk/stdext/preprocessor/repetition/For.h"
#include "pdk/stdext/preprocessor/seq/Seq.h"
#include "pdk/stdext/preprocessor/seq/Size.h"
#include "pdk/stdext/preprocessor/seq/internal/IsEmpty.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_SEQ_FOR_EACH_I
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SEQ_FOR_EACH_I(macro, data, seq) PDK_PP_SEQ_FOR_EACH_I_DETAIL_CHECK(macro, data, seq)
# else
#    define PDK_PP_SEQ_FOR_EACH_I(macro, data, seq) PDK_PP_SEQ_FOR_EACH_I_I(macro, data, seq)
#    define PDK_PP_SEQ_FOR_EACH_I_I(macro, data, seq) PDK_PP_SEQ_FOR_EACH_I_DETAIL_CHECK(macro, data, seq)
# endif
#
#    define PDK_PP_SEQ_FOR_EACH_I_DETAIL_CHECK_EXEC(macro, data, seq) PDK_PP_FOR((macro, data, seq, 0, PDK_PP_SEQ_SIZE(seq)), PDK_PP_SEQ_FOR_EACH_I_P, PDK_PP_SEQ_FOR_EACH_I_O, PDK_PP_SEQ_FOR_EACH_I_M)
#    define PDK_PP_SEQ_FOR_EACH_I_DETAIL_CHECK_EMPTY(macro, data, seq)
#
#    define PDK_PP_SEQ_FOR_EACH_I_DETAIL_CHECK(macro, data, seq) \
		PDK_PP_IIF \
			( \
			PDK_PP_SEQ_DETAIL_IS_NOT_EMPTY(seq), \
			PDK_PP_SEQ_FOR_EACH_I_DETAIL_CHECK_EXEC, \
			PDK_PP_SEQ_FOR_EACH_I_DETAIL_CHECK_EMPTY \
			) \
		(macro, data, seq) \
/**/

# define PDK_PP_SEQ_FOR_EACH_I_P(r, x) PDK_PP_TUPLE_ELEM(5, 4, x)

# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_SEQ_FOR_EACH_I_O(r, x) PDK_PP_SEQ_FOR_EACH_I_O_I x
# else
#    define PDK_PP_SEQ_FOR_EACH_I_O(r, x) PDK_PP_SEQ_FOR_EACH_I_O_I(PDK_PP_TUPLE_ELEM(5, 0, x), PDK_PP_TUPLE_ELEM(5, 1, x), PDK_PP_TUPLE_ELEM(5, 2, x), PDK_PP_TUPLE_ELEM(5, 3, x), PDK_PP_TUPLE_ELEM(5, 4, x))
# endif
#
# define PDK_PP_SEQ_FOR_EACH_I_O_I(macro, data, seq, i, sz) \
	PDK_PP_SEQ_FOR_EACH_I_O_I_DEC(macro, data, seq, i, PDK_PP_DEC(sz)) \
/**/
# define PDK_PP_SEQ_FOR_EACH_I_O_I_DEC(macro, data, seq, i, sz) \
	( \
	macro, \
	data, \
	PDK_PP_IF \
		( \
		sz, \
		PDK_PP_SEQ_FOR_EACH_I_O_I_TAIL, \
		PDK_PP_SEQ_FOR_EACH_I_O_I_NIL \
		) \
	(seq), \
	PDK_PP_INC(i), \
	sz \
	) \
/**/
# define PDK_PP_SEQ_FOR_EACH_I_O_I_TAIL(seq) PDK_PP_SEQ_TAIL(seq)
# define PDK_PP_SEQ_FOR_EACH_I_O_I_NIL(seq) PDK_PP_NIL
#
# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_SEQ_FOR_EACH_I_M(r, x) PDK_PP_SEQ_FOR_EACH_I_M_IM(r, PDK_PP_TUPLE_REM_5 x)
#    define PDK_PP_SEQ_FOR_EACH_I_M_IM(r, im) PDK_PP_SEQ_FOR_EACH_I_M_I(r, im)
# else
#    define PDK_PP_SEQ_FOR_EACH_I_M(r, x) PDK_PP_SEQ_FOR_EACH_I_M_I(r, PDK_PP_TUPLE_ELEM(5, 0, x), PDK_PP_TUPLE_ELEM(5, 1, x), PDK_PP_TUPLE_ELEM(5, 2, x), PDK_PP_TUPLE_ELEM(5, 3, x), PDK_PP_TUPLE_ELEM(5, 4, x))
# endif
#
# define PDK_PP_SEQ_FOR_EACH_I_M_I(r, macro, data, seq, i, sz) macro(r, data, i, PDK_PP_SEQ_HEAD(seq))
#
# /* PDK_PP_SEQ_FOR_EACH_I_R */
#
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SEQ_FOR_EACH_I_R(r, macro, data, seq) PDK_PP_SEQ_FOR_EACH_I_R_DETAIL_CHECK(r, macro, data, seq)
# else
#    define PDK_PP_SEQ_FOR_EACH_I_R(r, macro, data, seq) PDK_PP_SEQ_FOR_EACH_I_R_I(r, macro, data, seq)
#    define PDK_PP_SEQ_FOR_EACH_I_R_I(r, macro, data, seq) PDK_PP_SEQ_FOR_EACH_I_R_DETAIL_CHECK(r, macro, data, seq)
# endif
#
#    define PDK_PP_SEQ_FOR_EACH_I_R_DETAIL_CHECK_EXEC(r, macro, data, seq) PDK_PP_FOR_ ## r((macro, data, seq, 0, PDK_PP_SEQ_SIZE(seq)), PDK_PP_SEQ_FOR_EACH_I_P, PDK_PP_SEQ_FOR_EACH_I_O, PDK_PP_SEQ_FOR_EACH_I_M)
#    define PDK_PP_SEQ_FOR_EACH_I_R_DETAIL_CHECK_EMPTY(r, macro, data, seq)
#
#    define PDK_PP_SEQ_FOR_EACH_I_R_DETAIL_CHECK(r, macro, data, seq) \
		PDK_PP_IIF \
			( \
			PDK_PP_SEQ_DETAIL_IS_NOT_EMPTY(seq), \
			PDK_PP_SEQ_FOR_EACH_I_R_DETAIL_CHECK_EXEC, \
			PDK_PP_SEQ_FOR_EACH_I_R_DETAIL_CHECK_EMPTY \
			) \
		(r, macro, data, seq) \
/**/

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_FOREACH_I_H
