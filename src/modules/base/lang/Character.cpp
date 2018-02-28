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
   return !(FLAG(get_unicode_properties(ucs4)->m_category) & mask);
}

bool Character::isSpaceHelper(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Separator_Space))
         | FLAG(static_cast<int>(Category::Separator_Line))
         | FLAG(static_cast<int>(Category::Separator_Paragraph));
   return FLAG(get_unicode_properties(ucs4)->m_category) & mask;
}

bool Character::isMark(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Mark_NonSpacing)) |
         FLAG(static_cast<int>(Category::Mark_SpacingCombining)) |
         FLAG(static_cast<int>(Category::Mark_Enclosing));
   return FLAG(get_unicode_properties(ucs4)->m_category) & mask;
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
   return FLAG(get_unicode_properties(ucs4)->m_category) & mask;
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
   return FLAG(get_unicode_properties(ucs4)->m_category) & mask;
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
   return FLAG(get_unicode_properties(ucs4)->m_category) & mask;
}

bool Character::isNumberHelper(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   const int mask = FLAG(static_cast<int>(Category::Number_DecimalDigit)) |
         FLAG(static_cast<int>(Category::Number_Letter)) |
         FLAG(static_cast<int>(Category::Number_Other));
   return FLAG(get_unicode_properties(ucs4)->m_category) & mask;
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
   return FLAG(get_unicode_properties(ucs4)->m_category) & mask;
}

int Character::getDigitValue(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return -1;
   }
   return get_unicode_properties(ucs4)->m_digitValue;
}

Character::Category Character::getCategory(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return Category::Other_NotAssigned;
   }
   return static_cast<Category>(get_unicode_properties(ucs4)->m_category);
}

Character::Direction Character::getDirection(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return Direction::DirL;
   }
   return static_cast<Direction>(get_unicode_properties(ucs4)->m_direction);
}

Character::JoiningType Character::getJoiningType(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return JoiningType::Joining_None;
   }
   return static_cast<JoiningType>(get_unicode_properties(ucs4)->m_joining);
}

bool Character::hasMirrored(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return false;
   }
   return get_unicode_properties(ucs4)->m_mirrorDiff != 0;
}

char32_t Character::getMirroredCharacter(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return ucs4;
   }
   return ucs4 + get_unicode_properties(ucs4)->m_mirrorDiff;
}

const char16_t *uc_decomposition_trie = internal::unicodetables::uc_decomposition_trie;
const char16_t *uc_decomposition_map = internal::unicodetables::uc_decomposition_map;
const char16_t *special_case_map = internal::unicodetables::special_case_map;

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
const char16_t * PDK_FASTCALL decomposition_helper(char32_t ucs4, int *length, int *tag, 
                                                         char16_t *buffer)
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
   const char16_t index = PDK_GET_DECOMPOSITION_INDEX(ucs4);
   if (index == 0xffff) {
      *length = 0;
      *tag = static_cast<int>(Character::Decomposition::NoDecomposition);
      return 0;
   }
   const char16_t *decomposition = uc_decomposition_map + index;
   *tag = (*decomposition) & 0xff;
   *length = (*decomposition) >> 8;
   return decomposition + 1;
}

template <typename Traits, typename T>
PDK_DECL_CONST_FUNCTION inline T convert_case_helper(T ucs) noexcept
{
   const Properties *prop = get_unicode_properties(ucs);
   if (PDK_UNLIKELY(Traits::caseSpecial(prop))) {
      const char16_t *specialCase = special_case_map + Traits::caseDiff(prop);
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
   char16_t buffer[3];
   int length;
   int tag;
   const char16_t *d = decomposition_helper(ucs4, &length, &tag, buffer);
   return String();
}

Character::Decomposition Character::getDecompositionTag(char32_t ucs4) noexcept
{
   if (ucs4 >= Hangul_SBase && ucs4 < Hangul_SBase + Hangul_SCount)
      return Character::Decomposition::Canonical;
   const char16_t index = PDK_GET_DECOMPOSITION_INDEX(ucs4);
   if (index == 0xffff)
      return Character::Decomposition::NoDecomposition;
   return static_cast<Character::Decomposition>(uc_decomposition_map[index] & 0xff);
}

unsigned char Character::getCombiningClass(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return 0;
   }
   return static_cast<unsigned char>(get_unicode_properties(ucs4)->m_combiningClass);
}

