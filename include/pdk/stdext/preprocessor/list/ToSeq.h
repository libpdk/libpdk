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

#ifndef PDK_STDEXT_PREPROCESSOR_LIST_TO_SEQ_H
#define PDK_STDEXT_PREPROCESSOR_LIST_TO_SEQ_H

#include "pdk/stdext/preprocessor/list/ForEach.h"

// PDK_PP_LIST_TO_SEQ
# define PDK_PP_LIST_TO_SEQ(list) \
    PDK_PP_LIST_FOR_EACH(PDK_PP_LIST_TO_SEQ_MACRO, ~, list) \
    /**/
# define PDK_PP_LIST_TO_SEQ_MACRO(r, data, elem) (elem)

// PDK_PP_LIST_TO_SEQ_R
# define PDK_PP_LIST_TO_SEQ_R(r, list) \
    PDK_PP_LIST_FOR_EACH_R(r, PDK_PP_LIST_TO_SEQ_MACRO, ~, list) \
    /**/

#endif // PDK_STDEXT_PREPROCESSOR_LIST_TO_SEQ_H
