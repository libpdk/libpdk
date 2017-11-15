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
