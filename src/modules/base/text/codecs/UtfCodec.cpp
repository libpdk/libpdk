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

#include "pdk/base/text/codecs/internal/UtfCodecPrivate.h"
#include "pdk/global/SysInfo.h"
#include "pdk/global/Endian.h"
#include "pdk/kernel/Algorithms.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/lang/Character.h"
#include "pdk/base/lang/StringIterator.h"
#include "pdk/pal/kernel/Simd.h"

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

using pdk::ds::ByteArray;
using pdk::lang::String;
using pdk::lang::StringIterator;
using pdk::lang::Character;
using pdk::lang::Latin1Character;

enum { Endian = 0, Data = 1 };

static const uchar utf8bom[] = { 0xef, 0xbb, 0xbf };

#if (defined(__SSE2__) && defined(PDK_COMPILER_SUPPORTS_SSE2)) \
   || (defined(__ARM_NEON__) && defined(PDK_PROCESSOR_ARM_64))
static PDK_ALWAYS_INLINE uint bit_scan_reverse(unsigned v) noexcept
{
   uint result = pdk::count_leading_zero_bits(v);
   // Now Invert the result: clz will count *down* from the msb to the lsb, so the msb index is 31
   // and the lsb index is 0. The result for _bit_scan_reverse is expected to be the index when
   // counting up: msb index is 0 (because it starts there), and the lsb index is 31.
   result ^= sizeof(unsigned) * 8 - 1;
   return result;
}
#endif

#if defined(__SSE2__) && defined(PDK_COMPILER_SUPPORTS_SSE2)
static inline bool simd_encode_ascii(uchar *&dst, const char16_t *&nextAscii, const char16_t *&src, const char16_t *end)
{
   // do sixteen characters at a time
   for ( ; end - src >= 16; src += 16, dst += 16) {
      __m128i data1 = _mm_loadu_si128((const __m128i*)src);
      __m128i data2 = _mm_loadu_si128(1+(const __m128i*)src);
      
      
      // check if everything is ASCII
      // the highest ASCII value is U+007F
      // Do the packing directly:
      // The PACKUSWB instruction has packs a signed 16-bit integer to an unsigned 8-bit
      // with saturation. That is, anything from 0x0100 to 0x7fff is saturated to 0xff,
      // while all negatives (0x8000 to 0xffff) get saturated to 0x00. To detect non-ASCII,
      // we simply do a signed greater-than comparison to 0x00. That means we detect NULs as
      // "non-ASCII", but it's an acceptable compromise.
      __m128i packed = _mm_packus_epi16(data1, data2);
      __m128i nonAscii = _mm_cmpgt_epi8(packed, _mm_setzero_si128());
      
      // store, even if there are non-ASCII characters here
      _mm_storeu_si128((__m128i*)dst, packed);
      
      // n will contain 1 bit set per character in [data1, data2] that is non-ASCII (or NUL)
      char16_t n = ~_mm_movemask_epi8(nonAscii);
      if (n) {
         // find the next probable ASCII character
         // we don't want to load 32 bytes again in this loop if we know there are non-ASCII
         // characters still coming
         nextAscii = src + bit_scan_reverse(n) + 1;
         
         n = pdk::count_trailing_zero_bits(n);
         dst += n;
         src += n;
         return false;
      }
   }
   return src == end;
}

