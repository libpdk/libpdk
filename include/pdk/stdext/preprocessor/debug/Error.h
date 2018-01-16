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

#ifndef PDK_STDEXT_PREPROCESSOR_DEBUG_ERROR_H
#define PDK_STDEXT_PREPROCESSOR_DEBUG_ERROR_H

#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/Cat.h"

// PDK_PP_ERROR

# if PDK_PP_CONFIG_ERRORS
#    define PDK_PP_ERROR(code) PDK_PP_CAT(PDK_PP_ERROR_, code)
# endif

# define PDK_PP_ERROR_0x0000 PDK_PP_ERROR(0x0000, PDK_PP_INDEX_OUT_OF_BOUNDS)
# define PDK_PP_ERROR_0x0001 PDK_PP_ERROR(0x0001, PDK_PP_WHILE_OVERFLOW)
# define PDK_PP_ERROR_0x0002 PDK_PP_ERROR(0x0002, PDK_PP_FOR_OVERFLOW)
# define PDK_PP_ERROR_0x0003 PDK_PP_ERROR(0x0003, PDK_PP_REPEAT_OVERFLOW)
# define PDK_PP_ERROR_0x0004 PDK_PP_ERROR(0x0004, PDK_PP_LIST_FOLD_OVERFLOW)
# define PDK_PP_ERROR_0x0005 PDK_PP_ERROR(0x0005, PDK_PP_SEQ_FOLD_OVERFLOW)
# define PDK_PP_ERROR_0x0006 PDK_PP_ERROR(0x0006, PDK_PP_ARITHMETIC_OVERFLOW)
# define PDK_PP_ERROR_0x0007 PDK_PP_ERROR(0x0007, PDK_PP_DIVISION_BY_ZERO)

#endif // PDK_STDEXT_PREPROCESSOR_DEBUG_ERROR_H
