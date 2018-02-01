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

#include "pdk/base/text/codecs/internal/IsciiCodecPrivate.h"
#include "pdk/base/text/codecs/internal/TextCodecPrivate.h"
#include <list>

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

struct Codecs {
   const char m_name[10];
   ushort m_base;
};

static const Codecs codecs [] = {
   { "iscii-dev", 0x900 },
   { "iscii-bng", 0x980 },
   { "iscii-pnj", 0xa00 },
   { "iscii-gjr", 0xa80 },
   { "iscii-ori", 0xb00 },
   { "iscii-tml", 0xb80 },
   { "iscii-tlg", 0xc00 },
   { "iscii-knd", 0xc80 },
   { "iscii-mlm", 0xd00 }
};

TextCodec *IsciiCodec::create(const char *name)
{
   for (int i = 0; i < 9; ++i) {
      if (text_codec_name_match(name, codecs[i].m_name))
         return new IsciiCodec(i);
   }
   return 0;
}

IsciiCodec::~IsciiCodec()
{
}

ByteArray IsciiCodec::name() const
{
   return codecs[m_idx].m_name;
}

int IsciiCodec::mibEnum() const
{
   /* There is no MIBEnum for Iscii */
   return -3000 - m_idx;
}

static constexpr const uchar inv = 0xFF;

/* iscii range from 0xa0 - 0xff */
static const uchar sg_isciiToUniTable[0x60] = {
   0x00, 0x01, 0x02, 0x03,
   0x05, 0x06, 0x07, 0x08,
   0x09, 0x0a, 0x0b, 0x0e,
   0x0f, 0x20, 0x0d, 0x12,
   
   0x13, 0x14, 0x11, 0x15,
   0x16, 0x17, 0x18, 0x19,
   0x1a, 0x1b, 0x1c, 0x1d,
   0x1e, 0x1f, 0x20, 0x21,
   
   0x22, 0x23, 0x24, 0x25,
   0x26, 0x27, 0x28, 0x29,
   0x2a, 0x2b, 0x2c, 0x2d,
   0x2e, 0x2f, 0x5f, 0x30,
   
   0x31, 0x32, 0x33, 0x34,
   0x35, 0x36, 0x37, 0x38,
   0x39,  inv, 0x3e, 0x3f,
   0x40, 0x41, 0x42, 0x43,
   
   0x46, 0x47, 0x48, 0x45,
   0x4a, 0x4b, 0x4c, 0x49,
   0x4d, 0x3c, 0x64, 0x00,
   0x00, 0x00, 0x00, 0x00,
   
   0x00, 0x66, 0x67, 0x68,
   0x69, 0x6a, 0x6b, 0x6c,
   0x6d, 0x6e, 0x6f, 0x00,
   0x00, 0x00, 0x00, 0x00
};

static const uchar sg_uniToIsciiTable[0x80] = {
   0x00, 0xa1, 0xa2, 0xa3,
   0x00, 0xa4, 0xa5, 0xa6,
   0xa7, 0xa8, 0xa9, 0xaa,
   0x00, 0xae, 0xab, 0xac,
   
   0xad, 0xb2, 0xaf, 0xb0,
   0xb1, 0xb3, 0xb4, 0xb5,
   0xb6, 0xb7, 0xb8, 0xb9,
   0xba, 0xbb, 0xbc, 0xbd,
   
   0xbe, 0xbf, 0xc0, 0xc1,
   0xc2, 0xc3, 0xc4, 0xc5,
   0xc6, 0xc7, 0xc8, 0xc9,
   0xca, 0xcb, 0xcc, 0xcd,
   
   0xcf, 0xd0, 0xd1, 0xd2,
   0xd3, 0xd4, 0xd5, 0xd6,
   0xd7, 0xd8, 0x00, 0x00,
   0xe9, 0x00, 0xda, 0xdb,
   
   0xdc, 0xdd, 0xde, 0xdf,
   0x00, 0xe3, 0xe0, 0xe1,
   0xe2, 0xe7, 0xe4, 0xe5,
   0xe6, 0xe8, 0x00, 0x00,
   
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x01, 0x02, 0x03, 0x04, // decomposable into the uc codes listed here + nukta
   0x05, 0x06, 0x07, 0xce,
   
   0x00, 0x00, 0x00, 0x00,
   0xea, 0x08, 0xf1, 0xf2,
   0xf3, 0xf4, 0xf5, 0xf6,
   0xf7, 0xf8, 0xf9, 0xfa,
   
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00
};

