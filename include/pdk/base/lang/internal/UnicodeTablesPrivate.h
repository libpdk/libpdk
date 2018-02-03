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
// Created by softboy on 2017/11/15.

#ifndef PDK_M_BASE_LANG_INTERNAL_UNICODETABLES_H
#define PDK_M_BASE_LANG_INTERNAL_UNICODETABLES_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/Character.h"

#define PDK_UNICODE_DATA_VERSION pdk::lang::Character::UnicodeVersion::Unicode_8_0;

namespace pdk {
namespace lang {
namespace internal {
namespace unicodetables {

#define PDK_GET_PROP_INDEX(ucs4) \
   (ucs4 < 0x11000 \
   ? (uc_property_trie[uc_property_trie[ucs4>>5] + (ucs4 & 0x1f)])\
   : (uc_property_trie[uc_property_trie[((ucs4 - 0x11000)>>8) + 0x880] + (ucs4 & 0xff)]))

#define PDK_GET_PROP_INDEX_UCS2(ucs2) \
   (uc_property_trie[uc_property_trie[ucs2>>5] + (ucs2 & 0x1f)])

#define PDK_GET_DECOMPOSITION_INDEX(ucs4) \
   (ucs4 < 0x3400 \
   ? (uc_decomposition_trie[uc_decomposition_trie[(ucs4>>4)] + (ucs4 & 0xf)]) \
   : (ucs4 < 0x30000 \
   ? uc_decomposition_trie[uc_decomposition_trie[((ucs4 - 0x3400)>>8) + 0x340] + (ucs4 & 0xff)] \
   : 0xffff))

#define PDK_GET_LIGATURE_INDEX(ucs4) \
   (ucs4 < 0x3100 \
   ? (uc_ligature_trie[uc_ligature_trie[ucs4>>5] + (ucs4 & 0x1f)]) \
   : (ucs4 < 0x12000 \
   ? uc_ligature_trie[uc_ligature_trie[((ucs4 - 0x3100)>>8) + 0x188] + (ucs4 & 0xff)] \
   : 0xffff))

struct Properties 
{
   ushort category             : 8; /* 5 used */
   ushort direction           : 8; /* 5 used */
   ushort combiningClass      : 8;
   ushort joining             : 3;
   signed short digitValue    : 5;
   signed short mirrorDiff    : 16;
   ushort lowerCaseSpecial    : 1;
   signed short lowerCaseDiff : 15;
   ushort upperCaseSpecial    : 1;
   signed short upperCaseDiff : 15;
   ushort titleCaseSpecial    : 1;
   signed short titleCaseDiff : 15;
   ushort caseFoldSpecial     : 1;
   signed short caseFoldDiff  : 15;
   ushort unicodeVersion      : 8; /* 5 used */
   ushort nfQuickCheck        : 8;
   ushort graphemeBreakClass  : 4; /* 4 used */
   ushort wordBreakClass      : 4; /* 4 used */
   ushort sentenceBreakClass  : 8; /* 4 used */
   ushort lineBreakClass      : 8; /* 6 used */
   ushort script              : 8;
};

PDK_CORE_EXPORT const Properties * PDK_FASTCALL get_unicode_properties(char32_t ucs4) noexcept;
PDK_CORE_EXPORT const Properties * PDK_FASTCALL get_unicode_properties(char16_t ucs4) noexcept;

struct LowercaseTraits
{
   static inline signed short caseDiff(const Properties *prop)
   {
      return prop->lowerCaseDiff;
   }
   
   static inline bool caseSpecial(const Properties *prop)
   {
      return prop->lowerCaseSpecial;
   }
};

struct UppercaseTraits
{
   static inline signed short caseDiff(const Properties *prop)
   {
      return prop->upperCaseDiff;
   }
   
   static inline bool caseSpecial(const Properties *prop)
   {
      return prop->upperCaseSpecial;
   }
};


struct TitlecaseTraits
{
   static inline signed short caseDiff(const Properties *prop)
   { 
      return prop->titleCaseDiff; 
   }
   