static inline bool simd_decode_ascii(char16_t *&dst, const uchar *&nextAscii, const uchar *&src, const uchar *end)
{
   // do sixteen characters at a time
   for ( ; end - src >= 16; src += 16, dst += 16) {
      __m128i data = _mm_loadu_si128((const __m128i*)src);
      
#ifdef __AVX2__
      const int BitSpacing = 2;
      // load and zero extend to an YMM register
      const __m256i extended = _mm256_cvtepu8_epi16(data);
      
      uint n = _mm256_movemask_epi8(extended);
      if (!n) {
         // store
         _mm256_storeu_si256((__m256i*)dst, extended);
         continue;
      }
#else
      const int BitSpacing = 1;
      
      // check if everything is ASCII
      // movemask extracts the high bit of every byte, so n is non-zero if something isn't ASCII
      uint n = _mm_movemask_epi8(data);
      if (!n) {
         // unpack
         _mm_storeu_si128((__m128i*)dst, _mm_unpacklo_epi8(data, _mm_setzero_si128()));
         _mm_storeu_si128(1+(__m128i*)dst, _mm_unpackhi_epi8(data, _mm_setzero_si128()));
         continue;
      }
#endif
      
      // copy the front part that is still ASCII
      while (!(n & 1)) {
         *dst++ = *src++;
         n >>= BitSpacing;
      }
      
      // find the next probable ASCII character
      // we don't want to load 16 bytes again in this loop if we know there are non-ASCII
      // characters still coming
      n = bit_scan_reverse(n);
      nextAscii = src + (n / BitSpacing) + 1;
      return false;
      
   }
   return src == end;
}
#else
static inline bool simd_encode_ascii(uchar *, const char16_t *, const char16_t *, const char16_t *)
{
   return false;
}

static inline bool simd_decode_ascii(char16_t *, const uchar *, const uchar *, const uchar *)
{
   return false;
}
#endif

ByteArray Utf8::convertFromUnicode(const Character *uc, int len)
{
   // create a ByteArray with the worst case scenario size
   ByteArray result(len * 3, pdk::Uninitialized);
   uchar *dst = reinterpret_cast<uchar *>(const_cast<char *>(result.getConstRawData()));
   const char16_t *src = reinterpret_cast<const char16_t *>(uc);
   const char16_t *const end = src + len;
   
   while (src != end) {
      const char16_t *nextAscii = end;
      if (simd_encode_ascii(dst, nextAscii, src, end))
         break;
      
      do {
         char16_t uc = *src++;
         int res = Utf8Functions::toUtf8<Utf8BaseTraits>(uc, dst, src, end);
         if (res < 0) {
            // encoding error - append '?'
            *dst++ = '?';
         }
      } while (src < nextAscii);
   }
   
   result.truncate(dst - reinterpret_cast<uchar *>(const_cast<char *>(result.getConstRawData())));
   return result;
}

ByteArray Utf8::convertFromUnicode(const Character *uc, int len, TextCodec::ConverterState *state)
{
   uchar replacement = '?';
   int rlen = 3*len;
   int surrogate_high = -1;
   if (state) {
      if (state->m_flags & TextCodec::ConversionFlag::ConvertInvalidToNull)
         replacement = 0;
      if (!(state->m_flags & TextCodec::ConversionFlag::IgnoreHeader))
         rlen += 3;
      if (state->m_remainingChars)
         surrogate_high = state->m_stateData[0];
   }
   
   
   ByteArray rstr(rlen, pdk::Uninitialized);
   uchar *cursor = reinterpret_cast<uchar *>(const_cast<char *>(rstr.getConstRawData()));
   const char16_t *src = reinterpret_cast<const char16_t *>(uc);
   const char16_t *const end = src + len;
   
   int invalid = 0;
   if (state && !(state->m_flags & TextCodec::ConversionFlag::IgnoreHeader)) {
      // append UTF-8 BOM
      *cursor++ = utf8bom[0];
      *cursor++ = utf8bom[1];
      *cursor++ = utf8bom[2];
   }
   
   const char16_t *nextAscii = src;
   while (src != end) {
      int res;
      char16_t uc;
      if (surrogate_high != -1) {
         uc = surrogate_high;
         surrogate_high = -1;
         res = Utf8Functions::toUtf8<Utf8BaseTraits>(uc, cursor, src, end);
      } else {
         if (src >= nextAscii && simd_encode_ascii(cursor, nextAscii, src, end))
            break;
         
         uc = *src++;
         res = Utf8Functions::toUtf8<Utf8BaseTraits>(uc, cursor, src, end);
      }
      if (PDK_LIKELY(res >= 0))
         continue;
      
      if (res == Utf8BaseTraits::sm_error) {
         // encoding error
         ++invalid;
         *cursor++ = replacement;
      } else if (res == Utf8BaseTraits::sm_endOfString) {
         surrogate_high = uc;
         break;
      }
   }
   
   rstr.resize(cursor - (const uchar*)rstr.getConstRawData());
   if (state) {
      state->m_invalidChars += invalid;
      state->m_flags |= TextCodec::ConversionFlag::IgnoreHeader;
      state->m_remainingChars = 0;
      if (surrogate_high >= 0) {
         state->m_remainingChars = 1;
         state->m_stateData[0] = surrogate_high;
      }
   }
   return rstr;
}

