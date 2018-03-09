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
// Created by zzu_softboy on 2017/12/23.

#include "gtest/gtest.h"
//#include "pdk/utils/internal/LocaleToolsPrivate.h"
//#include "pdk/base/io/DataStream.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/base/io/fs/Resource.h"
#include "pdk/base/io/fs/FileInfo.h"
//#include "pdk/global/Random.h"
//#include "pdk/base/text/codecs/internal/SimpleCodecPrivate.h"
//#include "pdk/base/text/codecs/internal/JisCodecPrivate.h"
//#include "pdk/base/text/codecs/internal/SjisCodecPrivate.h"
//#include "pdk/base/text/codecs/internal/Big5CodecPrivate.h"
//#include "pdk/base/text/codecs/internal/EucjpCodecPrivate.h"
//#include "pdk/base/text/codecs/internal/EuckrCodecPrivate.h"
//#include "pdk/base/text/codecs/internal/Gb18030CodecPrivate.h"
//#include "pdk/base/text/codecs/internal/WindowsCodecPrivate.h"
//#include "pdk/base/text/codecs/internal/UtfCodecPrivate.h"
#include "pdk/base/ds/ByteArrayMatcher.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include "pdk/base/lang/Character.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/kernel/StringUtils.h"
#include <list>
#include <utility>
#include <string>
#include <algorithm>
#include <type_traits>
#include <tuple>

using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::lang::Character;
using pdk::lang::Latin1String;
using pdk::ds::ByteArray;

namespace
{

template <typename T>
class Arg;

template <typename T>
class Reversed
{};

class ArgBase
{
protected:
   String m_pinned;
   explicit ArgBase(const char *str)
      : m_pinned(String::fromLatin1(str))
   {}
};

template <>
class Arg<Character> : public ArgBase
{
public:
   explicit Arg(const char *str) : ArgBase(str)
   {}
   
   template <typename MemFunc>
   void apply0(String &s, MemFunc mf) const
   {
      for (Character ch : pdk::as_const(this->m_pinned)) {
         (s.*mf)(ch);
      }
   }
   
   template <typename MemFunc, typename Arg>
   void apply1(String &s, MemFunc mf, Arg arg) const
   {
      for (Character ch : pdk::as_const(this->m_pinned)) {
         (s.*mf)(arg, ch);
      }
   }
};

template <>
class Arg<Reversed<Character>> : public Arg<Character>
{
public:
   explicit Arg(const char *str) : Arg<Character>(str)
   {
      std::reverse(this->m_pinned.begin(), this->m_pinned.end());
   }
   using Arg<Character>::apply0;
   using Arg<Character>::apply1;
};

template <>
class Arg<String> : ArgBase
{
public:
   explicit Arg(const char *str) : ArgBase(str)
   {
      
   }
   
   template <typename MemFun>
   void apply0(String &str, MemFun mf) const
   {
      return (str.*mf)(this->m_pinned);
   }
   
   template <typename MemFun, typename ArgType>
   void apply1(String &str, MemFun mf, ArgType arg) const
   {
      return (str.*mf)(arg, this->m_pinned);
   }
};

template <>
class Arg<StringRef> : ArgBase
{
public:
   explicit Arg(const char *str) : ArgBase(str)
   {
      
   }
   
   template <typename MemFun>
   void apply0(String &str, MemFun mf) const
   {
      (str.*mf)(ref());
   }
   
   template <typename MemFun, typename ArgType>
   void apply1(String &str, MemFun mf, Arg arg) const
   {
      (str.*mf)(arg, ref());
   }
private:
   StringRef ref() const
   {
      return StringRef(&m_pinned);
   }
};

template <>
class Arg<std::pair<const Character *, int>> : ArgBase
{
public:
   explicit Arg(const char *str) : ArgBase(str)
   {}
   
   template <typename MemFun>
   void apply0(String &str, MemFun mf) const
   {
      (str.*mf)(this->m_pinned.getConstRawData(), this->m_pinned.length());
   }
   
   template <typename MemFun, typename Arg>
   void apply1(String &str, MemFun mf, Arg arg) const
   {
      (str.*mf)(arg, this->m_pinned.getConstRawData(), this->m_pinned.length());
   }
};

template <>
class Arg<Latin1String>
{
public:
   explicit Arg(const char *str)
      : m_str(str)
   {}
   
   template <typename MemFun>
   void apply0(String &str, MemFun mf) const
   {
      (str.*mf)(m_str);
   }
   
