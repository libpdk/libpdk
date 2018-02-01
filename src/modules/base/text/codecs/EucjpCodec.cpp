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
// Created by softboy on 2018/02/01.

/*
 * Copyright (C) 1999 Serika Kurusugawa, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "pdk/base/text/codecs/internal/EucjpCodecPrivate.h"

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

using pdk::ds::ByteArray;
using pdk::lang::String;
using pdk::lang::Character;
using pdk::lang::Latin1Character;

static const uchar Ss2 = 0x8e;        // Single Shift 2
static const uchar Ss3 = 0x8f;        // Single Shift 3

#define        IS_KANA(c)        (((c) >= 0xa1) && ((c) <= 0xdf))
#define        IS_EUC_CHAR(c)        (((c) >= 0xa1) && ((c) <= 0xfe))

#define        PDK_VALID_CHAR(u)        ((u) ? Character((ushort)(u)) : Character(Character::ReplacementCharacter))

/*!
  Constructs a EucJpCodec.
*/
EucJpCodec::EucJpCodec() 
   : conv(JpUnicodeConv::newConverter(JpUnicodeConv::Default))
{
}

/*!
  Destroys the codec.
*/
EucJpCodec::~EucJpCodec()
{
   delete (const JpUnicodeConv*)conv;
   conv = 0;
}

ByteArray EucJpCodec::convertFromUnicode(const Character *uc, int len, ConverterState *state) const
{
   char replacement = '?';
   if (state) {
      if (state->m_flags & ConversionFlag::ConvertInvalidToNull) {
         replacement = 0;
      }
      
   }
   int invalid = 0;
   
   int rlen = 3*len + 1;
   ByteArray rstr;
   rstr.resize(rlen);
   uchar* cursor = (uchar*)rstr.getRawData();
   for (int i = 0; i < len; i++) {
      Character ch = uc[i];
      uint j;
      if (ch.unicode() < 0x80) {
         // ASCII
         *cursor++ = ch.getCell();
      } else if ((j = conv->unicodeToJisx0201(ch.getRow(), ch.getCell())) != 0) {
         if (j < 0x80) {
            // JIS X 0201 Latin ?
            *cursor++ = j;
         } else {
            // JIS X 0201 Kana
            *cursor++ = Ss2;
            *cursor++ = j;
         }
      } else if ((j = conv->unicodeToJisx0208(ch.getRow(), ch.getCell())) != 0) {
         // JIS X 0208
         *cursor++ = (j >> 8)   | 0x80;
         *cursor++ = (j & 0xff) | 0x80;
      } else if ((j = conv->unicodeToJisx0212(ch.getRow(), ch.getCell())) != 0) {
         // JIS X 0212
         *cursor++ = Ss3;
         *cursor++ = (j >> 8)   | 0x80;
         *cursor++ = (j & 0xff) | 0x80;
      } else {
         // Error
         *cursor++ = replacement;
         ++invalid;
      }
   }
   rstr.resize(cursor - (const uchar*)rstr.getConstRawData());
   
   if (state) {
      state->m_invalidChars += invalid;
   }
   return rstr;
}


String EucJpCodec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
   uchar buf[2] = {0, 0};
   int nbuf = 0;
   Character replacement = Character::ReplacementCharacter;
   if (state) {
      if (state->m_flags & ConversionFlag::ConvertInvalidToNull)
         replacement = Character::Null;
      nbuf = state->m_remainingChars;
      buf[0] = state->m_stateData[0];
      buf[1] = state->m_stateData[1];
   }
   int invalid = 0;
   
   String result;
   for (int i=0; i<len; i++) {
      uchar ch = chars[i];
      switch (nbuf) {
      case 0:
         if (ch < 0x80) {
            // ASCII
            result += Latin1Character(ch);
         } else if (ch == Ss2 || ch == Ss3) {
            // JIS X 0201 Kana or JIS X 0212
            buf[0] = ch;
            nbuf = 1;
         } else if (IS_EUC_CHAR(ch)) {
            // JIS X 0208
            buf[0] = ch;
            nbuf = 1;
         } else {
            // Invalid
            result += replacement;
            ++invalid;
         }
         break;
      case 1:
         if (buf[0] == Ss2) {
            // JIS X 0201 Kana
            if (IS_KANA(ch)) {
               uint u = conv->jisx0201ToUnicode(ch);
               result += PDK_VALID_CHAR(u);
            } else {
               result += replacement;
               ++invalid;
            }
            nbuf = 0;
         } else if (buf[0] == Ss3) {
            // JIS X 0212-1990
            if (IS_EUC_CHAR(ch)) {
               buf[1] = ch;
               nbuf = 2;
            } else {
               // Error
               result += replacement;
               ++invalid;
               nbuf = 0;
            }
         } else {
            // JIS X 0208-1990
            if (IS_EUC_CHAR(ch)) {
               uint u = conv->jisx0208ToUnicode(buf[0] & 0x7f, ch & 0x7f);
               result += PDK_VALID_CHAR(u);
            } else {
               // Error
               result += replacement;
               ++invalid;
            }
            nbuf = 0;
         }
         break;
      case 2:
         // JIS X 0212
         if (IS_EUC_CHAR(ch)) {
            uint u = conv->jisx0212ToUnicode(buf[1] & 0x7f, ch & 0x7f);
            result += PDK_VALID_CHAR(u);
         } else {
            result += replacement;
            ++invalid;
         }
         nbuf = 0;
      }
   }
   if (state) {
      state->m_remainingChars = nbuf;
      state->m_stateData[0] = buf[0];
      state->m_stateData[1] = buf[1];
      state->m_invalidChars += invalid;
   }
   return result;
}

int EucJpCodec::mibEnumImpl()
{
   return 18;
}

ByteArray EucJpCodec::nameImpl()
{
   return "EUC-JP";
}

} // internal
} // codecs
} // text
} // pdk
