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

#ifndef PDK_STDEXT_PREPROCESSOR_SEQ_FOLD_LEFT_H
#define PDK_STDEXT_PREPROCESSOR_SEQ_FOLD_LEFT_H

#include "pdk/stdext/preprocessor/arithmetic/Dec.h"
#include "pdk/stdext/preprocessor/Cat.h"
#include "pdk/stdext/preprocessor/config/Config.h"
#include "pdk/stdext/preprocessor/control/If.h"
#include "pdk/stdext/preprocessor/debug/Error.h"
#include "pdk/stdext/preprocessor/internal/AutoRec.h"
#include "pdk/stdext/preprocessor/seq/Seq.h"
#include "pdk/stdext/preprocessor/seq/Size.h"

// PDK_PP_SEQ_FOLD_LEFT
# if 0
#    define PDK_PP_SEQ_FOLD_LEFT(op, state, seq) ...
# endif
#
# define PDK_PP_SEQ_FOLD_LEFT PDK_PP_CAT(PDK_PP_SEQ_FOLD_LEFT_, PDK_PP_AUTO_REC(PDK_PP_SEQ_FOLD_LEFT_P, 256))
# define PDK_PP_SEQ_FOLD_LEFT_P(n) PDK_PP_CAT(PDK_PP_SEQ_FOLD_LEFT_CHECK_, PDK_PP_SEQ_FOLD_LEFT_I_ ## n(PDK_PP_SEQ_FOLD_LEFT_O, PDK_PP_NIL, (nil), 1))
# define PDK_PP_SEQ_FOLD_LEFT_O(s, st, _) st
#
# define PDK_PP_SEQ_FOLD_LEFT_257(op, st, ss) PDK_PP_ERROR(0x0005)
# define PDK_PP_SEQ_FOLD_LEFT_I_257(op, st, ss, sz) PDK_PP_ERROR(0x0005)
#
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_NIL 1
#
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_1(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_2(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_3(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_4(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_5(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_6(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_7(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_8(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_9(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_10(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_11(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_12(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_13(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_14(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_15(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_16(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_17(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_18(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_19(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_20(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_21(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_22(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_23(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_24(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_25(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_26(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_27(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_28(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_29(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_30(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_31(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_32(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_33(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_34(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_35(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_36(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_37(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_38(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_39(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_40(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_41(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_42(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_43(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_44(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_45(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_46(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_47(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_48(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_49(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_50(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_51(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_52(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_53(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_54(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_55(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_56(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_57(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_58(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_59(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_60(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_61(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_62(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_63(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_64(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_65(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_66(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_67(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_68(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_69(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_70(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_71(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_72(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_73(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_74(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_75(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_76(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_77(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_78(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_79(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_80(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_81(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_82(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_83(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_84(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_85(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_86(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_87(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_88(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_89(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_90(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_91(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_92(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_93(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_94(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_95(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_96(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_97(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_98(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_99(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_100(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_101(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_102(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_103(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_104(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_105(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_106(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_107(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_108(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_109(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_110(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_111(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_112(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_113(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_114(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_115(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_116(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_117(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_118(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_119(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_120(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_121(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_122(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_123(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_124(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_125(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_126(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_127(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_128(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_129(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_130(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_131(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_132(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_133(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_134(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_135(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_136(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_137(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_138(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_139(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_140(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_141(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_142(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_143(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_144(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_145(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_146(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_147(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_148(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_149(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_150(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_151(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_152(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_153(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_154(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_155(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_156(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_157(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_158(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_159(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_160(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_161(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_162(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_163(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_164(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_165(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_166(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_167(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_168(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_169(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_170(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_171(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_172(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_173(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_174(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_175(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_176(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_177(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_178(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_179(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_180(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_181(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_182(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_183(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_184(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_185(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_186(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_187(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_188(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_189(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_190(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_191(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_192(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_193(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_194(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_195(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_196(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_197(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_198(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_199(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_200(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_201(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_202(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_203(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_204(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_205(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_206(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_207(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_208(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_209(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_210(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_211(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_212(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_213(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_214(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_215(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_216(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_217(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_218(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_219(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_220(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_221(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_222(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_223(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_224(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_225(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_226(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_227(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_228(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_229(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_230(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_231(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_232(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_233(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_234(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_235(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_236(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_237(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_238(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_239(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_240(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_241(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_242(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_243(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_244(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_245(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_246(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_247(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_248(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_249(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_250(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_251(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_252(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_253(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_254(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_255(op, st, ss, sz) 0
# define PDK_PP_SEQ_FOLD_LEFT_CHECK_PDK_PP_SEQ_FOLD_LEFT_I_256(op, st, ss, sz) 0
#
# define PDK_PP_SEQ_FOLD_LEFT_F(op, st, ss, sz) st
#
# define PDK_PP_SEQ_FOLD_LEFT_1(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_1(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_2(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_2(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_3(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_3(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_4(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_4(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_5(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_5(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_6(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_6(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_7(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_7(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_8(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_8(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_9(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_9(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_10(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_10(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_11(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_11(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_12(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_12(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_13(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_13(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_14(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_14(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_15(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_15(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_16(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_16(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_17(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_17(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_18(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_18(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_19(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_19(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_20(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_20(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_21(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_21(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_22(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_22(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_23(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_23(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_24(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_24(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_25(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_25(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_26(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_26(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_27(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_27(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_28(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_28(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_29(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_29(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_30(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_30(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_31(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_31(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_32(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_32(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_33(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_33(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_34(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_34(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_35(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_35(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_36(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_36(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_37(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_37(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_38(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_38(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_39(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_39(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_40(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_40(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_41(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_41(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_42(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_42(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_43(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_43(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_44(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_44(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_45(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_45(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_46(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_46(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_47(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_47(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_48(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_48(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_49(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_49(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_50(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_50(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_51(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_51(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_52(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_52(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_53(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_53(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_54(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_54(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_55(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_55(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_56(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_56(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_57(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_57(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_58(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_58(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_59(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_59(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_60(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_60(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_61(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_61(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_62(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_62(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_63(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_63(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_64(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_64(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_65(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_65(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_66(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_66(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_67(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_67(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_68(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_68(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_69(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_69(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_70(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_70(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_71(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_71(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_72(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_72(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_73(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_73(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_74(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_74(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_75(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_75(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_76(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_76(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_77(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_77(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_78(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_78(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_79(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_79(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_80(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_80(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_81(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_81(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_82(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_82(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_83(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_83(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_84(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_84(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_85(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_85(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_86(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_86(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_87(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_87(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_88(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_88(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_89(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_89(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_90(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_90(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_91(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_91(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_92(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_92(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_93(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_93(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_94(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_94(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_95(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_95(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_96(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_96(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_97(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_97(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_98(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_98(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_99(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_99(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_100(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_100(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_101(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_101(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_102(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_102(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_103(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_103(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_104(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_104(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_105(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_105(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_106(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_106(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_107(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_107(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_108(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_108(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_109(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_109(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_110(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_110(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_111(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_111(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_112(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_112(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_113(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_113(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_114(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_114(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_115(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_115(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_116(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_116(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_117(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_117(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_118(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_118(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_119(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_119(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_120(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_120(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_121(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_121(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_122(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_122(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_123(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_123(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_124(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_124(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_125(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_125(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_126(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_126(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_127(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_127(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_128(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_128(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_129(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_129(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_130(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_130(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_131(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_131(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_132(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_132(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_133(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_133(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_134(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_134(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_135(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_135(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_136(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_136(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_137(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_137(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_138(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_138(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_139(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_139(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_140(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_140(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_141(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_141(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_142(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_142(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_143(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_143(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_144(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_144(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_145(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_145(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_146(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_146(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_147(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_147(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_148(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_148(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_149(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_149(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_150(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_150(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_151(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_151(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_152(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_152(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_153(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_153(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_154(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_154(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_155(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_155(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_156(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_156(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_157(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_157(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_158(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_158(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_159(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_159(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_160(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_160(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_161(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_161(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_162(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_162(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_163(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_163(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_164(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_164(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_165(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_165(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_166(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_166(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_167(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_167(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_168(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_168(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_169(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_169(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_170(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_170(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_171(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_171(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_172(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_172(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_173(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_173(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_174(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_174(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_175(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_175(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_176(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_176(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_177(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_177(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_178(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_178(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_179(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_179(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_180(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_180(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_181(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_181(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_182(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_182(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_183(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_183(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_184(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_184(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_185(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_185(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_186(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_186(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_187(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_187(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_188(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_188(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_189(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_189(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_190(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_190(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_191(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_191(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_192(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_192(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_193(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_193(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_194(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_194(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_195(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_195(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_196(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_196(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_197(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_197(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_198(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_198(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_199(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_199(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_200(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_200(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_201(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_201(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_202(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_202(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_203(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_203(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_204(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_204(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_205(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_205(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_206(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_206(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_207(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_207(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_208(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_208(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_209(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_209(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_210(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_210(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_211(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_211(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_212(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_212(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_213(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_213(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_214(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_214(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_215(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_215(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_216(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_216(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_217(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_217(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_218(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_218(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_219(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_219(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_220(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_220(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_221(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_221(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_222(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_222(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_223(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_223(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_224(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_224(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_225(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_225(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_226(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_226(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_227(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_227(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_228(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_228(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_229(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_229(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_230(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_230(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_231(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_231(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_232(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_232(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_233(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_233(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_234(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_234(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_235(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_235(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_236(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_236(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_237(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_237(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_238(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_238(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_239(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_239(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_240(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_240(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_241(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_241(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_242(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_242(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_243(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_243(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_244(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_244(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_245(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_245(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_246(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_246(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_247(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_247(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_248(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_248(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_249(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_249(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_250(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_250(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_251(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_251(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_252(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_252(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_253(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_253(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_254(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_254(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_255(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_255(op, st, ss, PDK_PP_SEQ_SIZE(ss))
# define PDK_PP_SEQ_FOLD_LEFT_256(op, st, ss) PDK_PP_SEQ_FOLD_LEFT_I_256(op, st, ss, PDK_PP_SEQ_SIZE(ss))
#
# if ~PDK_PP_CONFIG_FLAGS() & PDK_PP_CONFIG_DMC()
#    define PDK_PP_SEQ_FOLD_LEFT_I_1(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_2, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(2, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_2(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_3, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(3, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_3(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_4, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(4, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_4(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_5, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(5, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_5(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_6, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(6, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_6(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_7, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(7, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_7(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_8, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(8, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_8(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_9, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(9, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_9(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_10, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(10, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_10(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_11, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(11, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_11(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_12, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(12, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_12(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_13, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(13, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_13(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_14, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(14, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_14(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_15, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(15, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_15(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_16, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(16, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_16(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_17, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(17, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_17(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_18, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(18, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_18(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_19, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(19, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_19(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_20, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(20, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_20(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_21, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(21, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_21(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_22, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(22, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_22(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_23, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(23, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_23(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_24, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(24, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_24(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_25, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(25, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_25(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_26, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(26, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_26(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_27, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(27, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_27(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_28, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(28, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_28(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_29, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(29, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_29(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_30, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(30, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_30(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_31, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(31, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_31(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_32, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(32, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_32(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_33, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(33, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_33(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_34, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(34, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_34(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_35, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(35, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_35(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_36, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(36, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_36(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_37, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(37, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_37(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_38, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(38, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_38(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_39, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(39, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_39(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_40, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(40, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_40(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_41, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(41, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_41(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_42, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(42, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_42(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_43, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(43, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_43(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_44, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(44, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_44(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_45, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(45, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_45(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_46, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(46, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_46(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_47, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(47, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_47(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_48, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(48, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_48(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_49, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(49, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_49(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_50, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(50, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_50(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_51, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(51, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_51(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_52, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(52, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_52(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_53, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(53, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_53(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_54, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(54, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_54(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_55, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(55, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_55(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_56, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(56, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_56(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_57, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(57, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_57(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_58, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(58, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_58(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_59, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(59, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_59(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_60, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(60, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_60(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_61, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(61, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_61(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_62, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(62, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_62(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_63, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(63, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_63(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_64, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(64, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_64(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_65, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(65, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_65(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_66, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(66, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_66(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_67, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(67, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_67(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_68, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(68, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_68(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_69, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(69, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_69(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_70, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(70, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_70(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_71, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(71, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_71(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_72, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(72, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_72(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_73, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(73, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_73(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_74, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(74, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_74(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_75, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(75, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_75(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_76, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(76, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_76(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_77, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(77, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_77(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_78, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(78, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_78(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_79, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(79, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_79(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_80, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(80, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_80(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_81, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(81, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_81(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_82, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(82, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_82(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_83, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(83, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_83(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_84, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(84, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_84(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_85, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(85, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_85(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_86, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(86, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_86(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_87, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(87, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_87(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_88, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(88, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_88(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_89, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(89, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_89(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_90, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(90, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_90(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_91, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(91, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_91(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_92, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(92, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_92(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_93, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(93, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_93(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_94, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(94, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_94(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_95, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(95, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_95(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_96, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(96, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_96(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_97, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(97, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_97(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_98, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(98, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_98(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_99, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(99, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_99(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_100, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(100, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_100(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_101, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(101, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_101(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_102, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(102, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_102(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_103, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(103, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_103(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_104, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(104, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_104(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_105, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(105, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_105(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_106, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(106, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_106(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_107, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(107, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_107(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_108, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(108, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_108(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_109, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(109, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_109(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_110, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(110, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_110(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_111, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(111, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_111(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_112, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(112, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_112(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_113, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(113, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_113(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_114, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(114, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_114(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_115, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(115, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_115(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_116, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(116, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_116(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_117, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(117, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_117(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_118, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(118, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_118(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_119, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(119, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_119(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_120, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(120, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_120(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_121, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(121, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_121(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_122, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(122, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_122(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_123, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(123, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_123(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_124, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(124, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_124(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_125, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(125, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_125(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_126, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(126, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_126(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_127, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(127, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_127(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_128, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(128, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_128(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_129, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(129, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_129(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_130, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(130, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_130(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_131, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(131, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_131(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_132, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(132, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_132(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_133, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(133, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_133(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_134, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(134, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_134(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_135, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(135, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_135(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_136, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(136, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_136(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_137, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(137, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_137(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_138, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(138, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_138(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_139, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(139, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_139(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_140, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(140, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_140(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_141, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(141, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_141(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_142, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(142, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_142(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_143, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(143, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_143(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_144, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(144, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_144(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_145, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(145, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_145(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_146, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(146, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_146(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_147, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(147, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_147(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_148, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(148, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_148(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_149, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(149, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_149(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_150, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(150, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_150(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_151, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(151, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_151(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_152, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(152, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_152(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_153, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(153, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_153(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_154, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(154, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_154(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_155, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(155, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_155(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_156, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(156, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_156(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_157, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(157, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_157(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_158, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(158, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_158(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_159, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(159, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_159(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_160, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(160, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_160(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_161, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(161, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_161(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_162, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(162, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_162(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_163, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(163, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_163(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_164, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(164, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_164(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_165, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(165, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_165(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_166, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(166, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_166(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_167, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(167, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_167(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_168, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(168, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_168(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_169, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(169, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_169(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_170, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(170, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_170(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_171, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(171, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_171(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_172, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(172, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_172(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_173, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(173, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_173(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_174, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(174, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_174(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_175, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(175, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_175(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_176, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(176, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_176(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_177, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(177, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_177(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_178, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(178, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_178(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_179, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(179, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_179(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_180, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(180, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_180(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_181, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(181, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_181(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_182, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(182, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_182(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_183, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(183, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_183(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_184, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(184, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_184(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_185, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(185, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_185(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_186, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(186, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_186(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_187, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(187, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_187(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_188, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(188, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_188(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_189, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(189, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_189(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_190, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(190, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_190(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_191, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(191, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_191(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_192, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(192, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_192(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_193, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(193, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_193(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_194, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(194, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_194(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_195, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(195, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_195(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_196, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(196, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_196(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_197, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(197, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_197(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_198, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(198, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_198(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_199, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(199, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_199(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_200, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(200, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_200(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_201, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(201, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_201(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_202, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(202, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_202(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_203, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(203, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_203(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_204, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(204, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_204(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_205, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(205, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_205(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_206, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(206, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_206(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_207, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(207, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_207(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_208, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(208, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_208(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_209, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(209, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_209(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_210, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(210, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_210(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_211, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(211, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_211(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_212, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(212, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_212(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_213, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(213, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_213(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_214, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(214, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_214(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_215, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(215, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_215(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_216, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(216, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_216(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_217, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(217, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_217(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_218, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(218, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_218(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_219, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(219, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_219(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_220, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(220, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_220(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_221, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(221, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_221(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_222, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(222, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_222(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_223, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(223, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_223(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_224, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(224, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_224(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_225, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(225, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_225(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_226, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(226, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_226(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_227, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(227, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_227(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_228, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(228, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_228(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_229, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(229, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_229(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_230, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(230, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_230(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_231, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(231, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_231(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_232, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(232, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_232(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_233, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(233, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_233(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_234, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(234, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_234(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_235, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(235, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_235(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_236, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(236, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_236(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_237, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(237, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_237(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_238, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(238, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_238(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_239, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(239, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_239(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_240, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(240, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_240(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_241, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(241, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_241(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_242, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(242, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_242(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_243, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(243, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_243(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_244, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(244, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_244(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_245, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(245, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_245(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_246, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(246, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_246(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_247, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(247, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_247(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_248, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(248, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_248(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_249, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(249, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_249(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_250, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(250, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_250(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_251, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(251, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_251(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_252, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(252, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_252(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_253, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(253, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_253(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_254, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(254, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_254(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_255, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(255, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_255(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_256, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(256, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_256(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_257, PDK_PP_SEQ_FOLD_LEFT_F)(op, op(257, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
# else
#    define PDK_PP_SEQ_FOLD_LEFT_I_1(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_2, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(2, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_2(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_3, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(3, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_3(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_4, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(4, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_4(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_5, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(5, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_5(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_6, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(6, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_6(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_7, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(7, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_7(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_8, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(8, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_8(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_9, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(9, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_9(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_10, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(10, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_10(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_11, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(11, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_11(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_12, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(12, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_12(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_13, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(13, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_13(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_14, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(14, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_14(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_15, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(15, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_15(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_16, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(16, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_16(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_17, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(17, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_17(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_18, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(18, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_18(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_19, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(19, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_19(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_20, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(20, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_20(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_21, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(21, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_21(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_22, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(22, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_22(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_23, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(23, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_23(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_24, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(24, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_24(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_25, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(25, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_25(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_26, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(26, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_26(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_27, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(27, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_27(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_28, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(28, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_28(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_29, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(29, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_29(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_30, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(30, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_30(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_31, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(31, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_31(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_32, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(32, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_32(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_33, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(33, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_33(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_34, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(34, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_34(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_35, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(35, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_35(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_36, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(36, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_36(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_37, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(37, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_37(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_38, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(38, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_38(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_39, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(39, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_39(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_40, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(40, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_40(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_41, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(41, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_41(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_42, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(42, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_42(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_43, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(43, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_43(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_44, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(44, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_44(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_45, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(45, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_45(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_46, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(46, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_46(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_47, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(47, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_47(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_48, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(48, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_48(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_49, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(49, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_49(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_50, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(50, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_50(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_51, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(51, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_51(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_52, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(52, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_52(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_53, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(53, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_53(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_54, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(54, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_54(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_55, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(55, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_55(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_56, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(56, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_56(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_57, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(57, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_57(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_58, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(58, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_58(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_59, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(59, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_59(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_60, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(60, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_60(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_61, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(61, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_61(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_62, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(62, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_62(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_63, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(63, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_63(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_64, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(64, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_64(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_65, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(65, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_65(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_66, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(66, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_66(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_67, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(67, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_67(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_68, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(68, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_68(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_69, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(69, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_69(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_70, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(70, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_70(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_71, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(71, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_71(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_72, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(72, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_72(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_73, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(73, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_73(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_74, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(74, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_74(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_75, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(75, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_75(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_76, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(76, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_76(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_77, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(77, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_77(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_78, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(78, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_78(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_79, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(79, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_79(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_80, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(80, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_80(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_81, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(81, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_81(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_82, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(82, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_82(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_83, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(83, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_83(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_84, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(84, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_84(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_85, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(85, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_85(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_86, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(86, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_86(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_87, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(87, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_87(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_88, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(88, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_88(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_89, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(89, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_89(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_90, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(90, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_90(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_91, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(91, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_91(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_92, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(92, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_92(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_93, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(93, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_93(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_94, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(94, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_94(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_95, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(95, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_95(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_96, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(96, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_96(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_97, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(97, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_97(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_98, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(98, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_98(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_99, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(99, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_99(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_100, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(100, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_100(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_101, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(101, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_101(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_102, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(102, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_102(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_103, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(103, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_103(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_104, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(104, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_104(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_105, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(105, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_105(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_106, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(106, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_106(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_107, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(107, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_107(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_108, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(108, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_108(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_109, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(109, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_109(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_110, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(110, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_110(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_111, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(111, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_111(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_112, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(112, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_112(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_113, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(113, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_113(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_114, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(114, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_114(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_115, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(115, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_115(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_116, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(116, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_116(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_117, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(117, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_117(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_118, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(118, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_118(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_119, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(119, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_119(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_120, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(120, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_120(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_121, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(121, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_121(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_122, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(122, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_122(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_123, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(123, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_123(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_124, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(124, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_124(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_125, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(125, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_125(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_126, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(126, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_126(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_127, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(127, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_127(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_128, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(128, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_128(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_129, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(129, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_129(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_130, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(130, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_130(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_131, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(131, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_131(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_132, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(132, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_132(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_133, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(133, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_133(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_134, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(134, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_134(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_135, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(135, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_135(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_136, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(136, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_136(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_137, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(137, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_137(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_138, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(138, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_138(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_139, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(139, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_139(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_140, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(140, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_140(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_141, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(141, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_141(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_142, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(142, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_142(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_143, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(143, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_143(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_144, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(144, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_144(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_145, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(145, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_145(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_146, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(146, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_146(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_147, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(147, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_147(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_148, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(148, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_148(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_149, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(149, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_149(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_150, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(150, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_150(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_151, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(151, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_151(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_152, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(152, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_152(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_153, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(153, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_153(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_154, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(154, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_154(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_155, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(155, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_155(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_156, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(156, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_156(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_157, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(157, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_157(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_158, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(158, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_158(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_159, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(159, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_159(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_160, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(160, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_160(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_161, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(161, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_161(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_162, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(162, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_162(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_163, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(163, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_163(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_164, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(164, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_164(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_165, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(165, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_165(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_166, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(166, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_166(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_167, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(167, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_167(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_168, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(168, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_168(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_169, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(169, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_169(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_170, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(170, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_170(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_171, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(171, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_171(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_172, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(172, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_172(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_173, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(173, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_173(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_174, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(174, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_174(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_175, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(175, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_175(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_176, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(176, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_176(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_177, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(177, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_177(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_178, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(178, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_178(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_179, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(179, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_179(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_180, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(180, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_180(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_181, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(181, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_181(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_182, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(182, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_182(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_183, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(183, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_183(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_184, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(184, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_184(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_185, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(185, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_185(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_186, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(186, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_186(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_187, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(187, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_187(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_188, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(188, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_188(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_189, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(189, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_189(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_190, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(190, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_190(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_191, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(191, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_191(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_192, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(192, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_192(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_193, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(193, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_193(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_194, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(194, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_194(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_195, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(195, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_195(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_196, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(196, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_196(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_197, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(197, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_197(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_198, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(198, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_198(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_199, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(199, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_199(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_200, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(200, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_200(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_201, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(201, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_201(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_202, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(202, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_202(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_203, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(203, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_203(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_204, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(204, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_204(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_205, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(205, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_205(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_206, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(206, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_206(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_207, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(207, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_207(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_208, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(208, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_208(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_209, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(209, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_209(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_210, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(210, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_210(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_211, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(211, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_211(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_212, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(212, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_212(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_213, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(213, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_213(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_214, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(214, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_214(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_215, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(215, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_215(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_216, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(216, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_216(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_217, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(217, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_217(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_218, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(218, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_218(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_219, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(219, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_219(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_220, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(220, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_220(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_221, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(221, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_221(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_222, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(222, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_222(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_223, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(223, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_223(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_224, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(224, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_224(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_225, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(225, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_225(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_226, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(226, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_226(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_227, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(227, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_227(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_228, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(228, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_228(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_229, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(229, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_229(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_230, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(230, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_230(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_231, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(231, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_231(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_232, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(232, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_232(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_233, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(233, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_233(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_234, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(234, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_234(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_235, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(235, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_235(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_236, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(236, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_236(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_237, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(237, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_237(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_238, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(238, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_238(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_239, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(239, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_239(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_240, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(240, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_240(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_241, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(241, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_241(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_242, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(242, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_242(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_243, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(243, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_243(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_244, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(244, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_244(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_245, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(245, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_245(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_246, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(246, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_246(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_247, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(247, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_247(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_248, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(248, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_248(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_249, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(249, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_249(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_250, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(250, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_250(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_251, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(251, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_251(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_252, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(252, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_252(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_253, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(253, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_253(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_254, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(254, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_254(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_255, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(255, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_255(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_256, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(256, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
#    define PDK_PP_SEQ_FOLD_LEFT_I_256(op, st, ss, sz) PDK_PP_IF(PDK_PP_DEC(sz), PDK_PP_SEQ_FOLD_LEFT_I_257, PDK_PP_SEQ_FOLD_LEFT_F)(op, op##(257, st, PDK_PP_SEQ_HEAD(ss)), PDK_PP_SEQ_TAIL(ss), PDK_PP_DEC(sz))
# endif

#endif // PDK_STDEXT_PREPROCESSOR_SEQ_FOLD_LEFT_H
