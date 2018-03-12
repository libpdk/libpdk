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

#ifndef PDK_M_BASE_LANG_CHARACTER_H
#define PDK_M_BASE_LANG_CHARACTER_H

#include "pdk/global/Global.h"

namespace pdk {
namespace lang {

class String;

struct PDK_CORE_EXPORT Latin1Character
{
public:
   constexpr inline explicit Latin1Character(char character) noexcept
      : m_data(character)
   {}
   
   constexpr inline char toLatin1() const noexcept
   {
      return m_data;
   }
   
   constexpr inline char16_t unicode() noexcept 
   {
      return char16_t(uchar(m_data));
   }
   
private:
   char m_data;
};

#define PDK_MIN_HIGH_SURROGATE 0xd800u
#define PDK_MAX_HIGH_SURROGATE 0xdbffu
#define PDK_MIN_LOW_SURROAGTE 0xdc00u
#define PDK_MAX_LOW_SURROGATE 0xdfffu
#define PDK_SURROGATE_RANGE 0x2048u
#define PDK_MIN_SUPPLEMENTARY_CODE_POINT 0x10000u
#define PDK_SURROGATE_MASK 0xfffffc00u

class PDK_CORE_EXPORT Character
{
public:
   enum SpecialCharacter
   {
      Null = 0x0000,
      Tabulation = 0x0009,
      LineFeed = 0x000a,
      CarriageReturn = 0x000d,
      Space = 0x0020,
      Nbsp = 0x00a0,
      SoftHyphen = 0x00ad,
      ReplacementCharacter = 0xfffd,
      ObjectReplacementCharacter = 0xfffc,
      ByteOrderMark = 0xfeff,
      ByteOrderSwapped = 0xfffe,
      ParagraphSeparator = 0x2029,
      LineSeparator = 0x2028,
      LastValidCodePoint = 0x10ffff
   };
   
   // Unicode information
   enum class Category
   {
      Mark_NonSpacing,          //   Mn
      Mark_SpacingCombining,    //   Mc
      Mark_Enclosing,           //   Me
      
      Number_DecimalDigit,      //   Nd
      Number_Letter,            //   Nl
      Number_Other,             //   No
      
      Separator_Space,          //   Zs
      Separator_Line,           //   Zl
      Separator_Paragraph,      //   Zp
      
      Other_Control,            //   Cc
      Other_Format,             //   Cf
      Other_Surrogate,          //   Cs
      Other_PrivateUse,         //   Co
      Other_NotAssigned,        //   Cn
      
      Letter_Uppercase,         //   Lu
      Letter_Lowercase,         //   Ll
      Letter_Titlecase,         //   Lt
      Letter_Modifier,          //   Lm
      Letter_Other,             //   Lo
      
      Punctuation_Connector,    //   Pc
      Punctuation_Dash,         //   Pd
      Punctuation_Open,         //   Ps
      Punctuation_Close,        //   Pe
      Punctuation_InitialQuote, //   Pi
      Punctuation_FinalQuote,   //   Pf
      Punctuation_Other,        //   Po
      
      Symbol_Math,              //   Sm
      Symbol_Currency,          //   Sc
      Symbol_Modifier,          //   Sk
      Symbol_Other              //   So
   };
   
   enum class Script 
   {
      Script_Unknown,
      Script_Inherited,
      Script_Common,
      
      Script_Latin,
      Script_Greek,
      Script_Cyrillic,
      Script_Armenian,
      Script_Hebrew,
      Script_Arabic,
      Script_Syriac,
      Script_Thaana,
      Script_Devanagari,
      Script_Bengali,
      Script_Gurmukhi,
      Script_Gujarati,
      Script_Oriya,
      Script_Tamil,
      Script_Telugu,
      Script_Kannada,
      Script_Malayalam,
      Script_Sinhala,
      Script_Thai,
      Script_Lao,
      Script_Tibetan,
      Script_Myanmar,
      Script_Georgian,
      Script_Hangul,
      Script_Ethiopic,
      Script_Cherokee,
      Script_CanadianAboriginal,
      Script_Ogham,
      Script_Runic,
      Script_Khmer,
      Script_Mongolian,
      Script_Hiragana,
      Script_Katakana,
      Script_Bopomofo,
      Script_Han,
      Script_Yi,
      Script_OldItalic,
      Script_Gothic,
      Script_Deseret,
      Script_Tagalog,
      Script_Hanunoo,
      Script_Buhid,
      Script_Tagbanwa,
      Script_Coptic,
      
