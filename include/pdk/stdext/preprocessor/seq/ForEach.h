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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_FOREACH_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_FOREACH_H

#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/If.h"
#include "pdk/stdext/preprocessor/control/Iif.h"
#include "pdk/stdext/preprocessor/repetition/For.h"
#include "pdk/stdext/preprocessor/seq/Seq.h"
#include "pdk/stdext/preprocessor/seq/Size.h"
#include "pdk/stdext/preprocessor/seq/internal/IsEmpty.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_SEQ_FOR_EACH
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SEQ_FOR_EACH(macro, data, seq) PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK(macro, data, seq)
# else
#    define PDK_PP_SEQ_FOR_EACH(macro, data, seq) PDK_PP_SEQ_FOR_EACH_D(macro, data, seq)
#    define PDK_PP_SEQ_FOR_EACH_D(macro, data, seq) PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK(macro, data, seq)
# endif
#
#    define PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_EXEC(macro, data, seq) PDK_PP_FOR((macro, data, seq, PDK_PP_SEQ_SIZE(seq)), PDK_PP_SEQ_FOR_EACH_P, PDK_PP_SEQ_FOR_EACH_O, PDK_PP_SEQ_FOR_EACH_M)
#    define PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_EMPTY(macro, data, seq)
#
#    define PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK(macro, data, seq) \
		PDK_PP_IIF \
			( \
			PDK_PP_SEQ_DETAIL_IS_NOT_EMPTY(seq), \
			PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_EXEC, \
			PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_EMPTY \
			) \
		(macro, data, seq) \
/**/

# define PDK_PP_SEQ_FOR_EACH_P(r, x) PDK_PP_TUPLE_ELEM(4, 3, x)

# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_SEQ_FOR_EACH_O(r, x) PDK_PP_SEQ_FOR_EACH_O_I x
# else
#    define PDK_PP_SEQ_FOR_EACH_O(r, x) PDK_PP_SEQ_FOR_EACH_O_I(PDK_PP_TUPLE_ELEM(4, 0, x), PDK_PP_TUPLE_ELEM(4, 1, x), PDK_PP_TUPLE_ELEM(4, 2, x), PDK_PP_TUPLE_ELEM(4, 3, x))
# endif

# define PDK_PP_SEQ_FOR_EACH_O_I(macro, data, seq, sz) \
	PDK_PP_SEQ_FOR_EACH_O_I_DEC(macro, data, seq, PDK_PP_DEC(sz)) \
/**/
# define PDK_PP_SEQ_FOR_EACH_O_I_DEC(macro, data, seq, sz) \
	( \
	macro, \
	data, \
	PDK_PP_IF \
		( \
		sz, \
		PDK_PP_SEQ_FOR_EACH_O_I_TAIL, \
		PDK_PP_SEQ_FOR_EACH_O_I_NIL \
		) \
	(seq), \
	sz \
	) \
/**/
# define PDK_PP_SEQ_FOR_EACH_O_I_TAIL(seq) PDK_PP_SEQ_TAIL(seq)
# define PDK_PP_SEQ_FOR_EACH_O_I_NIL(seq) PDK_PP_NIL
#
# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_STRICT()
#    define PDK_PP_SEQ_FOR_EACH_M(r, x) PDK_PP_SEQ_FOR_EACH_M_IM(r, PDK_PP_TUPLE_REM_4 x)
#    define PDK_PP_SEQ_FOR_EACH_M_IM(r, im) PDK_PP_SEQ_FOR_EACH_M_I(r, im)
# else
#    define PDK_PP_SEQ_FOR_EACH_M(r, x) PDK_PP_SEQ_FOR_EACH_M_I(r, PDK_PP_TUPLE_ELEM(4, 0, x), PDK_PP_TUPLE_ELEM(4, 1, x), PDK_PP_TUPLE_ELEM(4, 2, x), PDK_PP_TUPLE_ELEM(4, 3, x))
# endif
#
# define PDK_PP_SEQ_FOR_EACH_M_I(r, macro, data, seq, sz) macro(r, data, PDK_PP_SEQ_HEAD(seq))
#
# /* PDK_PP_SEQ_FOR_EACH_R */
#
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SEQ_FOR_EACH_R(r, macro, data, seq) PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_R(r, macro, data, seq)
# else
#    define PDK_PP_SEQ_FOR_EACH_R(r, macro, data, seq) PDK_PP_SEQ_FOR_EACH_R_I(r, macro, data, seq)
#    define PDK_PP_SEQ_FOR_EACH_R_I(r, macro, data, seq) PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_R(r, macro, data, seq)
# endif
#
#    define PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_EXEC_R(r, macro, data, seq) PDK_PP_FOR_ ## r((macro, data, seq, PDK_PP_SEQ_SIZE(seq)), PDK_PP_SEQ_FOR_EACH_P, PDK_PP_SEQ_FOR_EACH_O, PDK_PP_SEQ_FOR_EACH_M)
#    define PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_EMPTY_R(r, macro, data, seq)
#
#    define PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_R(r, macro, data, seq) \
		PDK_PP_IIF \
			( \
			PDK_PP_SEQ_DETAIL_IS_NOT_EMPTY(seq), \
			PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_EXEC_R, \
			PDK_PP_SEQ_FOR_EACH_DETAIL_CHECK_EMPTY_R \
			) \
		(r, macro, data, seq) \
/**/

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_FOREACH_H
