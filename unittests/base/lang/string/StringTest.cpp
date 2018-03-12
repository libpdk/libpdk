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
#include "pdk/base/lang/StringMatcher.h"
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
using pdk::lang::Latin1Character;
using pdk::ds::ByteArray;
using pdk::lang::StringMatcher;
using pdk::ds::ByteArrayMatcher;

#define CREATE_REF(string)                                              \
   const String padded = Latin1Character(' ') + string +  Latin1Character(' '); \
   StringRef ref = padded.substringRef(1, padded.size() - 2);

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
      (str.*mf)(this->m_pinned);
   }
   
   template <typename MemFun, typename ArgType>
   void apply1(String &str, MemFun mf, ArgType arg) const
   {
      (str.*mf)(arg, this->m_pinned);
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
   void apply1(String &str, MemFun mf, ArgType arg) const
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
   
   template <typename MemFun, typename ArgType>
   void apply1(String &str, MemFun mf, ArgType arg) const
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
   
   template <typename MemFun, typename ArgType>
   void apply1(String &str, MemFun mf, ArgType arg) const
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
   
   template <typename MemFun, typename ArgType>
   void apply1(String &str, MemFun mf, ArgType arg)
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
         s4.replace((uint) index, (uint) length, Character(after[0]).toLatin1());
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

TEST(StringTest, testReplaceExtra)
{
   /*
           This test is designed to be extremely slow if String::replace() doesn't optimize the case
           len == after.size().
       */
   String str(Latin1String("dsfkljfdsjklsdjsfjklfsdjkldfjslkjsdfkllkjdsfjklsfdkjsdflkjlsdfjklsdfkjldsflkjsddlkj"));
   for (int j = 1; j < 12; ++j) {
      str += str;
   }
   String str2(Latin1String("aaaaaaaaaaaaaaaaaaaa"));
   for (int i = 0; i < 2000000; ++i) {
      str.replace(10, 20, str2);
   }
   /*
           Make sure that replacing with itself works.
       */
   String copy(str);
   copy.detach();
   str.replace(0, str.length(), str);
   ASSERT_EQ(copy, str);
   
   /*
          Make sure that replacing a part of oneself with itself works.
      */
   String str3(Latin1String("abcdefghij"));
   str3.replace(0, 1, str3);
   ASSERT_EQ(str3, String(Latin1String("abcdefghijbcdefghij")));
   
   String str4(Latin1String("abcdefghij"));
   str4.replace(1, 3, str4);
   ASSERT_EQ(str4, String(Latin1String("aabcdefghijefghij")));
   
   String str5(Latin1String("abcdefghij"));
   str5.replace(8, 10, str5);
   ASSERT_EQ(str5, String(Latin1String("abcdefghabcdefghij")));
   
   // Replacements using only part of the string modified:
   String str6(Latin1String("abcdefghij"));
   str6.replace(1, 8, str6.getConstRawData() + 3, 3);
   ASSERT_EQ(str6, String(Latin1String("adefj")));
   
   String str7(Latin1String("abcdefghibcdefghij"));
   str7.replace(str7.getConstRawData() + 1, 6, str7.getConstRawData() + 2, 3);
   ASSERT_EQ(str7, String(Latin1String("acdehicdehij")));
   
   const int many = 1024;
   /*
         String::replace(const Character *, int, const Character *, int, pdk::CaseSensitivity)
         does its replacements in batches of many (please keep in sync with any
         changes to batch size), which lead to misbehaviour if ether Character * array
         was part of the data being modified.
       */
   String str8(Latin1String("abcdefg")), ans8(Latin1String("acdeg"));
   {
      // Make str8 and ans8 repeat themselves many + 1 times:
      int i = many;
      String big(str8), small(ans8);
      while (i && !(i & 1)) { // Exploit many being a power of 2:
         big += big;
         small += small;
         i >>= 1;
      }
      while (i-- > 0) {
         str8 += big;
         ans8 += small;
      }
   }
   
   str8.replace(str8.getConstRawData() + 1, 5, str8.getConstRawData() + 2, 3);
   // Pre-test the bit where the diff happens, so it gets displayed:
   ASSERT_EQ(str8.substring((many - 3) * 5), ans8.substring((many - 3) * 5));
   // Also check the full values match, of course:
   ASSERT_EQ(str8.size(), ans8.size());
   ASSERT_EQ(str8, ans8);
}

PDK_WARNING_PUSH
PDK_WARNING_DISABLE_GCC("-Wformat-security")
PDK_WARNING_DISABLE_CLANG("-Wformat-security")

TEST(StringTest, testIsNull)
{
   String str;
   ASSERT_TRUE(str.isNull());
   const char *zero = 0;
   str = str.asprintf(zero);
   ASSERT_TRUE(!str.isNull());
}

PDK_WARNING_POP

TEST(StringTest, testIsEmpty)
{
   String a;
   ASSERT_TRUE(a.isEmpty());
   String c(Latin1String("Not empty"));
   ASSERT_TRUE(!c.isEmpty());
}

TEST(StringTest, testConstructor)
{
   String a;
   String b; //b(10);
   String c(Latin1String("String C"));
   Character tmp[10];
   tmp[0] = 'S';
   tmp[1] = 't';
   tmp[2] = 'r';
   tmp[3] = 'i';
   tmp[4] = 'n';
   tmp[5] = 'g';
   tmp[6] = ' ';
   tmp[7] = 'D';
   tmp[8] = 'X';
   tmp[9] = '\0';
   String d(tmp,8);
   String ca(a);
   String cb(b);
   String cc(c);
   ASSERT_EQ(a, ca);
   ASSERT_TRUE(a.isNull());
   ASSERT_TRUE(a == String(Latin1String("")));
   ASSERT_EQ(b, cb);
   ASSERT_EQ(c, cc);
   ASSERT_EQ(d, Latin1String("String D"));
   String nullStr;
   ASSERT_TRUE(nullStr.isNull());
   ASSERT_TRUE(nullStr.isEmpty());
   String empty(Latin1String(""));
   ASSERT_TRUE(!empty.isNull());
   ASSERT_TRUE(empty.isEmpty());
}

//namespace {

//void constructor_bytearray_data(std::list<std::tuple<ByteArray, String>> &data)
//{
//   ByteArray ba(4, 0);
//   ba[0] = 'C';
//   ba[1] = 'O';
//   ba[2] = 'M';
//   ba[3] = 'P';
//   data.push_back(std::make_tuple(ba, String(Latin1String("COMP"))));

//   ByteArray ba1( 7, 0 );
//   ba1[0] = 'a';
//   ba1[1] = 'b';
//   ba1[2] = 'c';
//   ba1[3] = '\0';
//   ba1[4] = 'd';
//   ba1[5] = 'e';
//   ba1[6] = 'f';
//   data.push_back(std::make_tuple(ba1, String(Latin1String("abc"))));
//   data.push_back(std::make_tuple(ByteArray::fromRawData("abcd", 3), String(Latin1String("abc"))));
//   data.push_back(std::make_tuple(ByteArray("\xc3\xa9"), String(Latin1String("\xc3\xa9"))));
//   data.push_back(std::make_tuple(ByteArray("\xc3\xa9"), String::fromUtf8("\xc3\xa9")));
//}

//} // anonymous namespace

//TEST(StringTest, testConstructorFromByteArray)
//{
//   using DataType = std::list<std::tuple<ByteArray, String>>;
//   DataType data;
//   constructor_bytearray_data(data);
//   DataType::iterator iter = data.begin();
//   DataType::iterator end = data.end();
//   while (iter != end) {
//      auto item = *iter;
//      ByteArray src = std::get<0>(item);
//      String expected = std::get<1>(item);
//      String str1(String::fromLatin1(src));
//      ASSERT_EQ(str1.length(), expected.length());
//      ASSERT_EQ(str1, expected);      
//      String strBA(String::fromLatin1(src));
//      ASSERT_EQ(strBA, expected);
//      ++iter;
//   }
//}

TEST(StringTest, testSTL)
{
   std::string stdstr( "String" );
   
   String stlqt = String::fromStdString(stdstr);
   ASSERT_EQ(stlqt, String::fromLatin1(stdstr.c_str()));
   ASSERT_EQ(stlqt.toStdString(), stdstr);
   
   const wchar_t arr[] = {'h', 'e', 'l', 'l', 'o', 0};
   std::wstring stlStr = arr;
   
   String s = String::fromStdWString(stlStr);
   
   ASSERT_EQ(s, String::fromLatin1("hello"));
   ASSERT_EQ(stlStr, s.toStdWString());
}

TEST(StringTest, testTruncate)
{
   String e(Latin1String("String E"));
   e.truncate(4);
   ASSERT_EQ(e, Latin1String("Stri"));
   
   e = Latin1String("String E");
   e.truncate(0);
   ASSERT_EQ(e, Latin1String(""));
   ASSERT_TRUE(e.isEmpty());
   ASSERT_TRUE(!e.isNull());
}

namespace {

void chop_data(std::list<std::tuple<String, int, String>> &data)
{
   const String original(Latin1String("abcd"));
   data.push_back(std::make_tuple(original, 1, String(Latin1String("abc"))));
   data.push_back(std::make_tuple(original, 0, original));
   data.push_back(std::make_tuple(original, -1, original));
   data.push_back(std::make_tuple(original, original.size(), String()));
   data.push_back(std::make_tuple(original, 1000, String()));
}

inline const void *ptr_value(pdk::uintptr v)
{
   return reinterpret_cast<const void *>(v);
}

} // anonymous namespace

TEST(StringTest, testChop)
{
   using DataType = std::list<std::tuple<String, int, String>>;
   DataType data;
   chop_data(data);
   DataType::iterator iter = data.begin();
   DataType::iterator end = data.end();
   while (iter != end) {
      auto item = *iter;
      String input = std::get<0>(item);
      int count = std::get<1>(item);
      String result = std::get<2>(item);
      input.chop(count);
      ASSERT_EQ(input, result);
      ++iter;
   }
}

TEST(StringTest, testFill)
{
   String e;
   e.fill('e',1);
   ASSERT_EQ(e, Latin1String("e"));
   String f;
   f.fill('f',3);
   ASSERT_EQ(f, Latin1String("fff"));
   f.fill('F');
   ASSERT_EQ(f, Latin1String("FFF"));
}

TEST(StringTest, testAsprintf)
{
   ASSERT_EQ(String::asprintf("COMPARE"), Latin1String("COMPARE"));
   ASSERT_EQ(String::asprintf("%%%d", 1), Latin1String("%1"));
   ASSERT_EQ(String::asprintf("X%dY", 2), Latin1String("X2Y"));
   ASSERT_EQ(String::asprintf("X%9iY", 50000), Latin1String("X    50000Y"));
   ASSERT_EQ(String::asprintf("X%-9sY", "hello"), Latin1String("Xhello    Y"));
   ASSERT_EQ(String::asprintf("X%-9iY", 50000), Latin1String("X50000    Y"));
   ASSERT_EQ(String::asprintf("%lf", 1.23), Latin1String("1.230000"));
   ASSERT_EQ(String::asprintf("%lf", 1.23456789), Latin1String("1.234568"));
   ASSERT_EQ(String::asprintf("%p", ptr_value(0xbfffd350)), Latin1String("0xbfffd350"));
   ASSERT_EQ(String::asprintf("%p", ptr_value(0)), Latin1String("0x0"));
   
   int i = 6;
   long l = -2;
   float f = 4.023f;
   
   ASSERT_EQ(String::asprintf("%d %ld %f",i,l,f), Latin1String("6 -2 4.023000"));
   
   double d = -514.25683;
   ASSERT_EQ(String::asprintf("%f",d), Latin1String("-514.256830"));
}

TEST(StringTest, testAsprintfS)
{
   ASSERT_EQ(String::asprintf("%.3s", "Hello" ), Latin1String("Hel"));
   ASSERT_EQ(String::asprintf("%10.3s", "Hello" ), Latin1String("       Hel"));
   ASSERT_EQ(String::asprintf("%.10s", "Hello" ), Latin1String("Hello"));
   ASSERT_EQ(String::asprintf("%10.10s", "Hello" ), Latin1String("     Hello"));
   ASSERT_EQ(String::asprintf("%-10.10s", "Hello" ), Latin1String("Hello     "));
   ASSERT_EQ(String::asprintf("%-10.3s", "Hello" ), Latin1String("Hel       "));
   ASSERT_EQ(String::asprintf("%-5.5s", "Hello" ), Latin1String("Hello"));
   
   // Check utf8 conversion for %s
   ASSERT_EQ(String::asprintf("%s", "\303\266\303\244\303\274\303\226\303\204\303\234\303\270\303\246\303\245\303\230\303\206\303\205"), String::fromLatin1("\366\344\374\326\304\334\370\346\345\330\306\305"));
   
   int n1;
   ASSERT_EQ(String::asprintf("%s%n%s", "hello", &n1, "goodbye"), String(Latin1String("hellogoodbye")));
   ASSERT_EQ(n1, 5);
   pdk::plonglong n2;
   ASSERT_EQ(String::asprintf("%s%s%lln%s", "foo", "bar", &n2, "whiz"), String(Latin1String("foobarwhiz")));
   ASSERT_EQ((int)n2, 6);
   
   { // %ls
      ASSERT_EQ(String::asprintf("%.3ls",     pdk_utf16_printable(Latin1String("Hello"))), Latin1String("Hel"));
      ASSERT_EQ(String::asprintf("%10.3ls",   pdk_utf16_printable(Latin1String("Hello"))), Latin1String("       Hel"));
      ASSERT_EQ(String::asprintf("%.10ls",    pdk_utf16_printable(Latin1String("Hello"))), Latin1String("Hello"));
      ASSERT_EQ(String::asprintf("%10.10ls",  pdk_utf16_printable(Latin1String("Hello"))), Latin1String("     Hello"));
      ASSERT_EQ(String::asprintf("%-10.10ls", pdk_utf16_printable(Latin1String("Hello"))), Latin1String("Hello     "));
      ASSERT_EQ(String::asprintf("%-10.3ls",  pdk_utf16_printable(Latin1String("Hello"))), Latin1String("Hel       "));
      ASSERT_EQ(String::asprintf("%-5.5ls",   pdk_utf16_printable(Latin1String("Hello"))), Latin1String("Hello"));
      
      // Check utf16 is preserved for %ls
      ASSERT_EQ(String::asprintf("%ls",
                                 pdk_utf16_printable(String::fromLocal8Bit("\303\266\303\244\303\274\303\226\303\204\303\234\303\270\303\246\303\245\303\230\303\206\303\205"))),
                Latin1String("\366\344\374\326\304\334\370\346\345\330\306\305"));
      int n;
      ASSERT_EQ(String::asprintf("%ls%n%s", pdk_utf16_printable(Latin1String("hello")), &n, "goodbye"), Latin1String("hellogoodbye"));
      ASSERT_EQ(n, 5);
   }
}

namespace {
void indexof_data(std::list<std::tuple<String, String, int, bool, int>> &data)
{
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("a")), 0, true, 0));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("a")), 0, false, 0));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("A")), 0, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("A")), 0, false, 0));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("a")), 1, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("a")), 1, false, -1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("A")), 1, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("A")), 1, false, -1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("b")), 0, true, 1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("b")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("B")), 0, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("B")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("b")), 1, true, 1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("b")), 1, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("B")), 1, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("B")), 1, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("b")), 2, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("b")), 2, false, -1));
   
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("A")), 0, true, 0));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("A")), 0, false, 0));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("a")), 0, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("a")), 0, false, 0));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("A")), 1, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("A")), 1, false, -1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("a")), 1, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("a")), 1, false, -1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("B")), 0, true, 1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("B")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("b")), 0, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("b")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("B")), 1, true, 1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("B")), 1, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("b")), 1, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("b")), 1, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("B")), 2, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("ABC")), String(Latin1String("B")), 2, false, -1));
   
   data.push_back(std::make_tuple(String(Latin1String("aBc")), String(Latin1String("bc")), 0, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("aBc")), String(Latin1String("Bc")), 0, true, 1));
   data.push_back(std::make_tuple(String(Latin1String("aBc")), String(Latin1String("bC")), 0, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("aBc")), String(Latin1String("BC")), 0, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("aBc")), String(Latin1String("bc")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("aBc")), String(Latin1String("Bc")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("aBc")), String(Latin1String("bC")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("aBc")), String(Latin1String("BC")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("AbC")), String(Latin1String("bc")), 0, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("AbC")), String(Latin1String("Bc")), 0, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("AbC")), String(Latin1String("bC")), 0, true, 1));
   data.push_back(std::make_tuple(String(Latin1String("AbC")), String(Latin1String("BC")), 0, true, -1));
   data.push_back(std::make_tuple(String(Latin1String("AbC")), String(Latin1String("bc")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("AbC")), String(Latin1String("Bc")), 0, false, 1));
   
   data.push_back(std::make_tuple(String(Latin1String("AbC")), String(Latin1String("bC")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("AbC")), String(Latin1String("BC")), 0, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("AbC")), String(Latin1String("bc")), 1, false, 1));
   data.push_back(std::make_tuple(String(Latin1String("AbC")), String(Latin1String("Bc")), 2, false, -1));
   
   data.push_back(std::make_tuple(String(), String(), 0, false, 0));
   data.push_back(std::make_tuple(String(), String(Latin1String("")), 0, false, 0));
   data.push_back(std::make_tuple(String(Latin1String("")), String(), 0, false, 0));
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String("")), 0, false, 0));
   
   String s1 = Latin1String("abc");
   s1 += Character(0xb5);
   String s2;
   s2 += Character(0x3bc);
   data.push_back(std::make_tuple(s1, s2, 0, false, 3));
   s2.prepend(Latin1Character('C'));
   data.push_back(std::make_tuple(s1, s2, 0, false, 2));
   
   String veryBigHaystack(500, 'a');
   veryBigHaystack += 'B';
   data.push_back(std::make_tuple(veryBigHaystack, veryBigHaystack, 0, true, 0));
   data.push_back(std::make_tuple(String(veryBigHaystack + 'c'), veryBigHaystack, 0, true, 0));
   data.push_back(std::make_tuple(String('c' + veryBigHaystack), veryBigHaystack, 0, true, 1));
   data.push_back(std::make_tuple(veryBigHaystack, String(veryBigHaystack + 'c'), 0, true, -1));
   data.push_back(std::make_tuple(veryBigHaystack, String('c' + veryBigHaystack), 0, true, -1));
   
   data.push_back(std::make_tuple(String(veryBigHaystack + 'c'), String('c' + veryBigHaystack), 0, true, -1));
   data.push_back(std::make_tuple(String('d' + veryBigHaystack), String('c' + veryBigHaystack), 0, true, -1));
   
   data.push_back(std::make_tuple(veryBigHaystack, veryBigHaystack, 0, false, 0));
}
}