   template <typename MemFun, typename Arg>
   void apply1(String &str, MemFun mf, Arg arg) const
   {
      (str.*mf)(arg, m_str);
   }
   
private:
   Latin1String m_str;
};

template<>
class Arg<char>
{
protected:
   const char *m_str;
public:
   explicit Arg(const char *str)
      : m_str(str)
   {
      
   }
   
   template <typename MemFun>
   void apply0(String &str, MemFun mf)
   {
      if (m_str) {
         for (const char *it = m_str; *it; ++it) {
            (str.*mf)(*it);
         }
      }
   }
   
   template <typename MemFun, typename Arg>
   void apply1(String &str, MemFun mf, Arg arg)
   {
      if (m_str) {
         for (const char *it = m_str; *it; ++it) {
            (str.*mf)(arg ,*it);
         }
      }
   }
};

template <>
class Arg<Reversed<char>> : public Arg<char>
{
   static const char *dupAndReverse(const char *str)
   {
      char *s2 = pdk::strdup(str);
      std::reverse(s2, s2 + pdk::strlen(s2));
      return s2;
   }
   
public:
   explicit Arg(const char *str)
      : Arg<char>(dupAndReverse(str))
   {}
   ~Arg()
   {
      delete[] m_str;
   }
   
   using Arg<char>::apply0;
   using Arg<char>::apply1;
};

template<>
class Arg<const char *>
{
protected:
   const char *m_str;
public:
   explicit Arg(const char *str)
      : m_str(str)
   {
      
   }
   
   template <typename MemFun>
   void apply0(String &str, MemFun mf)
   {
      (str.*mf)(str);
   }
   
   template <typename MemFun, typename Arg>
   void apply1(String &str, MemFun mf, Arg arg)
   {
      (str.*mf)(arg, str);
   }
};

template <>
class Arg<ByteArray>
{
   ByteArray m_array;
public:
   explicit Arg(const char *str)
      : m_array(str)
   {
      
   }
   
   template <typename MemFun>
   void apply0(String &str, MemFun mf) const
   {
      (str.*mf)(m_array);
   }
   
