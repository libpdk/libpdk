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

#ifndef PDK_STDEXT_PREPROCESSOR_ARRAY_INTERNAL_GET_DATA_H
#define PDK_STDEXT_PREPROCESSOR_ARRAY_INTERNAL_GET_DATA_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/If.h"
#include "pdk/stdext/preprocessor/control/Iif.h"
#include "pdk/stdext/preprocessor/facilities/Is1.h"

// PDK_PP_ARRAY_DETAIL_GET_DATA
# define PDK_PP_ARRAY_DETAIL_GET_DATA_NONE(size, data)

# if PDK_PP_VARIADICS && !(PDK_PP_VARIADICS_MSVC && _MSC_VER <= 1400)
# 	 if PDK_PP_VARIADICS_MSVC
# 		define PDK_PP_ARRAY_DETAIL_GET_DATA_ANY_VC_DEFAULT(size, data) PDK_PP_TUPLE_REM(size) data
# 		define PDK_PP_ARRAY_DETAIL_GET_DATA_ANY_VC_CAT(size, data) PDK_PP_TUPLE_REM_CAT(size) data
# 		define PDK_PP_ARRAY_DETAIL_GET_DATA_ANY(size, data) \
			PDK_PP_IIF \
				( \
				PDK_PP_IS_1(size), \
				PDK_PP_ARRAY_DETAIL_GET_DATA_ANY_VC_CAT, \
				PDK_PP_ARRAY_DETAIL_GET_DATA_ANY_VC_DEFAULT \
				) \
			(size,data) \
/**/
#    else
# 		define PDK_PP_ARRAY_DETAIL_GET_DATA_ANY(size, data) PDK_PP_TUPLE_REM(size) data
#    endif
# else
# 	 define PDK_PP_ARRAY_DETAIL_GET_DATA_ANY(size, data) PDK_PP_TUPLE_REM(size) data
# endif

# define PDK_PP_ARRAY_DETAIL_GET_DATA(size, data) \
	PDK_PP_IF \
		( \
		size, \
		PDK_PP_ARRAY_DETAIL_GET_DATA_ANY, \
		PDK_PP_ARRAY_DETAIL_GET_DATA_NONE \
		) \
	(size,data) \
/**/

#endif // PDK_STDEXT_PREPROCESSOR_ARRAY_INTERNAL_GET_DATA_H