TEST(StringTest, testIndexOf)
{
   using DataType = std::list<std::tuple<String, String, int, bool, int>>;
   DataType data;
   indexof_data(data);
   DataType::iterator iter = data.begin();
   DataType::iterator end = data.end();
   while (iter != end) {
      auto item = *iter;
      String haystack = std::get<0>(item);
      String needle = std::get<1>(item);
      String &ref = needle;
      int startpos = std::get<2>(item);
      bool bcs = std::get<3>(item);
      int resultPos = std::get<4>(item);
      pdk::CaseSensitivity cs = bcs ? pdk::CaseSensitivity::Sensitive : pdk::CaseSensitivity::Insensitive;
      bool needleIsLatin = (String::fromLatin1(needle.toLatin1()) == needle);
      ASSERT_EQ(haystack.indexOf(needle, startpos, cs), resultPos);
      ASSERT_EQ(haystack.indexOf(ref, startpos, cs), resultPos);
      if (needleIsLatin) {
         ASSERT_EQ(haystack.indexOf(Latin1String(needle.toLatin1()), startpos, cs), resultPos);
         ASSERT_EQ(haystack.indexOf(Latin1String(needle.toLatin1().getRawData()), startpos, cs), resultPos);
      }
      
      if (cs == pdk::CaseSensitivity::Sensitive) {
         ASSERT_EQ(haystack.indexOf(needle, startpos), resultPos);
         ASSERT_EQ(haystack.indexOf(ref, startpos), resultPos);
         if (needleIsLatin) {
            ASSERT_EQ(haystack.indexOf(Latin1String(needle.toLatin1()), startpos), resultPos);
            ASSERT_EQ(haystack.indexOf(Latin1String(needle.toLatin1().getRawData()), startpos), resultPos);
         }
         if (startpos == 0) {
            ASSERT_EQ( haystack.indexOf(needle), resultPos);
            ASSERT_EQ( haystack.indexOf(ref), resultPos);
            if (needleIsLatin) {
               ASSERT_EQ( haystack.indexOf(Latin1String(needle.toLatin1())), resultPos);
               ASSERT_EQ( haystack.indexOf(Latin1String(needle.toLatin1().getRawData())), resultPos);
            }
         }
      }
      if (needle.size() == 1) {
         ASSERT_EQ(haystack.indexOf(needle.at(0), startpos, cs), resultPos);
         ASSERT_EQ(haystack.indexOf(ref.at(0), startpos, cs), resultPos);
      }
      ++iter;
   }
}

