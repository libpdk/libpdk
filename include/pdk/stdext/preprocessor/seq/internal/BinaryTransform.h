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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_INTERNAL_BINARY_TRANSFORM_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_INTERNAL_BINARY_TRANSFORM_H

#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/tuple/Eat.h"
#include "pdk/stdext/preprocessor/tuple/Rem.h"
#include "pdk/stdext/preprocessor/variadic/internal/IsSingleReturn.h"

// PDK_PP_SEQ_BINARY_TRANSFORM
# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()
#    define PDK_PP_SEQ_BINARY_TRANSFORM(seq) PDK_PP_SEQ_BINARY_TRANSFORM_I(, seq)
#    define PDK_PP_SEQ_BINARY_TRANSFORM_I(p, seq) PDK_PP_SEQ_BINARY_TRANSFORM_II(p ## seq)
#    define PDK_PP_SEQ_BINARY_TRANSFORM_II(seq) PDK_PP_SEQ_BINARY_TRANSFORM_III(seq)
#    define PDK_PP_SEQ_BINARY_TRANSFORM_III(seq) PDK_PP_CAT(PDK_PP_SEQ_BINARY_TRANSFORM_A seq, 0)
# else
#    define PDK_PP_SEQ_BINARY_TRANSFORM(seq) PDK_PP_CAT(PDK_PP_SEQ_BINARY_TRANSFORM_A seq, 0)
# endif
# if PDK_PP_VARIADICS
#    if PDK_PP_VARIADICS_MSVC
#		define PDK_PP_SEQ_BINARY_TRANSFORM_REM(data) data
#       define PDK_PP_SEQ_BINARY_TRANSFORM_A(...) (PDK_PP_SEQ_BINARY_TRANSFORM_REM, __VA_ARGS__)() PDK_PP_SEQ_BINARY_TRANSFORM_B
#       define PDK_PP_SEQ_BINARY_TRANSFORM_B(...) (PDK_PP_SEQ_BINARY_TRANSFORM_REM, __VA_ARGS__)() PDK_PP_SEQ_BINARY_TRANSFORM_A
#	 else
#       define PDK_PP_SEQ_BINARY_TRANSFORM_A(...) (PDK_PP_REM, __VA_ARGS__)() PDK_PP_SEQ_BINARY_TRANSFORM_B
#       define PDK_PP_SEQ_BINARY_TRANSFORM_B(...) (PDK_PP_REM, __VA_ARGS__)() PDK_PP_SEQ_BINARY_TRANSFORM_A
#	 endif
# else
#    define PDK_PP_SEQ_BINARY_TRANSFORM_A(e) (PDK_PP_REM, e)() PDK_PP_SEQ_BINARY_TRANSFORM_B
#    define PDK_PP_SEQ_BINARY_TRANSFORM_B(e) (PDK_PP_REM, e)() PDK_PP_SEQ_BINARY_TRANSFORM_A
# endif
# define PDK_PP_SEQ_BINARY_TRANSFORM_A0 (PDK_PP_EAT, ?)
# define PDK_PP_SEQ_BINARY_TRANSFORM_B0 (PDK_PP_EAT, ?)

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_INTERNAL_BINARY_TRANSFORM_H
