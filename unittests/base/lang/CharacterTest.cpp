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
// Created by zzu_softboy on 2017/11/15.

#include "gtest/gtest.h"
#include "pdk/base/lang/Character.h"
#include <list>
#include <utility>

using pdk::lang::Character;
using pdk::lang::Latin1Character;

TEST(CharacterTest, testEqualInt)
{
   {
      const Character ch = Latin1Character(' ');
      ASSERT_TRUE(ch != 0);
      ASSERT_TRUE(!(ch == 0));
      ASSERT_TRUE(ch == 0x20);
      ASSERT_TRUE(!(ch != 0x20));
      ASSERT_TRUE(0x20 == ch);
      ASSERT_TRUE(!(0x20 != ch));
   }
   {
      const Character ch = Latin1Character('\0');
      ASSERT_TRUE(ch == 0);
      ASSERT_TRUE(!(ch != 0));
      ASSERT_TRUE(ch != 0x20);
      ASSERT_TRUE(!(ch == 0x20));
      ASSERT_TRUE(0x20 != ch);
      ASSERT_TRUE(!(0x20 == ch));
   }
}

TEST(CharacterTest, testOperators)
{
   std::list<Character> leftItems;
   std::list<Character> rightItems;
   for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
         leftItems.push_back(Character(char16_t(i)));
         rightItems.push_back(Character(char16_t(j)));
      }
   }
   std::list<Character>::iterator lbegin = leftItems.begin();
   std::list<Character>::iterator lend = leftItems.end();
   std::list<Character>::iterator rbegin = rightItems.begin();
   while(lbegin != lend) {
      ASSERT_EQ(*lbegin == *rbegin, lbegin->unicode() == rbegin->unicode());
      ASSERT_EQ(*lbegin != *rbegin, lbegin->unicode() != rbegin->unicode());
      ASSERT_EQ(*lbegin < *rbegin, lbegin->unicode() < rbegin->unicode());
      ASSERT_EQ(*lbegin > *rbegin, lbegin->unicode() > rbegin->unicode());
      ASSERT_EQ(*lbegin <= *rbegin, lbegin->unicode() <= rbegin->unicode());
      ASSERT_EQ(*lbegin >= *rbegin, lbegin->unicode() >= rbegin->unicode());
      ++lbegin;
      ++rbegin;
   }
}

TEST(CharacterTest, testToUpper)
{
   ASSERT_EQ(Character('a').toUpper(), (char16_t)'A');
   ASSERT_EQ(Character('A').toUpper(), (char16_t)'A');
   ASSERT_EQ(Character(0x1c7).toUpper().unicode(), (char16_t)0x1c7);
   ASSERT_EQ(Character(0x1c8).toUpper().unicode(), (char16_t)0x1c7);
   ASSERT_EQ(Character(0x1c9).toUpper().unicode(), (char16_t)0x1c7);
   ASSERT_EQ(Character(0x25c).toUpper().unicode(), (char16_t)0xa7ab);
   ASSERT_EQ(Character(0x29e).toUpper().unicode(), (char16_t)0xa7b0);
   ASSERT_EQ(Character(0x1d79).toUpper().unicode(), (char16_t)0xa77d);
   ASSERT_EQ(Character(0x0265).toUpper().unicode(), (char16_t)0xa78d);
   
   ASSERT_EQ(Character::toUpper('a'), (char16_t)'A');
   ASSERT_EQ(Character::toUpper('A'), (char16_t)'A');
   ASSERT_EQ(Character::toUpper(0x1c7), (char16_t)0x1c7);
   ASSERT_EQ(Character::toUpper(0x1c8), (char16_t)0x1c7);
   ASSERT_EQ(Character::toUpper(0x1c9), (char16_t)0x1c7);
   ASSERT_EQ(Character::toUpper(0x25c), (char16_t)0xa7ab);
   ASSERT_EQ(Character::toUpper(0x29e), (char16_t)0xa7b0);
   ASSERT_EQ(Character::toUpper(0x1d79),(char16_t) 0xa77d);
   ASSERT_EQ(Character::toUpper(0x0265), (char16_t)0xa78d);
   
   ASSERT_EQ(Character::toUpper(0x10400),(char32_t) 0x10400);
   ASSERT_EQ(Character::toUpper(0x10428), (char32_t)0x10400);
}