String Utf8::convertToUnicode(const char *chars, int len)
{
   // UTF-8 to UTF-16 always needs the exact same number of words or less:
   //    UTF-8     UTF-16
   //   1 byte     1 word
   //   2 bytes    1 word
   //   3 bytes    1 word
   //   4 bytes    2 words (one surrogate pair)
   // That is, we'll use the full buffer if the input is US-ASCII (1-byte UTF-8),
   // half the buffer for U+0080-U+07FF text (e.g., Greek, Cyrillic, Arabic) or
   // non-BMP text, and one third of the buffer for U+0800-U+FFFF text (e.g, CJK).
   //
   // The table holds for invalid sequences too: we'll insert one replacement char
   // per invalid byte.
   String result(len, pdk::Uninitialized);
   Character *data = const_cast<Character *>(result.getConstRawData()); // we know we're not shared
   const Character *end = convertToUnicode(data, chars, len);
   result.truncate(end - data);
   return result;
}

/*!
    Converts the UTF-8 sequence of \a len octets beginning at \a chars to
    a sequence of Character starting at \a buffer. The buffer is expected to be
    large enough to hold the result. An upper bound for the size of the
    buffer is \a len Characters.
    
    If, during decoding, an error occurs, a Character::ReplacementCharacter is
    written.
    
    Returns a pointer to one past the last Character written.
    
    This function never throws.
*/

Character *Utf8::convertToUnicode(Character *buffer, const char *chars, int len) noexcept
{
   char16_t *dst = reinterpret_cast<char16_t *>(buffer);
   const uchar *src = reinterpret_cast<const uchar *>(chars);
   const uchar *end = src + len;
   
   // attempt to do a full decoding in SIMD
   const uchar *nextAscii = end;
   if (!simd_decode_ascii(dst, nextAscii, src, end)) {
      // at least one non-ASCII entry
      // check if we failed to decode the UTF-8 BOM; if so, skip it
      if (PDK_UNLIKELY(src == reinterpret_cast<const uchar *>(chars))
          && end - src >= 3
          && PDK_UNLIKELY(src[0] == utf8bom[0] && src[1] == utf8bom[1] && src[2] == utf8bom[2])) {
         src += 3;
      }
      
      while (src < end) {
         nextAscii = end;
         if (simd_decode_ascii(dst, nextAscii, src, end)) {
            break;
         }
         do {
            uchar b = *src++;
            int res = Utf8Functions::fromUtf8<Utf8BaseTraits>(b, dst, src, end);
            if (res < 0) {
               // decoding error
               *dst++ = Character::ReplacementCharacter;
            }
         } while (src < nextAscii);
      }
   }
   
   return reinterpret_cast<Character *>(dst);
}

