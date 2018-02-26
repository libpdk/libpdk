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

#include "pdk/global/PlatformDefs.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include "pdk/base/text/codecs/internal/TextCodecPrivate.h"
#include "pdk/base/ds/ByteArrayMatcher.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/kernel/internal/CoreGlobalDataPrivate.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/base/text/codecs/internal/UtfCodecPrivate.h"
#include "pdk/base/text/codecs/internal/LatinCodecPrivate.h"
#include "pdk/base/text/codecs/internal/TsciiCodecPrivate.h"
#include "pdk/base/text/codecs/internal/IsciiCodecPrivate.h"

#if PDK_CONFIG(ICU)
#include "pdk/base/text/codecs/internal/IcuCodecPrivate.h"
#endif

#if PDK_CONFIG(ICONV)
#include "pdk/base/text/codecs/internal/IconvCodecPrivate.h"
#endif
#ifdef PDK_OS_WIN
#  include "pdk/base/text/codecs/internal/WindowsCodecPrivate.h"
#endif

#include "pdk/base/text/codecs/internal/SimpleCodecPrivate.h"

#if !defined(PDK_NO_BIG_CODECS)
#  ifndef PDK_OS_INTEGRITY
#     include "pdk/base/text/codecs/internal/Gb18030CodecPrivate.h"
#     include "pdk/base/text/codecs/internal/EucjpCodecPrivate.h"
#     include "pdk/base/text/codecs/internal/JisCodecPrivate.h"
#     include "pdk/base/text/codecs/internal/SjisCodecPrivate.h"
#     include "pdk/base/text/codecs/internal/EuckrCodecPrivate.h"
#     include "pdk/base/text/codecs/internal/Big5CodecPrivate.h"
#  endif
#endif

#include <mutex>
#include <cstdlib>
#include <ctype.h>
#include <locale.h>
#if defined (_XOPEN_UNIX) && !defined(PDK_OS_QNX) && !defined(PDK_OS_OSF)
# include <langinfo.h>
#endif


