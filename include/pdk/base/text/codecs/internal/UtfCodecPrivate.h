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
// Created by softboy on 2018/02/01.

#ifndef PDK_M_BASE_TEXT_CODECS_INTERNAL_UTF_CODEC_PRIVATE_H
#define PDK_M_BASE_TEXT_CODECS_INTERNAL_UTF_CODEC_PRIVATE_H

#include "pdk/base/text/codecs/TextCodec.h"
#include "pdk/base/text/codecs/internal/TextCodecPrivate.h"

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

using pdk::lang::Character;

struct Utf8BaseTraits
{
   static const bool sm_isTrusted = false;
   static const bool sm_allowNonCharacters = true;
   static const bool sm_skipAsciiHandling = false;
   static const int sm_error = -1;
   static const int sm_endOfString = -2;
   
   static bool isValidCharacter(uint u)
   {
      return int(u) >= 0;
   }
   
   static void appendByte(uchar *&ptr, uchar b)
   {
      *ptr++ = b;
   }
   
   static uchar peekByte(const uchar *ptr, int n = 0)
   {
      return ptr[n];
   }
   
   static pdk::ptrdiff availableBytes(const uchar *ptr, const uchar *end)
   {
      return end - ptr;
   }
   
   static void advanceByte(const uchar *&ptr, int n = 1)
   {
      ptr += n;
   }
   
   static void appendUtf16(ushort *&ptr, ushort uc)
   {
      *ptr++ = uc;
   }
   
   static void appendUcs4(ushort *&ptr, uint uc)
   {
      appendUtf16(ptr, Character::getHighSurrogate(uc));
      appendUtf16(ptr, Character::getLowSurrogate(uc));
   }
   
   static ushort peekUtf16(const ushort *ptr, int n = 0)
   {
      return ptr[n];
   }
   
   static pdk::ptrdiff availableUtf16(const ushort *ptr, const ushort *end)
   {
      return end - ptr;
   }
   
   static void advanceUtf16(const ushort *&ptr, int n = 1)
   {
      ptr += n;
   }
   
   // it's possible to output to UCS-4 too
   static void appendUtf16(uint *&ptr, ushort uc)
   {
      *ptr++ = uc;
   }
   
   static void appendUcs4(uint *&ptr, uint uc)
   {
      *ptr++ = uc;
   }
};

struct Utf8BaseTraitsNoAscii : public Utf8BaseTraits
{
   static const bool sm_skipAsciiHandling = true;
};

