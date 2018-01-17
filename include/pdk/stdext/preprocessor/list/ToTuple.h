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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_TO_TUPLE_H
#define PDK_STDEXT_PREPROCESSOR_LIST_TO_TUPLE_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/list/Enum.h"
#include "pdk/stdext/preprocessor/control/Iif.h"

// PDK_PP_LIST_TO_TUPLE
# define PDK_PP_LIST_TO_TUPLE(list) \
	PDK_PP_IIF \
		( \
		PDK_PP_LIST_IS_NIL(list), \
		PDK_PP_LIST_TO_TUPLE_EMPTY, \
		PDK_PP_LIST_TO_TUPLE_DO \
		) \
	(list) \
/**/
# define PDK_PP_LIST_TO_TUPLE_EMPTY(list)

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_TO_TUPLE_DO(list) (PDK_PP_LIST_ENUM(list))
# else
#    define PDK_PP_LIST_TO_TUPLE_DO(list) PDK_PP_LIST_TO_TUPLE_I(list)
#    define PDK_PP_LIST_TO_TUPLE_I(list) (PDK_PP_LIST_ENUM(list))
# endif

// PDK_PP_LIST_TO_TUPLE_R
# define PDK_PP_LIST_TO_TUPLE_R(r, list) \
	PDK_PP_IIF \
		( \
		PDK_PP_LIST_IS_NIL(list), \
		PDK_PP_LIST_TO_TUPLE_R_EMPTY, \
		PDK_PP_LIST_TO_TUPLE_R_DO \
		) \
	(r, list) \
/**/
# define PDK_PP_LIST_TO_TUPLE_R_EMPTY(r,list)

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_TO_TUPLE_R_DO(r, list) (PDK_PP_LIST_ENUM_R(r, list))
# else
#    define PDK_PP_LIST_TO_TUPLE_R_DO(r, list) PDK_PP_LIST_TO_TUPLE_R_I(r, list)
#    define PDK_PP_LIST_TO_TUPLE_R_I(r, list) (PDK_PP_LIST_ENUM_R(r, list))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_TO_TUPLE_H