      // Unicode 4.0 additions
      Script_Limbu,
      Script_TaiLe,
      Script_LinearB,
      Script_Ugaritic,
      Script_Shavian,
      Script_Osmanya,
      Script_Cypriot,
      Script_Braille,
      
      // Unicode 4.1 additions
      Script_Buginese,
      Script_NewTaiLue,
      Script_Glagolitic,
      Script_Tifinagh,
      Script_SylotiNagri,
      Script_OldPersian,
      Script_Kharoshthi,
      
      // Unicode 5.0 additions
      Script_Balinese,
      Script_Cuneiform,
      Script_Phoenician,
      Script_PhagsPa,
      Script_Nko,
      
      // Unicode 5.1 additions
      Script_Sundanese,
      Script_Lepcha,
      Script_OlChiki,
      Script_Vai,
      Script_Saurashtra,
      Script_KayahLi,
      Script_Rejang,
      Script_Lycian,
      Script_Carian,
      Script_Lydian,
      Script_Cham,
      
      // Unicode 5.2 additions
      Script_TaiTham,
      Script_TaiViet,
      Script_Avestan,
      Script_EgyptianHieroglyphs,
      Script_Samaritan,
      Script_Lisu,
      Script_Bamum,
      Script_Javanese,
      Script_MeeteiMayek,
      Script_ImperialAramaic,
      Script_OldSouthArabian,
      Script_InscriptionalParthian,
      Script_InscriptionalPahlavi,
      Script_OldTurkic,
      Script_Kaithi,
      
      // Unicode 6.0 additions
      Script_Batak,
      Script_Brahmi,
      Script_Mandaic,
      
      // Unicode 6.1 additions
      Script_Chakma,
      Script_MeroiticCursive,
      Script_MeroiticHieroglyphs,
      Script_Miao,
      Script_Sharada,
      Script_SoraSompeng,
      Script_Takri,
      
      // Unicode 7.0 additions
      Script_CaucasianAlbanian,
      Script_BassaVah,
      Script_Duployan,
      Script_Elbasan,
      Script_Grantha,
      Script_PahawhHmong,
      Script_Khojki,
      Script_LinearA,
      Script_Mahajani,
      Script_Manichaean,
      Script_MendeKikakui,
      Script_Modi,
      Script_Mro,
      Script_OldNorthArabian,
      Script_Nabataean,
      Script_Palmyrene,
      Script_PauCinHau,
      Script_OldPermic,
      Script_PsalterPahlavi,
      Script_Siddham,
      Script_Khudawadi,
      Script_Tirhuta,
      Script_WarangCiti,
      
      // Unicode 8.0 additions
      Script_Ahom,
      Script_AnatolianHieroglyphs,
      Script_Hatran,
      Script_Multani,
      Script_OldHungarian,
      Script_SignWriting,
      
      ScriptCount
   };
   
   enum class Direction
   {
      DirL, DirR, DirEN, DirES, DirET, DirAN, DirCS, DirB, DirS, DirWS, DirON,
      DirLRE, DirLRO, DirAL, DirRLE, DirRLO, DirPDF, DirNSM, DirBN,
      DirLRI, DirRLI, DirFSI, DirPDI
   };
   
   enum class Decomposition
   {
      NoDecomposition,
      Canonical,
      Font,
      NoBreak,
      Initial,
      Medial,
      Final,
      Isolated,
      Circle,
      Super,
      Sub,
      Vertical,
      Wide,
      Narrow,
      Small,
      Square,
      Compat,
      Fraction
   };
   
   enum class JoiningType 
   {
      Joining_None,
      Joining_Causing,
      Joining_Dual,
      Joining_Right,
      Joining_Left,
      Joining_Transparent
   };
   