namespace Utf8Functions
{
template <typename Traits, typename OutputPtr, typename InputPtr> inline
int toUtf8(ushort u, OutputPtr &dst, InputPtr &src, InputPtr end)
{
   if (!Traits::sm_skipAsciiHandling && u < 0x80) {
      // U+0000 to U+007F (US-ASCII) - one byte
      Traits::appendByte(dst, uchar(u));
      return 0;
   } else if (u < 0x0800) {
      // U+0080 to U+07FF - two bytes
      // first of two bytes
      Traits::appendByte(dst, 0xc0 | uchar(u >> 6));
   } else {
      if (!Character::isSurrogate(u)) {
         // U+0800 to U+FFFF (except U+D800-U+DFFF) - three bytes
         if (!Traits::sm_allowNonCharacters && Character::isNonCharacter(u))
            return Traits::sm_error;
         
         // first of three bytes
         Traits::appendByte(dst, 0xe0 | uchar(u >> 12));
      } else {
         // U+10000 to U+10FFFF - four bytes
         // need to get one extra codepoint
         if (Traits::availableUtf16(src, end) == 0)
            return Traits::sm_endOfString;
         
         ushort low = Traits::peekUtf16(src);
         if (!Character::isHighSurrogate(u))
            return Traits::sm_error;
         if (!Character::isLowSurrogate(low))
            return Traits::sm_error;
         
         Traits::advanceUtf16(src);
         uint ucs4 = Character::surrogateToUcs4(u, low);
         
         if (!Traits::sm_allowNonCharacters && Character::isNonCharacter(ucs4))
            return Traits::sm_error;
         
         // first byte
         Traits::appendByte(dst, 0xf0 | (uchar(ucs4 >> 18) & 0xf));
         
         // second of four bytes
         Traits::appendByte(dst, 0x80 | (uchar(ucs4 >> 12) & 0x3f));
         
         // for the rest of the bytes
         u = ushort(ucs4);
      }
      
      // second to last byte
      Traits::appendByte(dst, 0x80 | (uchar(u >> 6) & 0x3f));
   }
   
   // last byte
   Traits::appendByte(dst, 0x80 | (u & 0x3f));
   return 0;
}

inline bool isContinuationByte(uchar b)
{
   return (b & 0xc0) == 0x80;
}

/// returns the number of characters consumed (including \a b) in case of success;
/// returns negative in case of error: Traits::sm_error or Traits::sm_endOfString
template <typename Traits, typename OutputPtr, typename InputPtr> inline
int fromUtf8(uchar b, OutputPtr &dst, InputPtr &src, InputPtr end)
{
   int charsNeeded;
   uint min_uc;
   uint uc;
   
   if (!Traits::sm_skipAsciiHandling && b < 0x80) {
      // US-ASCII
      Traits::appendUtf16(dst, b);
      return 1;
   }
   
   if (!Traits::sm_isTrusted && PDK_UNLIKELY(b <= 0xC1)) {
      // an UTF-8 first character must be at least 0xC0
      // however, all 0xC0 and 0xC1 first bytes can only produce overlong sequences
      return Traits::sm_error;
   } else if (b < 0xe0) {
      charsNeeded = 2;
      min_uc = 0x80;
      uc = b & 0x1f;
   } else if (b < 0xf0) {
      charsNeeded = 3;
      min_uc = 0x800;
      uc = b & 0x0f;
   } else if (b < 0xf5) {
      charsNeeded = 4;
      min_uc = 0x10000;
      uc = b & 0x07;
   } else {
      // the last Unicode character is U+10FFFF
      // it's encoded in UTF-8 as "\xF4\x8F\xBF\xBF"
      // therefore, a byte higher than 0xF4 is not the UTF-8 first byte
      return Traits::sm_error;
   }
   
   int bytesAvailable = Traits::availableBytes(src, end);
   if (PDK_UNLIKELY(bytesAvailable < charsNeeded - 1)) {
      // it's possible that we have an error instead of just unfinished bytes
      if (bytesAvailable > 0 && !isContinuationByte(Traits::peekByte(src, 0)))
         return Traits::sm_error;
      if (bytesAvailable > 1 && !isContinuationByte(Traits::peekByte(src, 1)))
         return Traits::sm_error;
      return Traits::sm_endOfString;
   }
   
   // first continuation character
   b = Traits::peekByte(src, 0);
   if (!isContinuationByte(b))
      return Traits::sm_error;
   uc <<= 6;
   uc |= b & 0x3f;
   
   if (charsNeeded > 2) {
      // second continuation character
      b = Traits::peekByte(src, 1);
      if (!isContinuationByte(b))
         return Traits::sm_error;
      uc <<= 6;
      uc |= b & 0x3f;
      
      if (charsNeeded > 3) {
         // third continuation character
         b = Traits::peekByte(src, 2);
         if (!isContinuationByte(b))
            return Traits::sm_error;
         uc <<= 6;
         uc |= b & 0x3f;
      }
   }
   
   // we've decoded something; safety-check it
   if (!Traits::sm_isTrusted) {
      if (uc < min_uc)
         return Traits::sm_error;
      if (Character::isSurrogate(uc) || uc > Character::LastValidCodePoint)
         return Traits::sm_error;
      if (!Traits::sm_allowNonCharacters && Character::isNonCharacter(uc))
         return Traits::sm_error;
   }
   
   // write the UTF-16 sequence
   if (!Character::requiresSurrogates(uc)) {
      // UTF-8 decoded and no surrogates are required
      // detach if necessary
      Traits::appendUtf16(dst, ushort(uc));
   } else {
      // UTF-8 decoded to something that requires a surrogate pair
      Traits::appendUcs4(dst, uc);
   }
   
   Traits::advanceByte(src, charsNeeded - 1);
   return charsNeeded;
}
}

