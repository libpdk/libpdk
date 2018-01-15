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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_AT_H
#define PDK_STDEXT_PREPROCESSOR_LIST_AT_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/list/Adt.h"
#include "pdk/stdext/preprocessor/list/RestN.h"

// PDK_PP_LIST_AT
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_AT(list, index) PDK_PP_LIST_FIRST(PDK_PP_LIST_REST_N(index, list))
# else
#    define PDK_PP_LIST_AT(list, index) PDK_PP_LIST_AT_I(list, index)
#    define PDK_PP_LIST_AT_I(list, index) PDK_PP_LIST_FIRST(PDK_PP_LIST_REST_N(index, list))
# endif

// PDK_PP_LIST_AT_D
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_LIST_AT_D(d, list, index) PDK_PP_LIST_FIRST(PDK_PP_LIST_REST_N_D(d, index, list))
# else
#    define PDK_PP_LIST_AT_D(d, list, index) PDK_PP_LIST_AT_D_I(d, list, index)
#    define PDK_PP_LIST_AT_D_I(d, list, index) PDK_PP_LIST_FIRST(PDK_PP_LIST_REST_N_D(d, index, list))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_LIST_AT_H
