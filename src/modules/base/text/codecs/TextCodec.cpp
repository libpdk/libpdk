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
namespace text {
namespace codecs {

using pdk::kernel::CoreApplicationPrivate;

using TextCodecListConstIter = std::list<TextCodec*>::const_iterator;
using ByteArrayListConstIter = std::list<ByteArray>::const_iterator;
using pdk::kernel::internal::TextCodecCache;

PDK_GLOBAL_STATIC(std::recursive_mutex, sg_textCodecsMutex);

std::recursive_mutex *pdk_text_codecs_mutex()
{
   return sg_textCodecsMutex();
}

#if !PDK_CONFIG(ICU)
namespace internal {
bool text_codec_name_match(const char *a, const char *b)
{}
} // internal

namespace {

char tolower(char c)
{
   if (c >= 'A' && c <= 'Z') {
      return c + 0x20;
   }
   return c;
}

bool isalnum(char c)
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
#  endif // !QT_NO_BIG_CODECS && !Q_OS_INTEGRITY
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
   
}

std::list<int> TextCodec::getAvailableMibs()
{
}

void TextCodec::setCodecForLocale(TextCodec *c)
{
   
}

TextCodec* TextCodec::getCodecForLocale()
{
   
}

std::list<ByteArray> TextCodec::aliases() const
{
}

TextDecoder* TextCodec::makeDecoder(TextCodec::ConversionFlags flags) const
{
   
}

TextEncoder* TextCodec::makeEncoder(TextCodec::ConversionFlags flags) const
{
   
}

#if PDK_STRINGVIEW_LEVEL < 2
ByteArray TextCodec::fromUnicode(const String& str) const
{
}
#endif

ByteArray TextCodec::fromUnicode(StringView str) const
{
}

String TextCodec::toUnicode(const ByteArray& a) const
{
}

bool TextCodec::canEncode(Character ch) const
{
}

#if PDK_STRINGVIEW_LEVEL < 2

bool TextCodec::canEncode(const String& s) const
{
}
#endif

bool TextCodec::canEncode(StringView s) const
{
}

String TextCodec::toUnicode(const char *chars) const
{
}

TextEncoder::TextEncoder(const TextCodec *codec, TextCodec::ConversionFlags flags)
   : m_codec(codec),
     m_state()
{
}

TextEncoder::~TextEncoder()
{
}

bool TextEncoder::hasFailure() const
{
}

#if PDK_STRINGVIEW_LEVEL < 2
ByteArray TextEncoder::fromUnicode(const String& str)
{
}
#endif

ByteArray TextEncoder::fromUnicode(StringView str)
{
   
}

ByteArray TextEncoder::fromUnicode(const Character *uc, int len)
{
}

TextDecoder::TextDecoder(const TextCodec *codec, TextCodec::ConversionFlags flags)
   : m_codec(codec),
     m_state()
{
}

TextDecoder::~TextDecoder()
{
}

String TextDecoder::toUnicode(const char *chars, int len)
{
}

void TextDecoder::toUnicode(String *target, const char *chars, int len)
{
}

String TextDecoder::toUnicode(const ByteArray &ba)
{
}


bool TextDecoder::hasFailure() const
{
   
}

TextCodec *TextCodec::codecForHtml(const ByteArray &ba, TextCodec *defaultCodec)
{
}

TextCodec *TextCodec::codecForHtml(const ByteArray &ba)
{
   
}

TextCodec *TextCodec::codecForUtfText(const ByteArray &ba, TextCodec *defaultCodec)
{
}

TextCodec *TextCodec::codecForUtfText(const ByteArray &ba)
{
   
}

} // codecs
} // text
} // pdk
