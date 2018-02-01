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

#include "pdk/base/text/codecs/internal/JisCodecPrivate.h"

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

using pdk::ds::ByteArray;
using pdk::lang::String;
using pdk::lang::Character;
using pdk::lang::Latin1Character;

enum {
   Esc = 0x1b,
   So = 0x0e,         // Shift Out
   Si = 0x0f,         // Shift In
   
   ReverseSolidus = 0x5c,
   YenSign = 0x5c,
   Tilde = 0x7e,
   Overline = 0x7e
};

#define        IS_KANA(c)        (((c) >= 0xa1) && ((c) <= 0xdf))
#define        IS_JIS_CHAR(c)        (((c) >= 0x21) && ((c) <= 0x7e))

#define        PDK_VALID_CHAR(u)        ((u) ? Character(static_cast<ushort>(u)) : Character(Character::ReplacementCharacter))

enum Iso2022State{ Ascii, MinState = Ascii,
                   JISX0201_Latin, JISX0201_Kana,
                   JISX0208_1978, JISX0208_1983,
                   JISX0212, MaxState = JISX0212,
                   UnknownState };

static const char Esc_CHARS[] = "()*+-./";

static const char Esc_Ascii[]                 = {Esc, '(', 'B', 0 };
static const char Esc_JISX0201_Latin[]        = {Esc, '(', 'J', 0 };
static const char Esc_JISX0201_Kana[]        = {Esc, '(', 'I', 0 };
static const char Esc_JISX0208_1978[]        = {Esc, '$', '@', 0 };
static const char Esc_JISX0208_1983[]        = {Esc, '$', 'B', 0 };
static const char Esc_JISX0212[]        = {Esc, '$', '(', 'D', 0 };
static const char * const Esc_SEQ[] = { Esc_Ascii,
                                        Esc_JISX0201_Latin,
                                        Esc_JISX0201_Kana,
                                        Esc_JISX0208_1978,
                                        Esc_JISX0208_1983,
                                        Esc_JISX0212 };

JisCodec::JisCodec() : conv(JpUnicodeConv::newConverter(JpUnicodeConv::Default))
{
}

JisCodec::~JisCodec()
{
   delete (const JpUnicodeConv*)conv;
   conv = 0;
}

ByteArray JisCodec::convertFromUnicode(const Character *uc, int len, ConverterState *cs) const
{
   char replacement = '?';
   if (cs) {
      if (cs->m_flags & ConversionFlag::ConvertInvalidToNull) {
         replacement = 0;
      }
         
   }
   int invalid = 0;
   ByteArray result;
   Iso2022State state = Ascii;
   Iso2022State prev = Ascii;
   for (int i = 0; i < len; i++) {
      Character ch = uc[i];
      uint j;
      if (ch.getRow() == 0x00 && ch.getCell() < 0x80) {
         // Ascii
         if (state != JISX0201_Latin ||
             ch.getCell() == ReverseSolidus || ch.getCell() == Tilde) {
            state = Ascii;
         }
         j = ch.getCell();
      } else if ((j = conv->unicodeToJisx0201(ch.getRow(), ch.getCell())) != 0) {
         if (j < 0x80) {
            // JIS X 0201 Latin
            if (state != Ascii ||
                ch.getCell() == YenSign || ch.getCell() == Overline) {
               state = JISX0201_Latin;
            }
         } else {
            // JIS X 0201 Kana
            state = JISX0201_Kana;
            j &= 0x7f;
         }
      } else if ((j = conv->unicodeToJisx0208(ch.getRow(), ch.getCell())) != 0) {
         // JIS X 0208
         state = JISX0208_1983;
      } else if ((j = conv->unicodeToJisx0212(ch.getRow(), ch.getCell())) != 0) {
         // JIS X 0212
         state = JISX0212;
      } else {
         // Invalid
         state = UnknownState;
         j = replacement;
         ++invalid;
      }
      if (state != prev) {
         if (state == UnknownState) {
            result += Esc_Ascii;
         } else {
            result += Esc_SEQ[state - MinState];
         }
         prev = state;
      }
      if (j < 0x0100) {
         result += j & 0xff;
      } else {
         result += (j >> 8) & 0xff;
         result += j & 0xff;
      }
   }
   if (prev != Ascii) {
      result += Esc_Ascii;
   }
   
   if (cs) {
      cs->m_invalidChars += invalid;
   }
   return result;
}

