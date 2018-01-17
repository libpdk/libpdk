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

#ifndef PDK_STDEXT_PREPROCESSOR_ARRAY_POP_BACK_H
#define PDK_STDEXT_PREPROCESSOR_ARRAY_POP_BACK_H

#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/array/Element.h"
#include "pdk/stdext/preprocessor/array/Size.h"
#include "pdk/stdext/preprocessor/repetition/Enum.h"
#include "pdk/stdext/preprocessor/repetition/DeduceZ.h"

// PDK_PP_ARRAY_POP_BACK
# define PDK_PP_ARRAY_POP_BACK(array) PDK_PP_ARRAY_POP_BACK_Z(PDK_PP_DEDUCE_Z(), array)

// PDK_PP_ARRAY_POP_BACK_Z
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_EDG()
#    define PDK_PP_ARRAY_POP_BACK_Z(z, array) PDK_PP_ARRAY_POP_BACK_I(z, PDK_PP_ARRAY_SIZE(array), array)
# else
#    define PDK_PP_ARRAY_POP_BACK_Z(z, array) PDK_PP_ARRAY_POP_BACK_Z_D(z, array)
#    define PDK_PP_ARRAY_POP_BACK_Z_D(z, array) PDK_PP_ARRAY_POP_BACK_I(z, PDK_PP_ARRAY_SIZE(array), array)
# endif

# define PDK_PP_ARRAY_POP_BACK_I(z, size, array) (PDK_PP_DEC(size), (PDK_PP_ENUM_ ## z(PDK_PP_DEC(size), PDK_PP_ARRAY_POP_BACK_M, array)))
# define PDK_PP_ARRAY_POP_BACK_M(z, n, data) PDK_PP_ARRAY_ELEM(n, data)

#endif // PDK_STDEXT_PREPROCESSOR_ARRAY_POP_BACK_H
