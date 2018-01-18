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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_SEQ_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_SEQ_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/seq/Element.h"

// PDK_PP_SEQ_HEAD
# define PDK_PP_SEQ_HEAD(seq) PDK_PP_SEQ_ELEM(0, seq)

// PDK_PP_SEQ_TAIL
# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MWCC()
#    define PDK_PP_SEQ_TAIL(seq) PDK_PP_SEQ_TAIL_1((seq))
#    define PDK_PP_SEQ_TAIL_1(par) PDK_PP_SEQ_TAIL_2 ## par
#    define PDK_PP_SEQ_TAIL_2(seq) PDK_PP_SEQ_TAIL_I ## seq
# elif PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()
#    define PDK_PP_SEQ_TAIL(seq) PDK_PP_SEQ_TAIL_ID(PDK_PP_SEQ_TAIL_I seq)
#    define PDK_PP_SEQ_TAIL_ID(id) id
# elif PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SEQ_TAIL(seq) PDK_PP_SEQ_TAIL_D(seq)
#    define PDK_PP_SEQ_TAIL_D(seq) PDK_PP_SEQ_TAIL_I seq
# else
#    define PDK_PP_SEQ_TAIL(seq) PDK_PP_SEQ_TAIL_I seq
# endif
#
# define PDK_PP_SEQ_TAIL_I(x)

// PDK_PP_SEQ_NIL
# define PDK_PP_SEQ_NIL(x) (x)

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_SEQ_H
