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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_FOR_EACH_H
#define PDK_STDEXT_PREPROCESSOR_LIST_FOR_EACH_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/list/ForEachI.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"

// PDK_PP_LIST_FOR_EACH
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FOR_EACH(macro, data, list) PDK_PP_LIST_FOR_EACH_I(PDK_PP_LIST_FOR_EACH_O, (macro, data), list)
# else
#    define PDK_PP_LIST_FOR_EACH(macro, data, list) PDK_PP_LIST_FOR_EACH_X(macro, data, list)
#    define PDK_PP_LIST_FOR_EACH_X(macro, data, list) PDK_PP_LIST_FOR_EACH_I(PDK_PP_LIST_FOR_EACH_O, (macro, data), list)
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FOR_EACH_O(r, md, i, elem) PDK_PP_LIST_FOR_EACH_O_D(r, PDK_PP_TUPLE_ELEM(2, 0, md), PDK_PP_TUPLE_ELEM(2, 1, md), elem)
# else
#    define PDK_PP_LIST_FOR_EACH_O(r, md, i, elem) PDK_PP_LIST_FOR_EACH_O_I(r, PDK_PP_TUPLE_REM_2 md, elem)
#    define PDK_PP_LIST_FOR_EACH_O_I(r, im, elem) PDK_PP_LIST_FOR_EACH_O_D(r, im, elem)
# endif
#
# define PDK_PP_LIST_FOR_EACH_O_D(r, m, d, elem) m(r, d, elem)

// PDK_PP_LIST_FOR_EACH_R
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_FOR_EACH_R(r, macro, data, list) PDK_PP_LIST_FOR_EACH_I_R(r, PDK_PP_LIST_FOR_EACH_O, (macro, data), list)
# else
#    define PDK_PP_LIST_FOR_EACH_R(r, macro, data, list) PDK_PP_LIST_FOR_EACH_R_X(r, macro, data, list)
#    define PDK_PP_LIST_FOR_EACH_R_X(r, macro, data, list) PDK_PP_LIST_FOR_EACH_I_R(r, PDK_PP_LIST_FOR_EACH_O, (macro, data), list)
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_FOR_EACH_H