TEST(CharacterTest, testToLower)
{
   ASSERT_EQ(Character('A').toLower(), (char16_t)'a');
   ASSERT_EQ(Character('a').toLower(), (char16_t)'a');
   ASSERT_EQ(Character(0x1c7).toLower().unicode(), (char16_t)0x1c9);
   ASSERT_EQ(Character(0x1c8).toLower().unicode(), (char16_t)0x1c9);
   ASSERT_EQ(Character(0x1c9).toLower().unicode(), (char16_t)0x1c9);
   ASSERT_EQ(Character(0xa77d).toLower().unicode(), (char16_t)0x1d79);
   ASSERT_EQ(Character(0xa78d).toLower().unicode(), (char16_t)0x0265);
   ASSERT_EQ(Character(0xa7ab).toLower().unicode(), (char16_t)0x25c);
   ASSERT_EQ(Character(0xa7b1).toLower().unicode(), (char16_t)0x287);
   
   ASSERT_EQ(Character::toLower('a'), (char16_t)'a');
   ASSERT_EQ(Character::toLower('A'), (char16_t)'a');
   ASSERT_EQ(Character::toLower(0x1c7), (char16_t)0x1c9);
   ASSERT_EQ(Character::toLower(0x1c8), (char16_t)0x1c9);
   ASSERT_EQ(Character::toLower(0x1c9), (char16_t)0x1c9);
   ASSERT_EQ(Character::toLower(0xa77d), (char16_t)0x1d79);
   ASSERT_EQ(Character::toLower(0xa78d), (char16_t)0x0265);
   ASSERT_EQ(Character::toLower(0xa7ab),(char16_t) 0x25c);
   ASSERT_EQ(Character::toLower(0xa7b1), (char16_t)0x287);
   
   ASSERT_EQ(Character::toLower(0x10400),(char32_t) 0x10428);
   ASSERT_EQ(Character::toLower(0x10428), (char32_t)0x10428);
}

TEST(CharacterTest, toTitle)
{
   ASSERT_EQ(Character('a').toTitleCase(), (char16_t)'A');
   ASSERT_EQ(Character('A').toTitleCase(), (char16_t)'A');
   ASSERT_EQ(Character(0x1c7).toTitleCase().unicode(), (char16_t)0x1c8);
   ASSERT_EQ(Character(0x1c8).toTitleCase().unicode(), (char16_t)0x1c8);
   ASSERT_EQ(Character(0x1c9).toTitleCase().unicode(), (char16_t)0x1c8);
   ASSERT_EQ(Character(0x25c).toTitleCase().unicode(), (char16_t)0xa7ab);
   ASSERT_EQ(Character(0x29e).toTitleCase().unicode(), (char16_t)0xa7b0);
   ASSERT_EQ(Character(0x1d79).toTitleCase().unicode(), (char16_t)0xa77d);
   ASSERT_EQ(Character(0x0265).toTitleCase().unicode(), (char16_t)0xa78d);
   
   ASSERT_EQ(Character::toTitleCase('a'), (char16_t)'A');
   ASSERT_EQ(Character::toTitleCase('A'), (char16_t)'A');
   ASSERT_EQ(Character::toTitleCase(0x1c7), (char16_t)0x1c8);
   ASSERT_EQ(Character::toTitleCase(0x1c8), (char16_t)0x1c8);
   ASSERT_EQ(Character::toTitleCase(0x1c9), (char16_t)0x1c8);
   ASSERT_EQ(Character::toTitleCase(0x25c), (char16_t)0xa7ab);
   ASSERT_EQ(Character::toTitleCase(0x29e), (char16_t)0xa7b0);
   ASSERT_EQ(Character::toTitleCase(0x1d79),(char16_t) 0xa77d);
   ASSERT_EQ(Character::toTitleCase(0x0265), (char16_t)0xa78d);
   
   ASSERT_EQ(Character::toTitleCase(0x10400),(char32_t) 0x10400);
   ASSERT_EQ(Character::toTitleCase(0x10428), (char32_t)0x10400);
}

