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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_TO_LIST_MSVC_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_TO_LIST_MSVC_H

#include "pdk/stdext/preprocessor/config/Config.h"

# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()

# include "pdk/stdext/preprocessor/Cat.h"
# include "pdk/stdext/preprocessor/arithmetic/Dec.h"
# include "pdk/stdext/preprocessor/control/While.h"
# include "pdk/stdext/preprocessor/tuple/Element.h"

# define PDK_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_RESULT(state) \
    PDK_PP_TUPLE_ELEM(2, 0, state) \
/**/
# define PDK_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_SIZE(state) \
    PDK_PP_TUPLE_ELEM(2, 1, state) \
/**/
# define PDK_PP_SEQ_DETAIL_TO_LIST_MSVC_PRED(d,state) \
    PDK_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_SIZE(state) \
/**/
# define PDK_PP_SEQ_DETAIL_TO_LIST_MSVC_OP(d,state) \
    ( \
    PDK_PP_CAT(PDK_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_RESULT(state),), \
    PDK_PP_DEC(PDK_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_SIZE(state)) \
    ) \
/**/
#
# /* PDK_PP_SEQ_DETAIL_TO_LIST_MSVC */
#
# define PDK_PP_SEQ_DETAIL_TO_LIST_MSVC(result,seqsize) \
    PDK_PP_SEQ_DETAIL_TO_LIST_MSVC_STATE_RESULT \
        ( \
        PDK_PP_WHILE \
            ( \
            PDK_PP_SEQ_DETAIL_TO_LIST_MSVC_PRED, \
            PDK_PP_SEQ_DETAIL_TO_LIST_MSVC_OP, \
            (result,seqsize) \
            ) \
        ) \
/**/
# endif // PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_TO_LIST_MSVC_H