namespace {

void indexof2_data(std::list<std::tuple<String, String, int>> &data)
{
   data.push_back(std::make_tuple(String(), String(), 0));
   data.push_back(std::make_tuple(String(), String(Latin1String("")), 0));
   data.push_back(std::make_tuple(String(Latin1String("")), String(), 0));
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String("")), 0));
   data.push_back(std::make_tuple(String(), String(Latin1String("a")), -1));
   data.push_back(std::make_tuple(String(), String(Latin1String("abcdefg")), -1));
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String("a")), -1));
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String("abcdefg")), -1));
   
   data.push_back(std::make_tuple(String(Latin1String("a")), String(), 0));
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("")), 0));
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("a")), 0));
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("b")), -1));
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("abcdefg")), -1));
   data.push_back(std::make_tuple(String(Latin1String("ab")), String(), 0));
   data.push_back(std::make_tuple(String(Latin1String("ab")), String(Latin1String("")), 0));
   data.push_back(std::make_tuple(String(Latin1String("ab")), String(Latin1String("a")), 0));
   data.push_back(std::make_tuple(String(Latin1String("ab")), String(Latin1String("b")), 1));
   data.push_back(std::make_tuple(String(Latin1String("ab")), String(Latin1String("ab")), 0));
   data.push_back(std::make_tuple(String(Latin1String("ab")), String(Latin1String("bc")), -1));
   data.push_back(std::make_tuple(String(Latin1String("ab")), String(Latin1String("abcdefg")), -1));
   
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("a")), 0));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("b")), 1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("c")), 2));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("d")), -1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("ab")), 0));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("bc")), 1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("cd")), -1));
   data.push_back(std::make_tuple(String(Latin1String("abc")), String(Latin1String("ac")), -1));
   
   // sizeof(whale) > 32
   String whale = Latin1String("a5zby6cx7dw8evf9ug0th1si2rj3qkp4lomn");
   String minnow = Latin1String("zby");
   data.push_back(std::make_tuple(whale, minnow, 2));
   data.push_back(std::make_tuple(String(whale + whale), minnow, 2));
   data.push_back(std::make_tuple(String(minnow + whale), minnow, 0));
   data.push_back(std::make_tuple(whale, whale, 0));
   data.push_back(std::make_tuple(String(whale + whale), whale, 0));
   data.push_back(std::make_tuple(whale, String(whale + whale), -1));
   data.push_back(std::make_tuple(String(whale + whale), String(whale + whale), 0));
   data.push_back(std::make_tuple(String(whale + whale), String(whale + minnow), -1));
   data.push_back(std::make_tuple(String(minnow + whale), whale, (int)minnow.length()));
}

void last_indexof_data(std::list<std::tuple<String, String, int, int, bool>> &data)
{
   String a = Latin1String("ABCDEFGHIEfGEFG");
   data.push_back(std::make_tuple(a, Latin1String("G"), a.size() - 1, 14, true));
   data.push_back(std::make_tuple(a, Latin1String("G"), -1, 14, true));
   data.push_back(std::make_tuple(a, Latin1String("G"), -3, 11, true));
   data.push_back(std::make_tuple(a, Latin1String("G"), -5, 6, true));
   data.push_back(std::make_tuple(a, Latin1String("G"), 14, 14, true));
   data.push_back(std::make_tuple(a, Latin1String("G"), 13, 11, true));
   data.push_back(std::make_tuple(a, Latin1String("B"), a.size() - 1, 1, true));
   data.push_back(std::make_tuple(a, Latin1String("B"), -1, 1, true));
   data.push_back(std::make_tuple(a, Latin1String("B"), 1, 1, true));
   data.push_back(std::make_tuple(a, Latin1String("B"), 0, -1, true));
   
   data.push_back(std::make_tuple(a, Latin1String("G"), - 1, a.size() - 1, true));
   data.push_back(std::make_tuple(a, Latin1String("G"), a.size()-1, a.size()-1, true));
   data.push_back(std::make_tuple(a, Latin1String("G"), a.size(), -1, true));
   data.push_back(std::make_tuple(a, Latin1String("A"), 0, 0, true));
   data.push_back(std::make_tuple(a, Latin1String("A"), -1 * a.size(), 0, true));
   
   data.push_back(std::make_tuple(a, Latin1String("efg"), 0, -1, false));
   data.push_back(std::make_tuple(a, Latin1String("efg"), a.size(), -1, false));
   data.push_back(std::make_tuple(a, Latin1String("efg"), -1 * a.size(), -1, false));
   data.push_back(std::make_tuple(a, Latin1String("efg"),  a.size() - 1, 12, false));
   data.push_back(std::make_tuple(a, Latin1String("efg"), 12, 12, false));
   data.push_back(std::make_tuple(a, Latin1String("efg"), -12, -1, false));
   data.push_back(std::make_tuple(a, Latin1String("efg"), 11, 9, false));
   
   data.push_back(std::make_tuple(Latin1String(""), Latin1String("asdf"), -1, -1, false));
   data.push_back(std::make_tuple(Latin1String("asd"), Latin1String("asdf"), -1, -1, false));
   data.push_back(std::make_tuple(Latin1String(""), String(), -1, -1, false));
   
   data.push_back(std::make_tuple(a, Latin1String(""), a.size(), a.size(), false));
   data.push_back(std::make_tuple(a, Latin1String(""), a.size() + 10, -1, false));
}

} // anonymous namespace

TEST(StringTest, testIndexOf2)
{
   using DataType = std::list<std::tuple<String, String, int>>;
   DataType data;
   indexof2_data(data);
   DataType::iterator iter = data.begin();
   DataType::iterator end = data.end();
   while (iter != end) {
      auto item = *iter;
      String haystack = std::get<0>(item);
      String needle = std::get<1>(item);
      String &ref = needle;
      int resultPos = std::get<2>(item);
      ByteArray chaystack = haystack.toLatin1();
      ByteArray cneedle = needle.toLatin1();
      int got;
      ASSERT_EQ(haystack.indexOf(needle, 0, pdk::CaseSensitivity::Sensitive), resultPos);
      ASSERT_EQ(haystack.indexOf(ref, 0, pdk::CaseSensitivity::Sensitive), resultPos);
      ASSERT_EQ(StringMatcher(needle, pdk::CaseSensitivity::Sensitive).indexIn(haystack, 0), resultPos);
      ASSERT_EQ(haystack.indexOf(needle, 0, pdk::CaseSensitivity::Insensitive), resultPos);
      ASSERT_EQ(haystack.indexOf(ref, 0, pdk::CaseSensitivity::Insensitive), resultPos);
      ASSERT_EQ(StringMatcher(needle, pdk::CaseSensitivity::Insensitive).indexIn(haystack, 0), resultPos);
      if (needle.length() > 0) {
         got = haystack.lastIndexOf( needle, -1, pdk::CaseSensitivity::Sensitive);
         ASSERT_TRUE(got == resultPos || (resultPos >= 0 && got >= resultPos));
         got = haystack.lastIndexOf(needle, -1, pdk::CaseSensitivity::Insensitive);
         ASSERT_TRUE(got == resultPos || (resultPos >= 0 && got >= resultPos));
      }
      ASSERT_EQ(chaystack.indexOf(cneedle, 0), resultPos);
      ASSERT_EQ(ByteArrayMatcher(cneedle).indexIn(chaystack, 0), resultPos);
      if ( cneedle.length() > 0 ) {
         got = chaystack.lastIndexOf(cneedle, -1);
         ASSERT_TRUE(got == resultPos || (resultPos >= 0 && got >= resultPos));
      }
      ++iter;
   }
}