TEST(CharacterTest, toCaseFolded)
{
   ASSERT_EQ(Character('a').toCaseFolded(), (char16_t)'a');
   ASSERT_EQ(Character('A').toCaseFolded(), (char16_t)'a');
   ASSERT_EQ(Character(0x1c7).toCaseFolded().unicode(), (char16_t)0x1c9);
   ASSERT_EQ(Character(0x1c8).toCaseFolded().unicode(), (char16_t)0x1c9);
   ASSERT_EQ(Character(0x1c9).toCaseFolded().unicode(), (char16_t)0x1c9);
   ASSERT_EQ(Character(0xa77d).toCaseFolded().unicode(), (char16_t)0x1d79);
   ASSERT_EQ(Character(0xa78d).toCaseFolded().unicode(), (char16_t)0x0265);
   ASSERT_EQ(Character(0xa7ab).toCaseFolded().unicode(), (char16_t)0x25c);
   ASSERT_EQ(Character(0xa7b1).toCaseFolded().unicode(), (char16_t)0x287);
   
   ASSERT_EQ(Character::toCaseFolded('a'), (char16_t)'a');
   ASSERT_EQ(Character::toCaseFolded('A'), (char16_t)'a');
   ASSERT_EQ(Character::toCaseFolded(0x1c7), (char16_t)0x1c9);
   ASSERT_EQ(Character::toCaseFolded(0x1c8), (char16_t)0x1c9);
   ASSERT_EQ(Character::toCaseFolded(0x1c9), (char16_t)0x1c9);
   ASSERT_EQ(Character::toCaseFolded(0xa77d), (char16_t)0x1d79);
   ASSERT_EQ(Character::toCaseFolded(0xa78d), (char16_t)0x0265);
   ASSERT_EQ(Character::toCaseFolded(0xa7ab),(char16_t) 0x25c);
   ASSERT_EQ(Character::toCaseFolded(0xa7b1), (char16_t)0x287);
   
   ASSERT_EQ(Character::toCaseFolded(0x10400),(char32_t) 0x10428);
   ASSERT_EQ(Character::toCaseFolded(0x10428), (char32_t)0x10428);
   ASSERT_EQ(Character::toCaseFolded(0xb5), (char32_t)0x3bc);
}

TEST(CharacterTest, testIsDigit)
{
   std::list<std::pair<Character, bool>> data;
   for(char16_t ucs = 0; ucs < 256; ++ucs)
   {
      bool isDigit = (ucs >= '0' && ucs <= '9');
      data.push_back(std::make_pair(Character(ucs), isDigit));
   }
   auto begin = data.begin();
   auto end = data.end();
   while (begin != end) {
      ASSERT_EQ((*begin).first.isDigit(), (*begin).second);
      ++begin;
   }
}

namespace
{

bool is_expected_letter(char16_t ucs)
{
   return (ucs >= 'a' && ucs <= 'z') || (ucs >= 'A' && ucs <= 'Z') ||
         ucs == 0xAA || ucs == 0xB5 || ucs == 0xBA ||
         (ucs >= 0xC0 && ucs <= 0xD6) ||
         (ucs >= 0xD8 && ucs <= 0xF6) ||
         (ucs >= 0xF8 && ucs <= 0xFF);
}

}

