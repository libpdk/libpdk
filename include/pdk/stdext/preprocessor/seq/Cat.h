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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_CAT_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_CAT_H

# include "pdk/stdext/preprocessor/config/Config.h"
# include "pdk/stdext/preprocessor/arithmetic/Dec.h"
# include "pdk/stdext/preprocessor/control/If.h"
# include "pdk/stdext/preprocessor/seq/Seq.h"
# include "pdk/stdext/preprocessor/seq/FoldLeft.h"
# include "pdk/stdext/preprocessor/seq/Size.h"
# include "pdk/stdext/preprocessor/tuple/Eat.h"

// PDK_PP_SEQ_CAT
# define PDK_PP_SEQ_CAT(seq) \
    PDK_PP_IF( \
        PDK_PP_DEC(PDK_PP_SEQ_SIZE(seq)), \
        PDK_PP_SEQ_CAT_I, \
        PDK_PP_SEQ_HEAD \
    )(seq) \
    /**/
# define PDK_PP_SEQ_CAT_I(seq) PDK_PP_SEQ_FOLD_LEFT(PDK_PP_SEQ_CAT_O, PDK_PP_SEQ_HEAD(seq), PDK_PP_SEQ_TAIL(seq))
#
# define PDK_PP_SEQ_CAT_O(s, st, elem) PDK_PP_SEQ_CAT_O_I(st, elem)
# define PDK_PP_SEQ_CAT_O_I(a, b) a ## b

// PDK_PP_SEQ_CAT_S
# define PDK_PP_SEQ_CAT_S(s, seq) \
    PDK_PP_IF( \
        PDK_PP_DEC(PDK_PP_SEQ_SIZE(seq)), \
        PDK_PP_SEQ_CAT_S_I_A, \
        PDK_PP_SEQ_CAT_S_I_B \
    )(s, seq) \
    /**/
# define PDK_PP_SEQ_CAT_S_I_A(s, seq) PDK_PP_SEQ_FOLD_LEFT_ ## s(PDK_PP_SEQ_CAT_O, PDK_PP_SEQ_HEAD(seq), PDK_PP_SEQ_TAIL(seq))
# define PDK_PP_SEQ_CAT_S_I_B(s, seq) PDK_PP_SEQ_HEAD(seq)

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_CAT_H
