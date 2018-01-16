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

#ifndef PDK_STDEXT_PREPROCESSOR_SLOT_INTERNAL_DEF_H
#define PDK_STDEXT_PREPROCESSOR_SLOT_INTERNAL_DEF_H

// PDK_PP_SLOT_OFFSET_x
# define PDK_PP_SLOT_OFFSET_10(x) (x) % 1000000000UL
# define PDK_PP_SLOT_OFFSET_9(x) PDK_PP_SLOT_OFFSET_10(x) % 100000000UL
# define PDK_PP_SLOT_OFFSET_8(x) PDK_PP_SLOT_OFFSET_9(x) % 10000000UL
# define PDK_PP_SLOT_OFFSET_7(x) PDK_PP_SLOT_OFFSET_8(x) % 1000000UL
# define PDK_PP_SLOT_OFFSET_6(x) PDK_PP_SLOT_OFFSET_7(x) % 100000UL
# define PDK_PP_SLOT_OFFSET_5(x) PDK_PP_SLOT_OFFSET_6(x) % 10000UL
# define PDK_PP_SLOT_OFFSET_4(x) PDK_PP_SLOT_OFFSET_5(x) % 1000UL
# define PDK_PP_SLOT_OFFSET_3(x) PDK_PP_SLOT_OFFSET_4(x) % 100UL
# define PDK_PP_SLOT_OFFSET_2(x) PDK_PP_SLOT_OFFSET_3(x) % 10UL

// PDK_PP_SLOT_CC_x
# define PDK_PP_SLOT_CC_2(a, b) PDK_PP_SLOT_CC_2_D(a, b)
# define PDK_PP_SLOT_CC_3(a, b, c) PDK_PP_SLOT_CC_3_D(a, b, c)
# define PDK_PP_SLOT_CC_4(a, b, c, d) PDK_PP_SLOT_CC_4_D(a, b, c, d)
# define PDK_PP_SLOT_CC_5(a, b, c, d, e) PDK_PP_SLOT_CC_5_D(a, b, c, d, e)
# define PDK_PP_SLOT_CC_6(a, b, c, d, e, f) PDK_PP_SLOT_CC_6_D(a, b, c, d, e, f)
# define PDK_PP_SLOT_CC_7(a, b, c, d, e, f, g) PDK_PP_SLOT_CC_7_D(a, b, c, d, e, f, g)
# define PDK_PP_SLOT_CC_8(a, b, c, d, e, f, g, h) PDK_PP_SLOT_CC_8_D(a, b, c, d, e, f, g, h)
# define PDK_PP_SLOT_CC_9(a, b, c, d, e, f, g, h, i) PDK_PP_SLOT_CC_9_D(a, b, c, d, e, f, g, h, i)
# define PDK_PP_SLOT_CC_10(a, b, c, d, e, f, g, h, i, j) PDK_PP_SLOT_CC_10_D(a, b, c, d, e, f, g, h, i, j)

# define PDK_PP_SLOT_CC_2_D(a, b) a ## b
# define PDK_PP_SLOT_CC_3_D(a, b, c) a ## b ## c
# define PDK_PP_SLOT_CC_4_D(a, b, c, d) a ## b ## c ## d
# define PDK_PP_SLOT_CC_5_D(a, b, c, d, e) a ## b ## c ## d ## e
# define PDK_PP_SLOT_CC_6_D(a, b, c, d, e, f) a ## b ## c ## d ## e ## f
# define PDK_PP_SLOT_CC_7_D(a, b, c, d, e, f, g) a ## b ## c ## d ## e ## f ## g
# define PDK_PP_SLOT_CC_8_D(a, b, c, d, e, f, g, h) a ## b ## c ## d ## e ## f ## g ## h
# define PDK_PP_SLOT_CC_9_D(a, b, c, d, e, f, g, h, i) a ## b ## c ## d ## e ## f ## g ## h ## i
# define PDK_PP_SLOT_CC_10_D(a, b, c, d, e, f, g, h, i, j) a ## b ## c ## d ## e ## f ## g ## h ## i ## j

#endif // PDK_STDEXT_PREPROCESSOR_SLOT_INTERNAL_DEF_H