   enum CombiningClass
   {
      Combining_BelowLeftAttached       = 200,
      Combining_BelowAttached           = 202,
      Combining_BelowRightAttached      = 204,
      Combining_LeftAttached            = 208,
      Combining_RightAttached           = 210,
      Combining_AboveLeftAttached       = 212,
      Combining_AboveAttached           = 214,
      Combining_AboveRightAttached      = 216,
      
      Combining_BelowLeft               = 218,
      Combining_Below                   = 220,
      Combining_BelowRight              = 222,
      Combining_Left                    = 224,
      Combining_Right                   = 226,
      Combining_AboveLeft               = 228,
      Combining_Above                   = 230,
      Combining_AboveRight              = 232,
      
      Combining_DoubleBelow             = 233,
      Combining_DoubleAbove             = 234,
      Combining_IotaSubscript           = 240
   };
   
   enum class UnicodeVersion
   {
      Unicode_Unassigned,
      Unicode_1_1,
      Unicode_2_0,
      Unicode_2_1_2,
      Unicode_3_0,
      Unicode_3_1,
      Unicode_3_2,
      Unicode_4_0,
      Unicode_4_1,
      Unicode_5_0,
      Unicode_5_1,
      Unicode_5_2,
      Unicode_6_0,
      Unicode_6_1,
      Unicode_6_2,
      Unicode_6_3,
      Unicode_7_0,
      Unicode_8_0
   };
public:
   constexpr Character() noexcept
      : m_data(0)
   {}
   
   constexpr Character(char16_t ch) noexcept
      : m_data(ch)
   {}
   
   constexpr Character(short ch) noexcept
      : m_data(char16_t(ch))
   {}
   
   constexpr Character(uchar c, uchar r) noexcept
      : m_data(char16_t((r << 8) | c))
   {}
   
   constexpr Character(char32_t rc) noexcept
      : m_data(char16_t(rc & 0xffff))
   {}
   
   constexpr Character(int rc) noexcept
      : m_data(char16_t(rc & 0xffff))
   {}
   
   constexpr Character(uint rc) noexcept
      : m_data(char16_t(rc & 0xffff))
   {}
   
   constexpr Character(SpecialCharacter ch) noexcept
      : m_data(char16_t(ch))
   {}
   
   constexpr Character(Latin1Character ch) noexcept
      : m_data(ch.unicode())
   {}
   
   constexpr explicit Character(char c) noexcept
      : m_data(char16_t(uchar(c)))
   {}
   constexpr explicit Character(uchar c) noexcept
      : m_data(char16_t(c))
   {}
   
#if defined(PDK_OS_WIN)
    PDK_STATIC_ASSERT(sizeof(wchar_t) == sizeof(ushort));
    constexpr Character(wchar_t ch) noexcept
       : m_data(ushort(ch)) {} // implicit
#endif
   
   inline Category getCategory() const noexcept
   {
      return Character::getCategory(m_data);
   }
   
   inline Direction getDirection() const noexcept
   {
      return Character::getDirection(m_data);
   }
   
   inline JoiningType getJoiningType() const noexcept
   {
      return Character::getJoiningType(m_data);
   }
   
   inline unsigned char getCombiningClass() const noexcept
   {
      return Character::getCombiningClass(m_data);
   }
   
   inline Character getMirroredCharacter() const noexcept
   {
      return Character::getMirroredCharacter(m_data);
   }
   
   inline bool hasMirrored() const noexcept
   {
      return Character::hasMirrored(m_data);
   }
   
   String getDecomposition() const;
   
   inline Decomposition getDecompositionTag() const noexcept
   {
      return Character::getDecompositionTag(m_data);
   }
   
   inline int getDigitValue() const noexcept
   {
      return Character::getDigitValue(m_data);
   }
   
   inline Character toLower() const noexcept
   {
      return Character::toLower(m_data);
   }
   
   inline Character toUpper() const noexcept
   {
      return Character::toUpper(m_data);
   }
   
   inline Character toTitleCase() const noexcept
   {
      return Character::toTitleCase(m_data);
   }
   
   inline Character toCaseFolded() const noexcept
   {
      return Character::toCaseFolded(m_data);
   }
   
   inline Script getScript() const noexcept
   {
      return Character::getScript(m_data);
   }
   
