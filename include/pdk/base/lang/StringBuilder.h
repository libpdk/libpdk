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
// Created by softboy on 2018/01/31.

#ifndef PDK_M_BASE_LANG_STRING_BUILDER_H
#define PDK_M_BASE_LANG_STRING_BUILDER_H

#include "pdk/base/lang/String.h"
#include "pdk/base/ds/ByteArray.h"
#include <cstring>

namespace pdk {
namespace lang {

using pdk::ds::ByteArray;

class PDK_CORE_EXPORT AbstractConcatenable
{
protected:
   static void convertFromAscii(const char *a, int len, Character *&out) noexcept;
   static inline void convertFromAscii(char a, Character *&out) noexcept
   {
      *out++ = Latin1Character(a);
   }
   static void appendLatin1To(const char *a, int len, Character *out) noexcept;
};

template <typename T>
struct Concatenable
{};

namespace stringbuilder
{
template <typename A, typename B>
struct ConvertToTypeHelper
{
   using ConvertTo = A;
};

template <typename T> 
struct ConvertToTypeHelper<T, String>
{
   typedef String ConvertTo;
};

} // stringbuilder

template<typename Builder, typename T>
struct StringBuilderCommon
{
   T toUpper() const
   {
      return resolved().toUpper();
   }
   
   T toLower() const
   {
      return resolved().toLower();
   }
   
protected:
   T resolved() const
   {
      return *static_cast<const Builder*>(this);
   }
};

template<typename Builder, typename T>
struct StringBuilderBase : public StringBuilderCommon<Builder, T>
{
};

template<typename Builder>
struct StringBuilderBase<Builder, String> : public StringBuilderCommon<Builder, String>
{
   ByteArray toLatin1() const
   {
      return this->resolved().toLatin1();
   }
   
   ByteArray toUtf8() const
   {
      return this->resolved().toUtf8();
   }
   
   ByteArray toLocal8Bit() const
   {
      return this->resolved().toLocal8Bit();
   }
};

template <typename A, typename B>
class StringBuilder : public StringBuilderBase<StringBuilder<A, B>, typename stringbuilder::ConvertToTypeHelper<typename Concatenable<A>::ConvertTo, typename Concatenable<B>::ConvertTo>::ConvertTo>
{
public:
   StringBuilder(const A &a, const B &b) 
      : m_a(a),
        m_b(b)
   {}
private:
   friend class ByteArray;
   friend class String;
   template <typename T> T convertTo() const
   {
      const uint len = Concatenable<StringBuilder<A, B>>::size(*this);
      T s(len, pdk::Uninitialized);
      
      // we abuse const_cast / constData here because we know we've just
      // allocated the data and we're the only reference count
      typename T::iterator d = const_cast<typename T::iterator>(s.getConstRawData());
      typename T::const_iterator const start = d;
      Concatenable< StringBuilder<A, B> >::appendTo(*this, d);
      if (!Concatenable<StringBuilder<A, B>>::ExactSize && int(len) != d - start) {
         // this resize is necessary since we allocate a bit too much
         // when dealing with variable sized 8-bit encodings
         s.resize(d - start);
      }
      return s;
   }
   
   typedef Concatenable<StringBuilder<A, B>> SelfConcatenable;
   typedef typename SelfConcatenable::ConvertTo ConvertTo;
public:
   operator ConvertTo() const
   {
      return convertTo<ConvertTo>();
   }
   
   int size() const
   {
      return SelfConcatenable::size(*this);
   }
   
   const A &m_a;
   const B &m_b;
};

template <>
class StringBuilder <String, String> : public StringBuilderBase<StringBuilder<String, String>, String>
{
public:
   StringBuilder(const String &a, const String &b) 
      : m_a(a),
        m_b(b)
   {}
   
   StringBuilder(const StringBuilder &other) 
      : m_a(other.m_a),
        m_b(other.m_b)
   {}
   
   operator String() const
   {
      String r(m_a);
      r += m_b;
      return r;
   }
   
