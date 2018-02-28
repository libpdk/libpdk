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
// Created by softboy on 2017/11/14.

#include "pdk/base/lang/Character.h"
#include "pdk/base/lang/internal/UnicodeTablesPrivate.h"
#include "UnicodeTables.cpp"
#include "pdk/base/lang/String.h"
#include "pdk/base/lang/internal/StringHelper.h"

namespace pdk {
namespace lang {

using internal::unicodetables::get_unicode_properties;
using internal::unicodetables::Properties;
using internal::unicodetables::LowercaseTraits;
using internal::unicodetables::TitlecaseTraits;
using internal::unicodetables::UppercaseTraits;
using internal::unicodetables::CasefoldTraits;

#define FLAG(x) (1 << (x))

bool Character::isPrintable(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Other_Control)) |
         FLAG(static_cast<int>(Category::Other_Format)) |
         FLAG(static_cast<int>(Category::Other_Surrogate)) |
         FLAG(static_cast<int>(Category::Other_PrivateUse)) |
         FLAG(static_cast<int>(Category::Other_NotAssigned));
   return !(FLAG(get_unicode_properties(ucs4)->category) & mask);
}

bool Character::isSpaceHelper(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Separator_Space))
         | FLAG(static_cast<int>(Category::Separator_Line))
         | FLAG(static_cast<int>(Category::Separator_Paragraph));
   return FLAG(get_unicode_properties(ucs4)->category) & mask;
}

bool Character::isMark(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Mark_NonSpacing)) |
         FLAG(static_cast<int>(Category::Mark_SpacingCombining)) |
         FLAG(static_cast<int>(Category::Mark_Enclosing));
   return FLAG(get_unicode_properties(ucs4)->category) & mask;
}

bool Character::isPunct(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Punctuation_Connector)) |
         FLAG(static_cast<int>(Category::Punctuation_Dash)) |
         FLAG(static_cast<int>(Category::Punctuation_Open)) |
         FLAG(static_cast<int>(Category::Punctuation_Close)) |
         FLAG(static_cast<int>(Category::Punctuation_InitialQuote)) |
         FLAG(static_cast<int>(Category::Punctuation_FinalQuote)) |
         FLAG(static_cast<int>(Category::Punctuation_Other));
   return FLAG(get_unicode_properties(ucs4)->category) & mask;
}

bool Character::isSymbol(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Symbol_Math)) |
         FLAG(static_cast<int>(Category::Symbol_Currency)) |
         FLAG(static_cast<int>(Category::Symbol_Modifier)) |
         FLAG(static_cast<int>(Category::Symbol_Other));
   return FLAG(get_unicode_properties(ucs4)->category) & mask;
}

bool Character::isLetterHelper(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Letter_Uppercase)) |
         FLAG(static_cast<int>(Category::Letter_Lowercase)) |
         FLAG(static_cast<int>(Category::Letter_Titlecase)) |
         FLAG(static_cast<int>(Category::Letter_Modifier)) |
         FLAG(static_cast<int>(Category::Letter_Other));
   return FLAG(get_unicode_properties(ucs4)->category) & mask;
}

bool Character::isNumberHelper(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Number_DecimalDigit)) |
         FLAG(static_cast<int>(Category::Number_Letter)) |
         FLAG(static_cast<int>(Category::Number_Other));
   return FLAG(get_unicode_properties(ucs4)->category) & mask;
}

bool Character::isLetterOrNumberHelper(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Letter_Uppercase)) |
         FLAG(static_cast<int>(Category::Letter_Lowercase)) |
         FLAG(static_cast<int>(Category::Letter_Titlecase)) |
         FLAG(static_cast<int>(Category::Letter_Modifier)) |
         FLAG(static_cast<int>(Category::Letter_Other)) |
         FLAG(static_cast<int>(Category::Number_DecimalDigit)) |
         FLAG(static_cast<int>(Category::Number_Letter)) |
         FLAG(static_cast<int>(Category::Number_Other));
   return FLAG(get_unicode_properties(ucs4)->category) & mask;
}

int Character::getDigitValue(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return -1;
   }
   return get_unicode_properties(ucs4)->digitValue;
}

Character::Category Character::getCategory(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return Category::Other_NotAssigned;
   }
   return static_cast<Category>(get_unicode_properties(ucs4)->category);
}

Character::Direction Character::getDirection(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return Direction::DirL;
   }
   return static_cast<Direction>(get_unicode_properties(ucs4)->direction);
}

Character::JoiningType Character::getJoiningType(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return JoiningType::Joining_None;
   }
   return static_cast<JoiningType>(get_unicode_properties(ucs4)->joining);
}

bool Character::hasMirrored(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   return get_unicode_properties(ucs4)->mirrorDiff != 0;
}

char32_t Character::getMirroredCharacter(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return ucs4;
   }
   return ucs4 + get_unicode_properties(ucs4)->mirrorDiff;
}

const unsigned short *uc_decomposition_trie = internal::unicodetables::uc_decomposition_trie;
const unsigned short *uc_decomposition_map = internal::unicodetables::uc_decomposition_map;
const unsigned short *special_case_map = internal::unicodetables::special_case_map;