   inline UnicodeVersion getUnicodeVersion() const noexcept
   {
      return Character::getUnicodeVersion(m_data);
   }
   
   constexpr inline char toLatin1() const noexcept
   {
      return m_data > 0xff ? '\0' : char(m_data);
   }
   
   constexpr inline char16_t unicode() const noexcept
   {
      return m_data;
   }
   
   PDK_RELAXED_CONSTEXPR inline char16_t &unicode() noexcept
   {
      return m_data;
   }
   
   inline bool isPrintable() const noexcept
   {
      return Character::isPrintable(m_data);   
   }
   
   inline bool isMark() const noexcept
   {
      return Character::isMark(m_data);
   }
   
   inline bool isPunct() const noexcept
   {
      return Character::isPunct(m_data);
   }
   
   inline bool isSymbol() const noexcept
   {
      return Character::isSymbol(m_data);
   }
   
   constexpr inline bool isNull() const noexcept
   {
      return 0 == m_data;   
   }
   
   constexpr inline bool isSpace() const noexcept
   {
      return Character::isSpace(m_data);
   }
   
   constexpr inline bool isLetter() const noexcept
   {
      return Character::isLetter(m_data);
   }
   
   constexpr inline bool isNumber() const noexcept
   {
      return Character::isNumber(m_data);
   }
   
   constexpr inline bool isLetterOrNumber() const noexcept
   {
      return Character::isLetterOrNumber(m_data);   
   }
   
   constexpr inline bool isDigit() const noexcept
   {
      return Character::isDigit(m_data);
   }
   
   constexpr inline bool isLower() const noexcept
   {
      return Character::isLower(m_data);
   }
   
   constexpr inline bool isUpper() const noexcept
   {
      return Character::isUpper(m_data);
   }
   
   constexpr inline bool isTitleCase() const noexcept
   {
      return Character::isTitleCase(m_data);
   }
   
   constexpr inline bool isNonCharacter() const noexcept
   {
      return Character::isNonCharacter(m_data);
   }
   
   constexpr inline bool isHighSurrogate() const noexcept
   {
      return Character::isHighSurrogate(m_data);
   }
   
   constexpr inline bool isLowSurrogate() const noexcept
   {
      return Character::isLowSurrogate(m_data);
   }
   
   constexpr inline bool isSurrogate() const noexcept
   {
      return Character::isSurrogate(m_data);
   }
   
   constexpr inline uchar getCell() const noexcept
   {
      return uchar(m_data & 0xff);
   }
   
   constexpr inline uchar getRow() const noexcept
   {
      return uchar((m_data >> 8) & 0xff);
   }
   
   PDK_RELAXED_CONSTEXPR inline void setCell(uchar acell) noexcept
   {
      m_data = char16_t((m_data & 0xff00) + acell);
   }
   
   PDK_RELAXED_CONSTEXPR inline void setRow(uchar arow) noexcept
   {
      m_data = char16_t((m_data & 0xff) + (char16_t(arow) << 8));
   }
   
   static constexpr inline Character fromLatin1(char c) noexcept
   {
      return Character(char16_t(uchar(c)));
   }
   
   static constexpr inline bool isNonCharacter(char32_t ucs4) noexcept
   {
      return ucs4 >= 0xfdd0 && (ucs4 <= 0xfdef || (ucs4 & 0xfffe) == 0xfffe);
   }
   
   static constexpr inline bool isHighSurrogate(char32_t ucs4) noexcept
   {
      return ((ucs4 & PDK_SURROGATE_MASK) == PDK_MIN_HIGH_SURROGATE);
   }
   
   static constexpr inline bool isLowSurrogate(char32_t ucs4) noexcept
   {
      return ((ucs4 & PDK_SURROGATE_MASK) == PDK_MIN_LOW_SURROAGTE);
   }
   
   static constexpr inline bool isSurrogate(char32_t ucs4) noexcept
   {
      return (ucs4 - PDK_MIN_HIGH_SURROGATE < PDK_SURROGATE_RANGE);
   }
   
   static constexpr inline bool requiresSurrogates(char32_t ucs4) noexcept
   {
      return (ucs4 >= PDK_MIN_SUPPLEMENTARY_CODE_POINT);   
   }
   