   const String &m_a;
   const String &m_b;
   
private:
   StringBuilder &operator=(const StringBuilder &) = delete;
};

template <>
class StringBuilder <ByteArray, ByteArray> 
      : public StringBuilderBase<StringBuilder<ByteArray, ByteArray>, ByteArray>
{
public:
   StringBuilder(const ByteArray &a, const ByteArray &b) 
      : m_a(a),
        m_b(b)
   {}
   
   StringBuilder(const StringBuilder &other) 
      : m_a(other.m_a),
        m_b(other.m_b)
   {}
   
   operator ByteArray() const
   {
      ByteArray r(m_a);
      r += m_b;
      return r;
   }
   
   const ByteArray &m_a;
   const ByteArray &m_b;
   
private:
   StringBuilder &operator=(const StringBuilder &) = delete;
};


template <> 
struct Concatenable<char> : private AbstractConcatenable
{
   typedef char type;
   typedef ByteArray ConvertTo;
   enum { ExactSize = true };
   static int size(const char)
   { 
      return 1;
   }
   
   static inline void appendTo(const char c, char *&out)
   {
      *out++ = c;
   }
};

template <> 
struct Concatenable<Latin1Character>
{
   typedef Latin1Character type;
   typedef String ConvertTo;
   enum { ExactSize = true };
   static int size(const Latin1Character)
   {
      return 1;
   }
   
   static inline void appendTo(const Latin1Character c, Character *&out)
   {
      *out++ = c;
   }
   
   static inline void appendTo(const Latin1Character c, char *&out)
   {
      *out++ = c.toLatin1();
   }
};

template <> struct Concatenable<Character> : private AbstractConcatenable
{
   typedef Character type;
   typedef String ConvertTo;
   enum { ExactSize = true };
   static int size(const Character)
   {
      return 1;
   }
   
   static inline void appendTo(const Character c, Character *&out)
   {
      *out++ = c;
   }
};

template <>
struct Concatenable<Character::SpecialCharacter> 
      : private AbstractConcatenable
{
   typedef Character::SpecialCharacter type;
   typedef String ConvertTo;
   enum { ExactSize = true };
   static int size(const Character::SpecialCharacter)
   {
      return 1;
   }
   
   static inline void appendTo(const Character::SpecialCharacter c, Character *&out)
   {
      *out++ = c;
   }
};

template <>
struct Concatenable<CharacterRef> 
      : private AbstractConcatenable
{
   typedef CharacterRef type;
   typedef String ConvertTo;
   enum { ExactSize = true };
   static int size(CharacterRef)
   {
      return 1;
   }
   
   static inline void appendTo(CharacterRef c, Character *&out)
   {
      *out++ = Character(c);
   }
};

template <> struct Concatenable<Latin1String> : private AbstractConcatenable
{
   typedef Latin1String type;
   typedef String ConvertTo;
   enum { ExactSize = true };
   static int size(const Latin1String a)
   {
      return a.size();
   }
   
   static inline void appendTo(const Latin1String a, Character *&out)
   {
      appendLatin1To(a.latin1(), a.size(), out);
      out += a.size();
   }
   static inline void appendTo(const Latin1String a, char *&out)
   {
      if (const char *data = a.getRawData()) {
         memcpy(out, data, a.size());
         out += a.size();
      }
   }
};

template <> struct Concatenable<String> : private AbstractConcatenable
{
   typedef String type;
   typedef String ConvertTo;
   enum { ExactSize = true };
   static int size(const String &a) 
   {
      return a.size();
   }
   
   static inline void appendTo(const String &a, Character *&out)
   {
      const int n = a.size();
      memcpy(out, reinterpret_cast<const char*>(a.getConstRawData()), sizeof(Character) * n);
      out += n;
   }
   
   static inline void appendTo(const String &a, char *&out)
   {
      const int n = a.size();
      memcpy(out, reinterpret_cast<const char*>(a.getConstRawData()), sizeof(Character) * n);
      out += n;
   }
};

template <> struct Concatenable<StringRef> : private AbstractConcatenable
{
   typedef StringRef type;
   typedef String ConvertTo;
   enum { ExactSize = true };
   static int size(const StringRef &a)
   {
      return a.size();
   }
   
   static inline void appendTo(const StringRef &a, Character *&out)
   {
      const int n = a.size();
      memcpy(out, reinterpret_cast<const char*>(a.getConstRawData()), sizeof(Character) * n);
      out += n;
   }
};

template <int N> struct Concatenable<const char[N]> : private AbstractConcatenable
{
   typedef const char type[N];
   typedef ByteArray ConvertTo;
   enum { ExactSize = false };
   static int size(const char[N]) { return N - 1; }
   static inline void appendTo(const char a[N], char *&out)
   {
      while (*a)
         *out++ = *a++;
   }
};

template <int N> struct Concatenable<char[N]> : Concatenable<const char[N]>
{
   typedef char type[N];
};

template <> struct Concatenable<const char *> : private AbstractConcatenable
{
   typedef const char *type;
   typedef ByteArray ConvertTo;
   enum { ExactSize = false };
   static int size(const char *a)
   {
      return pdk::strlen(a);
   }
   
   static inline void appendTo(const char *a, char *&out)
   {
      if (!a) {
         return;
      }
      while (*a) {
         *out++ = *a++;
      }
   }
};

template <> struct Concatenable<char *> : Concatenable<const char*>
{
   typedef char *type;
};

template <> struct Concatenable<ByteArray> : private AbstractConcatenable
{
   using type = ByteArray;
   using ConvertTo = ByteArray;
   enum { ExactSize = false };
   static int size(const ByteArray &ba)
   {
      return ba.size();
   }
   static inline void appendTo(const ByteArray &ba, char *&out)
   {
      const char *a = ba.getConstRawData();
      const char * const end = ba.end();
      while (a != end)
         *out++ = *a++;
   }
};


template <typename A, typename B>
struct Concatenable< StringBuilder<A, B> >
{
   using type = StringBuilder<A, B>;
   using ConvertTo = typename stringbuilder::ConvertToTypeHelper<typename Concatenable<A>::ConvertTo, typename Concatenable<B>::ConvertTo>::ConvertTo;
   enum { ExactSize = Concatenable<A>::ExactSize && Concatenable<B>::ExactSize };
   static int size(const type &p)
   {
      return Concatenable<A>::size(p.m_a) + Concatenable<B>::size(p.m_b);
   }
   
