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

#ifndef PDK_STDEXT_PREPROCESSOR_SLOT_SLOT_H
#define PDK_STDEXT_PREPROCESSOR_SLOT_SLOT_H

# include "pdk/stdext/preprocessor/Cat.h"
# include "pdk/stdext/preprocessor/slot/internal/Def.h"

// PDK_PP_ASSIGN_SLOT
# define PDK_PP_ASSIGN_SLOT(i) PDK_PP_CAT(PDK_PP_ASSIGN_SLOT_, i)
#
# define PDK_PP_ASSIGN_SLOT_1 "pdk/stdext/preprocessor/slot/internal/Slot1.h"
# define PDK_PP_ASSIGN_SLOT_2 "pdk/stdext/preprocessor/slot/internal/Slot2.h"
# define PDK_PP_ASSIGN_SLOT_3 "pdk/stdext/preprocessor/slot/internal/Slot3.h"
# define PDK_PP_ASSIGN_SLOT_4 "pdk/stdext/preprocessor/slot/internal/Slot4.h"
# define PDK_PP_ASSIGN_SLOT_5 "pdk/stdext/preprocessor/slot/internal/Slot5.h"

// PDK_PP_SLOT
# define PDK_PP_SLOT(i) PDK_PP_CAT(PDK_PP_SLOT_, i)()

#endif // PDK_STDEXT_PREPROCESSOR_SLOT_SLOT_H
