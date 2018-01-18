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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_TO_LIST_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_TO_LIST_H

#include "pdk/stdext/preprocessor/punctuation/Comma.h"
#include "pdk/stdext/preprocessor/punctuation/Paren.h"
#include "pdk/stdext/preprocessor/seq/internal/BinaryTransform.h"

// PDK_PP_SEQ_TO_LIST
# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()
# include "pdk/stdext/preprocessor/seq/Size.h"
# include "pdk/stdext/preprocessor/seq/internal/ToListMsvc.h"
# define PDK_PP_SEQ_TO_LIST(seq) \
    PDK_PP_SEQ_DETAIL_TO_LIST_MSVC \
        ( \
        PDK_PP_SEQ_TO_LIST_I(PDK_PP_SEQ_BINARY_TRANSFORM(seq)), \
        PDK_PP_SEQ_SIZE(seq) \
        ) \
/**/
# else
# define PDK_PP_SEQ_TO_LIST(seq) PDK_PP_SEQ_TO_LIST_I(PDK_PP_SEQ_BINARY_TRANSFORM(seq))
# endif
# define PDK_PP_SEQ_TO_LIST_I(bseq) PDK_PP_SEQ_TO_LIST_A bseq PDK_PP_NIL PDK_PP_SEQ_TO_LIST_B bseq
# define PDK_PP_SEQ_TO_LIST_A(m, e) m(PDK_PP_LPAREN() e PDK_PP_COMMA() PDK_PP_SEQ_TO_LIST_A_ID)
# define PDK_PP_SEQ_TO_LIST_A_ID() PDK_PP_SEQ_TO_LIST_A
# define PDK_PP_SEQ_TO_LIST_B(m, e) m(PDK_PP_RPAREN() PDK_PP_SEQ_TO_LIST_B_ID)
# define PDK_PP_SEQ_TO_LIST_B_ID() PDK_PP_SEQ_TO_LIST_B

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_TO_LIST_H
