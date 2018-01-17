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
// Created by softboy on 2018/01/17.

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

#ifndef PDK_STDEXT_PREPROCESSOR_ARRAY_PUSH_FRONT_H
#define PDK_STDEXT_PREPROCESSOR_ARRAY_PUSH_FRONT_H

#include "pdk/stdext/preprocessor/arithmetic/Inc.h"
#include "pdk/stdext/preprocessor/array/Data.h"
#include "pdk/stdext/preprocessor/array/Size.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/punctuation/CommaIf.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"
#include "pdk/stdext/preprocessor/array/internal/GetData.h"

// PDK_PP_ARRAY_PUSH_FRONT
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_ARRAY_PUSH_FRONT(array, elem) PDK_PP_ARRAY_PUSH_FRONT_I(PDK_PP_ARRAY_SIZE(array), PDK_PP_ARRAY_DATA(array), elem)
# else
#    define PDK_PP_ARRAY_PUSH_FRONT(array, elem) PDK_PP_ARRAY_PUSH_FRONT_D(array, elem)
#    define PDK_PP_ARRAY_PUSH_FRONT_D(array, elem) PDK_PP_ARRAY_PUSH_FRONT_I(PDK_PP_ARRAY_SIZE(array), PDK_PP_ARRAY_DATA(array), elem)
# endif

# define PDK_PP_ARRAY_PUSH_FRONT_I(size, data, elem) (PDK_PP_INC(size), (elem PDK_PP_COMMA_IF(size) PDK_PP_ARRAY_DETAIL_GET_DATA(size,data)))

#endif // PDK_STDEXT_PREPROCESSOR_ARRAY_PUSH_FRONT_H
