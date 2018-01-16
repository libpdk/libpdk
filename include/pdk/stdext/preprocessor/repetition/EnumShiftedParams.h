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
// Created by softboy on 2018/01/16.

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

#ifndef PDK_STDEXT_PREPROCESSOR_REPETITION_ENUM_SHIFTED_PARAMS_H
#define PDK_STDEXT_PREPROCESSOR_REPETITION_ENUM_SHIFTED_PARAMS_H

#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/arithmetic/Inc.h"
#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/punctuation/CommaIf.h"
#include "pdk/stdext/preprocessor/repetition/Repeat.h"

// PDK_PP_ENUM_SHIFTED_PARAMS
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_ENUM_SHIFTED_PARAMS(count, param) PDK_PP_REPEAT(PDK_PP_DEC(count), PDK_PP_ENUM_SHIFTED_PARAMS_M, param)
# else
#    define PDK_PP_ENUM_SHIFTED_PARAMS(count, param) PDK_PP_ENUM_SHIFTED_PARAMS_I(count, param)
#    define PDK_PP_ENUM_SHIFTED_PARAMS_I(count, param) PDK_PP_REPEAT(PDK_PP_DEC(count), PDK_PP_ENUM_SHIFTED_PARAMS_M, param)
# endif

# define PDK_PP_ENUM_SHIFTED_PARAMS_M(z, n, param) PDK_PP_COMMA_IF(n) PDK_PP_CAT(param, PDK_PP_INC(n))

// PDK_PP_ENUM_SHIFTED_PARAMS_Z
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_ENUM_SHIFTED_PARAMS_Z(z, count, param) PDK_PP_REPEAT_ ## z(PDK_PP_DEC(count), PDK_PP_ENUM_SHIFTED_PARAMS_M, param)
# else
#    define PDK_PP_ENUM_SHIFTED_PARAMS_Z(z, count, param) PDK_PP_ENUM_SHIFTED_PARAMS_Z_I(z, count, param)
#    define PDK_PP_ENUM_SHIFTED_PARAMS_Z_I(z, count, param) PDK_PP_REPEAT_ ## z(PDK_PP_DEC(count), PDK_PP_ENUM_SHIFTED_PARAMS_M, param)
# endif

#endif // PDK_STDEXT_PREPROCESSOR_REPETITION_ENUM_SHIFTED_PARAMS_H