static const uchar sg_uniToIsciiPairs[] = {
   0x00, 0x00,
   0x15, 0x3c, // 0x958
   0x16, 0x3c, // 0x959
   0x17, 0x3c, // 0x95a
   0x1c, 0x3c, // 0x95b
   0x21, 0x3c, // 0x95c
   0x22, 0x3c, // 0x95d
   0x2b, 0x3c, // 0x95e
   0x64, 0x64  // 0x965
};


ByteArray IsciiCodec::convertFromUnicode(const Character *uc, int len, ConverterState *state) const
{
   char replacement = '?';
   bool halant = false;
   if (state) {
      if (state->m_flags & ConversionFlag::ConvertInvalidToNull) {
         replacement = 0;
      }
      halant = state->m_stateData[0];
   }
   int invalid = 0;
   
   ByteArray result(2 * len, pdk::Uninitialized); //worst case
   
   uchar *ch = reinterpret_cast<uchar *>(result.getRawData());
   
   const int base = codecs[m_idx].m_base;
   
   for (int i =0; i < len; ++i) {
      const ushort codePoint = uc[i].unicode();
      
      /* The low 7 bits of ISCII is plain ASCII. However, we go all the
         * way up to 0xA0 such that we can roundtrip with convertToUnicode()'s
         * behavior. */
      if(codePoint < 0xA0) {
         *ch++ = static_cast<uchar>(codePoint);
         continue;
      }
      
      const int pos = codePoint - base;
      if (pos > 0 && pos < 0x80) {
         uchar iscii = sg_uniToIsciiTable[pos];
         if (iscii > 0x80) {
            *ch++ = iscii;
         } else if (iscii) {
            PDK_ASSERT((2 * iscii) < (sizeof(sg_uniToIsciiPairs) / sizeof(sg_uniToIsciiPairs[0])));
            const uchar *pair = sg_uniToIsciiPairs + 2 * iscii;
            *ch++ = *pair++;
            *ch++ = *pair++;
         } else {
            *ch++ = replacement;
            ++invalid;
         }
      } else {
         if (uc[i].unicode() == 0x200c) { // ZWNJ
            if (halant)
               // Consonant Halant ZWNJ -> Consonant Halant Halant
               *ch++ = 0xe8;
         } else if (uc[i].unicode() == 0x200d) { // ZWJ
            if (halant)
               // Consonant Halant ZWJ -> Consonant Halant Nukta
               *ch++ = 0xe9;
         } else {
            *ch++ = replacement;
            ++invalid;
         }
      }
      halant = (pos == 0x4d);
   }
   result.truncate(ch - (uchar *)result.getRawData());
   
   if (state) {
      state->m_invalidChars += invalid;
      state->m_stateData[0] = halant;
   }
   return result;
}

String IsciiCodec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
   bool halant = false;
   if (state) {
      halant = state->m_stateData[0];
   }
   
   String result(len, pdk::Uninitialized);
   Character *uc = result.getRawData();
   
   const int base = codecs[m_idx].m_base;
   
   for (int i = 0; i < len; ++i) {
      ushort ch = (uchar) chars[i];
      if (ch < 0xa0)
         *uc++ = ch;
      else {
         ushort c = sg_isciiToUniTable[ch - 0xa0];
         if (halant && (c == inv || c == 0xe9)) {
            // Consonant Halant inv -> Consonant Halant ZWJ
            // Consonant Halant Nukta -> Consonant Halant ZWJ
            *uc++ = Character(0x200d);
         } else if (halant && c == 0xe8) {
            // Consonant Halant Halant -> Consonant Halant ZWNJ
            *uc++ = Character(0x200c);
         } else {
            *uc++ = Character(c + base);
         }
      }
      halant = ((uchar)chars[i] == 0xe8);
   }
   result.resize(uc - result.unicode());
   if (state) {
      state->m_stateData[0] = halant;
   }
   return result;
}

} // internal
} // codecs
} // text
} // pdk