String Utf8::convertToUnicode(const char *chars, int len, TextCodec::ConverterState *state)
{
   bool headerdone = false;
   char16_t replacement = Character::ReplacementCharacter;
   int invalid = 0;
   int res;
   uchar ch = 0;
   
   // See above for buffer requirements for stateless decoding. However, that
   // fails if the state is not empty. The following situations can add to the
   // requirements:
   //  state contains      chars starts with           requirement
   //   1 of 2 bytes       valid continuation          0
   //   2 of 3 bytes       same                        0
   //   3 bytes of 4       same                        +1 (need to insert surrogate pair)
   //   1 of 2 bytes       invalid continuation        +1 (need to insert replacement and restart)
   //   2 of 3 bytes       same                        +1 (same)
   //   3 of 4 bytes       same                        +1 (same)
   String result(len + 1, pdk::Uninitialized);
   
   char16_t *dst = reinterpret_cast<char16_t *>(const_cast<Character *>(result.getConstRawData()));
   const uchar *src = reinterpret_cast<const uchar *>(chars);
   const uchar *end = src + len;
   
   if (state) {
      if (state->m_flags & TextCodec::ConversionFlag::IgnoreHeader)
         headerdone = true;
      if (state->m_flags & TextCodec::ConversionFlag::ConvertInvalidToNull)
         replacement = Character::Null;
      if (state->m_remainingChars) {
         // handle incoming state first
         uchar remainingCharsData[4]; // longest UTF-8 sequence possible
         int remainingCharsCount = state->m_remainingChars;
         int newCharsToCopy = std::min<int>(sizeof(remainingCharsData) - remainingCharsCount, end - src);
         
         memset(remainingCharsData, 0, sizeof(remainingCharsData));
         memcpy(remainingCharsData, &state->m_stateData[0], remainingCharsCount);
         memcpy(remainingCharsData + remainingCharsCount, src, newCharsToCopy);
         
         const uchar *begin = &remainingCharsData[1];
         res = Utf8Functions::fromUtf8<Utf8BaseTraits>(remainingCharsData[0], dst, begin,
               static_cast<const uchar *>(remainingCharsData) + remainingCharsCount + newCharsToCopy);
         if (res == Utf8BaseTraits::sm_error || (res == Utf8BaseTraits::sm_endOfString && len == 0)) {
            // special case for len == 0:
            // if we were supplied an empty string, terminate the previous, unfinished sequence with error
            ++invalid;
            *dst++ = replacement;
         } else if (res == Utf8BaseTraits::sm_endOfString) {
            // if we got EndOfString again, then there were too few bytes in src;
            // copy to our state and return
            state->m_remainingChars = remainingCharsCount + newCharsToCopy;
            memcpy(&state->m_stateData[0], remainingCharsData, state->m_remainingChars);
            return String();
         } else if (!headerdone && res >= 0) {
            // eat the UTF-8 BOM
            headerdone = true;
            if (dst[-1] == 0xfeff)
               --dst;
         }
         
         // adjust src now that we have maybe consumed a few chars
         if (res >= 0) {
            PDK_ASSERT(res > remainingCharsCount);
            src += res - remainingCharsCount;
         }
      }
   }
   
   // main body, stateless decoding
   res = 0;
   const uchar *nextAscii = src;
   const uchar *start = src;
   while (res >= 0 && src < end) {
      if (src >= nextAscii && simd_decode_ascii(dst, nextAscii, src, end))
         break;
      
      ch = *src++;
      res = Utf8Functions::fromUtf8<Utf8BaseTraits>(ch, dst, src, end);
      if (!headerdone && res >= 0) {
         headerdone = true;
         if (src == start + 3) { // 3 == sizeof(utf8-bom)
            // eat the UTF-8 BOM (it can only appear at the beginning of the string).
            if (dst[-1] == 0xfeff)
               --dst;
         }
      }
      if (res == Utf8BaseTraits::sm_error) {
         res = 0;
         ++invalid;
         *dst++ = replacement;
      }
   }
   
   if (!state && res == Utf8BaseTraits::sm_endOfString) {
      // unterminated UTF sequence
      *dst++ = Character::ReplacementCharacter;
      while (src++ < end)
         *dst++ = Character::ReplacementCharacter;
   }
   
   result.truncate(dst - (const char16_t *)result.unicode());
   if (state) {
      state->m_invalidChars += invalid;
      if (headerdone)
         state->m_flags |= TextCodec::ConversionFlag::IgnoreHeader;
      if (res == Utf8BaseTraits::sm_endOfString) {
         --src; // unread the byte in ch
         state->m_remainingChars = end - src;
         std::memcpy(&state->m_stateData[0], src, end - src);
      } else {
         state->m_remainingChars = 0;
      }
   }
   return result;
}