TEST(CharacterTest, testIsLetter)
{
   std::list<std::pair<Character, bool>> data;
   for(char16_t ucs = 0; ucs < 256; ++ucs)
   {
      data.push_back(std::make_pair(Character(ucs), is_expected_letter(ucs)));
   }
   auto begin = data.begin();
   auto end = data.end();
   while (begin != end) {
      ASSERT_EQ((*begin).first.isLetter(), (*begin).second);
      ++begin;
   }
}

TEST(CharacterTest, testIsLetterOrNumber)
{
   std::list<std::pair<Character, bool>> data;
   for(char16_t ucs = 0; ucs < 256; ++ucs)
   {
      bool isLetterOrNumber = is_expected_letter(ucs) ||
            (ucs >= '0' && ucs <= '9') ||
            ucs == 0xB2 || ucs == 0xB3 || ucs == 0xB9 ||
            (ucs >= 0xBC && ucs <= 0xBE);
      data.push_back(std::make_pair(Character(ucs), isLetterOrNumber));
   }
   auto begin = data.begin();
   auto end = data.end();
   while (begin != end) {
      ASSERT_EQ((*begin).first.isLetterOrNumber(), (*begin).second);
      ++begin;
   }
}

TEST(CharacterTest, testIsPrintable)
{
   // noncharacters, reserved (General_Gategory =Cn)
   ASSERT_FALSE(Character(0x2064).isPrintable());
   ASSERT_FALSE(Character(0x2069).isPrintable());
   ASSERT_FALSE(Character(0xfdd0).isPrintable());
   ASSERT_FALSE(Character(0xfdef).isPrintable());
   ASSERT_FALSE(Character(0xfff0).isPrintable());
   ASSERT_FALSE(Character(0xfff8).isPrintable());
   ASSERT_FALSE(Character(0xfffe).isPrintable());
   ASSERT_FALSE(Character(0xffff).isPrintable());
   
   ASSERT_FALSE(Character::isPrintable(0xe0000));
   ASSERT_FALSE(Character::isPrintable(0xe0002));
   ASSERT_FALSE(Character::isPrintable(0xe001f));
   ASSERT_FALSE(Character::isPrintable(0xe0080));
   ASSERT_FALSE(Character::isPrintable(0xe00ff));
   
   // Other_Default_Ignorable_Code_Point, Variation_Selector
   ASSERT_TRUE(Character(0x034f).isPrintable());
   ASSERT_TRUE(Character(0x115f).isPrintable());
   ASSERT_TRUE(Character(0x180b).isPrintable());
   ASSERT_TRUE(Character(0x180d).isPrintable());
   ASSERT_TRUE(Character(0x3164).isPrintable());
   ASSERT_TRUE(Character(0xfe00).isPrintable());
   ASSERT_TRUE(Character(0xfe0f).isPrintable());
   ASSERT_TRUE(Character(0xffa0).isPrintable());
   
   ASSERT_TRUE(Character::isPrintable(0xe0100));
   ASSERT_TRUE(Character::isPrintable(0xe01ef));
   
   // Cf, Cs, Cc, White_Space, Annotation Characters
   ASSERT_TRUE(!Character(0x0008).isPrintable());
   ASSERT_TRUE(!Character(0x000a).isPrintable());
   ASSERT_TRUE(Character(0x0020).isPrintable());
   ASSERT_TRUE(Character(0x00a0).isPrintable());
   ASSERT_TRUE(!Character(0x00ad).isPrintable());
   ASSERT_TRUE(!Character(0x0085).isPrintable());
   ASSERT_TRUE(!Character(0xd800).isPrintable());
   ASSERT_TRUE(!Character(0xdc00).isPrintable());
   ASSERT_TRUE(!Character(0xfeff).isPrintable());
   ASSERT_TRUE(!Character::isPrintable(0x1d173));
   
   ASSERT_TRUE(Character('0').isPrintable());
   ASSERT_TRUE(Character('A').isPrintable());
   ASSERT_TRUE(Character('a').isPrintable());
   
   ASSERT_TRUE(Character(0x0370).isPrintable());// assigned in 5.1
   ASSERT_TRUE(Character(0x0524).isPrintable());// assigned in 5.2
   ASSERT_TRUE(Character(0x0526).isPrintable());// assigned in 6.0
   ASSERT_TRUE(Character(0x08a0).isPrintable());// assigned in 6.1
   ASSERT_TRUE(!Character(0x1aff).isPrintable());// not assigned
   ASSERT_TRUE(Character(0x1e9e).isPrintable());// assigned in 5.1
   
   ASSERT_TRUE(Character::isPrintable(0x1b000));// assigned in 6.0
   ASSERT_TRUE(Character::isPrintable(0x110d0));// assigned in 5.1
   ASSERT_TRUE(!Character::isPrintable(0x1bca0));// assigned in 7.0
}

