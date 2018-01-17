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

#ifndef PDK_STDEXT_PREPROCESSOR_TO_SEQ_H
#define PDK_STDEXT_PREPROCESSOR_TO_SEQ_H

#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/array/Size.h"
#include "pdk/stdext/preprocessor/control/If.h"
#include "pdk/stdext/preprocessor/tuple/ToSeq.h"

# /* PDK_PP_ARRAY_TO_SEQ */
#
#    define PDK_PP_ARRAY_TO_SEQ(array) \
		PDK_PP_IF \
			( \
			PDK_PP_ARRAY_SIZE(array), \
			PDK_PP_ARRAY_TO_SEQ_DO, \
			PDK_PP_ARRAY_TO_SEQ_EMPTY \
			) \
		(array) \
/**/
#    define PDK_PP_ARRAY_TO_SEQ_EMPTY(array)
#
# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()
#    define PDK_PP_ARRAY_TO_SEQ_DO(array) PDK_PP_ARRAY_TO_SEQ_I(PDK_PP_TUPLE_TO_SEQ, array)
#    define PDK_PP_ARRAY_TO_SEQ_I(m, args) PDK_PP_ARRAY_TO_SEQ_II(m, args)
#    define PDK_PP_ARRAY_TO_SEQ_II(m, args) PDK_PP_CAT(m ## args,)
# elif PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MWCC()
#    define PDK_PP_ARRAY_TO_SEQ_DO(array) PDK_PP_ARRAY_TO_SEQ_I(array)
#    define PDK_PP_ARRAY_TO_SEQ_I(array) PDK_PP_TUPLE_TO_SEQ ## array
# else
#    define PDK_PP_ARRAY_TO_SEQ_DO(array) PDK_PP_TUPLE_TO_SEQ array
# endif

#endif // PDK_STDEXT_PREPROCESSOR_TO_SEQ_H