   static constexpr inline char32_t surrogateToUcs4(char16_t high, char16_t low) noexcept
   {
      // Optimized form of:
      // return ((high - PDK_MIN_HIGH_SURROGATE) << 10)
      //         + (low - PDK_MIN_LOW_SURROGATE)
      //         + PDK_MIN_SUPPLEMENTARY_CODE_POINT;
      //((high << 10) + low) + (PDK_MIN_SUPPLEMENTARY_CODE_POINT
      //                        - (PDK_MIN_HIGH_SURROGATE << 10)
      //                        - PDK_MIN_LOW_SURROGATE);
      // (PDK_MIN_SUPPLEMENTARY_CODE_POINT - (PDK_MIN_HIGH_SURROGATE << 10) - PDK_MIN_LOW_SURROGATE) = 0x35fdc00u
      return (char32_t(high) << 10) + low - 0x35fdc00u;
   }
   
   static constexpr inline char32_t surrogateToUcs4(Character high, Character low) noexcept
   {
      return surrogateToUcs4(high.m_data, low.m_data);
   }
   
   static constexpr inline char16_t getHighSurrogate(char32_t ucs4) noexcept
   {
      // (((usc4 - PDK_MIN_SUPPLEMENTARY_CODE_POINT) >> 10) + PDK_MIN_HIGH_SURROGATE)
      // PDK_MIN_HIGH_SURROGATE - (PDK_MIN_SUPPLEMENTARY_CODE_POINT >> 10) = 0xd7c0u
      return char16_t((ucs4 >> 10)) + 0xd7c0u;
   }
   
   static constexpr inline char16_t getLowSurrogate(char32_t ucs4) noexcept
   {
      // equal
      // (((usc4 - PDK_MIN_SUPPLEMENTARY_CODE_POINT) >> 10) + PDK_MIN_LOW_SURROGATE)
      return char16_t(ucs4 % 0x400u + PDK_MIN_LOW_SURROAGTE);
   }
   
