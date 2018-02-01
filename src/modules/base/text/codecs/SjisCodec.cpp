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

#include "pdk/base/text/codecs/internal/SjisCodecPrivate.h"

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

using pdk::ds::ByteArray;
using pdk::lang::String;
using pdk::lang::Character;
using pdk::lang::Latin1Character;

enum {
   Esc = 0x1b
};

#define        IS_KANA(c)        (((c) >= 0xa1) && ((c) <= 0xdf))
#define        IS_SJIS_CHAR1(c)        ((((c) >= 0x81) && ((c) <= 0x9f)) ||        \
   (((c) >= 0xe0) && ((c) <= 0xfc)))
#define        IS_SJIS_CHAR2(c)        (((c) >= 0x40) && ((c) != 0x7f) && ((c) <= 0xfc))
#define        IsUserDefinedChar1(c)        (((c) >= 0xf0) && ((c) <= 0xfc))

#define        PDK_VALID_CHAR(u)        ((u) ? Character((ushort)(u)) : Character(Character::ReplacementCharacter))


// Creates a Shift-JIS codec. Note that this is done automatically by
// the QApplication, you do not need construct your own.
SjisCodec::SjisCodec() : conv(JpUnicodeConv::newConverter(JpUnicodeConv::Default))
{
}

// Destroys the Shift-JIS codec.
SjisCodec::~SjisCodec()
{
   delete (const JpUnicodeConv*)conv;
   conv = 0;
}


ByteArray SjisCodec::convertFromUnicode(const Character *uc, int len, ConverterState *state) const
{
   char replacement = '?';
   if (state) {
      if (state->m_flags & ConversionFlag::ConvertInvalidToNull) {
         replacement = 0;
      }
   }
   int invalid = 0;
   
   int rlen = 2 * len + 1;
   ByteArray rstr;
   rstr.resize(rlen);
   uchar* cursor = (uchar*)rstr.getRawData();
   for (int i = 0; i < len; i++) {
      Character ch = uc[i];
      uint j;
      if (ch.getRow() == 0x00 && ch.getCell() < 0x80) {
         // ASCII
         *cursor++ = ch.getCell();
      } else if ((j = conv->unicodeToJisx0201(ch.getRow(), ch.getCell())) != 0) {
         // JIS X 0201 Latin or JIS X 0201 Kana
         *cursor++ = j;
      } else if ((j = conv->unicodeToSjis(ch.getRow(), ch.getCell())) != 0) {
         // JIS X 0208
         *cursor++ = (j >> 8);
         *cursor++ = (j & 0xff);
      } else if ((j = conv->unicodeToSjisibmvdc(ch.getRow(), ch.getCell())) != 0) {
         // JIS X 0208 IBM VDC
         *cursor++ = (j >> 8);
         *cursor++ = (j & 0xff);
      } else if ((j = conv->unicodeToCp932(ch.getRow(), ch.getCell())) != 0) {
         // CP932 (for lead bytes 87, ee & ed)
         *cursor++ = (j >> 8);
         *cursor++ = (j & 0xff);
      } else if ((j = conv->unicodeToJisx0212(ch.getRow(), ch.getCell())) != 0) {
         // JIS X 0212 (can't be encoded in ShiftJIS !)
         *cursor++ = 0x81;        // white square
         *cursor++ = 0xa0;        // white square
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

String SjisCodec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
   uchar buf[1] = {0};
   int nbuf = 0;
   Character replacement = Character::ReplacementCharacter;
   if (state) {
      if (state->m_flags & ConversionFlag::ConvertInvalidToNull) {
         replacement = Character::Null;
      }
      nbuf = state->m_remainingChars;
      buf[0] = state->m_stateData[0];
   }
   int invalid = 0;
   uint u= 0;
   String result;
   for (int i=0; i<len; i++) {
      uchar ch = chars[i];
      switch (nbuf) {
      case 0:
         if (ch < 0x80) {
            result += PDK_VALID_CHAR(ch);
         } else if (IS_KANA(ch)) {
            // JIS X 0201 Latin or JIS X 0201 Kana
            u = conv->jisx0201ToUnicode(ch);
            result += PDK_VALID_CHAR(u);
         } else if (IS_SJIS_CHAR1(ch)) {
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
         // JIS X 0208
         if (IS_SJIS_CHAR2(ch)) {
            if ((u = conv->sjisibmvdcToUnicode(buf[0], ch))) {
               result += PDK_VALID_CHAR(u);
            } else if ((u = conv->cp932ToUnicode(buf[0], ch))) {
               result += PDK_VALID_CHAR(u);
            }
            else if (IsUserDefinedChar1(buf[0])) {
               result += Character::ReplacementCharacter;
            } else {
               u = conv->sjisToUnicode(buf[0], ch);
               result += PDK_VALID_CHAR(u);
            }
         } else {
            // Invalid
            result += replacement;
            ++invalid;
         }
         nbuf = 0;
         break;
      }
   }
   
   if (state) {
      state->m_remainingChars = nbuf;
      state->m_stateData[0] = buf[0];
      state->m_invalidChars += invalid;
   }
   return result;
}

int SjisCodec::mibEnumImpl()
{
   return 17;
}

ByteArray SjisCodec::nameImpl()
{
   return "Shift_JIS";
}

std::list<ByteArray> SjisCodec::aliasesImpl()
{
   std::list<ByteArray> list;
   list.push_back("SJIS");
   list.push_back("MS_Kanji");
   return list;
}

} // internal
} // codecs
} // text
} // pdk
