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

#ifndef PDK_STDEXT_PREPROCESSOR_INTERNAL_SPLIT_H
#define PDK_STDEXT_PREPROCESSOR_INTERNAL_SPLIT_H

// PDK_PP_SPLIT

# if PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MWCC()
#    define PDK_PP_SPLIT(n, im) PDK_PP_SPLIT_I((n, im))
#    define PDK_PP_SPLIT_I(par) PDK_PP_SPLIT_II ## par
#    define PDK_PP_SPLIT_II(n, a, b) PDK_PP_SPLIT_ ## n(a, b)
# elif PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_MSVC()
#    define PDK_PP_SPLIT(n, im) PDK_PP_SPLIT_I(n((im)))
#    define PDK_PP_SPLIT_I(n) PDK_PP_SPLIT_ID(PDK_PP_SPLIT_II_ ## n)
#    define PDK_PP_SPLIT_II_0(s) PDK_PP_SPLIT_ID(PDK_PP_SPLIT_0 s)
#    define PDK_PP_SPLIT_II_1(s) PDK_PP_SPLIT_ID(PDK_PP_SPLIT_1 s)
#    define PDK_PP_SPLIT_ID(id) id
# else
#    define PDK_PP_SPLIT(n, im) PDK_PP_SPLIT_I(n)(im)
#    define PDK_PP_SPLIT_I(n) PDK_PP_SPLIT_ ## n
# endif

# define PDK_PP_SPLIT_0(a, b) a
# define PDK_PP_SPLIT_1(a, b) b

#endif // PDK_STDEXT_PREPROCESSOR_INTERNAL_SPLIT_H