   template <typename MemFun, typename Arg>
   void apply1(String &str, MemFun mf, Arg arg) const
   {
      (str.*mf)(arg, m_array);
   }
};

void length_data(std::list<std::tuple<String, int>> &data)
{
   data.push_back(std::make_tuple(String(Latin1String("Test")), 4));
   data.push_back(std::make_tuple(String(Latin1String("The quick brown fox jumps over the lazy dog")), 43));
   data.push_back(std::make_tuple(String(), 0));
   data.push_back(std::make_tuple(String(Latin1String("A")), 1));
   data.push_back(std::make_tuple(String(Latin1String("AB")), 2));
   data.push_back(std::make_tuple(String(Latin1String("AB\n")), 3));
   data.push_back(std::make_tuple(String(Latin1String("AB\nC")), 4));
   data.push_back(std::make_tuple(String(Latin1String("\n")), 1));
   data.push_back(std::make_tuple(String(Latin1String("\nA")), 2));
   data.push_back(std::make_tuple(String(Latin1String("\nAB")), 3));
   data.push_back(std::make_tuple(String(Latin1String("\nAB\nCDE")), 7));
   data.push_back(std::make_tuple(String(Latin1String("shdnftrheid fhgnt gjvnfmd chfugkh bnfhg thgjf vnghturkf chfnguh bjgnfhvygh hnbhgutjfv dhdnjds dcjs d")), 100));
}

void replace_character_character_data(std::list<std::tuple<String, Character, Character, int, String>> &data)
{
   data.push_back(std::make_tuple(String(Latin1String("foo")), Character('o'), Character('a'), 
                                  int(pdk::CaseSensitivity::Sensitive), String(Latin1String("faa"))));
   
   data.push_back(std::make_tuple(String(Latin1String("foo")), Character('o'), Character('a'), 
                                  int(pdk::CaseSensitivity::Insensitive), String(Latin1String("faa"))));
   
   data.push_back(std::make_tuple(String(Latin1String("foo")), Character('O'), Character('a'), 
                                  int(pdk::CaseSensitivity::Sensitive), String(Latin1String("foo"))));
   
   data.push_back(std::make_tuple(String(Latin1String("foo")), Character('O'), Character('a'), 
                                  int(pdk::CaseSensitivity::Insensitive), String(Latin1String("faa"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ababABAB")), Character('a'), Character(' '), 
                                  int(pdk::CaseSensitivity::Sensitive), String(Latin1String(" b bABAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ababABAB")), Character('a'), Character(' '), 
                                  int(pdk::CaseSensitivity::Insensitive), String(Latin1String(" b b B B"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ababABAB")), Character(), Character(' '), 
                                  int(pdk::CaseSensitivity::Insensitive), String(Latin1String("ababABAB"))));
}

void replace_character_string_data(std::list<std::tuple<String, Character, String, int, String>> &data)
{
   data.push_back(std::make_tuple(String(Latin1String("foo")), Character('o'), String(Latin1String("aA")), 
                                  int(pdk::CaseSensitivity::Sensitive), String(Latin1String("faAaA"))));
   
   data.push_back(std::make_tuple(String(Latin1String("foo")), Character('o'), String(Latin1String("aA")), 
                                  int(pdk::CaseSensitivity::Insensitive), String(Latin1String("faAaA"))));
   
   data.push_back(std::make_tuple(String(Latin1String("foo")), Character('O'), String(Latin1String("aA")), 
                                  int(pdk::CaseSensitivity::Sensitive), String(Latin1String("foo"))));
   
   data.push_back(std::make_tuple(String(Latin1String("foo")), Character('O'), String(Latin1String("aA")), 
                                  int(pdk::CaseSensitivity::Insensitive), String(Latin1String("faAaA"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ababABAB")), Character('a'), String(Latin1String("  ")), 
                                  int(pdk::CaseSensitivity::Sensitive), String(Latin1String("  b  bABAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ababABAB")), Character('a'), String(Latin1String("  ")), 
                                  int(pdk::CaseSensitivity::Insensitive), String(Latin1String("  b  b  B  B"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ababABAB")), Character(), String(Latin1String("  ")), 
                                  int(pdk::CaseSensitivity::Insensitive), String(Latin1String("ababABAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ababABAB")), Character(), String(Latin1String()), 
                                  int(pdk::CaseSensitivity::Insensitive), String(Latin1String("ababABAB"))));
}

void replace_uint_uint_data(std::list<std::tuple<String, int, int, String, String>> &data)
{
   data.push_back(std::make_tuple(String(Latin1String("-<>ABCABCABCABC>")), 0, 3, String(Latin1String("")), 
                                  String(Latin1String("ABCABCABCABC>"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ABCABCABCABC")), 1, 4, String(Latin1String("")), 
                                  String(Latin1String("ACABCABC"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCABC")), 8, 4, String(Latin1String("")), 
                                  String(Latin1String("ACABCABC"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCABC")), 7, 1, String(Latin1String("")), 
                                  String(Latin1String("ACABCAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 4, 0, String(Latin1String("")), 
                                  String(Latin1String("ACABCAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 4, 0, String(Latin1String("X")), 
                                  String(Latin1String("ACABXCAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABXCAB")), 4, 1, String(Latin1String("Y")), 
                                  String(Latin1String("ACABYCAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABXCAB")), 4, 1, String(Latin1String("")), 
                                  String(Latin1String("ACABCAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABXCAB")), 0, 9999, String(Latin1String("XX")), 
                                  String(Latin1String("XX"))));
   
   data.push_back(std::make_tuple(String(Latin1String("XX")), 0, 9999, String(Latin1String("")), 
                                  String(Latin1String(""))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 0, 2, String(Latin1String("XX")), 
                                  String(Latin1String("XXABCAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 1, 2, String(Latin1String("XX")), 
                                  String(Latin1String("AXXBCAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 2, 2, String(Latin1String("XX")), 
                                  String(Latin1String("ACXXCAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 3, 2, String(Latin1String("XX")), 
                                  String(Latin1String("ACAXXAB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 4, 2, String(Latin1String("XX")), 
                                  String(Latin1String("ACABXXB"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 5, 2, String(Latin1String("XX")), 
                                  String(Latin1String("ACABCXX"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 6, 2, String(Latin1String("XX")), 
                                  String(Latin1String("ACABCAXX"))));
   
   data.push_back(std::make_tuple(String(), 0, 10, String(Latin1String("X")), 
                                  String(Latin1String("X"))));
   
   data.push_back(std::make_tuple(String(Latin1String("short")), 0, 10, String(Latin1String("X")), 
                                  String(Latin1String("X"))));
   
   data.push_back(std::make_tuple(String(Latin1String()), 0, 10, String(Latin1String("XX")), 
                                  String(Latin1String("XX"))));
   
   data.push_back(std::make_tuple(String(Latin1String("short")), 0, 10, String(Latin1String("X")), 
                                  String(Latin1String("X"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 1, INT_MAX - 1, String(Latin1String("")), 
                                  String(Latin1String("A"))));
   
   data.push_back(std::make_tuple(String(Latin1String("ACABCAB")), 1, INT_MAX, String(Latin1String("")), 
                                  String(Latin1String("A"))));
}

void replace_string_data(std::list<std::tuple<String, String, String, String, bool>> &data)
{
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String("")), String(Latin1String("")), 
                                  String(Latin1String("")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("A")), String(Latin1String("")), String(Latin1String("")), 
                                  String(Latin1String("A")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("A")), String(Latin1String("A")), String(Latin1String("")), 
                                  String(Latin1String("")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("A")), String(Latin1String("B")), String(Latin1String("")), 
                                  String(Latin1String("A")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("AA")), String(Latin1String("A")), String(Latin1String("")), 
                                  String(Latin1String("")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("AB")), String(Latin1String("A")), String(Latin1String("")), 
                                  String(Latin1String("B")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("AB")), String(Latin1String("B")), String(Latin1String("")), 
                                  String(Latin1String("A")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("AB")), String(Latin1String("C")), String(Latin1String("")), 
                                  String(Latin1String("AB")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("ABA")), String(Latin1String("A")), String(Latin1String("")), 
                                  String(Latin1String("B")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("ABA")), String(Latin1String("B")), String(Latin1String("")), 
                                  String(Latin1String("AA")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("ABA")), String(Latin1String("C")), String(Latin1String("")), 
                                  String(Latin1String("ABA")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("banana")), String(Latin1String("an")), String(Latin1String("")), 
                                  String(Latin1String("ba")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String("A")), String(Latin1String("")), 
                                  String(Latin1String("")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String("A")), String(Latin1String()), 
                                  String(Latin1String("")), true));
   
   data.push_back(std::make_tuple(String(Latin1String()), String(Latin1String("A")), String(Latin1String("")), 
                                  String(Latin1String()), true));
   
   data.push_back(std::make_tuple(String(Latin1String()), String(Latin1String("A")), String(Latin1String()), 
                                  String(Latin1String()), true));
   
   data.push_back(std::make_tuple(String(Latin1String()), String(Latin1String("")), String(Latin1String("")), 
                                  String(Latin1String("")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String()), String(Latin1String("")), 
                                  String(Latin1String("")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("a")), String(Latin1String("")), 
                                  String(Latin1String("")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("A")), String(Latin1String("A")), String(Latin1String("")), 
                                  String(Latin1String("")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("A")), String(Latin1String("")), 
                                  String(Latin1String("")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("A")), String(Latin1String("a")), String(Latin1String("")), 
                                  String(Latin1String("")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("Alpha beta")), String(Latin1String("a")), String(Latin1String("")), 
                                  String(Latin1String("lph bet")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("B")), String(Latin1String("-")), 
                                  String(Latin1String("A-C")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("$()*+.?[\\]^{|}")), String(Latin1String("$()*+.?[\\]^{|}")), String(Latin1String("X")), 
                                  String(Latin1String("X")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("ABCDEF")), String(Latin1String("")), String(Latin1String("X")), 
                                  String(Latin1String("XAXBXCXDXEXFX")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String("")), String(Latin1String("X")), 
                                  String(Latin1String("X")), true));
   
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("a")), String(Latin1String("b")), 
                                  String(Latin1String("b")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("A")), String(Latin1String("A")), String(Latin1String("b")), 
                                  String(Latin1String("b")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("A")), String(Latin1String("b")), 
                                  String(Latin1String("b")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("A")), String(Latin1String("a")), String(Latin1String("b")), 
                                  String(Latin1String("b")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("a")), String(Latin1String("a")), 
                                  String(Latin1String("a")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("A")), String(Latin1String("A")), String(Latin1String("a")), 
                                  String(Latin1String("a")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("A")), String(Latin1String("a")), 
                                  String(Latin1String("a")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("A")), String(Latin1String("a")), String(Latin1String("a")), 
                                  String(Latin1String("a")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("Alpha beta")), String(Latin1String("a")), String(Latin1String("o")), 
                                  String(Latin1String("olpho beto")), false));
   
   data.push_back(std::make_tuple(String(Latin1String()), String(Latin1String("")), String(Latin1String("A")), 
                                  String(Latin1String("A")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String()), String(Latin1String("A")), 
                                  String(Latin1String("A")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("fooxbarxbazxblub")), String(Latin1String("x")), String(Latin1String("yz")), 
                                  String(Latin1String("fooyzbaryzbazyzblub")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("fooxbarxbazxblub")), String(Latin1String("x")), String(Latin1String("z")), 
                                  String(Latin1String("foozbarzbazzblub")), false));
   
   data.push_back(std::make_tuple(String(Latin1String("fooxybarxybazxyblub")), String(Latin1String("xy")), String(Latin1String("z")), 
                                  String(Latin1String("foozbarzbazzblub")), false));
}

}

TEST(StringTest, testRepaceCharacterAndCharacter)
{
   using DataType = std::list<std::tuple<String, Character, Character, int, String>>;
   DataType data;
   replace_character_character_data(data);
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      String src = std::get<0>(item);
      Character before = std::get<1>(item);
      Character after = std::get<2>(item);
      int cs = std::get<3>(item);
      String expected = std::get<4>(item);
      ASSERT_EQ(src.replace(before, after, pdk::CaseSensitivity(cs)), expected);
      ++begin;
   }
}

TEST(StringTest, testRepaceCharacterAndString)
{
   using DataType = std::list<std::tuple<String, Character, String, int, String>>;
   DataType data;
   replace_character_string_data(data);
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      String src = std::get<0>(item);
      Character before = std::get<1>(item);
      String after = std::get<2>(item);
      int cs = std::get<3>(item);
      String expected = std::get<4>(item);
      ASSERT_EQ(src.replace(before, after, pdk::CaseSensitivity(cs)), expected);
      ++begin;
   }
}

TEST(StringTest, testRepaceUintAndUint)
{
   using DataType = std::list<std::tuple<String, int, int, String, String>>;
   DataType data;
   replace_uint_uint_data(data);
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      String string = std::get<0>(item);
      int index = std::get<1>(item);
      int length = std::get<2>(item);
      String after = std::get<3>(item);
      String expected = std::get<4>(item);
      String s1 = string;
      s1.replace((uint)index, (int)length, after);
      ASSERT_EQ(s1, expected);
      if (after.length() == 1) {
         String s3 = string;
         s3.replace((uint)index, (uint) length, Character(after[0]));
         ASSERT_EQ(s3, expected);
         String s4 = string;
         s4.replace((uint) index, (uint) length, Character(after[0]).toLatin1() );
         ASSERT_EQ(s4, expected);
      }
      ++begin;
   }
}

TEST(StringTest, testReplaceString)
{
   using DataType = std::list<std::tuple<String, String, String, String, bool>>;
   DataType data;
   replace_string_data(data);
   DataType::iterator iter = data.begin();
   DataType::iterator end = data.end();
   while (iter != end) {
      auto item = *iter;
      String string = std::get<0>(item);
      String before = std::get<1>(item);
      String after = std::get<2>(item);
      String expected = std::get<3>(item);
      bool bcs = std::get<4>(item);
      pdk::CaseSensitivity cs = bcs ? pdk::CaseSensitivity::Sensitive : pdk::CaseSensitivity::Insensitive;
      if (before.length() == 1) {
         Character ch = before.at(0);
         String s1 = string;
         s1.replace(ch, after, cs);
         ASSERT_EQ(s1, expected);
         if (Character(ch.toLatin1()) == ch) {
            String s2 = string;
            s2.replace(ch.toLatin1(), after, cs );
            ASSERT_EQ(s2, expected);
         }
      }
      
      String s3 = string;
      s3.replace(before, after, cs);
      ASSERT_EQ(s3, expected);
      
      ++iter;
   }
}

TEST(StringTest, testLength)
{
   using DataType = std::list<std::tuple<String, int>>;
   DataType data;
   length_data(data);
   DataType::iterator begin = data.begin();
   DataType::iterator end = data.end();
   while (begin != end) {
      auto item = *begin;
      String string = std::get<0>(item);
      int length = std::get<1>(item);
      ASSERT_EQ(string.length(), length);
      ++begin;
   }
}

TEST(StringTest, testAccess)
{
   String a;
   String b;
   String bb;
   String c = String::fromLatin1("String C");
   Character temp[10];
   temp[0] = 'S';
   temp[1] = 't';
   temp[2] = 'r';
   temp[3] = 'i';
   temp[4] = 'n';
   temp[5] = 'g';
   temp[6] = ' ';
   temp[7] = 'D';
   temp[8] = 'X';
   temp[9] = '\0';
   String d(temp, 8);
   String ca(a);
   String cb(b);
   String cc(c);
   
   ASSERT_EQ(a, ca);
   ASSERT_TRUE(a.isNull());
   String e(Latin1String("String E"));
   String f;
   f = e;
   f[7] = 'F';
   ASSERT_EQ(e, Latin1String("String E"));
   char text[]="String f";
   f = text;
   text[7]='!';
   ASSERT_EQ(f, Latin1String("String f"));
   f[7] = 'F';
   ASSERT_EQ(text[7], '!');
   
   a = Latin1String("");
   a[0] = 'A';
   ASSERT_EQ(a, Latin1String("A"));
   ASSERT_EQ(a.length(), 1);
   
   a[1] = 'B';
   ASSERT_EQ(a, Latin1String("AB"));
   ASSERT_EQ(a.length(), 2);
   a[2] = 'C';
   ASSERT_EQ(a, Latin1String("ABC"));
   ASSERT_EQ(a.length(), 3);
   
   a = Latin1String("123");
   b = Latin1String("456");
   a[0] = a[1];
   ASSERT_EQ(a, Latin1String("223"));
   a[1] = b[1];
   ASSERT_EQ(b, Latin1String("456"));
   ASSERT_EQ(a, Latin1String("253"));
   
   char t[]="TEXT";
   a = Latin1String("A");
   a = t;
   ASSERT_EQ(a, Latin1String("TEXT"));
   ASSERT_EQ(a, (String)t);
   
   a[0] = 'X';
   ASSERT_EQ(a, Latin1String("XEXT"));
   ASSERT_EQ(t[0], 'T');
   t[0] = 'Z';
   ASSERT_EQ(a, Latin1String("XEXT"));
   
   a = Latin1String("ABC");
   ASSERT_EQ(char(a.toLatin1()[1]), 'B');
   int ret = strcmp(a.toLatin1(), ByteArrayLiteral("ABC"));
   ASSERT_EQ(ret, 0);
   a += Latin1String("DEF");
   
   ASSERT_EQ(a, Latin1String("ABCDEF"));
   ASSERT_EQ(a += 'G', Latin1String("ABCDEFG"));
   a += Latin1String(((const char*)(0)));
   ASSERT_EQ(a, Latin1String("ABCDEFG"));
   
   a = Latin1String("ABC");
   b = Latin1String("ABC");
   c = Latin1String("ACB");
   d = Latin1String("ABCD");
   
   ASSERT_TRUE(a == b);
   ASSERT_TRUE(!(a == d));
   ASSERT_TRUE(!(a != b));
   ASSERT_TRUE(a != d);
   ASSERT_TRUE(!(a < b));
   ASSERT_TRUE(a < c);
   ASSERT_TRUE(a < d);
   ASSERT_TRUE(!(d < a));
   ASSERT_TRUE(!(c < a));
   ASSERT_TRUE(a <= b);
   ASSERT_TRUE(a <= c);
   ASSERT_TRUE(a <= d);
   
   ASSERT_TRUE(!(c <= a));
   ASSERT_TRUE(!(d <= a));
   
   ASSERT_EQ(String(a + b), Latin1String("ABCABC"));
   ASSERT_EQ(String(a + Latin1String("XXXX")), Latin1String("ABCXXXX"));
   ASSERT_EQ(String(a + Latin1String("X")), Latin1String("ABCX"));
   ASSERT_EQ(String(Latin1String("XXXX") + a), Latin1String("XXXXABC"));
   ASSERT_EQ(String(Latin1String("X") + a), Latin1String("XABC"));
   a = String::fromUtf8((const char*)0);
   ASSERT_TRUE(a.isNull());
   ASSERT_TRUE(*a.toLatin1().getConstRawData() == '\0');
   
//   {
//      File f("COMPARE.txt");
//      f.open(IoDevice::OpenMode::ReadOnly);
//      TextStream ts(&f);
//      ts.setCodec(TextCodec::codecForName("UTF-16"));
//      ts << "Abc";
//   }
}