TEST(StringTest, testLastIndexOf)
{
   using DataType = std::list<std::tuple<String, String, int, int, bool>>;
   DataType data;
   last_indexof_data(data);
   DataType::iterator iter = data.begin();
   DataType::iterator end = data.end();
   while (iter != end) {
      auto item = *iter;
      String haystack = std::get<0>(item);
      String needle = std::get<1>(item);
      int from = std::get<2>(item);
      int expected = std::get<3>(item);
      bool caseSensitive = std::get<4>(item);
      String &ref = needle;
      pdk::CaseSensitivity cs = (caseSensitive ? pdk::CaseSensitivity::Sensitive : pdk::CaseSensitivity::Insensitive);
      
      ASSERT_EQ(haystack.lastIndexOf(needle, from, cs), expected);
      ASSERT_EQ(haystack.lastIndexOf(ref, from, cs), expected);
      ASSERT_EQ(haystack.lastIndexOf(Latin1String(needle.toLatin1()), from, cs), expected);
      ASSERT_EQ(haystack.lastIndexOf(Latin1String(needle.toLatin1().getRawData()), from, cs), expected);
      
      if (cs == pdk::CaseSensitivity::Sensitive) {
         ASSERT_EQ(haystack.lastIndexOf(needle, from), expected);
         ASSERT_EQ(haystack.lastIndexOf(ref, from), expected);
         ASSERT_EQ(haystack.lastIndexOf(Latin1String(needle.toLatin1()), from), expected);
         ASSERT_EQ(haystack.lastIndexOf(Latin1String(needle.toLatin1().getRawData()), from), expected);
         if (from == -1) {
            ASSERT_EQ(haystack.lastIndexOf(needle), expected);
            ASSERT_EQ(haystack.lastIndexOf(ref), expected);
            ASSERT_EQ(haystack.lastIndexOf(Latin1String(needle.toLatin1())), expected);
            ASSERT_EQ(haystack.lastIndexOf(Latin1String(needle.toLatin1().getRawData())), expected);
         }
      }
      if (needle.size() == 1) {
         ASSERT_EQ(haystack.lastIndexOf(needle.at(0), from), expected);
         ASSERT_EQ(haystack.lastIndexOf(ref.at(0), from), expected);
      }      
      ++iter;
   }
}

TEST(StringTest, testCount)
{
   String str;
   str = Latin1String("ABCDEFGHIEfGEFG");
   ASSERT_EQ(str.count('A'),1);
   ASSERT_EQ(str.count('Z'),0);
   ASSERT_EQ(str.count('E'),3);
   ASSERT_EQ(str.count('F'),2);
   ASSERT_EQ(str.count('F',pdk::CaseSensitivity::Insensitive),3);
   ASSERT_EQ(str.count(Latin1String("FG")),2);
   ASSERT_EQ(str.count(Latin1String("FG"), pdk::CaseSensitivity::Insensitive),3);
   ASSERT_EQ(str.count(String(), pdk::CaseSensitivity::Insensitive), 16);
   ASSERT_EQ(str.count(Latin1String(""), pdk::CaseSensitivity::Insensitive), 16);
   
   CREATE_REF(Latin1String("FG"));
   ASSERT_EQ(str.count(ref),2);
   ASSERT_EQ(str.count(ref,pdk::CaseSensitivity::Insensitive),3);
   ASSERT_EQ(str.count(StringRef(), pdk::CaseSensitivity::Insensitive), 16);
   StringRef emptyRef(&str, 0, 0);
   ASSERT_EQ(str.count(emptyRef, pdk::CaseSensitivity::Insensitive), 16);
}

TEST(StringTest, testContains)
{
   String str;
   str = Latin1String("ABCDEFGHIEfGEFG"); // 15 chars
   ASSERT_TRUE(str.contains('A'));
   ASSERT_TRUE(!str.contains('Z'));
   ASSERT_TRUE(str.contains('E'));
   ASSERT_TRUE(str.contains('F'));
   ASSERT_TRUE(str.contains('F', pdk::CaseSensitivity::Insensitive));
   ASSERT_TRUE(str.contains(Latin1String("FG")));
   ASSERT_TRUE(str.contains(Latin1String("FG"), pdk::CaseSensitivity::Insensitive));
   ASSERT_TRUE(str.contains(Latin1String("FG")));
   ASSERT_TRUE(str.contains(Latin1String("fg"), pdk::CaseSensitivity::Insensitive));
   ASSERT_TRUE(str.contains(String(), pdk::CaseSensitivity::Insensitive));
   ASSERT_TRUE(str.contains(Latin1String(""), pdk::CaseSensitivity::Insensitive));
   
   CREATE_REF(Latin1String("FG"));
   ASSERT_TRUE(str.contains(ref));
   ASSERT_TRUE(str.contains(ref, pdk::CaseSensitivity::Insensitive));
   ASSERT_TRUE(str.contains(StringRef(), pdk::CaseSensitivity::Insensitive));
   StringRef emptyRef(&str, 0, 0);
   ASSERT_TRUE(str.contains(emptyRef, pdk::CaseSensitivity::Insensitive));
}

TEST(StringTest, testLeft)
{
   String a;
   a = Latin1String("ABCDEFGHIEfGEFG"); // 15 chars
   ASSERT_EQ(a.left(3), Latin1String("ABC"));
   ASSERT_TRUE(!a.left(0).isNull());
   ASSERT_EQ(a.left(0), Latin1String(""));
   
   String n;
   ASSERT_TRUE(n.left(3).isNull());
   ASSERT_TRUE(n.left(0).isNull());
   ASSERT_TRUE(n.left(0).isNull());
   
   String l = Latin1String("Left");
   ASSERT_EQ(l.left(-1), l);
   ASSERT_EQ(l.left(100), l);
}

TEST(StringTest, testLeftRef)
{
   String a;
   a = Latin1String("ABCDEFGHIEfGEFG"); // 15 chars
   ASSERT_EQ(a.leftRef(3).toString(), Latin1String("ABC"));
   
   ASSERT_TRUE(a.leftRef(0).toString().isEmpty());
   ASSERT_EQ(a.leftRef(0).toString(), Latin1String(""));
   
   String n;
   ASSERT_TRUE(n.leftRef(3).toString().isEmpty());
   ASSERT_TRUE(n.leftRef(0).toString().isEmpty());
   ASSERT_TRUE(n.leftRef(0).toString().isEmpty());
   
   String l = Latin1String("Left");
   ASSERT_EQ(l.leftRef(-1).toString(), l);
   ASSERT_EQ(l.leftRef(100).toString(), l);
}

TEST(StringTest, testRight)
{
   String a;
   a = Latin1String("ABCDEFGHIEfGEFG"); // 15 chars
   ASSERT_EQ(a.right(3), Latin1String("EFG"));
   ASSERT_EQ(a.right(0), Latin1String(""));
   
   String n;
   ASSERT_TRUE(n.right(3).isNull());
   ASSERT_TRUE(n.right(0).isNull());
   
   String r = Latin1String("Right");
   ASSERT_EQ(r.right(-1), r);
   ASSERT_EQ(r.right(100), r);
}

TEST(StringTest, testRightRef)
{
   String a;
   a = Latin1String("ABCDEFGHIEfGEFG"); // 15 chars
   ASSERT_EQ(a.rightRef(3).toString(), Latin1String("EFG"));
   ASSERT_EQ(a.rightRef(0).toString(), Latin1String(""));
   
   String n;
   ASSERT_TRUE(n.rightRef(3).toString().isEmpty());
   ASSERT_TRUE(n.rightRef(0).toString().isEmpty());
   
   String r = Latin1String("Right");
   ASSERT_EQ(r.rightRef(-1).toString(), r);
   ASSERT_EQ(r.rightRef(100).toString(), r);
}