TEST(CharacterTest, testIsUpper)
{
   ASSERT_TRUE(Character('A').isUpper());
   ASSERT_TRUE(Character('Z').isUpper());
   ASSERT_TRUE(!Character('a').isUpper());
   ASSERT_TRUE(!Character('z').isUpper());
   ASSERT_TRUE(!Character('?').isUpper());
   
   ASSERT_TRUE(Character(0xC2).isUpper()); // A with ^
   ASSERT_TRUE(!Character(0xE2).isUpper()); // a with ^
   
   for (char32_t codepoint = 0; codepoint <= Character::LastValidCodePoint; ++codepoint) {
      if (Character::isUpper(codepoint)) {
         ASSERT_EQ(codepoint, Character::toUpper(codepoint));
      }
   }
}

TEST(CharacterTest, testIsLower)
{
   ASSERT_TRUE(!Character('A').isLower());
   ASSERT_TRUE(!Character('Z').isLower());
   ASSERT_TRUE(Character('a').isLower());
   ASSERT_TRUE(Character('z').isLower());
   ASSERT_TRUE(!Character('?').isLower());
   
   ASSERT_TRUE(!Character(0xC2).isLower()); // A with ^
   ASSERT_TRUE(Character(0xE2).isLower()); // a with ^
   
   for (char32_t codepoint = 0; codepoint <= Character::LastValidCodePoint; ++codepoint) {
      if (Character::isLower(codepoint)) {
         ASSERT_EQ(codepoint, Character::toLower(codepoint));
      }
   }
}

TEST(CharacterTest, testTitleCase)
{
   for (char32_t codepoint = 0; codepoint <= Character::LastValidCodePoint; ++codepoint) {
      if (Character::isTitleCase(codepoint)) {
         ASSERT_EQ(codepoint, Character::toTitleCase(codepoint));
      }
   }
}

TEST(CharacterTest, testIsSpace)
{
   std::list<std::pair<Character, bool>> data;
   for(char16_t ucs = 0; ucs < 256; ++ucs)
   {
      bool isSpace = (ucs <= 0x0D && ucs >= 0x09) || ucs == 0x20 || ucs == 0xA0 || ucs == 0x85;
      data.push_back(std::make_pair(Character(ucs), isSpace));
   }
   auto begin = data.begin();
   auto end = data.end();
   while (begin != end) {
      ASSERT_EQ((*begin).first.isSpace(), (*begin).second);
      ++begin;
   }
}

TEST(CharacterTest, testIsSpaceSpecial)
{
   ASSERT_TRUE(!Character(Character::Null).isSpace());
   ASSERT_TRUE(Character(Character::Nbsp).isSpace());
   ASSERT_TRUE(Character(Character::ParagraphSeparator).isSpace());
   ASSERT_TRUE(Character(Character::LineSeparator).isSpace());
   ASSERT_TRUE(Character(Character::LineFeed).isSpace());
   ASSERT_TRUE(Character(0x1680).isSpace());
}

