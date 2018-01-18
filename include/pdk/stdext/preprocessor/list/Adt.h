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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_ADT_H
#define PDK_STDEXT_PREPROCESSOR_LIST_ADT_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/internal/IsBinary.h"
#include "pdk/stdext/preprocessor/logical/Compl.h"
#include "pdk/stdext/preprocessor/tuple/Eat.h"

// PDK_PP_LIST_CONS
# define PDK_PP_LIST_CONS(head, tail) (head, tail)

// PDK_PP_LIST_NIL

# define PDK_PP_LIST_NIL PDK_PP_NIL

// PDK_PP_LIST_FIRST

# define PDK_PP_LIST_FIRST(list) PDK_PP_LIST_FIRST_D(list)
#
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MWCC()
#    define PDK_PP_LIST_FIRST_D(list) PDK_PP_LIST_FIRST_I list
# else
#    define PDK_PP_LIST_FIRST_D(list) PDK_PP_LIST_FIRST_I ## list
# endif

# define PDK_PP_LIST_FIRST_I(head, tail) head

// PDK_PP_LIST_REST

# define PDK_PP_LIST_REST(list) PDK_PP_LIST_REST_D(list)
#
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MWCC()
#    define PDK_PP_LIST_REST_D(list) PDK_PP_LIST_REST_I list
# else
#    define PDK_PP_LIST_REST_D(list) PDK_PP_LIST_REST_I ## list
# endif

# define PDK_PP_LIST_REST_I(head, tail) tail

# /* PDK_PP_LIST_IS_CONS */

# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_BCC()
#    define PDK_PP_LIST_IS_CONS(list) PDK_PP_LIST_IS_CONS_D(list)
#    define PDK_PP_LIST_IS_CONS_D(list) PDK_PP_LIST_IS_CONS_ ## list
#    define PDK_PP_LIST_IS_CONS_(head, tail) 1
#    define PDK_PP_LIST_IS_CONS_PDK_PP_NIL 0
# else
#    define PDK_PP_LIST_IS_CONS(list) PDK_PP_IS_BINARY(list)
# endif

// PDK_PP_LIST_IS_NIL

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_BCC()
#    define PDK_PP_LIST_IS_NIL(list) PDK_PP_COMPL(PDK_PP_IS_BINARY(list))
# else
#    define PDK_PP_LIST_IS_NIL(list) PDK_PP_COMPL(PDK_PP_LIST_IS_CONS(list))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_ADT_H
