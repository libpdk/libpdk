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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_FOLD_RIGHT_H
#define PDK_STDEXT_PREPROCESSOR_LIST_FOLD_RIGHT_H

#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/control/While.h"
#include "pdk/stdext/preprocessor/internal/AutoRec.h"

# if 0
#    define PDK_PP_LIST_FOLD_RIGHT(op, state, list)
# endif
#
# define PDK_PP_LIST_FOLD_RIGHT PDK_PP_CAT(PDK_PP_LIST_FOLD_RIGHT_, PDK_PP_AUTO_REC(PDK_PP_WHILE_P, 256))
#
# define PDK_PP_LIST_FOLD_RIGHT_257(o, s, l) PDK_PP_ERROR(0x0004)
#
# define PDK_PP_LIST_FOLD_RIGHT_D(d, o, s, l) PDK_PP_LIST_FOLD_RIGHT_ ## d(o, s, l)
# define PDK_PP_LIST_FOLD_RIGHT_2ND PDK_PP_LIST_FOLD_RIGHT
# define PDK_PP_LIST_FOLD_RIGHT_2ND_D PDK_PP_LIST_FOLD_RIGHT_D
#
# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    include "pdk/stdext/preprocessor/list/internal/edg/FoldRight.h"
# else
#    include "pdk/stdext/preprocessor/list/internal/FoldRight.h"
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_FOLD_RIGHT_H