ByteArray Utf16::convertFromUnicode(const Character *uc, int len, TextCodec::ConverterState *state, DataEndianness e)
{
   DataEndianness endian = e;
   int length =  2*len;
   if (!state || (!(state->m_flags & TextCodec::ConversionFlag::IgnoreHeader))) {
      length += 2;
   }
   if (e == DetectEndianness) {
      endian = (SysInfo::ByteOrder == SysInfo::BigEndian) ? BigEndianness : LittleEndianness;
   }
   
   ByteArray d;
   d.resize(length);
   char *data = d.getRawData();
   if (!state || !(state->m_flags & TextCodec::ConversionFlag::IgnoreHeader)) {
      Character bom(Character::ByteOrderMark);
      if (endian == BigEndianness) {
         data[0] = bom.getRow();
         data[1] = bom.getCell();
      } else {
         data[0] = bom.getCell();
         data[1] = bom.getRow();
      }
      data += 2;
   }
   if (endian == BigEndianness) {
      for (int i = 0; i < len; ++i) {
         *(data++) = uc[i].getRow();
         *(data++) = uc[i].getCell();
      }
   } else {
      for (int i = 0; i < len; ++i) {
         *(data++) = uc[i].getCell();
         *(data++) = uc[i].getRow();
      }
   }
   
   if (state) {
      state->m_remainingChars = 0;
      state->m_flags |= TextCodec::ConversionFlag::IgnoreHeader;
   }
   return d;
}

String Utf16::convertToUnicode(const char *chars, int len, TextCodec::ConverterState *state, DataEndianness e)
{
   DataEndianness endian = e;
   bool half = false;
   uchar buf = 0;
   bool headerdone = false;
   if (state) {
      headerdone = state->m_flags & TextCodec::ConversionFlag::IgnoreHeader;
      if (endian == DetectEndianness)
         endian = (DataEndianness)state->m_stateData[Endian];
      if (state->m_remainingChars) {
         half = true;
         buf = state->m_stateData[Data];
      }
   }
   if (headerdone && endian == DetectEndianness)
      endian = (SysInfo::ByteOrder == SysInfo::BigEndian) ? BigEndianness : LittleEndianness;
   
   String result(len, pdk::Uninitialized); // worst case
   Character *qch = (Character *)result.getRawData();
   while (len--) {
      if (half) {
         Character ch;
         if (endian == LittleEndianness) {
            ch.setRow(*chars++);
            ch.setCell(buf);
         } else {
            ch.setRow(buf);
            ch.setCell(*chars++);
         }
         if (!headerdone) {
            headerdone = true;
            if (endian == DetectEndianness) {
               if (ch == Character::ByteOrderSwapped) {
                  endian = LittleEndianness;
               } else if (ch == Character::ByteOrderMark) {
                  endian = BigEndianness;
               } else {
                  if (SysInfo::ByteOrder == SysInfo::BigEndian) {
                     endian = BigEndianness;
                  } else {
                     endian = LittleEndianness;
                     ch = Character((ch.unicode() >> 8) | ((ch.unicode() & 0xff) << 8));
                  }
                  *qch++ = ch;
               }
            } else if (ch != Character::ByteOrderMark) {
               *qch++ = ch;
            }
         } else {
            *qch++ = ch;
         }
         half = false;
      } else {
         buf = *chars++;
         half = true;
      }
   }
   result.truncate(qch - result.unicode());
   
   if (state) {
      if (headerdone)
         state->m_flags |= TextCodec::ConversionFlag::IgnoreHeader;
      state->m_stateData[Endian] = endian;
      if (half) {
         state->m_remainingChars = 1;
         state->m_stateData[Data] = buf;
      } else {
         state->m_remainingChars = 0;
         state->m_stateData[Data] = 0;
      }
   }
   return result;
}