String JisCodec::convertToUnicode(const char* chars, int len, ConverterState *cs) const
{
   uchar buf[4] = {0, 0, 0, 0};
   int nbuf = 0;
   Iso2022State state = Ascii, prev = Ascii;
   bool esc = false;
   Character replacement = Character::ReplacementCharacter;
   if (cs) {
      if (cs->m_flags & ConversionFlag::ConvertInvalidToNull)
         replacement = Character::Null;
      nbuf = cs->m_remainingChars;
      buf[0] = (cs->m_stateData[0] >> 24) & 0xff;
      buf[1] = (cs->m_stateData[0] >> 16) & 0xff;
      buf[2] = (cs->m_stateData[0] >>  8) & 0xff;
      buf[3] = (cs->m_stateData[0] >>  0) & 0xff;
      state = (Iso2022State)((cs->m_stateData[1] >>  0) & 0xff);
      prev = (Iso2022State)((cs->m_stateData[1] >>  8) & 0xff);
      esc = cs->m_stateData[2];
   }
   int invalid = 0;
   
   String result;
   for (int i=0; i<len; i++) {
      uchar ch = chars[i];
      if (esc) {
         // Escape sequence
         state = UnknownState;
         switch (nbuf) {
         case 0:
            if (ch == '$' || strchr(Esc_CHARS, ch)) {
               buf[nbuf++] = ch;
            } else {
               nbuf = 0;
               esc = false;
            }
            break;
         case 1:
            if (buf[0] == '$') {
               if (strchr(Esc_CHARS, ch)) {
                  buf[nbuf++] = ch;
               } else {
                  switch (ch) {
                  case '@':
                     state = JISX0208_1978;        // Esc $ @
                     break;
                  case 'B':
                     state = JISX0208_1983;        // Esc $ B
                     break;
                  }
                  nbuf = 0;
                  esc = false;
               }
            } else {
               if (buf[0] == '(') {
                  switch (ch) {
                  case 'B':
                     state = Ascii;        // Esc (B
                     break;
                  case 'I':
                     state = JISX0201_Kana;        // Esc (I
                     break;
                  case 'J':
                     state = JISX0201_Latin;        // Esc (J
                     break;
                  }
               }
               nbuf = 0;
               esc = false;
            }
            break;
         case 2:
            if (buf[1] == '(') {
               switch (ch) {
               case 'D':
                  state = JISX0212;        // Esc $ (D
                  break;
               }
            }
            nbuf = 0;
            esc = false;
            break;
         }
      } else {
         if (ch == Esc) {
            // Escape sequence
            nbuf = 0;
            esc = true;
         } else if (ch == So) {
            // Shift out
            prev = state;
            state = JISX0201_Kana;
            nbuf = 0;
         } else if (ch == Si) {
            // Shift in
            if (prev == Ascii || prev == JISX0201_Latin) {
               state = prev;
            } else {
               state = Ascii;
            }
            nbuf = 0;
         } else {
            uint u;
            switch (nbuf) {
            case 0:
               switch (state) {
               case Ascii:
                  if (ch < 0x80) {
                     result += Latin1Character(ch);
                     break;
                  }
                  PDK_FALLTHROUGH();
               case JISX0201_Latin:
                  u = conv->jisx0201ToUnicode(ch);
                  result += PDK_VALID_CHAR (u);
                  break;
               case JISX0201_Kana:
                  u = conv->jisx0201ToUnicode(ch | 0x80);
                  result += PDK_VALID_CHAR (u);
                  break;
               case JISX0208_1978:
               case JISX0208_1983:
               case JISX0212:
                  buf[nbuf++] = ch;
                  break;
               default:
                  result += Character::ReplacementCharacter;
                  break;
               }
               break;
            case 1:
               switch (state) {
               case JISX0208_1978:
               case JISX0208_1983:
                  u = conv->jisx0208ToUnicode(buf[0] & 0x7f, ch & 0x7f);
                  result += PDK_VALID_CHAR (u);
                  break;
               case JISX0212:
                  u = conv->jisx0212ToUnicode(buf[0] & 0x7f, ch & 0x7f);
                  result += PDK_VALID_CHAR (u);
                  break;
               default:
                  result += replacement;
                  ++invalid;
                  break;
               }
               nbuf = 0;
               break;
            }
         }
      }
   }
   
   if (cs) {
      cs->m_remainingChars = nbuf;
      cs->m_invalidChars += invalid;
      cs->m_stateData[0] = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
      cs->m_stateData[1] = (prev << 8) + state;
      cs->m_stateData[2] = esc;
   }
   
   return result;
}

int JisCodec::mibEnumImpl()
{
   return 39;
}

ByteArray JisCodec::nameImpl()
{
   return "ISO-2022-JP";
}

std::list<ByteArray> JisCodec::aliasesImpl()
{
   std::list<ByteArray> list;
   list.push_back("JIS7");
   return list;
}

} // internal
} // codecs
} // text
} // pdk