TEST(StringTest, testSubString)
{
   String str;
   str = Latin1String("ABCDEFGHIEfGEFG"); // 15 chars
   
   ASSERT_EQ(str.substring(3,3), Latin1String("DEF"));
   ASSERT_EQ(str.substring(0,0), Latin1String(""));
   ASSERT_TRUE(!str.substring(15,0).isNull());
   ASSERT_TRUE(str.substring(15,0).isEmpty());
   ASSERT_TRUE(!str.substring(15,1).isNull());
   ASSERT_TRUE(str.substring(15,1).isEmpty());
   ASSERT_TRUE(str.substring(9999).isNull());
   ASSERT_TRUE(str.substring(9999,1).isNull());
   
   ASSERT_EQ(str.substring(-1, 6), str.substring(0, 5));
   ASSERT_TRUE(str.substring(-100, 6).isEmpty());
   ASSERT_TRUE(str.substring(INT_MIN, 0).isEmpty());
   ASSERT_EQ(str.substring(INT_MIN, -1), str);
   ASSERT_TRUE(str.substring(INT_MIN, INT_MAX).isNull());
   ASSERT_TRUE(str.substring(INT_MIN + 1, INT_MAX).isEmpty());
   ASSERT_EQ(str.substring(INT_MIN + 2, INT_MAX), str.left(1));
   ASSERT_EQ(str.substring(INT_MIN + str.size() + 1, INT_MAX), str);
   ASSERT_TRUE(str.substring(INT_MAX).isNull());
   ASSERT_TRUE(str.substring(INT_MAX, INT_MAX).isNull());
   ASSERT_EQ(str.substring(-5, INT_MAX), str);
   ASSERT_EQ(str.substring(-1, INT_MAX), str);
   ASSERT_EQ(str.substring(0, INT_MAX), str);
   ASSERT_EQ(str.substring(1, INT_MAX), String(Latin1String("BCDEFGHIEfGEFG")));
   ASSERT_EQ(str.substring(5, INT_MAX), String(Latin1String("FGHIEfGEFG")));
   ASSERT_TRUE(str.substring(20, INT_MAX).isNull());
   ASSERT_EQ(str.substring(-1, -1), str);
   
   String n;
   ASSERT_TRUE(n.substring(3,3).isNull());
   ASSERT_TRUE(n.substring(0,0).isNull());
   ASSERT_TRUE(n.substring(9999,0).isNull());
   ASSERT_TRUE(n.substring(9999,1).isNull());
   
   ASSERT_TRUE(n.substring(-1, 6).isNull());
   ASSERT_TRUE(n.substring(-100, 6).isNull());
   ASSERT_TRUE(n.substring(INT_MIN, 0).isNull());
   ASSERT_TRUE(n.substring(INT_MIN, -1).isNull());
   ASSERT_TRUE(n.substring(INT_MIN, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(INT_MIN + 1, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(INT_MIN + 2, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(INT_MIN + n.size() + 1, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(INT_MAX).isNull());
   ASSERT_TRUE(n.substring(INT_MAX, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(-5, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(-1, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(0, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(1, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(5, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(20, INT_MAX).isNull());
   ASSERT_TRUE(n.substring(-1, -1).isNull());
   
   String x = Latin1String("Nine pineapples");
   ASSERT_EQ(x.substring(5, 4), String(Latin1String("pine")));
   ASSERT_EQ(x.substring(5), String(Latin1String("pineapples")));
   
   ASSERT_EQ(x.substring(-1, 6), x.substring(0, 5));
   ASSERT_TRUE(x.substring(-100, 6).isEmpty());
   ASSERT_TRUE(x.substring(INT_MIN, 0).isEmpty());
   ASSERT_EQ(x.substring(INT_MIN, -1), x);
   ASSERT_TRUE(x.substring(INT_MIN, INT_MAX).isNull());
   ASSERT_TRUE(x.substring(INT_MIN + 1, INT_MAX).isEmpty());
   ASSERT_EQ(x.substring(INT_MIN + 2, INT_MAX), x.left(1));
   ASSERT_EQ(x.substring(INT_MIN + x.size() + 1, INT_MAX), x);
   ASSERT_TRUE(x.substring(INT_MAX).isNull());
   ASSERT_TRUE(x.substring(INT_MAX, INT_MAX).isNull());
   ASSERT_EQ(x.substring(-5, INT_MAX), x);
   ASSERT_EQ(x.substring(-1, INT_MAX), x);
   ASSERT_EQ(x.substring(0, INT_MAX), x);
   ASSERT_EQ(x.substring(1, INT_MAX), String(Latin1String("ine pineapples")));
   ASSERT_EQ(x.substring(5, INT_MAX), String(Latin1String("pineapples")));
   ASSERT_TRUE(x.substring(20, INT_MAX).isNull());
   ASSERT_EQ(x.substring(-1, -1), x);
}

TEST(StringTest, testSubStringRef)
{
   String str;
   str = Latin1String("ABCDEFGHIEfGEFG"); // 15 chars
   
   ASSERT_EQ(str.substringRef(3,3).toString(), Latin1String("DEF"));
   ASSERT_EQ(str.substringRef(0,0).toString(), Latin1String(""));
   ASSERT_TRUE(!str.substringRef(15,0).toString().isNull());
   ASSERT_TRUE(str.substringRef(15,0).toString().isEmpty());
   ASSERT_TRUE(!str.substringRef(15,1).toString().isNull());
   ASSERT_TRUE(str.substringRef(15,1).toString().isEmpty());
   ASSERT_TRUE(str.substringRef(9999).toString().isEmpty());
   ASSERT_TRUE(str.substringRef(9999,1).toString().isEmpty());
   
   ASSERT_EQ(str.substringRef(-1, 6), str.substringRef(0, 5));
   ASSERT_TRUE(str.substringRef(-100, 6).isEmpty());
   ASSERT_TRUE(str.substringRef(INT_MIN, 0).isEmpty());
   ASSERT_EQ(str.substringRef(INT_MIN, -1).toString(), str);
   ASSERT_TRUE(str.substringRef(INT_MIN, INT_MAX).isNull());
   ASSERT_TRUE(str.substringRef(INT_MIN + 1, INT_MAX).isEmpty());
   ASSERT_EQ(str.substringRef(INT_MIN + 2, INT_MAX), str.leftRef(1));
   ASSERT_EQ(str.substringRef(INT_MIN + str.size() + 1, INT_MAX).toString(), str);
   ASSERT_TRUE(str.substringRef(INT_MAX).isNull());
   ASSERT_TRUE(str.substringRef(INT_MAX, INT_MAX).isNull());
   ASSERT_EQ(str.substringRef(-5, INT_MAX).toString(), str);
   ASSERT_EQ(str.substringRef(-1, INT_MAX).toString(), str);
   ASSERT_EQ(str.substringRef(0, INT_MAX).toString(), str);
   ASSERT_EQ(str.substringRef(1, INT_MAX).toString(), String(Latin1String("BCDEFGHIEfGEFG")));
   ASSERT_EQ(str.substringRef(5, INT_MAX).toString(), String(Latin1String("FGHIEfGEFG")));
   ASSERT_TRUE(str.substringRef(20, INT_MAX).isNull());
   ASSERT_EQ(str.substringRef(-1, -1).toString(), str);
   
   String n;
   ASSERT_TRUE(n.substringRef(3,3).toString().isEmpty());
   ASSERT_TRUE(n.substringRef(0,0).toString().isEmpty());
   ASSERT_TRUE(n.substringRef(9999,0).toString().isEmpty());
   ASSERT_TRUE(n.substringRef(9999,1).toString().isEmpty());
   
   ASSERT_TRUE(n.substringRef(-1, 6).isNull());
   ASSERT_TRUE(n.substringRef(-100, 6).isNull());
   ASSERT_TRUE(n.substringRef(INT_MIN, 0).isNull());
   ASSERT_TRUE(n.substringRef(INT_MIN, -1).isNull());
   ASSERT_TRUE(n.substringRef(INT_MIN, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(INT_MIN + 1, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(INT_MIN + 2, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(INT_MIN + n.size() + 1, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(INT_MAX, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(-5, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(-1, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(0, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(1, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(5, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(20, INT_MAX).isNull());
   ASSERT_TRUE(n.substringRef(-1, -1).isNull());
   
   String x = Latin1String("Nine pineapples");
   ASSERT_EQ(x.substringRef(5, 4).toString(), String(Latin1String("pine")));
   ASSERT_EQ(x.substringRef(5).toString(), String(Latin1String("pineapples")));
   
   ASSERT_EQ(x.substringRef(-1, 6), x.substringRef(0, 5));
   ASSERT_TRUE(x.substringRef(-100, 6).isEmpty());
   ASSERT_TRUE(x.substringRef(INT_MIN, 0).isEmpty());
   ASSERT_EQ(x.substringRef(INT_MIN, -1).toString(), x);
   ASSERT_TRUE(x.substringRef(INT_MIN, INT_MAX).isNull());
   ASSERT_TRUE(x.substringRef(INT_MIN + 1, INT_MAX).isEmpty());
   ASSERT_EQ(x.substringRef(INT_MIN + 2, INT_MAX), x.leftRef(1));
   ASSERT_EQ(x.substringRef(INT_MIN + x.size() + 1, INT_MAX).toString(), x);
   ASSERT_TRUE(x.substringRef(INT_MAX).isNull());
   ASSERT_TRUE(x.substringRef(INT_MAX, INT_MAX).isNull());
   ASSERT_EQ(x.substringRef(-5, INT_MAX).toString(), x);
   ASSERT_EQ(x.substringRef(-1, INT_MAX).toString(), x);
   ASSERT_EQ(x.substringRef(0, INT_MAX).toString(), x);
   ASSERT_EQ(x.substringRef(1, INT_MAX).toString(), String(Latin1String("ine pineapples")));
   ASSERT_EQ(x.substringRef(5, INT_MAX).toString(), String(Latin1String("pineapples")));
   ASSERT_TRUE(x.substringRef(20, INT_MAX).isNull());
   ASSERT_EQ(x.substringRef(-1, -1).toString(), x);
}

TEST(StringTest, testStringRef)
{
   String str;
   str = Latin1String("ABCDEFGHIEfGEFG"); // 15 chars
   
   ASSERT_TRUE(StringRef(&str, 0, 0) == String(Latin1String("")));
   
   ASSERT_TRUE(StringRef(&str, 3, 3) == String(Latin1String("DEF")));
   ASSERT_TRUE(StringRef(&str, 3, 3) == Latin1String("DEF"));
   ASSERT_TRUE(String(Latin1String("DEF")) == StringRef(&str, 3, 3));
   ASSERT_TRUE(Latin1String("DEF") == StringRef(&str, 3, 3));
   
   ASSERT_TRUE(StringRef(&str, 3, 3) != String(Latin1String("DE")));
   ASSERT_TRUE(StringRef(&str, 3, 3) != Latin1String("DE"));
   ASSERT_TRUE(String(Latin1String("DE")) != StringRef(&str, 3, 3));
   ASSERT_TRUE(Latin1String("DE") != StringRef(&str, 3, 3));
   
   String s_alpha(Latin1String("alpha"));
   String s_beta(Latin1String("beta"));
   StringRef alpha(&s_alpha);
   StringRef beta(&s_beta);
   
   ASSERT_TRUE(alpha < beta);
   ASSERT_TRUE(alpha <= beta);
   ASSERT_TRUE(alpha <= alpha);
   ASSERT_TRUE(beta > alpha);
   ASSERT_TRUE(beta >= alpha);
   ASSERT_TRUE(beta >= beta);
   
   String s_alpha2(Latin1String("alpha"));
   
   std::map<StringRef, String> map;
   map[alpha] = Latin1String("alpha");
   map[beta] = Latin1String("beta");
   ASSERT_TRUE(alpha == map.at(StringRef(&s_alpha2)));
}

TEST(StringTest, testLeftJustified)
{
   String str;
   str = Latin1String("ABC");
   ASSERT_EQ(str.leftJustified(5,'-'), Latin1String("ABC--"));
   ASSERT_EQ(str.leftJustified(4,'-'), Latin1String("ABC-"));
   ASSERT_EQ(str.leftJustified(4), Latin1String("ABC "));
   ASSERT_EQ(str.leftJustified(3), Latin1String("ABC"));
   ASSERT_EQ(str.leftJustified(2), Latin1String("ABC"));
   ASSERT_EQ(str.leftJustified(1), Latin1String("ABC"));
   ASSERT_EQ(str.leftJustified(0), Latin1String("ABC"));
   
   String n;
   ASSERT_TRUE(!n.leftJustified(3).isNull());
   ASSERT_EQ(str.leftJustified(4,' ',true), Latin1String("ABC "));
   ASSERT_EQ(str.leftJustified(3,' ',true), Latin1String("ABC"));
   ASSERT_EQ(str.leftJustified(2,' ',true), Latin1String("AB"));
   ASSERT_EQ(str.leftJustified(1,' ',true), Latin1String("A"));
   ASSERT_EQ(str.leftJustified(0,' ',true), Latin1String(""));
}

TEST(StringTest, testRightJustified)
{
   String str;
   str = Latin1String("ABC");
   ASSERT_EQ(str.rightJustified(5,'-'), Latin1String("--ABC"));
   ASSERT_EQ(str.rightJustified(4,'-'), Latin1String("-ABC"));
   ASSERT_EQ(str.rightJustified(4), Latin1String(" ABC"));
   ASSERT_EQ(str.rightJustified(3), Latin1String("ABC"));
   ASSERT_EQ(str.rightJustified(2), Latin1String("ABC"));
   ASSERT_EQ(str.rightJustified(1), Latin1String("ABC"));
   ASSERT_EQ(str.rightJustified(0), Latin1String("ABC"));
   
   String n;
   ASSERT_TRUE(!n.rightJustified(3).isNull());
   ASSERT_EQ(str.rightJustified(4,'-',true), Latin1String("-ABC"));
   ASSERT_EQ(str.rightJustified(4,' ',true), Latin1String(" ABC"));
   ASSERT_EQ(str.rightJustified(3,' ',true), Latin1String("ABC"));
   ASSERT_EQ(str.rightJustified(2,' ',true), Latin1String("AB"));
   ASSERT_EQ(str.rightJustified(1,' ',true), Latin1String("A"));
   ASSERT_EQ(str.rightJustified(0,' ',true), Latin1String(""));
   ASSERT_EQ(str, Latin1String("ABC"));
}

TEST(StringTest, testToUpper)
{
   ASSERT_EQ(String().toUpper(), String());
   ASSERT_EQ(String(Latin1String("")).toUpper(), String(Latin1String("")));
   String literal = StringLiteral("text").toUpper();
   ASSERT_EQ(literal, String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("text")).toUpper(), String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("Text")).toUpper(), String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("tExt")).toUpper(), String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("teXt")).toUpper(), String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("texT")).toUpper(), String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("TExt")).toUpper(), String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("teXT")).toUpper(), String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("tEXt")).toUpper(), String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("tExT")).toUpper(), String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("TEXT")).toUpper(), String(Latin1String("TEXT")));
   ASSERT_EQ(String(Latin1String("@ABYZ[")).toUpper(), String(Latin1String("@ABYZ[")));
   ASSERT_EQ(String(Latin1String("@abyz[")).toUpper(), String(Latin1String("@ABYZ[")));
   ASSERT_EQ(String(Latin1String("`ABYZ{")).toUpper(), String(Latin1String("`ABYZ{")));
   ASSERT_EQ(String(Latin1String("`abyz{")).toUpper(), String(Latin1String("`ABYZ{")));
   
   ASSERT_EQ(String(1, Character(0xdf)).toUpper(), String(Latin1String("SS")));
   
   {
      String s = String::fromUtf8("Gro\xc3\x9fstra\xc3\x9f""e");
      // call lvalue-ref version, mustn't change the original
      ASSERT_EQ(s.toUpper(), String(Latin1String("GROSSSTRASSE")));
      ASSERT_EQ(s, String::fromUtf8("Gro\xc3\x9fstra\xc3\x9f""e"));
      // call rvalue-ref while shared (the original mustn't change)
      String copy = s;
      ASSERT_EQ(std::move(copy).toUpper(), String(Latin1String("GROSSSTRASSE")));
      ASSERT_EQ(s, String::fromUtf8("Gro\xc3\x9fstra\xc3\x9f""e"));
      // call rvalue-ref version on detached case
      copy.clear();
      ASSERT_EQ(std::move(s).toUpper(), String(Latin1String("GROSSSTRASSE")));
   }
   
   String lower, upper;
   lower += Character(Character::getHighSurrogate(0x10428));
   lower += Character(Character::getLowSurrogate(0x10428));
   upper += Character(Character::getHighSurrogate(0x10400));
   upper += Character(Character::getLowSurrogate(0x10400));
   ASSERT_EQ(lower.toUpper(), upper);
   lower += lower;
   upper += upper;
   ASSERT_EQ(lower.toUpper(), upper);
   
   // test for broken surrogate pair handling (low low hi low hi low)
   lower.prepend(Character(Character::getLowSurrogate(0x10428)));
   lower.prepend(Character(Character::getLowSurrogate(0x10428)));
   upper.prepend(Character(Character::getLowSurrogate(0x10428)));
   upper.prepend(Character(Character::getLowSurrogate(0x10428)));
   ASSERT_EQ(lower.toUpper(), upper);
   // test for broken surrogate pair handling (low low hi low hi low hi hi)
   lower += Character(Character::getHighSurrogate(0x10428));
   lower += Character(Character::getHighSurrogate(0x10428));
   upper += Character(Character::getHighSurrogate(0x10428));
   upper += Character(Character::getHighSurrogate(0x10428));
   ASSERT_EQ(lower.toUpper(), upper);
   
   for (int i = 0; i < 65536; ++i) {
      String str(1, Character(i));
      String upper = str.toUpper();
      ASSERT_TRUE(upper.length() >= 1);
      if (upper.length() == 1) {
         ASSERT_TRUE(upper == String(1, Character(i).toUpper()));
      }
   }
}


TEST(StringTest, testToLower)
{
   ASSERT_EQ(String().toLower(), String());
   ASSERT_EQ(String(Latin1String("")).toLower(), String(Latin1String("""")));
   ASSERT_EQ(String(Latin1String("text")).toLower(), String(Latin1String("text")));
   String literal = StringLiteral("Text").toLower();
   ASSERT_EQ(literal, String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("Text")).toLower(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("tExt")).toLower(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("teXt")).toLower(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("texT")).toLower(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("TExt")).toLower(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("teXT")).toLower(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("tEXt")).toLower(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("tExT")).toLower(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("TEXT")).toLower(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("@ABYZ[")).toLower(), String(Latin1String("@abyz[")));
   ASSERT_EQ(String(Latin1String("@abyz[")).toLower(), String(Latin1String("@abyz[")));
   ASSERT_EQ(String(Latin1String("`ABYZ{")).toLower(), String(Latin1String("`abyz{")));
   ASSERT_EQ(String(Latin1String("`abyz{")).toLower(), String(Latin1String("`abyz{")));
   
   ASSERT_EQ(String(1, Character(0x130)).toLower(), String(String(1, Character(0x69)) + Character(0x307)));
   
   String lower, upper;
   lower += Character(Character::getHighSurrogate(0x10428));
   lower += Character(Character::getLowSurrogate(0x10428));
   upper += Character(Character::getHighSurrogate(0x10400));
   upper += Character(Character::getLowSurrogate(0x10400));
   ASSERT_EQ(upper.toLower(), lower);
   lower += lower;
   upper += upper;
   ASSERT_EQ(upper.toLower(), lower);
   
   // test for broken surrogate pair handling (low low hi low hi low)
   lower.prepend(Character(Character::getLowSurrogate(0x10400)));
   lower.prepend(Character(Character::getLowSurrogate(0x10400)));
   upper.prepend(Character(Character::getLowSurrogate(0x10400)));
   upper.prepend(Character(Character::getLowSurrogate(0x10400)));
   ASSERT_EQ(upper.toLower(), lower);
   // test for broken surrogate pair handling (low low hi low hi low hi hi)
   lower += Character(Character::getHighSurrogate(0x10400));
   lower += Character(Character::getHighSurrogate(0x10400));
   upper += Character(Character::getHighSurrogate(0x10400));
   upper += Character(Character::getHighSurrogate(0x10400));
   ASSERT_EQ(upper.toLower(), lower);
   
   for (int i = 0; i < 65536; ++i) {
      String str(1, Character(i));
      String lower = str.toLower();
      ASSERT_TRUE(lower.length() >= 1);
      if (lower.length() == 1) {
         ASSERT_TRUE(str.toLower() == String(1, Character(i).toLower()));
      }
   }
}

TEST(StringTest, testToCaseFolded)
{
   ASSERT_EQ(String().toCaseFolded(), String());
   ASSERT_EQ(String(Latin1String("")).toCaseFolded(), String(Latin1String("")));
   ASSERT_EQ(String(Latin1String("text")).toCaseFolded(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("Text")).toCaseFolded(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("tExt")).toCaseFolded(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("teXt")).toCaseFolded(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("texT")).toCaseFolded(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("TExt")).toCaseFolded(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("teXT")).toCaseFolded(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("tEXt")).toCaseFolded(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("tExT")).toCaseFolded(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("TEXT")).toCaseFolded(), String(Latin1String("text")));
   ASSERT_EQ(String(Latin1String("@ABYZ[")).toCaseFolded(), String(Latin1String("@abyz[")));
   ASSERT_EQ(String(Latin1String("@abyz[")).toCaseFolded(), String(Latin1String("@abyz[")));
   ASSERT_EQ(String(Latin1String("`ABYZ{")).toCaseFolded(), String(Latin1String("`abyz{")));
   ASSERT_EQ(String(Latin1String("`abyz{")).toCaseFolded(), String(Latin1String("`abyz{")));
   
   ASSERT_EQ(String(1, Character(0xa77d)).toCaseFolded(), String(1, Character(0x1d79)));
   ASSERT_EQ(String(1, Character(0xa78d)).toCaseFolded(), String(1, Character(0x0265)));
   
   String lower, upper;
   upper += Character(Character::getHighSurrogate(0x10400));
   upper += Character(Character::getLowSurrogate(0x10400));
   lower += Character(Character::getHighSurrogate(0x10428));
   lower += Character(Character::getLowSurrogate(0x10428));
   ASSERT_EQ(upper.toCaseFolded(), lower);
   lower += lower;
   upper += upper;
   ASSERT_EQ(upper.toCaseFolded(), lower);
   
   // test for broken surrogate pair handling (low low hi low hi low)
   lower.prepend(Character(Character::getLowSurrogate(0x10400)));
   lower.prepend(Character(Character::getLowSurrogate(0x10400)));
   upper.prepend(Character(Character::getLowSurrogate(0x10400)));
   upper.prepend(Character(Character::getLowSurrogate(0x10400)));
   ASSERT_EQ(upper.toCaseFolded(), lower);
   // test for broken surrogate pair handling (low low hi low hi low hi hi)
   lower += Character(Character::getHighSurrogate(0x10400));
   lower += Character(Character::getHighSurrogate(0x10400));
   upper += Character(Character::getHighSurrogate(0x10400));
   upper += Character(Character::getHighSurrogate(0x10400));
   ASSERT_EQ(upper.toCaseFolded(), lower);
   
   //### we currently don't support full case foldings
   for (int i = 0; i < 65536; ++i) {
      String str(1, Character(i));
      String lower = str.toCaseFolded();
      ASSERT_TRUE(lower.length() >= 1);
      if (lower.length() == 1) {
         ASSERT_TRUE(str.toCaseFolded() == String(1, Character(i).toCaseFolded()));
      }
   }
}


TEST(StringTest, testTrimmed)
{
   String str;
   str = Latin1String("Text");
   ASSERT_EQ(str, Latin1String("Text"));
   ASSERT_EQ(str.trimmed(), Latin1String("Text"));
   ASSERT_EQ(str, Latin1String("Text"));
   str = Latin1String(" ");
   ASSERT_EQ(str.trimmed(), Latin1String(""));
   ASSERT_EQ(str, Latin1String(" "));
   str = Latin1String(" a   ");
   ASSERT_EQ(str.trimmed(), Latin1String("a"));
   
   str = Latin1String("Text");
   ASSERT_EQ(std::move(str).trimmed(), Latin1String("Text"));
   str = Latin1String(" ");
   ASSERT_EQ(std::move(str).trimmed(), Latin1String(""));
   str = Latin1String(" a   ");
   ASSERT_EQ(std::move(str).trimmed(), Latin1String("a"));
}

namespace {

void simplified_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(String(), String()));
   data.push_back(std::make_tuple(String(Latin1String("")), String(Latin1String(""))));
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("a"))));
   data.push_back(std::make_tuple(String(Latin1String("foo")), String(Latin1String("foo"))));
   data.push_back(std::make_tuple(String(Latin1String("a b")), String(Latin1String("a b"))));
   data.push_back(std::make_tuple(String(Latin1String("foo bar")), String(Latin1String("foo bar"))));
   data.push_back(std::make_tuple(String(Latin1String("  \t\v ")), String(Latin1String(""))));
   data.push_back(std::make_tuple(String(Latin1String("a ")), String(Latin1String("a"))));
   data.push_back(std::make_tuple(String(Latin1String("a\t")), String(Latin1String("a"))));
   data.push_back(std::make_tuple(String(Latin1String("a    ")), String(Latin1String("a"))));
   data.push_back(std::make_tuple(String(Latin1String("a    \t")), String(Latin1String("a"))));
   data.push_back(std::make_tuple(String(Latin1String(" a")), String(Latin1String("a"))));
   data.push_back(std::make_tuple(String(Latin1String("\ta")), String(Latin1String("a"))));
   data.push_back(std::make_tuple(String(Latin1String("    a")), String(Latin1String("a"))));
   data.push_back(std::make_tuple(String(Latin1String("\t    a")), String(Latin1String("a"))));
   data.push_back(std::make_tuple(String(Latin1String("a    b")), String(Latin1String("a b"))));
   data.push_back(std::make_tuple(String(Latin1String("foo  bar")), String(Latin1String("foo bar"))));
   data.push_back(std::make_tuple(String(Latin1String("  foo  \t")), String(Latin1String("foo"))));
   data.push_back(std::make_tuple(String(Latin1String(" a    b ")), String(Latin1String("a b"))));
   data.push_back(std::make_tuple(String(Latin1String(" foo    bar ")), String(Latin1String("foo bar"))));
   data.push_back(std::make_tuple(String(Latin1String("a\t b")), String(Latin1String("a b"))));
   data.push_back(std::make_tuple(String(Latin1String("a \tb")), String(Latin1String("a b"))));
   data.push_back(std::make_tuple(String(Latin1String("  just some    random	text here")), String(Latin1String("just some random text here"))));
   data.push_back(std::make_tuple(String(Latin1String("a\nb\nc")), String(Latin1String("a b c"))));
   data.push_back(std::make_tuple(String(Latin1String("a\nb\nc\n")), String(Latin1String("a b c"))));
}

} // anonymous namespace

TEST(StringTest, testSimplified)
{
   using DataType = std::list<std::tuple<String, String>>;
   DataType data;
   simplified_data(data);
   DataType::iterator iter = data.begin();
   DataType::iterator endMark = data.end();
   while (iter != endMark) {
      auto item = *iter;
      String full = std::get<0>(item);
      String simple = std::get<1>(item);
      String origfull = full;
      origfull.getRawData();
      String result = full.simplified();
      if (simple.isNull()) {
         ASSERT_TRUE(result.isNull()) << pdk_printable(Latin1String("'") + full + Latin1String("' did not yield null: ") + result);
      } else if (simple.isEmpty()) {
         ASSERT_TRUE(result.isEmpty() && !result.isNull()) << pdk_printable(Latin1String("'") + full + Latin1String("' did not yield empty: ") + result);
      } else {
         ASSERT_EQ(result, simple);
      }
      ASSERT_EQ(full, origfull);
      // without detaching:
      String copy1 = full;
      ASSERT_EQ(std::move(full).simplified(), simple);
      ASSERT_EQ(full, origfull);
      
      // force a detach
      if (!full.isEmpty()) {
         full[0] = full[0];
      }
      ASSERT_EQ(std::move(full).simplified(), simple);
      ++iter;
   }
}

class CharStarContainer
{
   const char *m_str;
public:
   explicit constexpr CharStarContainer(const char *s = nullptr) 
      : m_str(s)
   {}
   
   constexpr operator const char *() const
   {
      return m_str; 
   }
};

namespace {

void insert_data(bool emptyIsNoop, std::list<std::tuple<String, CharStarContainer, int, String>> &data)
{
   const CharStarContainer nullC;
   const CharStarContainer emptyC("");
   const CharStarContainer aC("a");
   const CharStarContainer bC("b");
   //const CharStarContainer abC("ab");
   const CharStarContainer baC("ba");
   
   const String null;
   const String empty(Latin1String(""));
   const String a(Latin1String("a"));
   const String b(Latin1String("b"));
   const String ab(Latin1String("ab"));
   const String ba(Latin1String("ba"));
   data.push_back(std::make_tuple(null, nullC, 0, null));
   data.push_back(std::make_tuple(null, emptyC, 0,  (emptyIsNoop ? null : empty)));
   data.push_back(std::make_tuple(null, aC, 0, a));
   data.push_back(std::make_tuple(empty, nullC, 0, empty));
   data.push_back(std::make_tuple(empty, emptyC, 0, empty));
   data.push_back(std::make_tuple(empty, aC, 0, a));
   data.push_back(std::make_tuple(a, nullC, 0, a));
   data.push_back(std::make_tuple(a, emptyC, 0, a));
   data.push_back(std::make_tuple(a, bC, 0, ba));
   data.push_back(std::make_tuple(a, baC, 0, ba + a));
   data.push_back(std::make_tuple(a, nullC, 1, a));
   data.push_back(std::make_tuple(a, emptyC, 1, a));
   data.push_back(std::make_tuple(a, bC, 1, ab));
   data.push_back(std::make_tuple(a, baC, 1, a + ba));
   data.push_back(std::make_tuple(ba, aC, 1, ba + a));
   data.push_back(std::make_tuple(ba, bC, 2, ba + b));
}

template <typename ArgType, typename A1, typename MemFun, typename DataType>
void do_apply1(MemFun mf, const DataType &data)
{
   typename DataType::const_iterator iter = data.begin();
   typename DataType::const_iterator endMark = data.end();
   while (iter != endMark) {
      auto item = *iter;
      String s = std::get<0>(item);
      CharStarContainer arg = std::get<1>(item);
      int a1 = std::get<2>(item);
      String expected = std::get<3>(item);
      Arg<ArgType>(arg).apply1(s, mf, a1);
      ASSERT_EQ(s, expected);
      ASSERT_EQ(s.isEmpty(), expected.isEmpty());
      ASSERT_EQ(s.isNull(), expected.isNull());
      ++iter;
   }
}

template <typename ArgType, typename MemFun, typename DataType>
void insert_impl(const DataType &data)
{
   do_apply1<ArgType, int>(MemFun(&String::insert), data);
}

template <typename ArgType, typename DataType>
void insert_impl(const DataType &data)
{
   insert_impl<ArgType, String &(String::*)(int, const ArgType&)>(data);
}
} // anonymous namespace

TEST(StringTest, testInsertString)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, int, String>>;
   DataType data;
   insert_data(true, data);
   insert_impl<String>(data);
}

TEST(StringTest, testInsertStringRef)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, int, String>>;
   DataType data;
   insert_data(true, data);
   insert_impl<StringRef>(data);
}

TEST(StringTest, testInsertLatin1String)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, int, String>>;
   DataType data;
   insert_data(true, data);
   insert_impl<Latin1String, String &(String::*)(int, Latin1String)>(data);
}

TEST(StringTest, testInsertCharacterInt)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, int, String>>;
   DataType data;
   insert_data(true, data);
   insert_impl<std::pair<const Character *, int>, String &(String::*)(int, const Character*, int) >(data);
}

TEST(StringTest, testInsertCharacter)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, int, String>>;
   DataType data;
   insert_data(true, data);
   insert_impl<Reversed<Character>, String &(String::*)(int, Character)>(data);
}

TEST(StringTest, testInsertChar)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, int, String>>;
   DataType data;
   insert_data(true, data);
   insert_impl<Reversed<char>, String &(String::*)(int, Character)>(data);
}

TEST(StringTest, testInsertSpecialCase)
{
   String str;
   str = Latin1String("Ys");
   ASSERT_EQ(str.insert(1,'e'), String(Latin1String("Yes")));
   ASSERT_EQ(str.insert(3,'!'), String(Latin1String("Yes!")));
   ASSERT_EQ(str.insert(5,'?'), String(Latin1String("Yes! ?")));
   
   str = Latin1String("ABC");
   ASSERT_EQ(str.insert(5, Latin1String("DEF")), String(Latin1String("ABC  DEF")));
   
   str = Latin1String("ABC");
   ASSERT_EQ(str.insert(2, String()), String(Latin1String("ABC")));
   ASSERT_EQ(str.insert(0, Latin1String("ABC")), String(Latin1String("ABCABC")));
   ASSERT_EQ(str, String(Latin1String("ABCABC")));
   ASSERT_EQ(str.insert(0, str), String(Latin1String("ABCABCABCABC")));
   
   ASSERT_EQ(str, String(Latin1String("ABCABCABCABC")));
   ASSERT_EQ(str.insert(0,'<'), String(Latin1String("<ABCABCABCABC")));
   ASSERT_EQ(str.insert(1,'>'), String(Latin1String("<>ABCABCABCABC")));
   
   str = Latin1String("Meal");
   const String montreal = StringLiteral("Montreal");
   ASSERT_EQ(str.insert(1, Latin1String("ontr")), montreal);
   ASSERT_EQ(str.insert(4, Latin1String("")), montreal);
   ASSERT_EQ(str.insert(3, Latin1String("")), montreal);
   ASSERT_EQ(str.insert(3, Latin1String(0)), montreal);
   ASSERT_EQ(str.insert(3, Latin1String(static_cast<const char *>(0))), montreal);
   ASSERT_EQ(str.insert(0, Latin1String("a")), Latin1String("aMontreal"));
}

namespace {

void append_data(std::list<std::tuple<String, CharStarContainer, String>> &data, bool emptyIsNoop = false)
{
   const CharStarContainer nullC;
   const CharStarContainer emptyC("");
   const CharStarContainer aC("a");
   const CharStarContainer bC("b");
   //const CharStarContainer abC("ab");
   
   const String null;
   const String empty(Latin1String(""));
   const String a(Latin1String("a"));
   //const String b("b");
   const String ab(Latin1String("ab"));
   
   data.push_back(std::make_tuple(null, nullC, null));
   data.push_back(std::make_tuple(null, emptyC, (emptyIsNoop ? null : empty)));
   data.push_back(std::make_tuple(null, aC, a));
   data.push_back(std::make_tuple(empty, nullC, empty));
   data.push_back(std::make_tuple(empty, emptyC, empty));
   data.push_back(std::make_tuple(empty, aC, a));
   data.push_back(std::make_tuple(a, nullC, a));
   data.push_back(std::make_tuple(a, emptyC, a));
   data.push_back(std::make_tuple(a, bC, ab));
}

template <typename ArgType, typename MemFun, typename DataType>
void do_apply0(MemFun mf, const DataType &data)
{
   typename DataType::const_iterator iter = data.begin();
   typename DataType::const_iterator endMark = data.end();
   while (iter != endMark) {
      auto item = *iter;
      String s = std::get<0>(item);
      CharStarContainer arg = std::get<1>(item);
      String expected = std::get<2>(item);
      Arg<ArgType>(arg).apply0(s, mf);
      ASSERT_EQ(s, expected);
      ASSERT_EQ(s.isEmpty(), expected.isEmpty());
      ASSERT_EQ(s.isNull(), expected.isNull());
      ++iter;
   }
}

template <typename ArgType, typename MemFun, typename DataType>
void append_impl(const DataType &data)
{
   do_apply0<ArgType>(MemFun(&String::append), data);
}

template <typename ArgType, typename DataType>
void append_impl(const DataType &data)
{
   append_impl<ArgType, String &(String::*)(const ArgType&)>(data);
}

} // anonymous namespace

TEST(StringTest, testAppendString)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, false);
   append_impl<String>(data);
}

TEST(StringTest, testAppendStringRef)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, false);
   append_impl<StringRef>(data);
}

TEST(StringTest, testAppendLatin1String)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, false);
   append_impl<Latin1String, String &(String::*)(Latin1String)>(data);
}

TEST(StringTest, testAppendCharInt)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, true);
   append_impl<std::pair<const Character *, int>, String& (String::*)(const Character *, int)>(data);
}

TEST(StringTest, testAppendCharacter)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, true);
   append_impl<Character, String &(String::*)(Character)>(data);
}

TEST(StringTest, testAppendChar)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, true);
   append_impl<Reversed<char>, String &(String::*)(Character)>(data);
}

TEST(StrinTest, testAppendSpecialCases)
{
   String str;
   static const Character unicode[] = { 'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!' };
   str.append(unicode, sizeof unicode / sizeof *unicode);
   ASSERT_EQ(str, Latin1String("Hello, World!"));
   static const Character nl('\n');
   str.append(&nl, 1);
   ASSERT_EQ(str, Latin1String("Hello, World!\n"));
   str.append(unicode, sizeof unicode / sizeof *unicode);
   ASSERT_EQ(str, Latin1String("Hello, World!\nHello, World!"));
   str.append(unicode, 0); // no-op
   ASSERT_EQ(str, Latin1String("Hello, World!\nHello, World!"));
   str.append(unicode, -1); // no-op
   ASSERT_EQ(str, Latin1String("Hello, World!\nHello, World!"));
   str.append(0, 1); // no-op
   ASSERT_EQ(str, Latin1String("Hello, World!\nHello, World!"));
}

namespace
{

void prepend_data(std::list<std::tuple<String, CharStarContainer, String>> &data, bool emptyIsNoop = false)
{
   const CharStarContainer nullC;
   const CharStarContainer emptyC("");
   const CharStarContainer aC("a");
   const CharStarContainer bC("b");
   const CharStarContainer baC("ba");
   
   const String null;
   const String empty(Latin1String(""));
   const String a(Latin1String("a"));
   //const String b("b");
   const String ba(Latin1String("ba"));
   
   data.push_back(std::make_tuple(null, nullC, null));
   data.push_back(std::make_tuple(null, emptyC, (emptyIsNoop ? null : empty)));
   data.push_back(std::make_tuple(null, aC, a));
   data.push_back(std::make_tuple(empty, nullC, empty));
   data.push_back(std::make_tuple(empty, emptyC, empty));
   data.push_back(std::make_tuple(empty, aC, a));
   data.push_back(std::make_tuple(a, nullC, a));
   data.push_back(std::make_tuple(a, emptyC, a));
   data.push_back(std::make_tuple(a, bC, ba));
   data.push_back(std::make_tuple(a, baC, (ba + a)));
}

template <typename ArgType, typename MemFun, typename DataType>
void prepend_impl(const DataType &data)
{
   do_apply0<ArgType>(MemFun(&String::prepend), data);
}

template <typename ArgType, typename DataType>
void prepend_impl(const DataType &data)
{
   prepend_impl<ArgType, String &(String::*)(const ArgType&)>(data);
}

} // anonymous namespace

TEST(StringTest, testPrependString)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   prepend_data(data, true);
   prepend_impl<String>(data);
}

TEST(StringTest, testPrependStringRef)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   prepend_data(data, true);
   prepend_impl<StringRef>(data);
}

TEST(StringTest, testPrependLatin1String)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   prepend_data(data, true);
   prepend_impl<Latin1String, String &(String::*)(Latin1String)>(data);
}

TEST(StringTest, testPrependCharInt)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   prepend_data(data, true);
   prepend_impl<std::pair<const Character *, int>, String& (String::*)(const Character *, int)>(data);
}

TEST(StringTest, testPrependCharacter)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   prepend_data(data, true);
   prepend_impl<Reversed<Character>, String &(String::*)(Character)>(data);
}

TEST(StringTest, testPrependChar)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   prepend_data(data, true);
   prepend_impl<Reversed<char>, String &(String::*)(Character)>(data);
}

namespace {

template <typename ArgType, typename MemFun, typename DataType>
void operator_pluseq_impl(const DataType &data)
{
   do_apply0<ArgType>(MemFun(&String::operator+=), data);
}

template <typename ArgType, typename DataType>
void operator_pluseq_impl(const DataType &data)
{
   operator_pluseq_impl<ArgType, String &(String::*)(const ArgType&)>(data);
}

void operator_pluseq_data(std::list<std::tuple<String, CharStarContainer, String>> &data, bool emptyIsNoop = false)
{
   append_data(data, emptyIsNoop);  
}

} // anonymous namespace

TEST(StringTest, testOperatorPluseqString)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, false);
   operator_pluseq_impl<String>(data);
}

TEST(StringTest, testOperatorPluseqStringRef)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, false);
   operator_pluseq_impl<StringRef>(data);
}

TEST(StringTest, testOperatorPluseqLatin1String)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, false);
   operator_pluseq_impl<Latin1String, String &(String::*)(Latin1String)>(data);
}

TEST(StringTest, testOperatorPluseqCharacter)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, true);
   operator_pluseq_impl<Character, String &(String::*)(Character)>(data);
}

TEST(StringTest, testOperatorPluseqChar)
{
   using DataType = std::list<std::tuple<String, CharStarContainer, String>>;
   DataType data;
   append_data(data, true);
   operator_pluseq_impl<char, String &(String::*)(Character)>(data);
}