   static inline bool caseSpecial(const Properties *prop)
   { 
      return prop->titleCaseSpecial; 
   }
};

struct CasefoldTraits
{
   static inline signed short caseDiff(const Properties *prop)
   { 
      return prop->caseFoldDiff;
   }
   
   static inline bool caseSpecial(const Properties *prop)
   { 
      return prop->caseFoldSpecial; 
   }
};

enum class GraphemeBreakClass
{
   GraphemeBreak_Other,
   GraphemeBreak_CR,
   GraphemeBreak_LF,
   GraphemeBreak_Control,
   GraphemeBreak_Extend,
   GraphemeBreak_RegionalIndicator,
   GraphemeBreak_Prepend,
   GraphemeBreak_SpacingMark,
   GraphemeBreak_L,
   GraphemeBreak_V,
   GraphemeBreak_T,
   GraphemeBreak_LV,
   GraphemeBreak_LVT
};

enum class WordBreakClass 
{
   WordBreak_Other,
   WordBreak_CR,
   WordBreak_LF,
   WordBreak_Newline,
   WordBreak_Extend,
   WordBreak_RegionalIndicator,
   WordBreak_Katakana,
   WordBreak_HebrewLetter,
   WordBreak_ALetter,
   WordBreak_SingleQuote,
   WordBreak_DoubleQuote,
   WordBreak_MidNumLet,
   WordBreak_MidLetter,
   WordBreak_MidNum,
   WordBreak_Numeric,
   WordBreak_ExtendNumLet
};

enum class SentenceBreakClass
{
   SentenceBreak_Other,
   SentenceBreak_CR,
   SentenceBreak_LF,
   SentenceBreak_Sep,
   SentenceBreak_Extend,
   SentenceBreak_Sp,
   SentenceBreak_Lower,
   SentenceBreak_Upper,
   SentenceBreak_OLetter,
   SentenceBreak_Numeric,
   SentenceBreak_ATerm,
   SentenceBreak_SContinue,
   SentenceBreak_STerm,
   SentenceBreak_Close
};

// see http://www.unicode.org/reports/tr14/tr14-30.html
// we don't use the XX and AI classes and map them to AL instead.
enum class LineBreakClass
{
   LineBreak_OP, LineBreak_CL, LineBreak_CP, LineBreak_QU, LineBreak_GL,
   LineBreak_NS, LineBreak_EX, LineBreak_SY, LineBreak_IS, LineBreak_PR,
   LineBreak_PO, LineBreak_NU, LineBreak_AL, LineBreak_HL, LineBreak_ID,
   LineBreak_IN, LineBreak_HY, LineBreak_BA, LineBreak_BB, LineBreak_B2,
   LineBreak_ZW, LineBreak_CM, LineBreak_WJ, LineBreak_H2, LineBreak_H3,
   LineBreak_JL, LineBreak_JV, LineBreak_JT, LineBreak_RI, LineBreak_CB,
   LineBreak_SA, LineBreak_SG, LineBreak_SP, LineBreak_CR, LineBreak_LF,
   LineBreak_BK
};

PDK_CORE_EXPORT GraphemeBreakClass PDK_FASTCALL grapheme_break_class(char32_t ucs4) noexcept;
inline GraphemeBreakClass grapheme_break_class(Character ch) noexcept
{
   return grapheme_break_class(ch.unicode());
}

PDK_CORE_EXPORT WordBreakClass PDK_FASTCALL word_break_class(char32_t ucs4) noexcept;
inline WordBreakClass word_break_class(Character ch) noexcept
{
   return word_break_class(ch.unicode());
}

PDK_CORE_EXPORT SentenceBreakClass PDK_FASTCALL sentence_break_class(char32_t ucs4) noexcept;
inline SentenceBreakClass sentence_break_class(Character ch) noexcept
{
   return sentence_break_class(ch.unicode());
}

PDK_CORE_EXPORT LineBreakClass PDK_FASTCALL line_break_class(char32_t ucs4) noexcept;
inline LineBreakClass line_break_class(Character ch) noexcept
{
   return line_break_class(ch.unicode());
}

} // unicodetables
} // internal
} // lang
} // pdk

#endif // PDK_M_BASE_LANG_INTERNAL_UNICODETABLES_H
