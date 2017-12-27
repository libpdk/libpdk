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
#include "pdk/base/lang/Character.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/kernel/StringUtils.h"
#include <list>
#include <utility>
#include <string>
#include <algorithm>
#include <type_traits>

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
   
//   ASSERT_EQ(a, ca);
//   ASSERT_TRUE(a.isNull());
//   // error: C-style cast from 'const char [1]' to 'pdk::lang::String' uses deleted function
//   //ASSERT_TRUE(a == (String)"");
//   ASSERT_EQ(b, cb);
//   ASSERT_EQ(c, cc);
//   ASSERT_EQ(d, Latin1String("String D"));
}
