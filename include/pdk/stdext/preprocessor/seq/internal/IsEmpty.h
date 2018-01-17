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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_INTERNAL_IS_EMPTY_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_INTERNAL_IS_EMPTY_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/logical/Bool.h"
#include "pdk/stdext/preprocessor/logical/Compl.h"
#include "pdk/stdext/preprocessor/seq/Size.h"

/* An empty seq is one that is just PDK_PP_SEQ_NIL */
# define PDK_PP_SEQ_DETAIL_IS_EMPTY(seq) \
	PDK_PP_COMPL \
		( \
		PDK_PP_SEQ_DETAIL_IS_NOT_EMPTY(seq) \
		) \
/**/
#
# define PDK_PP_SEQ_DETAIL_IS_EMPTY_SIZE(size) \
	PDK_PP_COMPL \
		( \
		PDK_PP_SEQ_DETAIL_IS_NOT_EMPTY_SIZE(size) \
		) \
/**/
#
# define PDK_PP_SEQ_DETAIL_IS_NOT_EMPTY(seq) \
	PDK_PP_SEQ_DETAIL_IS_NOT_EMPTY_SIZE(PDK_PP_SEQ_DETAIL_EMPTY_SIZE(seq)) \
/**/
#
# define PDK_PP_SEQ_DETAIL_IS_NOT_EMPTY_SIZE(size) \
	PDK_PP_BOOL(size) \
/**/
#
# define PDK_PP_SEQ_DETAIL_EMPTY_SIZE(seq) \
	PDK_PP_DEC(PDK_PP_SEQ_SIZE(seq (nil))) \
/**/

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_INTERNAL_IS_EMPTY_H