ByteArray Utf32::convertFromUnicode(const Character *uc, int len, TextCodec::ConverterState *state, DataEndianness e)
{
   DataEndianness endian = e;
   int length =  4*len;
   if (!state || (!(state->m_flags & TextCodec::ConversionFlag::IgnoreHeader))) {
      length += 4;
   }
   if (e == DetectEndianness) {
      endian = (SysInfo::ByteOrder == SysInfo::BigEndian) ? BigEndianness : LittleEndianness;
   }
   
   ByteArray d(length, pdk::Uninitialized);
   char *data = d.getRawData();
   if (!state || !(state->m_flags & TextCodec::ConversionFlag::IgnoreHeader)) {
      if (endian == BigEndianness) {
         data[0] = 0;
         data[1] = 0;
         data[2] = (char)0xfe;
         data[3] = (char)0xff;
      } else {
         data[0] = (char)0xff;
         data[1] = (char)0xfe;
         data[2] = 0;
         data[3] = 0;
      }
      data += 4;
   }
   
   StringIterator i(uc, uc + len);
   if (endian == BigEndianness) {
      while (i.hasNext()) {
         uint cp = i.next();
         *(data++) = cp >> 24;
         *(data++) = (cp >> 16) & 0xff;
         *(data++) = (cp >> 8) & 0xff;
         *(data++) = cp & 0xff;
      }
   } else {
      while (i.hasNext()) {
         uint cp = i.next();
         
         *(data++) = cp & 0xff;
         *(data++) = (cp >> 8) & 0xff;
         *(data++) = (cp >> 16) & 0xff;
         *(data++) = cp >> 24;
      }
   }
   
   if (state) {
      state->m_remainingChars = 0;
      state->m_flags |= TextCodec::ConversionFlag::IgnoreHeader;
   }
   return d;
}

String Utf32::convertToUnicode(const char *chars, int len, TextCodec::ConverterState *state, DataEndianness e)
{
   DataEndianness endian = e;
   uchar tuple[4];
   int num = 0;
   bool headerdone = false;
   if (state) {
      headerdone = state->m_flags & TextCodec::ConversionFlag::IgnoreHeader;
      if (endian == DetectEndianness) {
         endian = (DataEndianness)state->m_stateData[Endian];
      }
      num = state->m_remainingChars;
      memcpy(tuple, &state->m_stateData[Data], 4);
   }
   if (headerdone && endian == DetectEndianness)
      endian = (SysInfo::ByteOrder == SysInfo::BigEndian) ? BigEndianness : LittleEndianness;
   
   String result;
   result.resize((num + len) >> 2 << 1); // worst case
   Character *qch = (Character *)result.getRawData();
   
   const char *end = chars + len;
   while (chars < end) {
      tuple[num++] = *chars++;
      if (num == 4) {
         if (!headerdone) {
            if (endian == DetectEndianness) {
               if (tuple[0] == 0xff && tuple[1] == 0xfe && tuple[2] == 0 && tuple[3] == 0 && endian != BigEndianness) {
                  endian = LittleEndianness;
                  num = 0;
                  continue;
               } else if (tuple[0] == 0 && tuple[1] == 0 && tuple[2] == 0xfe && tuple[3] == 0xff && endian != LittleEndianness) {
                  endian = BigEndianness;
                  num = 0;
                  continue;
               } else if (SysInfo::ByteOrder == SysInfo::BigEndian) {
                  endian = BigEndianness;
               } else {
                  endian = LittleEndianness;
               }
            } else if (((endian == BigEndianness) ? pdk::from_big_endian<pdk::puint32>(tuple) : pdk::from_little_endian<pdk::puint32>(tuple)) == Character::ByteOrderMark) {
               num = 0;
               continue;
            }
         }
         uint code = (endian == BigEndianness) ? pdk::from_big_endian<pdk::puint32>(tuple) : pdk::from_little_endian<pdk::puint32>(tuple);
         if (Character::requiresSurrogates(code)) {
            *qch++ = Character::getHighSurrogate(code);
            *qch++ = Character::getLowSurrogate(code);
         } else {
            *qch++ = code;
         }
         num = 0;
      }
   }
   result.truncate(qch - result.unicode());
   
   if (state) {
      if (headerdone)
         state->m_flags |= TextCodec::ConversionFlag::IgnoreHeader;
      state->m_stateData[Endian] = endian;
      state->m_remainingChars = num;
      std::memcpy(&state->m_stateData[Data], tuple, 4);
   }
   return result;
}

