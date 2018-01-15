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

#ifndef PDK_STDEXT_PREPROCESSOR_INTERNAL_CHECK_H
#define PDK_STDEXT_PREPROCESSOR_INTERNAL_CHECK_H

#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/config/Config.h"

// PDK_PP_CHECK

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MWCC()
#    define PDK_PP_CHECK(x, type) PDK_PP_CHECK_D(x, type)
# else
#    define PDK_PP_CHECK(x, type) PDK_PP_CHECK_OO((x, type))
#    define PDK_PP_CHECK_OO(par) PDK_PP_CHECK_D ## par
# endif

# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC() && ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_DMC()
#    define PDK_PP_CHECK_D(x, type) PDK_PP_CHECK_1(PDK_PP_CAT(PDK_PP_CHECK_RESULT_, type x))
#    define PDK_PP_CHECK_1(chk) PDK_PP_CHECK_2(chk)
#    define PDK_PP_CHECK_2(res, _) res
# elif PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()
#    define PDK_PP_CHECK_D(x, type) PDK_PP_CHECK_1(type x)
#    define PDK_PP_CHECK_1(chk) PDK_PP_CHECK_2(chk)
#    define PDK_PP_CHECK_2(chk) PDK_PP_CHECK_3((PDK_PP_CHECK_RESULT_ ## chk))
#    define PDK_PP_CHECK_3(im) PDK_PP_CHECK_5(PDK_PP_CHECK_4 im)
#    define PDK_PP_CHECK_4(res, _) res
#    define PDK_PP_CHECK_5(res) res
# else /* DMC */
#    define PDK_PP_CHECK_D(x, type) PDK_PP_CHECK_OO((type x))
#    define PDK_PP_CHECK_OO(par) PDK_PP_CHECK_0 ## par
#    define PDK_PP_CHECK_0(chk) PDK_PP_CHECK_1(PDK_PP_CAT(PDK_PP_CHECK_RESULT_, chk))
#    define PDK_PP_CHECK_1(chk) PDK_PP_CHECK_2(chk)
#    define PDK_PP_CHECK_2(res, _) res
# endif

# define PDK_PP_CHECK_RESULT_1 1, PDK_PP_NIL

#endif // PDK_STDEXT_PREPROCESSOR_INTERNAL_CHECK_H