   template<typename T> static inline void appendTo(const type &p, T *&out)
   {
      Concatenable<A>::appendTo(p.m_a, out);
      Concatenable<B>::appendTo(p.m_b, out);
   }
};

template <typename A, typename B>
StringBuilder<typename Concatenable<A>::type, typename Concatenable<B>::type>
operator%(const A &a, const B &b)
{
   return StringBuilder<typename Concatenable<A>::type, typename Concatenable<B>::type>(a, b);
}

template <typename A, typename B>
StringBuilder<typename Concatenable<A>::type, typename Concatenable<B>::type>
operator+(const A &a, const B &b)
{
   return StringBuilder<typename Concatenable<A>::type, typename Concatenable<B>::type>(a, b);
}

namespace stringbuilder {
template <typename A, typename B>
ByteArray &appendToByteArray(ByteArray &a, const StringBuilder<A, B> &b, char)
{
   // append 8-bit data to a byte array
   int len = a.size() + Concatenable< StringBuilder<A, B> >::size(b);
   a.reserve(len);
   char *it = a.getRawData() + a.size();
   Concatenable< StringBuilder<A, B> >::appendTo(b, it);
   a.resize(len); //we need to resize after the appendTo for the case str+=foo+str
   return a;
}

template <typename A, typename B>
ByteArray &appendToByteArray(ByteArray &a, const StringBuilder<A, B> &b, Character)
{
   return a += String(b).toUtf8();
}
}

template <typename A, typename B>
ByteArray &operator+=(ByteArray &a, const StringBuilder<A, B> &b)
{
   return stringbuilder::appendToByteArray(a, b,
                                           typename Concatenable< StringBuilder<A, B> >::ConvertTo::value_type());
}

template <typename A, typename B>
String &operator+=(String &a, const StringBuilder<A, B> &b)
{
   int len = a.size() + Concatenable<StringBuilder<A, B>>::size(b);
   a.reserve(len);
   Character *it = a.getRawData() + a.size();
   Concatenable< StringBuilder<A, B> >::appendTo(b, it);
   a.resize(int(it - a.getConstRawData())); //may be smaller than len if there was conversion from utf8
   return a;
}


} // lang
} // pdk

#endif // PDK_M_BASE_LANG_STRING_BUILDER_H