Utf8Codec::~Utf8Codec()
{
}

ByteArray Utf8Codec::convertFromUnicode(const Character *uc, int len, ConverterState *state) const
{
   return Utf8::convertFromUnicode(uc, len, state);
}

void Utf8Codec::convertToUnicode(String *target, const char *chars, int len, ConverterState *state) const
{
   *target += Utf8::convertToUnicode(chars, len, state);
}

String Utf8Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
{
   return Utf8::convertToUnicode(chars, len, state);
}

ByteArray Utf8Codec::name() const
{
   return "UTF-8";
}

int Utf8Codec::mibEnum() const
{
   return 106;
}

Utf16Codec::~Utf16Codec()
{
}

ByteArray Utf16Codec::convertFromUnicode(const Character *uc, int len, ConverterState *state) const
{
   return Utf16::convertFromUnicode(uc, len, state, e);
}

String Utf16Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
{
   return Utf16::convertToUnicode(chars, len, state, e);
}

int Utf16Codec::mibEnum() const
{
   return 1015;
}

ByteArray Utf16Codec::name() const
{
   return "UTF-16";
}

std::list<ByteArray> Utf16Codec::aliases() const
{
   return std::list<ByteArray>();
}

int Utf16BECodec::mibEnum() const
{
   return 1013;
}

ByteArray Utf16BECodec::name() const
{
   return "UTF-16BE";
}

std::list<ByteArray> Utf16BECodec::aliases() const
{
   std::list<ByteArray> list;
   return list;
}

int Utf16LECodec::mibEnum() const
{
   return 1014;
}

ByteArray Utf16LECodec::name() const
{
   return "UTF-16LE";
}

std::list<ByteArray> Utf16LECodec::aliases() const
{
   std::list<ByteArray> list;
   return list;
}

Utf32Codec::~Utf32Codec()
{
}

ByteArray Utf32Codec::convertFromUnicode(const Character *uc, int len, ConverterState *state) const
{
   return Utf32::convertFromUnicode(uc, len, state, e);
}

String Utf32Codec::convertToUnicode(const char *chars, int len, ConverterState *state) const
{
   return Utf32::convertToUnicode(chars, len, state, e);
}

int Utf32Codec::mibEnum() const
{
   return 1017;
}

ByteArray Utf32Codec::name() const
{
   return "UTF-32";
}

std::list<ByteArray> Utf32Codec::aliases() const
{
   std::list<ByteArray> list;
   return list;
}

int Utf32BECodec::mibEnum() const
{
   return 1018;
}

ByteArray Utf32BECodec::name() const
{
   return "UTF-32BE";
}

std::list<ByteArray> Utf32BECodec::aliases() const
{
   std::list<ByteArray> list;
   return list;
}

int Utf32LECodec::mibEnum() const
{
   return 1019;
}

ByteArray Utf32LECodec::name() const
{
   return "UTF-32LE";
}

std::list<ByteArray> Utf32LECodec::aliases() const
{
   std::list<ByteArray> list;
   return list;
}

} // internal
} // codecs
} // text
} // pdk