TEST(CharacterTest, testGetCategory)
{
   ASSERT_EQ(Character('a').getCategory(), Character::Category::Letter_Lowercase);
   ASSERT_EQ(Character('A').getCategory(), Character::Category::Letter_Uppercase);
   
   ASSERT_EQ(Character::getCategory('a'), Character::Category::Letter_Lowercase);
   ASSERT_EQ(Character::getCategory('A'), Character::Category::Letter_Uppercase);
   
   ASSERT_EQ(Character::getCategory(0xe0100), Character::Category::Mark_NonSpacing);
   ASSERT_NE(Character::getCategory(0xeffff), Character::Category::Other_PrivateUse);
   ASSERT_EQ(Character::getCategory(0xf0000), Character::Category::Other_PrivateUse);
   ASSERT_EQ(Character::getCategory(0xf0001), Character::Category::Other_PrivateUse);
   
   ASSERT_EQ(Character::getCategory(0xd900), Character::Category::Other_Surrogate);
   ASSERT_EQ(Character::getCategory(0xdc00), Character::Category::Other_Surrogate);
   ASSERT_EQ(Character::getCategory(0xdc01), Character::Category::Other_Surrogate);
   
   ASSERT_EQ(Character::getCategory(0x1aff), Character::Category::Other_NotAssigned);
   ASSERT_EQ(Character::getCategory(0x10fffd), Character::Category::Other_PrivateUse);
   ASSERT_EQ(Character::getCategory(0x10ffff), Character::Category::Other_NotAssigned);
   ASSERT_EQ(Character::getCategory(0x110000), Character::Category::Other_NotAssigned);
}

TEST(CharacterTest, testGetDirection)
{
   ASSERT_EQ(Character::getDirection(0x200E), Character::Direction::DirL);
   ASSERT_EQ(Character::getDirection(0x200F), Character::Direction::DirR);
   ASSERT_EQ(Character::getDirection(0x202A), Character::Direction::DirLRE);
   ASSERT_EQ(Character::getDirection(0x202B), Character::Direction::DirRLE);
   ASSERT_EQ(Character::getDirection(0x202C), Character::Direction::DirPDF);
   ASSERT_EQ(Character::getDirection(0x202D), Character::Direction::DirLRO);
   ASSERT_EQ(Character::getDirection(0x202E), Character::Direction::DirRLO);
   ASSERT_EQ(Character::getDirection(0x2066), Character::Direction::DirLRI);
   ASSERT_EQ(Character::getDirection(0x2067), Character::Direction::DirRLI);
   ASSERT_EQ(Character::getDirection(0x2068), Character::Direction::DirFSI);
   ASSERT_EQ(Character::getDirection(0x2069), Character::Direction::DirPDI);
   
   ASSERT_EQ(Character('a').getDirection(), Character::Direction::DirL);
   ASSERT_EQ(Character('0').getDirection(), Character::Direction::DirEN);
   ASSERT_EQ(Character(0x627).getDirection(), Character::Direction::DirAL);
   ASSERT_EQ(Character(0x5d0).getDirection(), Character::Direction::DirR);
   
   ASSERT_EQ(Character::getDirection('a'), Character::Direction::DirL);
   ASSERT_EQ(Character::getDirection('0'), Character::Direction::DirEN);
   ASSERT_EQ(Character::getDirection(0x627), Character::Direction::DirAL);
   ASSERT_EQ(Character::getDirection(0x5d0), Character::Direction::DirR);
   
   ASSERT_EQ(Character::getDirection(0xE01DA), Character::Direction::DirNSM);
   ASSERT_EQ(Character::getDirection(0xf0000), Character::Direction::DirL);
   ASSERT_EQ(Character::getDirection(0xE0030), Character::Direction::DirBN);
   ASSERT_EQ(Character::getDirection(0x2FA17), Character::Direction::DirL);
}
