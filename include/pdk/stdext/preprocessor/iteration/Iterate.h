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

#ifndef PDK_STDEXT_PREPROCESSOR_ITERATION_ITERATE_H
#define PDK_STDEXT_PREPROCESSOR_ITERATION_ITERATE_H

#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/arithmetic/Inc.h"
#include "pdk/stdext/preprocessor/array/Element.h"
#include "pdk/stdext/preprocessor/array/Size.h"
#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/slot/Slot.h"
#include "pdk/stdext/preprocessor/tuple/Element.h"

// PDK_PP_ITERATION_DEPTH
# define PDK_PP_ITERATION_DEPTH() 0

// PDK_PP_ITERATION
# define PDK_PP_ITERATION() PDK_PP_CAT(PDK_PP_ITERATION_, PDK_PP_ITERATION_DEPTH())

// PDK_PP_ITERATION_START && PDK_PP_ITERATION_FINISH
# define PDK_PP_ITERATION_START() PDK_PP_CAT(PDK_PP_ITERATION_START_, PDK_PP_ITERATION_DEPTH())
# define PDK_PP_ITERATION_FINISH() PDK_PP_CAT(PDK_PP_ITERATION_FINISH_, PDK_PP_ITERATION_DEPTH())

// PDK_PP_ITERATION_FLAGS
# define PDK_PP_ITERATION_FLAGS() (PDK_PP_CAT(PDK_PP_ITERATION_FLAGS_, PDK_PP_ITERATION_DEPTH())())

// PDK_PP_FRAME_ITERATION
# define PDK_PP_FRAME_ITERATION(i) PDK_PP_CAT(PDK_PP_ITERATION_, i)

// PDK_PP_FRAME_START && PDK_PP_FRAME_FINISH
# define PDK_PP_FRAME_START(i) PDK_PP_CAT(PDK_PP_ITERATION_START_, i)
# define PDK_PP_FRAME_FINISH(i) PDK_PP_CAT(PDK_PP_ITERATION_FINISH_, i)

// PDK_PP_FRAME_FLAGS
# define PDK_PP_FRAME_FLAGS(i) (PDK_PP_CAT(PDK_PP_ITERATION_FLAGS_, i)())

// PDK_PP_RELATIVE_ITERATION
# define PDK_PP_RELATIVE_ITERATION(i) PDK_PP_CAT(PDK_PP_RELATIVE_, i)(PDK_PP_ITERATION_)

# define PDK_PP_RELATIVE_0(m) PDK_PP_CAT(m, PDK_PP_ITERATION_DEPTH())
# define PDK_PP_RELATIVE_1(m) PDK_PP_CAT(m, PDK_PP_DEC(PDK_PP_ITERATION_DEPTH()))
# define PDK_PP_RELATIVE_2(m) PDK_PP_CAT(m, PDK_PP_DEC(PDK_PP_DEC(PDK_PP_ITERATION_DEPTH())))
# define PDK_PP_RELATIVE_3(m) PDK_PP_CAT(m, PDK_PP_DEC(PDK_PP_DEC(PDK_PP_DEC(PDK_PP_ITERATION_DEPTH()))))
# define PDK_PP_RELATIVE_4(m) PDK_PP_CAT(m, PDK_PP_DEC(PDK_PP_DEC(PDK_PP_DEC(PDK_PP_DEC(PDK_PP_ITERATION_DEPTH())))))

// PDK_PP_RELATIVE_START && PDK_PP_RELATIVE_FINISH
# define PDK_PP_RELATIVE_START(i) PDK_PP_CAT(PDK_PP_RELATIVE_, i)(PDK_PP_ITERATION_START_)
# define PDK_PP_RELATIVE_FINISH(i) PDK_PP_CAT(PDK_PP_RELATIVE_, i)(PDK_PP_ITERATION_FINISH_)

// PDK_PP_RELATIVE_FLAGS
# define PDK_PP_RELATIVE_FLAGS(i) (PDK_PP_CAT(PDK_PP_RELATIVE_, i)(PDK_PP_ITERATION_FLAGS_)())

// PDK_PP_ITERATE
# define PDK_PP_ITERATE() PDK_PP_CAT(PDK_PP_ITERATE_, PDK_PP_INC(PDK_PP_ITERATION_DEPTH()))

# define PDK_PP_ITERATE_1 "pdk/stdext/preprocessor/iteration/internal/iter/Forward1.h"
# define PDK_PP_ITERATE_2 "pdk/stdext/preprocessor/iteration/internal/iter/Forward2.h"
# define PDK_PP_ITERATE_3 "pdk/stdext/preprocessor/iteration/internal/iter/Forward5.h"
# define PDK_PP_ITERATE_4 "pdk/stdext/preprocessor/iteration/internal/iter/Forward5.h"
# define PDK_PP_ITERATE_5 "pdk/stdext/preprocessor/iteration/internal/iter/Forward5.h"

#endif // PDK_STDEXT_PREPROCESSOR_ITERATION_ITERATE_H
