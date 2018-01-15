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

#ifndef PDK_STDEXT_PREPROCESSOR_ARITHMETIC_SUB_H
#define PDK_STDEXT_PREPROCESSOR_ARITHMETIC_SUB_H

#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/While.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"

// PDK_PP_SUB
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SUB(x, y) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE(PDK_PP_SUB_P, PDK_PP_SUB_O, (x, y)))
# else
#    define PDK_PP_SUB(x, y) PDK_PP_SUB_I(x, y)
#    define PDK_PP_SUB_I(x, y) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE(PDK_PP_SUB_P, PDK_PP_SUB_O, (x, y)))
# endif

# define PDK_PP_SUB_P(d, xy) PDK_PP_TUPLE_ELEM(2, 1, xy)

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MWCC()
#    define PDK_PP_SUB_O(d, xy) PDK_PP_SUB_O_I xy
# else
#    define PDK_PP_SUB_O(d, xy) PDK_PP_SUB_O_I(PDK_PP_TUPLE_ELEM(2, 0, xy), PDK_PP_TUPLE_ELEM(2, 1, xy))
# endif

# define PDK_PP_SUB_O_I(x, y) (PDK_PP_DEC(x), PDK_PP_DEC(y))

// PDK_PP_SUB_D
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_SUB_D(d, x, y) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE_ ## d(PDK_PP_SUB_P, PDK_PP_SUB_O, (x, y)))
# else
#    define PDK_PP_SUB_D(d, x, y) PDK_PP_SUB_D_I(d, x, y)
#    define PDK_PP_SUB_D_I(d, x, y) PDK_PP_TUPLE_ELEM(2, 0, PDK_PP_WHILE_ ## d(PDK_PP_SUB_P, PDK_PP_SUB_O, (x, y)))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_ARITHMETIC_SUB_H
