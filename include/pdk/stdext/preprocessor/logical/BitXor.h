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
// Created by softboy on 2018/01/14.

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

#ifndef PDK_STDEXT_PREPROCESSOR_LOGICAL_BIT_XOR_H
#define PDK_STDEXT_PREPROCESSOR_LOGICAL_BIT_XOR_H

#include "pdk/stdext/preprocessor/config/Config.h"

// PDK_PP_BITXOR

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MWCC()
#    define PDK_PP_BITXOR(x, y) PDK_PP_BITXOR_I(x, y)
# else
#    define PDK_PP_BITXOR(x, y) PDK_PP_BITXOR_OO((x, y))
#    define PDK_PP_BITXOR_OO(par) PDK_PP_BITXOR_I ## par
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()
#    define PDK_PP_BITXOR_I(x, y) PDK_PP_BITXOR_ ## x ## y
# else
#    define PDK_PP_BITXOR_I(x, y) PDK_PP_BITXOR_ID(PDK_PP_BITXOR_ ## x ## y)
#    define PDK_PP_BITXOR_ID(id) id
# endif

# define PDK_PP_BITXOR_00 0
# define PDK_PP_BITXOR_01 1
# define PDK_PP_BITXOR_10 1
# define PDK_PP_BITXOR_11 0

#endif // PDK_STDEXT_PREPROCESSOR_LOGICAL_BIT_XOR_H