namespace  {

enum {
   Hangul_SBase = 0xac00,
   Hangul_LBase = 0x1100,
   Hangul_VBase = 0x1161,
   Hangul_TBase = 0x11a7,
   Hangul_LCount = 19,
   Hangul_VCount = 21,
   Hangul_TCount = 28,
   Hangul_NCount = Hangul_VCount * Hangul_TCount,
   Hangul_SCount = Hangul_LCount * Hangul_NCount
};

// buffer has to have a length of 3. It's needed for Hangul decomposition
const unsigned short * PDK_FASTCALL decomposition_helper(char32_t ucs4, int *length, int *tag, 
                                                         unsigned short *buffer)
{
   if (ucs4 >= Hangul_SBase && ucs4 < Hangul_SBase + Hangul_SCount) {
      // compute Hangul syllable decomposition as per UAX #15
      const char32_t SIndex = ucs4 - Hangul_SBase;
      buffer[0] = Hangul_LBase + SIndex / Hangul_NCount; // L
      buffer[1] = Hangul_VBase + (SIndex % Hangul_NCount) / Hangul_TCount; // V
      buffer[2] = Hangul_TBase + SIndex % Hangul_TCount; // T
      *length = buffer[2] == Hangul_TBase ? 2 : 3;
      *tag = static_cast<int>(Character::Decomposition::Canonical);
      return buffer;
   }
   const unsigned short index = PDK_GET_DECOMPOSITION_INDEX(ucs4);
   if (index == 0xffff) {
      *length = 0;
      *tag = static_cast<int>(Character::Decomposition::NoDecomposition);
      return 0;
   }
   const unsigned short *decomposition = uc_decomposition_map + index;
   *tag = (*decomposition) & 0xff;
   *length = (*decomposition) >> 8;
   return decomposition + 1;
}

template <typename Traits, typename T>
PDK_DECL_CONST_FUNCTION inline T convert_case_helper(T ucs) noexcept
{
   const Properties *prop = get_unicode_properties(ucs);
   if (PDK_UNLIKELY(Traits::caseSpecial(prop))) {
      const ushort *specialCase = special_case_map + Traits::caseDiff(prop);
      // so far, there are no special cases beyond BMP (guaranteed by the unicodetables generator)
      if (*specialCase == 1) {
         return specialCase[1];
      }
      return ucs;
   }
   return ucs + Traits::caseDiff(prop);
}

}

namespace internal {

char32_t fold_case(const char16_t *ch, const char16_t *start) noexcept
{
   char32_t ucs4 = *ch;
   if (Character::isLowSurrogate(ucs4) && ch > start && Character::isHighSurrogate(*(ch - 1))) {
      ucs4 = Character::surrogateToUcs4(*(ch - 1), ucs4);
   }
   return convert_case_helper<CasefoldTraits>(ucs4);
}

char32_t fold_case(char32_t ch, char32_t &last) noexcept
{
    char32_t ucs4 = ch;
    if (Character::isLowSurrogate(ucs4) && Character::isHighSurrogate(last)) {
       ucs4 = Character::surrogateToUcs4(last, ucs4);
    }
    last = ch;
    return convert_case_helper<CasefoldTraits>(ucs4);
}

char16_t fold_case(char16_t ch) noexcept
{
    return convert_case_helper<CasefoldTraits>(ch);
}

Character fold_case(Character ch) noexcept
{
   return Character(fold_case(ch.unicode()));
}

}

String Character::getDecomposition() const
{
   return Character::getDecomposition(m_data);
}

// @TODO string decomposition
String Character::getDecomposition(char32_t ucs4)
{
   unsigned short buffer[3];
   int length;
   int tag;
   const unsigned short *d = decomposition_helper(ucs4, &length, &tag, buffer);
   return String();
}

Character::Decomposition Character::getDecompositionTag(char32_t ucs4) noexcept
{
   if (ucs4 >= Hangul_SBase && ucs4 < Hangul_SBase + Hangul_SCount)
      return Character::Decomposition::Canonical;
   const unsigned short index = PDK_GET_DECOMPOSITION_INDEX(ucs4);
   if (index == 0xffff)
      return Character::Decomposition::NoDecomposition;
   return static_cast<Character::Decomposition>(uc_decomposition_map[index] & 0xff);
}

unsigned char Character::getCombiningClass(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return 0;
   }
   return static_cast<unsigned char>(get_unicode_properties(ucs4)->combiningClass);
}

Character::Script Character::getScript(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return Character::Script::Script_Unknown;
   }
   return static_cast<Character::Script>(get_unicode_properties(ucs4)->script);
}

Character::UnicodeVersion Character::getUnicodeVersion(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return Character::UnicodeVersion::Unicode_Unassigned;
   }
   return static_cast<Character::UnicodeVersion>(get_unicode_properties(ucs4)->unicodeVersion);
}

Character::UnicodeVersion Character::getCurrentUnicodeVersion() noexcept
{
   return PDK_UNICODE_DATA_VERSION;
}

char32_t Character::toLower(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return ucs4;
   }
   return convert_case_helper<LowercaseTraits>(ucs4);
}

char32_t Character::toUpper(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return ucs4;
   }
   return convert_case_helper<UppercaseTraits>(ucs4);
}

char32_t Character::toTitleCase(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return ucs4;
   }
   return convert_case_helper<TitlecaseTraits>(ucs4);
}

char32_t Character::toCaseFolded(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return ucs4;
   }
   return convert_case_helper<CasefoldTraits>(ucs4);
}

} // lang
} // pdk