Character::Script Character::getScript(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return Character::Script::Script_Unknown;
   }
   return static_cast<Character::Script>(get_unicode_properties(ucs4)->m_script);
}

Character::UnicodeVersion Character::getUnicodeVersion(char32_t ucs4) noexcept
{
   if (ucs4 > LastValidCodePoint) {
      return Character::UnicodeVersion::Unicode_Unassigned;
   }
   return static_cast<Character::UnicodeVersion>(get_unicode_properties(ucs4)->m_unicodeVersion);
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

struct UCS2Pair
{
   char16_t m_u1;
   char16_t m_u2;
};

inline bool operator<(const UCS2Pair &ligature1, const UCS2Pair &ligature2)
{
   return ligature1.m_u1 < ligature2.m_u1;
}

inline bool operator<(char16_t u1, const UCS2Pair &ligature)
{
   return u1 < ligature.m_u1;
}

inline bool operator<(const UCS2Pair &ligature, char16_t u1)
{
   return ligature.m_u1 < u1;
}

namespace internal {

// buffer has to have a length of 3. It's needed for Hangul decomposition
const char16_t * PDK_FASTCALL decomposition_helper
(char32_t ucs4, int *length, int *tag, char16_t *buffer)
{
   if (ucs4 >= Hangul_SBase && ucs4 < Hangul_SBase + Hangul_SCount) {
      // compute Hangul syllable decomposition as per UAX #15
      const uint SIndex = ucs4 - Hangul_SBase;
      buffer[0] = Hangul_LBase + SIndex / Hangul_NCount; // L
      buffer[1] = Hangul_VBase + (SIndex % Hangul_NCount) / Hangul_TCount; // V
      buffer[2] = Hangul_TBase + SIndex % Hangul_TCount; // T
      *length = buffer[2] == Hangul_TBase ? 2 : 3;
      *tag = pdk::as_integer<Character::Decomposition>(Character::Decomposition::Canonical);
      return buffer;
   }
   
   const char16_t index = PDK_GET_DECOMPOSITION_INDEX(ucs4);
   if (index == 0xffff) {
      *length = 0;
      *tag = pdk::as_integer<Character::Decomposition>(Character::Decomposition::NoDecomposition);
      return 0;
   }
   
   const char16_t *decomposition = uc_decomposition_map+index;
   *tag = (*decomposition) & 0xff;
   *length = (*decomposition) >> 8;
   return decomposition+1;
}

void decompose_helper(String *str, bool canonical, Character::UnicodeVersion version, int from)
{
   int length;
   int tag;
   char16_t buffer[3];
   String &s = *str;
   const char16_t *utf16 = reinterpret_cast<char16_t *>(s.getRawData());
   const char16_t *uc = utf16 + s.length();
   while (uc != utf16 + from) {
      char32_t ucs4 = *(--uc);
      if (Character(ucs4).isLowSurrogate() && uc != utf16) {
         ushort high = *(uc - 1);
         if (Character(high).isHighSurrogate()) {
            --uc;
            ucs4 = Character::surrogateToUcs4(high, ucs4);
         }
      }
      if (Character::getUnicodeVersion(ucs4) > version) {
         continue;
      }
      const char16_t *d = decomposition_helper(ucs4, &length, &tag, buffer);
      if (!d || (canonical && tag != pdk::as_integer<Character::Decomposition>(Character::Decomposition::Canonical)))
         continue;
      
      int pos = uc - utf16;
      s.replace(pos, Character::requiresSurrogates(ucs4) ? 2 : 1, reinterpret_cast<const Character *>(d), length);
      // since the replace invalidates the pointers and we do decomposition recursive
      utf16 = reinterpret_cast<char16_t *>(s.getRawData());
      uc = utf16 + pos + length;
   }
}

struct UCS2SurrogatePair
{
   UCS2Pair m_p1;
   UCS2Pair m_p2;
};

inline bool operator<(const UCS2SurrogatePair &ligature1, const UCS2SurrogatePair &ligature2)
{
   return Character::surrogateToUcs4(ligature1.m_p1.m_u1, ligature1.m_p1.m_u2) 
         < Character::surrogateToUcs4(ligature2.m_p1.m_u1, ligature2.m_p1.m_u2); 
}
inline bool operator<(uint u1, const UCS2SurrogatePair &ligature)
{
   return u1 < Character::surrogateToUcs4(ligature.m_p1.m_u1, ligature.m_p1.m_u2);
}

inline bool operator<(const UCS2SurrogatePair &ligature, uint u1)
{
   return Character::surrogateToUcs4(ligature.m_p1.m_u1, ligature.m_p1.m_u2) < u1;
}

using namespace pdk::lang::internal::unicodetables;

char32_t inline ligature_helper(char32_t u1, char32_t u2)
{
   if (u1 >= Hangul_LBase && u1 <= Hangul_SBase + Hangul_SCount) {
      // compute Hangul syllable composition as per UAX #15
      // hangul L-V pair
      const uint LIndex = u1 - Hangul_LBase;
      if (LIndex < Hangul_LCount) {
         const uint VIndex = u2 - Hangul_VBase;
         if (VIndex < Hangul_VCount)
            return Hangul_SBase + (LIndex * Hangul_VCount + VIndex) * Hangul_TCount;
      }
      // hangul LV-T pair
      const uint SIndex = u1 - Hangul_SBase;
      if (SIndex < Hangul_SCount && (SIndex % Hangul_TCount) == 0) {
         const uint TIndex = u2 - Hangul_TBase;
         if (TIndex <= Hangul_TCount)
            return u1 + TIndex;
      }
   }
   
   const char16_t index = PDK_GET_LIGATURE_INDEX(u2);
   if (index == 0xffff) {
      return 0;
   }
   const char16_t *ligatures = uc_ligature_map+index;
   ushort length = *ligatures++;
   if (Character::requiresSurrogates(u1)) {
      const UCS2SurrogatePair *data = reinterpret_cast<const UCS2SurrogatePair *>(ligatures);
      const UCS2SurrogatePair *r = std::lower_bound(data, data + length, u1);
      if (r != data + length && Character::surrogateToUcs4(r->m_p1.m_u1, r->m_p1.m_u2) == u1)
         return Character::surrogateToUcs4(r->m_p2.m_u1, r->m_p2.m_u2);
   } else {
      const UCS2Pair *data = reinterpret_cast<const UCS2Pair *>(ligatures);
      const UCS2Pair *r = std::lower_bound(data, data + length, ushort(u1));
      if (r != data + length && r->m_u1 == ushort(u1)) {
         return r->m_u2;
      }         
   }
   return 0;
}

void compose_helper(String *str, Character::UnicodeVersion version, int from)
{
   String &s = *str;
   if (from < 0 || s.length() - from < 2) {
      return;
   }
   uint stcode = 0; // starter code point
   int starter = -1; // starter position
   int next = -1; // to prevent i == next
   int lastCombining = 255; // to prevent combining > lastCombining
   
   int pos = from;
   while (pos < s.length()) {
      int i = pos;
      char32_t uc = s.at(pos).unicode();
      if (Character(uc).isHighSurrogate() && pos < s.length()-1) {
         ushort low = s.at(pos+1).unicode();
         if (Character(low).isLowSurrogate()) {
            uc = Character::surrogateToUcs4(uc, low);
            ++pos;
         }
      }
      
      const Properties *p = get_unicode_properties(uc);
      if (p->m_unicodeVersion > pdk::as_integer<Character::UnicodeVersion>(version)) {
         starter = -1;
         next = -1; // to prevent i == next
         lastCombining = 255; // to prevent combining > lastCombining
         ++pos;
         continue;
      }
      
      int combining = p->m_combiningClass;
      if ((i == next || combining > lastCombining) && starter >= from) {
         // allowed to form ligature with S
         char32_t ligature = ligature_helper(stcode, uc);
         if (ligature) {
            stcode = ligature;
            Character *d = s.getRawData();
            // ligatureHelper() never changes planes
            if (Character::requiresSurrogates(ligature)) {
               d[starter] = Character::getHighSurrogate(ligature);
               d[starter + 1] = Character::getLowSurrogate(ligature);
               s.remove(i, 2);
            } else {
               d[starter] = ligature;
               s.remove(i, 1);
            }
            continue;
         }
      }
      if (combining == 0) {
         starter = i;
         stcode = uc;
         next = pos + 1;
      }
      lastCombining = combining;
      
      ++pos;
   }
}

void canonical_order_helper(String *str, Character::UnicodeVersion version, int from)
{
   String &s = *str;
   const int l = s.length()-1;
   
   char32_t u1, u2;
   char16_t c1, c2;
   
   int pos = from;
   while (pos < l) {
      int p2 = pos+1;
      u1 = s.at(pos).unicode();
      if (Character(u1).isHighSurrogate()) {
         ushort low = s.at(p2).unicode();
         if (Character(low).isLowSurrogate()) {
            u1 = Character::surrogateToUcs4(u1, low);
            if (p2 >= l) {
               break;
            }
            ++p2;
         }
      }
      c1 = 0;
      
advance:
      u2 = s.at(p2).unicode();
      if (Character(u2).isHighSurrogate() && p2 < l) {
         ushort low = s.at(p2+1).unicode();
         if (Character(low).isLowSurrogate()) {
            u2 = Character::surrogateToUcs4(u2, low);
            ++p2;
         }
      }
      
      c2 = 0;
      {
         const Properties *p = get_unicode_properties(u2);
         if (p->m_unicodeVersion <= pdk::as_integer<Character::UnicodeVersion>(version)) {
            c2 = p->m_combiningClass;
         }
      }
      if (c2 == 0) {
         pos = p2+1;
         continue;
      }
      
      if (c1 == 0) {
         const Properties *p = get_unicode_properties(u1);
         if (p->m_unicodeVersion <= pdk::as_integer<Character::UnicodeVersion>(version)) {
            c1 = p->m_combiningClass;
         }
      }
      
      if (c1 > c2) {
         Character *uc = s.getRawData();
         int p = pos;
         // exchange characters
         if (!Character::requiresSurrogates(u2)) {
            uc[p++] = u2;
         } else {
            uc[p++] = Character::getHighSurrogate(u2);
            uc[p++] = Character::getLowSurrogate(u2);
         }
         if (!Character::requiresSurrogates(u1)) {
            uc[p++] = u1;
         } else {
            uc[p++] = Character::getHighSurrogate(u1);
            uc[p++] = Character::getLowSurrogate(u1);
         }
         if (pos > 0) {
            --pos;
         }
         if (pos > 0 && s.at(pos).isLowSurrogate()) {
            --pos;
         }
      } else {
         ++pos;
         if (Character::requiresSurrogates(u1))
            ++pos;
         
         u1 = u2;
         c1 = c2; // != 0
         p2 = pos + 1;
         if (Character::requiresSurrogates(u1))
            ++p2;
         if (p2 > l)
            break;
         
         goto advance;
      }
   }
}

// returns true if the text is in a desired Normalization Form already; false otherwise.
// sets lastStable to the position of the last stable code point
bool normalization_quick_check_helper(String *str, String::NormalizationForm mode, int from, int *lastStable)
{
   PDK_STATIC_ASSERT((int)String::NormalizationForm::Form_D == 0);
   PDK_STATIC_ASSERT((int)String::NormalizationForm::Form_C == 1);
   PDK_STATIC_ASSERT((int)String::NormalizationForm::Form_KD == 2);
   PDK_STATIC_ASSERT((int)String::NormalizationForm::Form_KC == 3);
   enum { NFQC_YES = 0, NFQC_NO = 1, NFQC_MAYBE = 3 };
   const char16_t *string = reinterpret_cast<const char16_t *>(str->getConstRawData());
   int length = str->length();
   
   // this avoids one out of bounds check in the loop
   while (length > from && Character::isHighSurrogate(string[length - 1])) {
      --length;
   }
   uchar lastCombining = 0;
   for (int i = from; i < length; ++i) {
      int pos = i;
      char32_t uc = string[i];
      if (uc < 0x80) {
         // ASCII characters are stable code points
         lastCombining = 0;
         *lastStable = pos;
         continue;
      }
      
      if (Character::isHighSurrogate(uc)) {
         ushort low = string[i + 1];
         if (!Character::isLowSurrogate(low)) {
            // treat surrogate like stable code point
            lastCombining = 0;
            *lastStable = pos;
            continue;
         }
         ++i;
         uc = Character::surrogateToUcs4(uc, low);
      }
      
      const Properties *p = get_unicode_properties(uc);
      
      if (p->m_combiningClass < lastCombining && p->m_combiningClass > 0) {
         return false;
      }
      const uchar check = (p->m_nfQuickCheck >> (pdk::as_integer<String::NormalizationForm>(mode) << 1)) & 0x03;
      if (check != NFQC_YES) {
         return false; // ### can we quick check NFQC_MAYBE ?
      }
      lastCombining = p->m_combiningClass;
      if (lastCombining == 0) {
         *lastStable = pos;
      }
   }
   
   if (length != str->length()) {// low surrogate parts at the end of text
      *lastStable = str->length() - 1;
   }
   return true;
}

} // internal

} // lang
} // pdk