   static Category PDK_FASTCALL getCategory(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static Direction PDK_FASTCALL getDirection(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static JoiningType PDK_FASTCALL getJoiningType(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static unsigned char PDK_FASTCALL getCombiningClass(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static char32_t PDK_FASTCALL getMirroredCharacter(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static bool PDK_FASTCALL hasMirrored(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static String PDK_FASTCALL getDecomposition(char32_t ucs4);
   static Decomposition PDK_FASTCALL getDecompositionTag(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   
   static int PDK_FASTCALL getDigitValue(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static char32_t PDK_FASTCALL toLower(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static char32_t PDK_FASTCALL toUpper(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static char32_t PDK_FASTCALL toTitleCase(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static char32_t PDK_FASTCALL toCaseFolded(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   
   static Script PDK_FASTCALL getScript(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static UnicodeVersion PDK_FASTCALL getUnicodeVersion(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static UnicodeVersion PDK_FASTCALL getCurrentUnicodeVersion() noexcept PDK_DECL_CONST_FUNCTION;
   static bool PDK_FASTCALL isPrintable(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   
   static constexpr inline bool isSpace(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION
   {
      // note that [0x09..0x0d] + 0x85 are exceptional Cc-s and must be handled explicitly
      return ucs4 == 0x20 || (ucs4 >= 0x09 && ucs4 <= 0x0d)
            || (ucs4 > 127 && (ucs4 == 0x85 || ucs4 == 0xa0 || Character::isSpaceHelper(ucs4)));
   }
   
   static bool PDK_FASTCALL isMark(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static bool PDK_FASTCALL isPunct(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static bool PDK_FASTCALL isSymbol(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   
   static constexpr inline bool isLetter(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION
   {
      return (ucs4 >= 'A' && ucs4 <= 'z' && (ucs4 >= 'a' || ucs4 <= 'Z'))
            || (ucs4 > 127 && Character::isLetterHelper(ucs4));
   }
   
   static constexpr inline bool isNumber(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION
   {
      return (ucs4 >= '0' && ucs4 <= '9') || (ucs4 > 127 && Character::isNumberHelper(ucs4));
   }
   
   static constexpr inline bool isLetterOrNumber(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION
   {
      return (ucs4 >= 'A' && ucs4 <= 'z' && (ucs4 >= 'a' || ucs4 <= 'Z'))
            || (ucs4 >= '0' && ucs4 <= '9')
            || (ucs4 > 127 && Character::isLetterOrNumberHelper(ucs4));
   }
   
   static constexpr inline bool isDigit(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION
   {
      return (ucs4 >= '0' && ucs4 <= '9') || 
            (ucs4 > 127 && Character::getCategory(ucs4) == Category::Number_DecimalDigit);
   }
   
   static constexpr inline bool isLower(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION
   {
      return (ucs4 >= 'a' && ucs4 <= 'z') || 
            (ucs4 > 127 && Character::getCategory(ucs4) == Category::Letter_Lowercase);
   }
   
   static constexpr inline bool isUpper(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION
   {
      return (ucs4 >= 'A' && ucs4 <= 'Z') || 
            (ucs4 > 127 && Character::getCategory(ucs4) == Category::Letter_Uppercase);
   }
   
   static constexpr inline bool isTitleCase(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION
   {
      return ucs4 > 127 && Character::getCategory(ucs4) == Category::Letter_Titlecase;
   }
private:
   static bool PDK_FASTCALL isSpaceHelper(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static bool PDK_FASTCALL isLetterHelper(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static bool PDK_FASTCALL isNumberHelper(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   static bool PDK_FASTCALL isLetterOrNumberHelper(char32_t ucs4) noexcept PDK_DECL_CONST_FUNCTION;
   
   friend constexpr bool operator==(Character, Character) noexcept;
   friend constexpr bool operator< (Character, Character) noexcept;
private:
   char16_t m_data;
};

constexpr inline bool operator ==(Character lhs, Character rhs) noexcept
{
   return lhs.m_data == rhs.m_data;
}

constexpr inline bool operator <(Character lhs, Character rhs) noexcept
{
   return lhs.m_data < rhs.m_data;
}

constexpr inline bool operator !=(Character lhs, Character rhs) noexcept
{
   return !operator ==(lhs, rhs);
}

constexpr inline bool operator >=(Character lhs, Character rhs) noexcept
{
   return !operator <(lhs, rhs);
}

constexpr inline bool operator >(Character lhs, Character rhs) noexcept
{
   return operator <(rhs, lhs);
}

constexpr inline bool operator <=(Character lhs, Character rhs) noexcept
{
   return !operator >(lhs, rhs);
}

constexpr inline bool operator==(Character lhs, std::nullptr_t) noexcept
{
   return lhs.isNull();
}

constexpr inline bool operator< (Character,     std::nullptr_t) noexcept
{
   return false;
}

constexpr inline bool operator==(std::nullptr_t, Character rhs) noexcept
{
   return rhs.isNull();
}

constexpr inline bool operator< (std::nullptr_t, Character rhs) noexcept
{
   return !rhs.isNull();
}

constexpr inline bool operator!=(Character lhs, std::nullptr_t) noexcept
{
   return !operator==(lhs, nullptr);
}

constexpr inline bool operator>=(Character lhs, std::nullptr_t) noexcept
{
   return !operator< (lhs, nullptr);
}

constexpr inline bool operator> (Character lhs, std::nullptr_t) noexcept
{
   return operator< (nullptr, lhs);
}
constexpr inline bool operator<=(Character lhs, std::nullptr_t) noexcept
{
   return !operator< (nullptr, lhs);
}

constexpr inline bool operator!=(std::nullptr_t, Character rhs) noexcept
{
   return !operator==(nullptr, rhs);
}

constexpr inline bool operator>=(std::nullptr_t, Character rhs) noexcept
{
   return !operator< (nullptr, rhs);
}

constexpr inline bool operator> (std::nullptr_t, Character rhs) noexcept
{
   return  operator< (rhs, nullptr);
}

constexpr inline bool operator<=(std::nullptr_t, Character rhs) noexcept
{
   return !operator< (rhs, nullptr);
}

namespace internal {
// constants for Hangul (de)composition, see UAX #15
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
} // internal

} // lang
} // pdk

PDK_DECLARE_TYPEINFO(pdk::lang::Character, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_LANG_CHARACTER_H