enum DataEndianness
{
   DetectEndianness,
   BigEndianness,
   LittleEndianness
};

struct Utf8
{
   static Character *convertToUnicode(Character *, const char *, int) noexcept;
   static String convertToUnicode(const char *, int);
   static String convertToUnicode(const char *, int, TextCodec::ConverterState *);
   static ByteArray convertFromUnicode(const Character *, int);
   static ByteArray convertFromUnicode(const Character *, int, TextCodec::ConverterState *);
};

struct Utf16
{
   static String convertToUnicode(const char *, int, TextCodec::ConverterState *, DataEndianness = DetectEndianness);
   static ByteArray convertFromUnicode(const Character *, int, TextCodec::ConverterState *, DataEndianness = DetectEndianness);
};

struct Utf32
{
   static String convertToUnicode(const char *, int, TextCodec::ConverterState *, DataEndianness = DetectEndianness);
   static ByteArray convertFromUnicode(const Character *, int, TextCodec::ConverterState *, DataEndianness = DetectEndianness);
};

class Utf8Codec : public TextCodec
{
public:
   ~Utf8Codec();
   
   ByteArray name() const override;
   int mibEnum() const override;
   
   String convertToUnicode(const char *, int, ConverterState *) const override;
   ByteArray convertFromUnicode(const Character *, int, ConverterState *) const override;
   void convertToUnicode(String *target, const char *, int, ConverterState *) const;
};

class Utf16Codec : public TextCodec
{
protected:
public:
   Utf16Codec() { e = DetectEndianness; }
   ~Utf16Codec();
   
   ByteArray name() const override;
   std::list<ByteArray> aliases() const override;
   int mibEnum() const override;
   
   String convertToUnicode(const char *, int, ConverterState *) const override;
   ByteArray convertFromUnicode(const Character *, int, ConverterState *) const override;
   
protected:
   DataEndianness e;
};

class Utf16BECodec : public Utf16Codec
{
public:
   Utf16BECodec() : Utf16Codec() { e = BigEndianness; }
   ByteArray name() const override;
   std::list<ByteArray> aliases() const override;
   int mibEnum() const override;
};

class Utf16LECodec : public Utf16Codec
{
public:
   Utf16LECodec() : Utf16Codec() { e = LittleEndianness; }
   ByteArray name() const override;
   std::list<ByteArray> aliases() const override;
   int mibEnum() const override;
};

class Utf32Codec : public TextCodec
{
public:
   Utf32Codec() { e = DetectEndianness; }
   ~Utf32Codec();
   
   ByteArray name() const override;
   std::list<ByteArray> aliases() const override;
   int mibEnum() const override;
   
   String convertToUnicode(const char *, int, ConverterState *) const override;
   ByteArray convertFromUnicode(const Character *, int, ConverterState *) const override;
   
protected:
   DataEndianness e;
};

class Utf32BECodec : public Utf32Codec
{
public:
   Utf32BECodec() : Utf32Codec() { e = BigEndianness; }
   ByteArray name() const override;
   std::list<ByteArray> aliases() const override;
   int mibEnum() const override;
};

class Utf32LECodec : public Utf32Codec
{
public:
   Utf32LECodec() : Utf32Codec() { e = LittleEndianness; }
   ByteArray name() const override;
   std::list<ByteArray> aliases() const override;
   int mibEnum() const override;
};

} // internal
} // codecs
} // text
} // pdk

#endif // PDK_M_BASE_TEXT_CODECS_INTERNAL_UTF_CODEC_PRIVATE_H