namespace pdk {

namespace lang {
namespace internal {
void utf16_from_latin1(char16_t *dest, const char *str, size_t size) noexcept;
} // internal
} // lang

namespace text {
namespace codecs {

using pdk::kernel::CoreApplicationPrivate;

using TextCodecListConstIter = std::list<TextCodec *>::const_iterator;
using ByteArrayListConstIter = std::list<ByteArray>::const_iterator;
using pdk::kernel::internal::TextCodecCache;

PDK_GLOBAL_STATIC(std::recursive_mutex, sg_textCodecsMutex);

std::recursive_mutex *pdk_text_codecs_mutex()
{
   return sg_textCodecsMutex();
}

#if !PDK_CONFIG(ICU)

namespace {

char pdk_tolower(char c)
{
   if (c >= 'A' && c <= 'Z') {
      return c + 0x20;
   }
   return c;
}

bool pdk_isalnum(char c)
{
   return (c >= '0' && c <= '9') || ((c | 0x20) >= 'a' && (c | 0x20) <= 'z');
}

#if !defined(PDK_OS_WIN32) && !defined(PDK_LOCALE_IS_UTF8)
TextCodec *check_for_codec(const ByteArray &name) {
   TextCodec *c = TextCodec::codecForName(name);
   if (!c) {
      const int index = name.indexOf('@');
      if (index != -1) {
         c = TextCodec::codecForName(name.left(index));
      }
   }
   return c;
}
#endif

void setup();

// this returns the codec the method sets up as locale codec to
// avoid a race condition in codecForLocale() when
// setCodecForLocale(0) is called at the same time.
TextCodec *setup_locale_mapper()
{
   CoreGlobalData *globalData = CoreGlobalData::getInstance();
   TextCodec *locale = nullptr;
   
   {
      std::lock_guard<std::recursive_mutex> locker(*sg_textCodecsMutex());
      if (globalData->m_allCodecs.empty()) {
         setup();
      }
   }
   
   //CoreApplicationPrivate::initLocale();
#if defined(PDK_LOCALE_IS_UTF8)
   locale = TextCodec::codecForName("UTF-8");
#elif defined(PDK_OS_WIN)
   locale = TextCodec::codecForName("System");
#else
   
   // First try getting the codecs name from nl_langinfo and see
   // if we have a builtin codec for it.
   // Only fall back to using iconv if we can't find a builtin codec
   // This is because the builtin utf8 codec is around 5 times faster
   // then the using QIconvCodec
   
#if defined (_XOPEN_UNIX) && !defined(PDK_OS_OSF)
   char *charset = nl_langinfo(CODESET);
   if (charset)
      locale = TextCodec::codecForName(charset);
#endif
#if PDK_CONFIG(ICONV)
   if (!locale) {
      // no builtin codec for the locale found, let's try using iconv
      (void) new IconvCodec();
      locale = TextCodec::codecForName("System");
   }
#endif
   
   if (!locale) {
      // Very poorly defined and followed standards causes lots of
      // code to try to get all the cases... This logic is
      // duplicated in QIconvCodec, so if you change it here, change
      // it there too.
      
      // Try to determine locale codeset from locale name assigned to
      // LC_CTYPE category.
      
      // First part is getting that locale name.  First try setlocale() which
      // definitely knows it, but since we cannot fully trust it, get ready
      // to fall back to environment variables.
      const ByteArray ctype = setlocale(LC_CTYPE, 0);
      
      // Get the first nonempty value from $LC_ALL, $LC_CTYPE, and $LANG
      // environment variables.
      ByteArray lang = pdk::pdk_getenv("LC_ALL");
      if (lang.isEmpty() || lang == "C") {
         lang = pdk_getenv("LC_CTYPE");
      }
      if (lang.isEmpty() || lang == "C") {
         lang = pdk_getenv("LANG");
      }
      
      // Now try these in order:
      // 1. CODESET from ctype if it contains a .CODESET part (e.g. en_US.ISO8859-15)
      // 2. CODESET from lang if it contains a .CODESET part
      // 3. ctype (maybe the locale is named "ISO-8859-1" or something)
      // 4. locale (ditto)
      // 5. check for "@euro"
      // 6. guess locale from ctype unless ctype is "C"
      // 7. guess locale from lang
      
      // 1. CODESET from ctype if it contains a .CODESET part (e.g. en_US.ISO8859-15)
      int indexOfDot = ctype.indexOf('.');
      if (indexOfDot != -1)
         locale = checkForCodec( ctype.mid(indexOfDot + 1) );
      
      // 2. CODESET from lang if it contains a .CODESET part
      if (!locale) {
         indexOfDot = lang.indexOf('.');
         if (indexOfDot != -1)
            locale = checkForCodec( lang.mid(indexOfDot + 1) );
      }
      
      // 3. ctype (maybe the locale is named "ISO-8859-1" or something)
      if (!locale && !ctype.isEmpty() && ctype != "C")
         locale = checkForCodec(ctype);
      
      // 4. locale (ditto)
      if (!locale && !lang.isEmpty())
         locale = checkForCodec(lang);
      
      // 5. "@euro"
      if ((!locale && ctype.contains("@euro")) || lang.contains("@euro"))
         locale = checkForCodec("ISO 8859-15");
   }
   
#endif
   // If everything failed, we default to 8859-1
   if (!locale) {
      locale = TextCodec::codecForName("ISO 8859-1");
   }      
   globalData->m_codecForLocale.storeRelease(locale);
   return locale;
}

// textCodecsMutex need to be locked to enter this function
void setup()
{
   static bool initialized = false;
   if (initialized) {
      return;
   }
   initialized = true;
#if !defined(PDK_NO_CODECS)
   (void)new internal::TsciiCodec;
   for (int i = 0; i < 9; ++i)
      (void)new internal::IsciiCodec(i);
   for (int i = 0; i < internal::SimpleTextCodec::NUM_SIMPLE_CODEC; ++i)
      (void)new internal::SimpleTextCodec(i);
   
#  if !defined(PDK_NO_BIG_CODECS) && !defined(PDK_OS_INTEGRITY)
   (void)new internal::Gb18030Codec;
   (void)new internal::GbkCodec;
   (void)new internal::Gb2312Codec;
   (void)new internal::EucJpCodec;
   (void)new internal::JisCodec;
   (void)new internal::SjisCodec;
   (void)new internal::EucKrCodec;
   (void)new internal::CP949Codec;
   (void)new internal::Big5Codec;
   (void)new internal::Big5hkscsCodec;
#  endif // !PDK_NO_BIG_CODECS && !Q_OS_INTEGRITY
#if PDK_CONFIG(ICONV)
   (void) new internal::IconvCodec;
#endif
#if defined(PDK_OS_WIN32)
   (void) new internal::WindowsLocalCodec;
#endif // PDK_OS_WIN32
#endif // !PDK_NO_CODECS
   
   (void)new internal::Utf16Codec;
   (void)new internal::Utf16BECodec;
   (void)new internal::Utf16LECodec;
   (void)new internal::Utf32Codec;
   (void)new internal::Utf32BECodec;
   (void)new internal::Utf32LECodec;
   (void)new internal::Latin15Codec;
   (void)new internal::Latin1Codec;
   (void)new internal::Utf8Codec;
}

} // anonymous namespace

namespace internal {
bool text_codec_name_match(const char *n, const char *h)
{
   if (pdk::stricmp(n, h) == 0) {
      return true;
   }
   // if the letters and numbers are the same, we have a match
   while (*n != '\0') {
      if (std::isalnum(*n)) {
         for (;;) {
            if (*h == '\0'){
               return false;
            }
            if (pdk_isalnum(*h)) {
               break;
            }
            ++h;
         }
         if (pdk_tolower(*n) != pdk_tolower(*h)) {
            return false;
         }
         ++h;
      }
      ++n;
   }
   while (*h && !pdk_isalnum(*h)) {
      ++h;
   }
   return (*h == '\0');
}
} // internal

#else
namespace {
void setup()
{}
}
#endif // !PDK_CONFIG(ICU)

TextCodec::ConverterState::~ConverterState()
{
   if (m_flags & ConversionFlag::FreeFunction) {
      (internal::TextCodecUnalignedPointer::decode(m_stateData))(this);
   } else if (m_data) {
      free(m_data);
   }
}

TextCodec::TextCodec()
{
   std::lock_guard<std::recursive_mutex> locker(*sg_textCodecsMutex());
   CoreGlobalData *globalInstance = CoreGlobalData::getInstance();
   if (globalInstance->m_allCodecs.empty()) {
      setup();
   }
   globalInstance->m_allCodecs.push_front(this);
}

TextCodec::~TextCodec()
{}

TextCodec *TextCodec::codecForName(const ByteArray &name)
{
   if (name.isEmpty()) {
      return nullptr;
   }
   std::lock_guard<std::recursive_mutex> locker(*sg_textCodecsMutex());
   CoreGlobalData *globalData = CoreGlobalData::getInstance();
   if (!globalData) {
      return nullptr;
   }
   setup();
#if !PDK_CONFIG(ICU)
   TextCodecCache *cache = &globalData->m_codecCache;
   
   if (cache) {
      auto iter = cache->find(name);
      if (iter != cache->end()) {
         return iter->second;
      }
   }
   
   for (TextCodecListConstIter iter = globalData->m_allCodecs.cbegin(), cend = globalData->m_allCodecs.cend(); iter != cend; ++iter) {
      TextCodec *cursor = *iter;
      if (internal::text_codec_name_match(cursor->name(), name)) {
         if (cache) {
            (*cache)[name] = cursor;
         }
         return cursor;
      }
      std::list<ByteArray> aliases = cursor->aliases();
      for (ByteArrayListConstIter ait = aliases.cbegin(), acend = aliases.cend(); ait != acend; ++ait) {
         if (internal::text_codec_name_match(*ait, name)) {
            if (cache) {
               (*cache)[name] = cursor;
            }
            return cursor;
         }
      }
   }
   
   return 0;
#else
   return IcuCodec::codecForNameUnlocked(name);
#endif
}

TextCodec* TextCodec::codecForMib(int mib)
{
   std::lock_guard<std::recursive_mutex> locker(*sg_textCodecsMutex());
   CoreGlobalData *globalData = CoreGlobalData::getInstance();
   if (!globalData) {
      return 0;
   }
   if (globalData->m_allCodecs.empty()) {
      setup();
   }
   ByteArray key = "MIB: " + ByteArray::number(mib);
   TextCodecCache *cache = &globalData->m_codecCache;
   
   if (cache) {
      auto iter = cache->find(key);
      if (iter != cache->end()) {
         return iter->second;
      }
   }
   
   for (TextCodecListConstIter iter = globalData->m_allCodecs.cbegin(), cend = globalData->m_allCodecs.cend(); iter != cend; ++iter) {
      TextCodec *cursor = *iter;
      if (cursor->mibEnum() == mib) {
         if (cache) {
            (*cache)[key] = cursor;
         }
         return cursor;
      }
   }
   
#if PDK_CONFIG(ICU)
   return IcuCodec::codecForMibUnlocked(mib);
#else
   return 0;
#endif
}

std::list<ByteArray> TextCodec::getAvailableCodecs()
{
   std::lock_guard<std::recursive_mutex> locker(*sg_textCodecsMutex());
   CoreGlobalData *globalData = CoreGlobalData::getInstance();
   if (globalData->m_allCodecs.empty())
      setup();
   
   std::list<ByteArray> codecs;
   
   for (TextCodecListConstIter iter = globalData->m_allCodecs.cbegin(), cend = globalData->m_allCodecs.cend(); iter != cend; ++iter) {
      codecs.push_back((*iter)->name());
      codecs.merge((*iter)->aliases());
   }
   
#if PDK_CONFIG(ICU)
   codecs += internal::IcuCodec::availableCodecs();
#endif
   
   return codecs;
}

std::list<int> TextCodec::getAvailableMibs()
{
#if PDK_CONFIG(ICU)
   return QIcuCodec::availableMibs();
#else
   std::lock_guard<std::recursive_mutex> locker(*sg_textCodecsMutex());
   CoreGlobalData *globalData = CoreGlobalData::getInstance();
   if (globalData->m_allCodecs.empty()) {
      setup();
   }
   std::list<int> codecs;
   for (TextCodecListConstIter iter = globalData->m_allCodecs.cbegin(), cend = globalData->m_allCodecs.cend(); iter != cend; ++iter) {
      codecs.push_back((*iter)->mibEnum());
   }
   return codecs;
#endif
}

void TextCodec::setCodecForLocale(TextCodec *c)
{
   CoreGlobalData::getInstance()->m_codecForLocale.storeRelease(c);
}

TextCodec* TextCodec::getCodecForLocale()
{
   CoreGlobalData *globalData = CoreGlobalData::getInstance();
   if (!globalData) {
      return 0;
   }
   TextCodec *codec = globalData->m_codecForLocale.loadAcquire();
   if (!codec) {
#if PDK_CONFIG(ICU)
      sg_textCodecsMutex()->lock();
      codec = internal::IcuCodec::defaultCodecUnlocked();
      sg_textCodecsMutex()->unlock();
#else
      // setup_locale_mapper locks as necessary
      codec = setup_locale_mapper();
#endif
   }
   
   return codec;
}

std::list<ByteArray> TextCodec::aliases() const
{
   return std::list<ByteArray>();
}

TextDecoder* TextCodec::makeDecoder(TextCodec::ConversionFlags flags) const
{
   return new TextDecoder(this, flags);
}

TextEncoder* TextCodec::makeEncoder(TextCodec::ConversionFlags flags) const
{
   return new TextEncoder(this, flags);
}

#if PDK_STRINGVIEW_LEVEL < 2
ByteArray TextCodec::fromUnicode(const String& str) const
{
   return convertFromUnicode(str.getConstRawData(), str.length(), 0);
}
#endif

ByteArray TextCodec::fromUnicode(StringView str) const
{
   return convertFromUnicode(str.data(), str.length(), nullptr);
}

String TextCodec::toUnicode(const ByteArray& array) const
{
   return convertToUnicode(array.getConstRawData(), array.length(), 0);
}

bool TextCodec::canEncode(Character ch) const
{
   ConverterState state;
   state.m_flags = ConversionFlag::ConvertInvalidToNull;
   convertFromUnicode(&ch, 1, &state);
   return (state.m_invalidChars == 0);
}

#if PDK_STRINGVIEW_LEVEL < 2

bool TextCodec::canEncode(const String& str) const
{
   ConverterState state;
   state.m_flags = ConversionFlag::ConvertInvalidToNull;
   convertFromUnicode(str.getConstRawData(), str.length(), &state);
   return (state.m_invalidChars == 0);
}
#endif

bool TextCodec::canEncode(StringView str) const
{
   ConverterState state;
   state.m_flags = ConversionFlag::ConvertInvalidToNull;
   convertFromUnicode(str.data(), str.length(), &state);
   return !state.m_invalidChars;
}

String TextCodec::toUnicode(const char *chars) const
{
   int len = pdk::strlen(chars);
   return convertToUnicode(chars, len, 0);
}

TextEncoder::TextEncoder(const TextCodec *codec, TextCodec::ConversionFlags flags)
   : m_codec(codec),
     m_state()
{
   m_state.m_flags = flags;
}

TextEncoder::~TextEncoder()
{
}

bool TextEncoder::hasFailure() const
{
   return m_state.m_invalidChars != 0;
}

#if PDK_STRINGVIEW_LEVEL < 2
ByteArray TextEncoder::fromUnicode(const String& str)
{
   return m_codec->fromUnicode(str.getConstRawData(), str.length(), &m_state);
}
#endif

ByteArray TextEncoder::fromUnicode(StringView str)
{
   return m_codec->fromUnicode(str.data(), str.length(), &m_state);
}

ByteArray TextEncoder::fromUnicode(const Character *uc, int len)
{
   return m_codec->fromUnicode(uc, len, &m_state);
}

TextDecoder::TextDecoder(const TextCodec *codec, TextCodec::ConversionFlags flags)
   : m_codec(codec),
     m_state()
{
   m_state.m_flags = flags;
}

TextDecoder::~TextDecoder()
{
}

String TextDecoder::toUnicode(const char *chars, int len)
{
   return m_codec->toUnicode(chars, len, &m_state);
}

using ::pdk::lang::internal::utf16_from_latin1;

void TextDecoder::toUnicode(String *target, const char *chars, int len)
{
   PDK_ASSERT(target);
   switch (m_codec->mibEnum()) {
   case 106: // utf8
      static_cast<const internal::Utf8Codec*>(m_codec)->convertToUnicode(target, chars, len, &m_state);
      break;
   case 4: // latin1
      target->resize(len);
      utf16_from_latin1((char16_t*)target->getRawData(), chars, len);
      break;
   default:
      *target = m_codec->toUnicode(chars, len, &m_state);
   }
}

String TextDecoder::toUnicode(const ByteArray &ba)
{
   return m_codec->toUnicode(ba.getConstRawData(), ba.length(), &m_state);
}

bool TextDecoder::hasFailure() const
{
   return m_state.m_invalidChars != 0;
}

TextCodec *TextCodec::codecForHtml(const ByteArray &ba, TextCodec *defaultCodec)
{
   // determine charset
   TextCodec *c = TextCodec::codecForUtfText(ba, 0);
   if (!c) {
      static PDK_RELAXED_CONSTEXPR auto matcher = pdk::ds::pdk_make_static_byte_array_matcher("meta ");
      ByteArray header = ba.left(1024).toLower();
      int pos = matcher.indexIn(header);
      if (pos != -1) {
         static PDK_RELAXED_CONSTEXPR auto matcher = pdk::ds::pdk_make_static_byte_array_matcher("charset=");
         pos = matcher.indexIn(header, pos);
         if (pos != -1) {
            pos += pdk::strlen("charset=");
            int pos2 = pos;
            // The attribute can be closed with either """, "'", ">" or "/",
            // none of which are valid charset characters.
            while (++pos2 < header.size()) {
               char ch = header.at(pos2);
               if (ch == '\"' || ch == '\'' || ch == '>') {
                  ByteArray name = header.mid(pos, pos2 - pos);
                  if (name == "unicode") // QTBUG-41998, ICU will return UTF-16.
                     name = ByteArrayLiteral("UTF-8");
                  c = TextCodec::codecForName(name);
                  return c ? c : defaultCodec;
               }
            }
         }
      }
   }
   if (!c) {
      c = defaultCodec;
   }
   return c;
}

TextCodec *TextCodec::codecForHtml(const ByteArray &ba)
{
   return codecForHtml(ba, TextCodec::codecForName("ISO-8859-1"));
}

TextCodec *TextCodec::codecForUtfText(const ByteArray &ba, TextCodec *defaultCodec)
{
   const int arraySize = ba.size();
   if (arraySize > 3) {
      if ((uchar)ba[0] == 0x00
          && (uchar)ba[1] == 0x00
          && (uchar)ba[2] == 0xFE
          && (uchar)ba[3] == 0xFF)
         return TextCodec::codecForMib(1018); // utf-32 be
      else if ((uchar)ba[0] == 0xFF
               && (uchar)ba[1] == 0xFE
               && (uchar)ba[2] == 0x00
               && (uchar)ba[3] == 0x00)
         return TextCodec::codecForMib(1019); // utf-32 le
   }
   
   if (arraySize < 2) {
      return defaultCodec;
   }
   if ((uchar)ba[0] == 0xfe && (uchar)ba[1] == 0xff) {
      return TextCodec::codecForMib(1013); // utf16 be
   } else if ((uchar)ba[0] == 0xff && (uchar)ba[1] == 0xfe) {
      return TextCodec::codecForMib(1014); // utf16 le
   }
   if (arraySize < 3) {
      return defaultCodec;
   }
   if ((uchar)ba[0] == 0xef
       && (uchar)ba[1] == 0xbb
       && (uchar)ba[2] == 0xbf) {
      return TextCodec::codecForMib(106); // utf-8
   }
   return defaultCodec;
}

TextCodec *TextCodec::codecForUtfText(const ByteArray &ba)
{
   return codecForUtfText(ba, TextCodec::codecForMib(/*Latin 1*/ 4));
}

} // codecs
} // text
} // pdk
