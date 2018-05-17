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
// Created by softboy on 2018/02/23.

#include "pdk/global/PlatformDefs.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/fs/Settings.h"

#ifndef PDK_NO_SETTINGS

#include "pdk/base/io/fs/internal/SettingsPrivate.h"
#include "pdk/utils/Cache.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/global/LibraryInfo.h"
#include "pdk/base/io/fs/TemporaryFile.h"
#include "pdk/base/io/fs/StandardPaths.h"
#include "pdk/base/io/DataStream.h"

#ifndef PDK_NO_TEXTCODEC
#  include "pdk/base/text/codecs/TextCodec.h"
#endif

#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/io/fs/SaveFile.h"
#include "pdk/base/io/fs/LockFile.h"

#ifdef PDK_OS_VXWORKS
#  include <ioLib.h>
#endif

#include <algorithm>
#include <cstdlib>
#include <map>
#include <mutex>

#ifdef PDK_OS_WIN // for homedirpath reading from registry
#  include "pdk/global/Windows.h"
#  include <shlobj.h>
#endif

#if defined(PDK_OS_UNIX) && !defined(PDK_OS_MAC)
#define PDK_XDG_PLATFORM
#endif

#if !defined(PDK_NO_STANDARDPATHS) && (defined(PDK_XDG_PLATFORM) || defined(PDK_PLATFORM_UIKIT))
#define SETTINGS_USE_STANDARDPATHS
#endif

namespace pdk {
namespace io {
namespace fs {

using pdk::utils::Cache;
using internal::ConfFile;
using pdk::lang::Latin1Character;
using pdk::lang::Latin1String;
using pdk::lang::StringRef;
using pdk::ds::StringList;
using pdk::kernel::CoreApplication;
using pdk::kernel::Event;
using pdk::ds::ByteArray;
using pdk::io::fs::LockFile;
using pdk::io::fs::FileInfo;
using pdk::io::fs::File;
using pdk::io::fs::SaveFile;

struct ConfFileCustomFormat
{
   String m_extension;
   Settings::ReadFunc m_readFunc;
   Settings::WriteFunc m_writeFunc;
   pdk::CaseSensitivity m_caseSensitivity;
};

using ConfFileHash = std::map<String, ConfFile *>;
using ConfFileCache = Cache<String, ConfFile>;

namespace {
struct Path
{
   // Note: Defining constructors explicitly because of buggy C++11
   // implementation in MSVC (uniform initialization).
   Path()
   {}
   
   Path(const String & p, bool ud)
      : m_path(p), 
        m_userDefined(ud)
   {}
   String m_path;
   bool m_userDefined; //!< true - user defined, overridden by setPath
};
} // anonymous namespace

using PathHash = std::map<int, Path>;
using CustomFormatVector = std::vector<ConfFileCustomFormat>;

PDK_GLOBAL_STATIC(ConfFileHash, sg_usedHashFunc);
PDK_GLOBAL_STATIC(ConfFileCache, sg_unusedCacheFunc);
PDK_GLOBAL_STATIC(PathHash, sg_pathHashFunc);
PDK_GLOBAL_STATIC(CustomFormatVector, sg_customFormatVectorFunc);

static std::mutex sg_settingsGlobalMutex;
static Settings::Format sg_globalDefaultFormat = Settings::Format::NativeFormat;

namespace internal {

ConfFile::ConfFile(const String &fileName, bool userPerms)
   : m_name(fileName),
     m_size(0),
     m_ref(1),
     m_userPerms(userPerms)
{
   (*sg_usedHashFunc())[m_name] = this;
}

ConfFile::~ConfFile()
{
   if (sg_usedHashFunc()) {
      sg_usedHashFunc()->erase(m_name);
   }
}

ParsedSettingsMap ConfFile::getMergedKeyMap() const
{
   ParsedSettingsMap result = m_originalKeys;
   ParsedSettingsMap::const_iterator iter;
   for (iter = m_removedKeys.begin(); iter != m_removedKeys.end(); ++iter) {
      result.erase(iter->first);
   }
   for (iter = m_addedKeys.begin(); iter != m_addedKeys.end(); ++iter) {
      result[iter->first] = iter->second;
   }
   return result;
}

bool ConfFile::isWritable() const
{
   FileInfo fileInfo(m_name);
   
#ifndef PDK_NO_TEMPORARYFILE
   if (fileInfo.exists()) {
#endif
      File file(m_name);
      return file.open(File::OpenMode::ReadWrite);
#ifndef PDK_NO_TEMPORARYFILE
   } else {
      // Create the directories to the file.
      Dir dir(fileInfo.getAbsolutePath());
      if (!dir.exists()) {
         if (!dir.mkpath(dir.getAbsolutePath())) {
            return false;
         }
      }
      // we use a temporary file to avoid race conditions
      TemporaryFile file(m_name);
      return file.open();
   }
#endif
}

ConfFile *ConfFile::fromName(const String &fileName, bool userPerms)
{
   String absPath = FileInfo(fileName).getAbsoluteFilePath();
   
   ConfFileHash *usedHash = sg_usedHashFunc();
   ConfFileCache *unusedCache = sg_unusedCacheFunc();
   
   ConfFile *confFile = 0;
   std::scoped_lock<std::mutex> locker(sg_settingsGlobalMutex);
   
   if (!(confFile = usedHash->at(absPath))) {
      if ((confFile = unusedCache->take(absPath))) {
         (*usedHash)[absPath] = confFile;
      }
   }
   if (confFile) {
      confFile->m_ref.ref();
      return confFile;
   }
   return new ConfFile(absPath, userPerms);
}

void ConfFile::clearCache()
{
   std::scoped_lock<std::mutex> locker(sg_settingsGlobalMutex);
   sg_unusedCacheFunc()->clear();
}

SettingsPrivate::SettingsPrivate(Settings::Format format)
   : m_format(format),
     m_scope(Settings::Scope::UserScope /* nothing better to put */),
     m_iniCodec(0),
     m_fallbacks(true),
     m_pendingChanges(false),
     m_status(Settings::Status::NoError)
{}

SettingsPrivate::SettingsPrivate(Settings::Format format, Settings::Scope scope,
                                 const String &organization, const String &application)
   : m_format(format),
     m_scope(scope),
     m_orgName(organization),
     m_appName(application),
     m_iniCodec(0), 
     m_fallbacks(true), 
     m_pendingChanges(false), 
     m_status(Settings::Status::NoError)
{}

SettingsPrivate::~SettingsPrivate()
{}

String SettingsPrivate::actualKey(const String &key) const
{
   String n = normalizedKey(key);
   PDK_ASSERT_X(!n.isEmpty(), "Settings", "empty key");
   return m_groupPrefix + n;
}

/*
    Returns a string that never starts nor ends with a slash (or an
    empty string). Examples:
    
            "foo"            becomes   "foo"
            "/foo//bar///"   becomes   "foo/bar"
            "///"            becomes   ""
            
    This function is optimized to avoid a String deep copy in the
    common case where the key is already normalized.
*/
String SettingsPrivate::normalizedKey(const String &key)
{
   String result = key;
   
   int i = 0;
   while (i < result.size()) {
      while (result.at(i) == Latin1Character('/')) {
         result.remove(i, 1);
         if (i == result.size()) {
            goto after_loop;
         }
      }
      while (result.at(i) != Latin1Character('/')) {
         ++i;
         if (i == result.size()) {
            return result;
         }
      }
      ++i; // leave the slash alone
   }
   
after_loop:
   if (!result.isEmpty()) {
      result.truncate(i - 1); // remove the trailing slash
   }
   return result;
}

// see also settings_win.cpp and settings_mac.cpp

#if !defined(PDK_OS_WIN) && !defined(PDK_OS_MAC)
SettingsPrivate *SettingsPrivate::create(Settings::Format format, Settings::Scope scope,
                                         const String &organization, const String &application)
{
   return new ConfFileSettingsPrivate(format, scope, organization, application);
}
#endif

#if !defined(PDK_OS_WIN)
SettingsPrivate *SettingsPrivate::create(const String &fileName, Settings::Format format)
{
   return new ConfFileSettingsPrivate(fileName, format);
}
#endif

void SettingsPrivate::processChild(StringRef key, ChildSpec spec, StringList &result)
{
   if (spec != ChildSpec::AllKeys) {
      int slashPos = key.indexOf(Latin1Character('/'));
      if (slashPos == -1) {
         if (spec != ChildSpec::ChildKeys)
            return;
      } else {
         if (spec != ChildSpec::ChildGroups)
            return;
         key.truncate(slashPos);
      }
   }
   result.push_back(key.toString());
}

void SettingsPrivate::beginGroupOrArray(const SettingsGroup &group)
{
   m_groupStack.push(group);
   const String name = group.getName();
   if (!name.isEmpty()) {
      m_groupPrefix += name + Latin1Character('/');
   }
}

/*
    We only set an error if there isn't one set already. This way the user always gets the
    first error that occurred. We always allow clearing errors.
*/

void SettingsPrivate::setStatus(Settings::Status status) const
{
   if (status == Settings::Status::NoError || this->m_status == Settings::Status::NoError) {
      this->m_status = status;
   }
}

void SettingsPrivate::update()
{
   flush();
   m_pendingChanges = false;
}

void SettingsPrivate::requestUpdate()
{
   if (!m_pendingChanges) {
      m_pendingChanges = true;
      PDK_Q(Settings);
      CoreApplication::postEvent(apiPtr, new Event(Event::Type::UpdateRequest));
   }
}

StringList SettingsPrivate::anyListToStringList(const std::list<std::any> &list)
{
   StringList result;
   result.resize(list.size());
   std::list<std::any>::const_iterator iter = list.cbegin();
   for (; iter != list.cend(); ++iter) {
      result.push_back(anyToString(*iter));
   }
   return result;
}

std::any SettingsPrivate::stringListToAnyList(const StringList &list)
{
   StringList outStringList = list;
   for (int i = 0; static_cast<StringList::size_type>(i) < outStringList.size(); ++i) {
      const String &str = outStringList.at(i);
      if (str.startsWith(Latin1Character('@'))) {
         if (str.length() >= 2 && str.at(1) == Latin1Character('@')) {
            outStringList[i].remove(0, 1);
         } else {
            std::list<std::any> anyList;
            const int stringCount = list.size();
            anyList.resize(stringCount);
            for (int j = 0; j < stringCount; ++j) {
               anyList.push_back(stringToAny(list.at(j)));
            }
            return anyList;
         }
      }
   }
   return outStringList;
}

String SettingsPrivate::anyToString(const std::any &v)
{
   String result;
   if (!v.has_value()) {
      result = Latin1String("@Invalid()");
   } else {
      size_t hashCode = v.type().hash_code();
      if (hashCode == typeid (ByteArray).hash_code()) {
         ByteArray a = std::any_cast<ByteArray>(v);
         result = Latin1String("@ByteArray(")
               + Latin1String(a.getConstRawData(), a.size())
               + Latin1Character(')');
      } else if (hashCode == typeid (String).hash_code() ||
                 hashCode == typeid (pdk::plonglong).hash_code() ||
                 hashCode == typeid (pdk::pulonglong).hash_code() ||
                 hashCode == typeid (int).hash_code() ||
                 hashCode == typeid (uint).hash_code() ||
                 hashCode == typeid (bool).hash_code() ||
                 hashCode == typeid (double).hash_code()) {
         result = std::any_cast<String>(v);
         if (result.contains(Character::Null)) {
            result = Latin1String("@String(") + result + Latin1Character(')');
         }
         else if (result.startsWith(Latin1Character('@'))) {
            result.prepend(Latin1Character('@'));
         }
      } else {
#ifndef PDK_NO_DATASTREAM
         DataStream::Version version;
         const char *typeSpec;
         if (v.type().hash_code() == typeid(DateTime).hash_code()) {
            version = DataStream::Version::pdk_1_0;
            typeSpec = "@DateTime(";
         } else {
            version = DataStream::Version::pdk_1_0;
            typeSpec = "@std::any(";
         }
         ByteArray a;
         {
            DataStream s(&a, IoDevice::OpenMode::WriteOnly);
            s.setVersion(version);
            // @TODO DataStream
            //s << v;
         }
         result = Latin1String(typeSpec)
               + Latin1String(a.getConstRawData(), a.size())
               + Latin1Character(')');
#else
         PDK_ASSERT(!"Settings: Cannot save custom types without DataStream support");
#endif
      }
   }
   return result;
}

std::any SettingsPrivate::stringToAny(const String &s)
{
   if (s.startsWith(Latin1Character('@'))) {
      if (s.endsWith(Latin1Character(')'))) {
         if (s.startsWith(Latin1String("@ByteArray("))) {
            return std::any(s.substringRef(11, s.size() - 12).toLatin1());
         } else if (s.startsWith(Latin1String("@String("))) {
            return std::any(s.substringRef(8, s.size() - 9).toString());
         } else if (s.startsWith(Latin1String("@Variant("))
                    || s.startsWith(Latin1String("@DateTime("))) {
#ifndef PDK_NO_DATASTREAM
            DataStream::Version version;
            int offset;
            if (s.at(1) == Latin1Character('D')) {
               version = DataStream::Version::pdk_1_0;
               offset = 10;
            } else {
               version = DataStream::Version::pdk_1_0;
               offset = 9;
            }
            ByteArray a = s.substringRef(offset).toLatin1();
            DataStream stream(&a, IoDevice::OpenMode::ReadOnly);
            stream.setVersion(version);
            std::any result;
            //            stream >> result;
            return result;
#else
            PDK_ASSERT(!"Settings: Cannot load custom types without DataStream support");
#endif
         } else if (s == Latin1String("@Invalid()")) {
            return std::any();
         }
      }
      if (s.startsWith(Latin1String("@@"))) {
         return std::any(s.substring(1));
      }
   }
   
   return std::any(s);
}

static const char sg_hexDigits[] = "0123456789ABCDEF";

void SettingsPrivate::iniEscapedKey(const String &key, ByteArray &result)
{
   result.reserve(result.length() + key.length() * 3 / 2);
   for (int i = 0; i < key.size(); ++i) {
      uint ch = key.at(i).unicode();
      
      if (ch == '/') {
         result += '\\';
      } else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')
                 || ch == '_' || ch == '-' || ch == '.') {
         result += (char)ch;
      } else if (ch <= 0xFF) {
         result += '%';
         result += sg_hexDigits[ch / 16];
         result += sg_hexDigits[ch % 16];
      } else {
         result += "%U";
         ByteArray hexCode;
         for (int i = 0; i < 4; ++i) {
            hexCode.prepend(sg_hexDigits[ch % 16]);
            ch >>= 4;
         }
         result += hexCode;
      }
   }
}

bool SettingsPrivate::iniUnescapedKey(const ByteArray &key, int from, int to, String &result)
{
   bool lowercaseOnly = true;
   int i = from;
   result.reserve(result.length() + (to - from));
   while (i < to) {
      int ch = (uchar)key.at(i);
      if (ch == '\\') {
         result += Latin1Character('/');
         ++i;
         continue;
      }
      
      if (ch != '%' || i == to - 1) {
         if (uint(ch - 'A') <= 'Z' - 'A') // only for ASCII
            lowercaseOnly = false;
         result += Latin1Character(ch);
         ++i;
         continue;
      }
      
      int numDigits = 2;
      int firstDigitPos = i + 1;
      
      ch = key.at(i + 1);
      if (ch == 'U') {
         ++firstDigitPos;
         numDigits = 4;
      }
      
      if (firstDigitPos + numDigits > to) {
         result += Latin1Character('%');
         // ### missing U
         ++i;
         continue;
      }
      
      bool ok;
      ch = key.mid(firstDigitPos, numDigits).toInt(&ok, 16);
      if (!ok) {
         result += Latin1Character('%');
         // ### missing U
         ++i;
         continue;
      }
      
      Character qch(ch);
      if (qch.isUpper()) {
         lowercaseOnly = false;
      }
      result += qch;
      i = firstDigitPos + numDigits;
   }
   return lowercaseOnly;
}

void SettingsPrivate::iniEscapedString(const String &str, ByteArray &result, TextCodec *codec)
{
   bool needsQuotes = false;
   bool escapeNextIfDigit = false;
   bool useCodec = codec && !str.startsWith(Latin1String("@ByteArray("))
         && !str.startsWith(Latin1String("@Variant("));
   
   int i;
   int startPos = result.size();
   
   result.reserve(startPos + str.size() * 3 / 2);
   const Character *unicode = str.unicode();
   for (i = 0; i < str.size(); ++i) {
      uint ch = unicode[i].unicode();
      if (ch == ';' || ch == ',' || ch == '=') {
         needsQuotes = true;
      }
      if (escapeNextIfDigit
          && ((ch >= '0' && ch <= '9')
              || (ch >= 'a' && ch <= 'f')
              || (ch >= 'A' && ch <= 'F'))) {
         result += "\\x" + ByteArray::number(ch, 16);
         continue;
      }
      
      escapeNextIfDigit = false;
      
      switch (ch) {
      case '\0':
         result += "\\0";
         escapeNextIfDigit = true;
         break;
      case '\a':
         result += "\\a";
         break;
      case '\b':
         result += "\\b";
         break;
      case '\f':
         result += "\\f";
         break;
      case '\n':
         result += "\\n";
         break;
      case '\r':
         result += "\\r";
         break;
      case '\t':
         result += "\\t";
         break;
      case '\v':
         result += "\\v";
         break;
      case '"':
      case '\\':
         result += '\\';
         result += (char)ch;
         break;
      default:
         if (ch <= 0x1F || (ch >= 0x7F && !useCodec)) {
            result += "\\x" + ByteArray::number(ch, 16);
            escapeNextIfDigit = true;
#ifndef PDK_NO_TEXTCODEC
         } else if (useCodec) {
            // slow
            result += codec->fromUnicode(&unicode[i], 1);
#endif
         } else {
            result += (char)ch;
         }
      }
   }
   
   if (needsQuotes
       || (startPos < result.size() && (result.at(startPos) == ' '
                                        || result.at(result.size() - 1) == ' '))) {
      result.insert(startPos, '"');
      result += '"';
   }
}

namespace {
inline void ini_chop_trailing_spaces(String &str, int limit)
{
   int n = str.size() - 1;
   Character ch;
   while (n >= limit && ((ch = str.at(n)) == Latin1Character(' ') || ch == Latin1Character('\t'))) {
      str.truncate(n--);
   }
}
} // anonymous namespace

void SettingsPrivate::iniEscapedStringList(const StringList &strs, ByteArray &result, TextCodec *codec)
{
   if (strs.empty()) {
      // We need to distinguish between empty lists and one-item
      // lists that contain an empty string. Ideally, we'd have a
      // @EmptyList() symbol but that would break compatibility
      //  with pdk 4.0. @Invalid() stands for QVariant(), and
      // std::any() cast to StringList returns an empty StringList,
      // so we're in good shape.
      result += "@Invalid()";
   } else {
      for (int i = 0; static_cast<StringList::size_type>(i) < strs.size(); ++i) {
         if (i != 0) {
            result += ", ";
         }
         iniEscapedString(strs.at(i), result, codec);
      }
   }
}

bool SettingsPrivate::iniUnescapedStringList(const ByteArray &str, int from, int to,
                                             String &stringResult, StringList &stringListResult,
                                             TextCodec *codec)
{
   static const char escapeCodes[][2] =
   {
      { 'a', '\a' },
      { 'b', '\b' },
      { 'f', '\f' },
      { 'n', '\n' },
      { 'r', '\r' },
      { 't', '\t' },
      { 'v', '\v' },
      { '"', '"' },
      { '?', '?' },
      { '\'', '\'' },
      { '\\', '\\' }
   };
   static const int numEscapeCodes = sizeof(escapeCodes) / sizeof(escapeCodes[0]);
   
   bool isStringList = false;
   bool inQuotedString = false;
   bool currentValueIsQuoted = false;
   int escapeVal = 0;
   int i = from;
   char ch;
   
StSkipSpaces:
   while (i < to && ((ch = str.at(i)) == ' ' || ch == '\t')) {
      ++i;
   }
   // fallthrough
   
StNormal:
   int chopLimit = stringResult.length();
   while (i < to) {
      switch (str.at(i)) {
      case '\\':
         ++i;
         if (i >= to) {
            goto end;
         }
         ch = str.at(i++);
         for (int j = 0; j < numEscapeCodes; ++j) {
            if (ch == escapeCodes[j][0]) {
               stringResult += Latin1Character(escapeCodes[j][1]);
               goto StNormal;
            }
         }
         
         if (ch == 'x') {
            escapeVal = 0;
            if (i >= to) {
               goto end;
            }
            ch = str.at(i);
            if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
               goto StHexEscape;
            } 
         } else if (ch >= '0' && ch <= '7') {
            escapeVal = ch - '0';
            goto StOctEscape;
         } else if (ch == '\n' || ch == '\r') {
            if (i < to) {
               char ch2 = str.at(i);
               // \n, \r, \r\n, and \n\r are legitimate line terminators in INI files
               if ((ch2 == '\n' || ch2 == '\r') && ch2 != ch) {
                  ++i;
               }
            }
         } else {
            // the character is skipped
         }
         chopLimit = stringResult.length();
         break;
      case '"':
         ++i;
         currentValueIsQuoted = true;
         inQuotedString = !inQuotedString;
         if (!inQuotedString) {
            goto StSkipSpaces;
         }
         break;
      case ',':
         if (!inQuotedString) {
            if (!currentValueIsQuoted)
               ini_chop_trailing_spaces(stringResult, chopLimit);
            if (!isStringList) {
               isStringList = true;
               stringListResult.clear();
               stringResult.squeeze();
            }
            stringListResult.push_back(stringResult);
            stringResult.clear();
            currentValueIsQuoted = false;
            ++i;
            goto StSkipSpaces;
         }
         PDK_FALLTHROUGH();
      default: {
         int j = i + 1;
         while (j < to) {
            ch = str.at(j);
            if (ch == '\\' || ch == '"' || ch == ',')
               break;
            ++j;
         }
         
#ifdef PDK_NO_TEXTCODEC
         PDK_UNUSED(codec)
      #else
         if (codec) {
            stringResult += codec->toUnicode(str.getConstRawData() + i, j - i);
         } else
#endif
         {
            int n = stringResult.size();
            stringResult.resize(n + (j - i));
            Character *resultData = stringResult.getRawData() + n;
            for (int k = i; k < j; ++k) {
               *resultData++ = Latin1Character(str.at(k));
            }
         }
         i = j;
      }
      }
   }
   if (!currentValueIsQuoted) {
      ini_chop_trailing_spaces(stringResult, chopLimit);
   }
   goto end;
   
StHexEscape:
   if (i >= to) {
      stringResult += Character(escapeVal);
      goto end;
   }
   
   ch = str.at(i);
   if (ch >= 'a')
      ch -= 'a' - 'A';
   if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')) {
      escapeVal <<= 4;
      escapeVal += strchr(sg_hexDigits, ch) - sg_hexDigits;
      ++i;
      goto StHexEscape;
   } else {
      stringResult += Character(escapeVal);
      goto StNormal;
   }
   
StOctEscape:
   if (i >= to) {
      stringResult += Character(escapeVal);
      goto end;
   }
   
   ch = str.at(i);
   if (ch >= '0' && ch <= '7') {
      escapeVal <<= 3;
      escapeVal += ch - '0';
      ++i;
      goto StOctEscape;
   } else {
      stringResult += Character(escapeVal);
      goto StNormal;
   }
   
end:
   if (isStringList) {
      stringListResult.push_back(stringResult);
   }
   return isStringList;
}

StringList SettingsPrivate::splitArgs(const String &s, int idx)
{
   int l = s.length();
   PDK_ASSERT(l > 0);
   PDK_ASSERT(s.at(idx) == Latin1Character('('));
   PDK_ASSERT(s.at(l - 1) == Latin1Character(')'));
   
   StringList result;
   String item;
   
   for (++idx; idx < l; ++idx) {
      Character c = s.at(idx);
      if (c == Latin1Character(')')) {
         PDK_ASSERT(idx == l - 1);
         result.push_back(item);
      } else if (c == Latin1Character(' ')) {
         result.push_back(item);
         item.clear();
      } else {
         item.append(c);
      }
   }
   
   return result;
}

void ConfFileSettingsPrivate::initFormat()
{
   m_extension = (m_format == Settings::Format::NativeFormat) ? Latin1String(".conf") : Latin1String(".ini");
   m_readFunc = nullptr;
   m_writeFunc = nullptr;
#if defined(PDK_OS_MAC)
   m_caseSensitivity = (m_format == Settings::Format::NativeFormat) ? pdk::CaseSensitivity::Sensitive : IniCaseSensitivity;
#else
   m_caseSensitivity = IniCaseSensitivity;
#endif
   
   if (m_format > Settings::Format::IniFormat) {
      std::scoped_lock<std::mutex> locker(sg_settingsGlobalMutex);
      const CustomFormatVector *customFormatVector = sg_customFormatVectorFunc();
      
      int i = (int)m_format - (int)Settings::Format::CustomFormat1;
      if (i >= 0 && static_cast<size_t>(i) < customFormatVector->size()) {
         ConfFileCustomFormat info = customFormatVector->at(i);
         m_extension = info.m_extension;
         m_readFunc = info.m_readFunc;
         m_writeFunc = info.m_writeFunc;
         m_caseSensitivity = info.m_caseSensitivity;
      }
   }
}

void ConfFileSettingsPrivate::initAccess()
{
   if (!m_confFiles.empty()) {
      if (m_format > Settings::Format::IniFormat) {
         if (!m_readFunc) {
            setStatus(Settings::Status::AccessError);
         }
      }
   }
   sync();       // loads the files the first time
}

namespace { 
#if defined(PDK_OS_WIN)
String windows_config_path(const KNOWNFOLDERID &type)
{
   String result;
   PWSTR path = nullptr;
   if (SHGetKnownFolderPath(type, KF_FLAG_DONT_VERIFY, NULL, &path) == S_OK) {
      result = String::fromWCharArray(path);
      CoTaskMemFree(path);
   }
   if (result.isEmpty()) {
      if (type == FOLDERID_ProgramData) {
         result = Latin1String("C:\\temp\\pdk-common");
      } else if (type == FOLDERID_RoamingAppData) {
         result = Latin1String("C:\\temp\\pdk-user");
      }
   }
   return result;
}
#endif // PDK_OS_WIN

inline int path_hash_key(Settings::Format format, Settings::Scope scope)
{
   return int((uint(format) << 1) | uint(scope == Settings::Scope::SystemScope));
}

#ifndef PDK_OS_WIN
String make_user_path()
{
   static constexpr Character sep = Latin1Character('/');
#ifndef SETTINGS_USE_STANDARDPATHS
   // Non XDG platforms (OS X, iOS, Android...) have used this code path erroneously
   // for some time now. Moving away from that would require migrating existing settings.
   ByteArray env = pdk::get_env("XDG_CONFIG_HOME");
   if (env.isEmpty()) {
      return Dir::getHomePath() + Latin1String("/.config/");
   } else if (env.startsWith('/')) {
      return File::decodeName(env) + sep;
   } else {
      return Dir::getHomePath() + sep + File::decodeName(env) + sep;
   }
#else
   // When using a proper XDG platform, use QStandardPaths rather than the above hand-written code;
   // it makes the use of test mode from unit tests possible.
   // Ideally all platforms should use this, but see above for the migration issue.
   return StandardPaths::writableLocation(StandardPaths::StandardLocation::GenericConfigLocation) + sep;
#endif
}
#endif // !PDK_OS_WIN

void init_default_paths(std::unique_lock<std::mutex> locker)
{
   PathHash *pathHash = sg_pathHashFunc();
   locker.unlock();
   
   // LibraryInfo::location() uses Settings, so in order to
   // avoid a dead-lock, we can't hold the global mutex while
   // calling it.
   String systemPath = LibraryInfo::getPath(LibraryInfo::LibraryLocation::SettingsPath) + Latin1Character('/');
   
   locker.lock();
   if (pathHash->empty()) {
      // Lazy initialization of pathHash. We initialize the
      // IniFormat paths and (on Unix) the NativeFormat paths.
      // (The NativeFormat paths are not configurable for the
      // Windows registry and the Mac CFPreferences.)
#ifdef PDK_OS_WIN
      
      const String roamingAppDataFolder = windows_config_path(FOLDERID_RoamingAppData);
      const String programDataFolder = windows_config_path(FOLDERID_ProgramData);
      pathHash->insert(path_hash_key(Settings::Format::IniFormat, Settings::Scope::UserScope),
                       Path(roamingAppDataFolder + Dir::getSeparator(), false));
      pathHash->insert(path_hash_key(Settings::Format::IniFormat, Settings::Scope::SystemScope),
                       Path(programDataFolder + Dir::getSeparator(), false));
#else
      const String userPath = make_user_path();
      (*pathHash)[path_hash_key(Settings::Format::IniFormat, Settings::Scope::UserScope)] = Path(userPath, false);
      (*pathHash)[path_hash_key(Settings::Format::IniFormat, Settings::Scope::SystemScope)] = Path(systemPath, false);
#ifndef PDK_OS_MAC
      (*pathHash)[path_hash_key(Settings::Format::NativeFormat, Settings::Scope::UserScope)] = Path(userPath, false);
      (*pathHash)[path_hash_key(Settings::Format::NativeFormat, Settings::Scope::SystemScope)] = Path(systemPath, false);
#endif
#endif // PDK_OS_WIN
   }
}

Path get_path(Settings::Format format, Settings::Scope scope)
{
   PDK_ASSERT((int)Settings::Format::NativeFormat == 0);
   PDK_ASSERT((int)Settings::Format::IniFormat == 1);
   
   std::unique_lock<std::mutex> locker(sg_settingsGlobalMutex);
   PathHash *pathHash = sg_pathHashFunc();
   if (pathHash->empty()) {
      init_default_paths(std::move(locker));
   }
   Path result = pathHash->at(path_hash_key(format, scope));
   if (!result.m_path.isEmpty()) {
      return result;
   }
   // fall back on INI path
   return pathHash->at(path_hash_key(Settings::Format::IniFormat, scope));
}

} // anonymous namespace

#if defined(PDK_XDG_PLATFORM) && !defined(PDK_NO_STANDARDPATHS)
// Note: Suitable only for autotests.
void Q_AUTOTEST_EXPORT clear_default_paths()
{
   std::lock_guard<std::mutex> locker(sg_settingsGlobalMutex);
   sg_pathHashFunc()->clear();
}
#endif // PDK_XDG_PLATFORM && !PDK_NO_STANDARDPATHS

ConfFileSettingsPrivate::ConfFileSettingsPrivate(Settings::Format format,
                                                 Settings::Scope scope,
                                                 const String &organization,
                                                 const String &application)
   : SettingsPrivate(format, scope, organization, application),
     m_nextPosition(0x40000000) // big positive number
{
   initFormat();
   
   String org = organization;
   if (org.isEmpty()) {
      setStatus(Settings::Status::AccessError);
      org = Latin1String("Unknown Organization");
   }
   
   String appFile = org + Dir::getSeparator() + application + m_extension;
   String orgFile = org + m_extension;
   
   if (scope == Settings::Scope::UserScope) {
      Path userPath = get_path(format, Settings::Scope::UserScope);
      if (!application.isEmpty()) {
         m_confFiles.push_back(ConfFile::fromName(userPath.m_path + appFile, true));
      }
      m_confFiles.push_back(ConfFile::fromName(userPath.m_path + orgFile, true));
   }
   Path systemPath = get_path(format, Settings::Scope::SystemScope);
#if defined(PDK_XDG_PLATFORM) && !defined(PDK_NO_STANDARDPATHS)
   // check if the systemPath wasn't overridden by Settings::setPath()
   if (!systemPath.m_userDefined) {
      // Note: We can't use QStandardPaths::locateAll() as we need all the
      // possible files (not just the existing ones) and there is no way
      // to exclude user specific (XDG_CONFIG_HOME) directory from the search.
      StringList dirs = StandardPaths::standardLocations(StandardPaths::StandardLocation::GenericConfigLocation);
      // remove the StandardLocation::writableLocation() (XDG_CONFIG_HOME)
      if (!dirs.empty()) {
         dirs.takeFirst();
      }
      StringList paths;
      if (!application.isEmpty()) {
         paths.resize(dirs.size() * 2);
         for (const auto &dir : std::as_const(dirs)) {
            paths.push_back(dir + Latin1Character('/') + appFile);
         }  
      } else {
         paths.resize(dirs.size());
      }
      for (const auto &dir : std::as_const(dirs)) {
         paths.push_back(dir + Latin1Character('/') + orgFile);
      }
      // Note: No check for existence of files is done intentionaly.
      for (const auto &path : std::as_const(paths)) {
         m_confFiles.push_back(ConfFile::fromName(path, false));
      }
   } else
#endif // PDK_XDG_PLATFORM && !PDK_NO_STANDARDPATHS
   {
      if (!application.isEmpty()) {
         m_confFiles.push_back(ConfFile::fromName(systemPath.m_path + appFile, false));
      }
      m_confFiles.push_back(ConfFile::fromName(systemPath.m_path + orgFile, false));
   }
   initAccess();
}

ConfFileSettingsPrivate::ConfFileSettingsPrivate(const String &fileName,
                                                 Settings::Format format)
   : SettingsPrivate(format),
     m_nextPosition(0x40000000) // big positive number
{
   initFormat();
   m_confFiles.push_back(ConfFile::fromName(fileName, true));
   initAccess();
}

ConfFileSettingsPrivate::~ConfFileSettingsPrivate()
{
   std::lock_guard<std::mutex> locker(sg_settingsGlobalMutex);
   ConfFileHash *usedHash = sg_usedHashFunc();
   ConfFileCache *unusedCache = sg_unusedCacheFunc();
   for (auto confFile : std::as_const(m_confFiles)) {
      if (!confFile->m_ref.deref()) {
         if (confFile->m_size == 0) {
            delete confFile;
         } else {
            if (usedHash)
               usedHash->erase(confFile->m_name);
            if (unusedCache) {
               try {
                  // compute a better size?
                  unusedCache->insert(confFile->m_name, confFile,
                                      10 + (confFile->m_originalKeys.size() / 4));
               } catch(...) {
                  // out of memory. Do not cache the file.
                  delete confFile;
               }
            } else {
               // unusedCache is gone - delete the entry to prevent a memory leak
               delete confFile;
            }
         }
      }
   }
}

void ConfFileSettingsPrivate::remove(const String &key)
{
   if (m_confFiles.empty()) {
      return;
   }
   
   // Note: First config file is always the most specific.
   ConfFile *confFile = m_confFiles.at(0);
   
   SettingsKey theKey(key, m_caseSensitivity);
   SettingsKey prefix(key + Latin1Character('/'), m_caseSensitivity);
   std::lock_guard<std::mutex> locker(confFile->m_mutex);
   
   ensureSectionParsed(confFile, theKey);
   ensureSectionParsed(confFile, prefix);
   
   ParsedSettingsMap::iterator iter1 = confFile->m_addedKeys.lower_bound(prefix);
   while (iter1 != confFile->m_addedKeys.end() && iter1->first.startsWith(prefix)) {
      iter1 = confFile->m_addedKeys.erase(iter1);
   }
   confFile->m_addedKeys.erase(theKey);
   
   ParsedSettingsMap::const_iterator iter2 = const_cast<const ParsedSettingsMap *>(&confFile->m_originalKeys)->lower_bound(prefix);
   while (iter2 != confFile->m_originalKeys.cend() && iter2->first.startsWith(prefix)) {
      confFile->m_removedKeys[iter2->first] = std::any();
      ++iter2;
   }
   if (confFile->m_originalKeys.find(theKey) != confFile->m_originalKeys.end()) {
      confFile->m_removedKeys[theKey] = std::any();
   }
}

void ConfFileSettingsPrivate::set(const String &key, const std::any &value)
{
   if (m_confFiles.empty()) {
      return;
   }
   // Note: First config file is always the most specific.
   ConfFile *confFile = m_confFiles.at(0);
   SettingsKey theKey(key, m_caseSensitivity, m_nextPosition++);
   std::lock_guard<std::mutex> locker(confFile->m_mutex);
   confFile->m_removedKeys.erase(theKey);
   confFile->m_addedKeys[theKey] = value;
}

bool ConfFileSettingsPrivate::get(const String &key, std::any *value) const
{
   SettingsKey theKey(key, m_caseSensitivity);
   ParsedSettingsMap::const_iterator j;
   bool found = false;
   
   for (auto confFile : std::as_const(m_confFiles)) {
      std::lock_guard<std::mutex> locker(confFile->m_mutex);
      if (!confFile->m_addedKeys.empty()) {
         j = std::as_const(confFile->m_addedKeys).find(theKey);
         found = (j != confFile->m_addedKeys.cend());
      }
      if (!found) {
         ensureSectionParsed(confFile, theKey);
         j = std::as_const(confFile->m_originalKeys).find(theKey);
         found = (j != confFile->m_originalKeys.cend()
               && confFile->m_removedKeys.find(theKey) == confFile->m_removedKeys.end());
      }
      if (found && value) {
         *value = *j;
      }
      if (found) {
         return true;
      }
      if (!m_fallbacks) {
         break;
      }
   }
   return false;
}

StringList ConfFileSettingsPrivate::getChildren(const String &prefix, ChildSpec spec) const
{
   StringList result;
   ParsedSettingsMap::const_iterator j;
   
   SettingsKey thePrefix(prefix, m_caseSensitivity);
   int startPos = prefix.size();
   
   for (auto confFile : std::as_const(m_confFiles)) {
      std::lock_guard<std::mutex> locker(confFile->m_mutex);
      if (thePrefix.isEmpty()) {
         ensureAllSectionsParsed(confFile);
      } else {
         ensureSectionParsed(confFile, thePrefix);
      }
      j = const_cast<const ParsedSettingsMap *>(
               &confFile->m_originalKeys)->lower_bound(thePrefix);
      while (j != confFile->m_originalKeys.cend() && j->first.startsWith(thePrefix)) {
         if (confFile->m_removedKeys.find(j->first) == confFile->m_removedKeys.end()) {
            processChild(j->first.getOriginalCaseKey().substringRef(startPos), spec, result);
         }
         ++j;
      }
      
      j = const_cast<const ParsedSettingsMap *>(
               &confFile->m_addedKeys)->lower_bound(thePrefix);
      while (j != confFile->m_addedKeys.cend() && j->first.startsWith(thePrefix)) {
         processChild(j->first.getOriginalCaseKey().substringRef(startPos), spec, result);
         ++j;
      }
      if (!m_fallbacks) {
         break;
      }
   }
   result.sort();
   result.erase(std::unique(result.begin(), result.end()),
                result.end());
   return result;
}

void ConfFileSettingsPrivate::clear()
{
   if (m_confFiles.empty()) {
      return;
   }
   // Note: First config file is always the most specific.
   ConfFile *confFile = m_confFiles.at(0);
   
   std::lock_guard<std::mutex> locker(confFile->m_mutex);
   ensureAllSectionsParsed(confFile);
   confFile->m_addedKeys.clear();
   confFile->m_removedKeys = confFile->m_originalKeys;
}

void ConfFileSettingsPrivate::sync()
{
   // people probably won't be checking the status a whole lot, so in case of
   // error we just try to go on and make the best of it
   for (auto confFile : std::as_const(m_confFiles)) {
      std::lock_guard<std::mutex> locker(confFile->m_mutex);
      syncConfFile(confFile);
   }
}

void ConfFileSettingsPrivate::flush()
{
   sync();
}

String ConfFileSettingsPrivate::getFileName() const
{
   if (m_confFiles.empty()) {
      return String();
   }
   // Note: First config file is always the most specific.
   return m_confFiles.at(0)->m_name;
}

bool ConfFileSettingsPrivate::isWritable() const
{
   if (m_format > Settings::Format::IniFormat && !m_writeFunc) {
      return false;
   }
   if (m_confFiles.empty()) {
      return false;
   }
   return m_confFiles.at(0)->isWritable();
}

void ConfFileSettingsPrivate::syncConfFile(ConfFile *confFile)
{
   bool readOnly = confFile->m_addedKeys.empty() && confFile->m_removedKeys.empty();
   
   /*
        We can often optimize the read-only case, if the file on disk
        hasn't changed.
    */
   if (readOnly && confFile->m_size > 0) {
      FileInfo fileInfo(confFile->m_name);
      if (confFile->m_size == fileInfo.getSize() && confFile->m_timeStamp == fileInfo.getLastModified()) {
         return;
      } 
   }
   
   if (!readOnly && !confFile->isWritable()) {
      setStatus(Settings::Status::AccessError);
      return;
   }
   /*
        Use a lockfile in order to protect us against other Settings instances
        trying to write the same settings at the same time.
        
        We only need to lock if we are actually writing as only concurrent writes are a problem.
        Concurrent read and write are not a problem because the writing operation is atomic.
    */
   LockFile lockFile(confFile->m_name + Latin1String(".lock"));
   if (!readOnly && !lockFile.lock() && m_atomicSyncOnly) {
      setStatus(Settings::Status::AccessError);
      return;
   }
   
   /*
        We hold the lock. Let's reread the file if it has changed
        since last time we read it.
    */
   FileInfo fileInfo(confFile->m_name);
   bool mustReadFile = true;
   bool createFile = !fileInfo.exists();
   
   if (!readOnly)
      mustReadFile = (confFile->m_size != fileInfo.getSize()
            || (confFile->m_size != 0 && confFile->m_timeStamp != fileInfo.getLastModified()));
   
   if (mustReadFile) {
      confFile->m_unparsedIniSections.clear();
      confFile->m_originalKeys.clear();
      File file(confFile->m_name);
      if (!createFile && !file.open(File::OpenMode::ReadOnly)) {
         setStatus(Settings::Status::AccessError);
         return;
      }
      /*
            Files that we can't read (because of permissions or
            because they don't exist) are treated as empty files.
        */
      if (file.isReadable() && file.getSize() != 0) {
         bool ok = false;
#ifdef PDK_OS_MAC
         if (m_format == Settings::Format::NativeFormat) {
            ByteArray data = file.readAll();
            ok = readPlistFile(data, &confFile->m_originalKeys);
         } else
#endif
            if (m_format <= Settings::Format::IniFormat) {
               ByteArray data = file.readAll();
               ok = readIniFile(data, &confFile->m_unparsedIniSections);
            } else if (m_readFunc) {
               Settings::SettingsMap tempNewKeys;
               ok = m_readFunc(file, tempNewKeys);
               if (ok) {
                  Settings::SettingsMap::const_iterator iter = tempNewKeys.cbegin();
                  while (iter != tempNewKeys.cend()) {
                     confFile->m_originalKeys[SettingsKey(iter->first, m_caseSensitivity)] = iter->second;
                     ++iter;
                  }
               }
            }
         
         if (!ok) {
            setStatus(Settings::Status::FormatError);
         }
      }
      
      confFile->m_size = fileInfo.getSize();
      confFile->m_timeStamp = fileInfo.getLastModified();
   }
   
   /*
        We also need to save the file. We still hold the file lock,
        so everything is under control.
    */
   if (!readOnly) {
      bool ok = false;
      ensureAllSectionsParsed(confFile);
      ParsedSettingsMap mergedKeys = confFile->getMergedKeyMap();
      
#if PDK_CONFIG(temporaryfile)
      SaveFile sf(confFile->m_name);
      sf.setDirectWriteFallback(!m_atomicSyncOnly);
#else
      File sf(confFile->m_name);
#endif
      if (!sf.open(IoDevice::OpenMode::WriteOnly)) {
         setStatus(Settings::Status::AccessError);
         return;
      }
      
#ifdef PDK_OS_MAC
      if (m_format == Settings::Format::NativeFormat) {
         ok = writePlistFile(sf, mergedKeys);
      } else
#endif
         if (m_format <= Settings::Format::IniFormat) {
            ok = writeIniFile(sf, mergedKeys);
         } else if (m_writeFunc) {
            Settings::SettingsMap tempOriginalKeys;
            
            ParsedSettingsMap::const_iterator iter = mergedKeys.cbegin();
            while (iter != mergedKeys.cend()) {
               tempOriginalKeys[iter->first] = iter->second;
               ++iter;
            }
            ok = m_writeFunc(sf, tempOriginalKeys);
         }
      
#if PDK_CONFIG(temporaryfile)
      if (ok) {
         ok = sf.commit();
      }
#endif
      
      if (ok) {
         confFile->m_unparsedIniSections.clear();
         confFile->m_originalKeys = mergedKeys;
         confFile->m_addedKeys.clear();
         confFile->m_removedKeys.clear();
         
         FileInfo fileInfo(confFile->m_name);
         confFile->m_size = fileInfo.getSize();
         confFile->m_timeStamp = fileInfo.getLastModified();
         
         // If we have created the file, apply the file perms
         if (createFile) {
            File::Permissions perms = fileInfo.getPermissions() | File::Permission::ReadOwner | File::Permission::WriteOwner;
            if (!confFile->m_userPerms) {
               perms |= File::Permission::ReadGroup;
               perms |= File::Permission::ReadOther;
            }
            File(confFile->m_name).setPermissions(perms);
         }
      } else {
         setStatus(Settings::Status::AccessError);
      }
   }
}

enum { Space = 0x1, Special = 0x2 };

static const char sg_charTraits[256] =
{
   // Space: '\t', '\n', '\r', ' '
   // Special: '\n', '\r', '"', ';', '=', '\\'
   
   0, 0, 0, 0, 0, 0, 0, 0, 0, Space, Space | Special, 0, 0, Space | Special, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   Space, 0, Special, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Special, 0, Special, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Special, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

bool ConfFileSettingsPrivate::readIniLine(const ByteArray &data, int &dataPos,
                                          int &lineStart, int &lineLen, int &equalsPos)
{
   int dataLen = data.length();
   bool inQuotes = false;
   equalsPos = -1;
   lineStart = dataPos;
   while (lineStart < dataLen && (sg_charTraits[uint(uchar(data.at(lineStart)))] & Space)) {
      ++lineStart;
   }
   int i = lineStart;
   while (i < dataLen) {
      while (!(sg_charTraits[uint(uchar(data.at(i)))] & Special)) {
         if (++i == dataLen) {
            goto break_out_of_outer_loop;
         } 
      }
      char ch = data.at(i++);
      if (ch == '=') {
         if (!inQuotes && equalsPos == -1) {
            equalsPos = i - 1;
         }
      } else if (ch == '\n' || ch == '\r') {
         if (i == lineStart + 1) {
            ++lineStart;
         } else if (!inQuotes) {
            --i;
            goto break_out_of_outer_loop;
         }
      } else if (ch == '\\') {
         if (i < dataLen) {
            char ch = data.at(i++);
            if (i < dataLen) {
               char ch2 = data.at(i);
               // \n, \r, \r\n, and \n\r are legitimate line terminators in INI files
               if ((ch == '\n' && ch2 == '\r') || (ch == '\r' && ch2 == '\n')) {
                  ++i;
               }
            }
         }
      } else if (ch == '"') {
         inQuotes = !inQuotes;
      } else {
         PDK_ASSERT(ch == ';');
         if (i == lineStart + 1) {
            char ch;
            while (i < dataLen && (((ch = data.at(i)) != '\n') && ch != '\r')) {
               ++i;
            }
            lineStart = i;
         } else if (!inQuotes) {
            --i;
            goto break_out_of_outer_loop;
         }
      }
   }
break_out_of_outer_loop:
   dataPos = i;
   lineLen = i - lineStart;
   return lineLen > 0;
}

bool ConfFileSettingsPrivate::readIniFile(const ByteArray &data,
                                          UnparsedSettingsMap *unparsedIniSections)
{
#define FLUSH_CURRENT_SECTION() \
   { \
   ByteArray &sectionData = (*unparsedIniSections)[SettingsKey(currentSection, \
   IniCaseSensitivity, \
   sectionPosition)]; \
   if (!sectionData.isEmpty()) \
   sectionData.append('\n'); \
   sectionData += data.mid(currentSectionStart, lineStart - currentSectionStart); \
   sectionPosition = ++position; \
}
   
   String currentSection;
   int currentSectionStart = 0;
   int dataPos = 0;
   int lineStart;
   int lineLen;
   int equalsPos;
   int position = 0;
   int sectionPosition = 0;
   bool ok = true;
   
#ifndef PDK_NO_TEXTCODEC
   // detect utf8 BOM
   const uchar *dd = (const uchar *)data.getConstRawData();
   if (data.size() >= 3 && dd[0] == 0xef && dd[1] == 0xbb && dd[2] == 0xbf) {
      m_iniCodec = TextCodec::codecForName("UTF-8");
      dataPos = 3;
   }
#endif
   while (readIniLine(data, dataPos, lineStart, lineLen, equalsPos)) {
      char ch = data.at(lineStart);
      if (ch == '[') {
         FLUSH_CURRENT_SECTION();
         // this is a section
         ByteArray iniSection;
         int idx = data.indexOf(']', lineStart);
         if (idx == -1 || idx >= lineStart + lineLen) {
            ok = false;
            iniSection = data.mid(lineStart + 1, lineLen - 1);
         } else {
            iniSection = data.mid(lineStart + 1, idx - lineStart - 1);
         }
         iniSection = iniSection.trimmed();
         if (pdk::stricmp(iniSection.getConstRawData(), "general") == 0) {
            currentSection.clear();
         } else {
            if (pdk::stricmp(iniSection.getConstRawData(), "%general") == 0) {
               currentSection = Latin1String(iniSection.getConstRawData() + 1);
            } else {
               currentSection.clear();
               iniUnescapedKey(iniSection, 0, iniSection.size(), currentSection);
            }
            currentSection += Latin1Character('/');
         }
         currentSectionStart = dataPos;
      }
      ++position;
   }
   PDK_ASSERT(lineStart == data.length());
   FLUSH_CURRENT_SECTION();
   return ok;
   
#undef FLUSH_CURRENT_SECTION
}

bool ConfFileSettingsPrivate::readIniSection(const SettingsKey &section, const ByteArray &data,
                                             ParsedSettingsMap *settingsMap, TextCodec *codec)
{
   StringList strListValue;
   bool sectionIsLowercase = (section == section.getOriginalCaseKey());
   int equalsPos;
   bool ok = true;
   int dataPos = 0;
   int lineStart;
   int lineLen;
   int position = section.getOriginalKeyPosition();
   
   while (readIniLine(data, dataPos, lineStart, lineLen, equalsPos)) {
      char ch = data.at(lineStart);
      PDK_ASSERT(ch != '[');
      if (equalsPos == -1) {
         if (ch != ';') {
            ok = false;
         }
         continue;
      }
      int keyEnd = equalsPos;
      while (keyEnd > lineStart && ((ch = data.at(keyEnd - 1)) == ' ' || ch == '\t')) {
         --keyEnd;
      }
      int valueStart = equalsPos + 1;
      String key = section.getOriginalCaseKey();
      bool keyIsLowercase = (iniUnescapedKey(data, lineStart, keyEnd, key) && sectionIsLowercase);
      
      String strValue;
      strValue.reserve(lineLen - (valueStart - lineStart));
      bool isStringList = iniUnescapedStringList(data, valueStart, lineStart + lineLen,
                                                 strValue, strListValue, codec);
      std::any anyValue;
      if (isStringList) {
         anyValue = stringListToAnyList(strListValue);
      } else {
         anyValue = stringToAny(strValue);
      }
      
      /*
            We try to avoid the expensive toLower() call in
            SettingsKey by passing pdk::CaseSensitivity when the
            key is already in lowercase.
        */
      (*settingsMap)[SettingsKey(key, keyIsLowercase ? pdk::CaseSensitivity::Sensitive
                                                     : IniCaseSensitivity, position)] = anyValue;
      ++position;
   }
   
   return ok;
}

class SettingsIniKey : public String
{
public:
   inline SettingsIniKey() 
      : m_position(-1)
   {}
   
   inline SettingsIniKey(const String &str, int pos = -1)
      : String(str),
        m_position(pos) {}
   
   int m_position;
};

namespace {

bool operator<(const SettingsIniKey &k1, const SettingsIniKey &k2)
{
   if (k1.m_position != k2.m_position) {
      return k1.m_position < k2.m_position;
   }
   return static_cast<const String &>(k1) < static_cast<const String &>(k2);
}

} // anonymous namespace

using IniKeyMap = std::map<SettingsIniKey, std::any>;

struct SettingsIniSection
{
   int m_position;
   IniKeyMap m_keyMap;
   
   inline SettingsIniSection() 
      : m_position(-1)
   {}
};

using IniMap = std::map<String, SettingsIniSection>;

/*
    This would be more straightforward if we didn't try to remember the original
    key order in the .ini file, but we do.
*/
bool ConfFileSettingsPrivate::writeIniFile(IoDevice &device, const ParsedSettingsMap &map)
{
   IniMap iniMap;
   IniMap::const_iterator i;
   
#ifdef PDK_OS_WIN
   const char * const eol = "\r\n";
#else
   const char eol = '\n';
#endif
   
   for (ParsedSettingsMap::const_iterator j = map.cbegin(); j != map.cend(); ++j) {
      String section;
      SettingsIniKey key(j->first.getOriginalCaseKey(), j->first.getOriginalKeyPosition());
      int slashPos;
      if ((slashPos = key.indexOf(Latin1Character('/'))) != -1) {
         section = key.left(slashPos);
         key.remove(0, slashPos + 1);
      }
      
      SettingsIniSection &iniSection = iniMap[section];
      // -1 means infinity
      if (uint(key.m_position) < uint(iniSection.m_position)) {
         iniSection.m_position = key.m_position;
      }
      iniSection.m_keyMap[key] = j->second;
   }
   
   const int sectionCount = iniMap.size();
   std::vector<SettingsIniKey> sections;
   sections.resize(sectionCount);
   for (i = iniMap.cbegin(); i != iniMap.cend(); ++i) {
      sections.push_back(SettingsIniKey(i->first, i->second.m_position));
   }
   
   std::sort(sections.begin(), sections.end());
   
   bool writeError = false;
   for (int j = 0; !writeError && j < sectionCount; ++j) {
      i = std::as_const(iniMap).find(sections.at(j));
      PDK_ASSERT(i != iniMap.cend());
      ByteArray realSection;
      iniEscapedKey(i->first, realSection);
      if (realSection.isEmpty()) {
         realSection = "[General]";
      } else if (pdk::stricmp(realSection.getConstRawData(), "general") == 0) {
         realSection = "[%General]";
      } else {
         realSection.prepend('[');
         realSection.append(']');
      }
      if (j != 0) {
         realSection.prepend(eol);
      }
      realSection += eol;
      device.write(realSection);
      const IniKeyMap &ents = i->second.m_keyMap;
      for (IniKeyMap::const_iterator j = ents.cbegin(); j != ents.cend(); ++j) {
         ByteArray block;
         iniEscapedKey(j->first, block);
         block += '=';
         const std::any &value = j->second;
         /*
                The size() != 1 trick is necessary because
                std::any_cast<std::list>(std::any(String("foo"))) returns an empty
                list, not a list containing "foo".
            */
         if (value.type() == typeid(StringList)
             || (value.type() == typeid (std::list<std::any>) && std::any_cast<std::list<std::any>>(value).size() != 1)) {
            iniEscapedStringList(anyListToStringList(std::any_cast<std::list<std::any>>(value)), block, m_iniCodec);
         } else {
            iniEscapedString(anyToString(value), block, m_iniCodec);
         }
         block += eol;
         if (device.write(block) == -1) {
            writeError = true;
            break;
         }
      }
   }
   return !writeError;
}

void ConfFileSettingsPrivate::ensureAllSectionsParsed(ConfFile *confFile) const
{
   UnparsedSettingsMap::const_iterator i = confFile->m_unparsedIniSections.cbegin();
   const UnparsedSettingsMap::const_iterator end = confFile->m_unparsedIniSections.cend();
   
   for (; i != end; ++i) {
      if (!ConfFileSettingsPrivate::readIniSection(i->first, i->second, &confFile->m_originalKeys, m_iniCodec)) {
         setStatus(Settings::Status::FormatError);
      } 
   }
   confFile->m_unparsedIniSections.clear();
}

void ConfFileSettingsPrivate::ensureSectionParsed(ConfFile *confFile,
                                                  const SettingsKey &key) const
{
   if (confFile->m_unparsedIniSections.empty()) {
      return;
   }
   UnparsedSettingsMap::iterator i;
   int indexOfSlash = key.indexOf(Latin1Character('/'));
   if (indexOfSlash != -1) {
      i = confFile->m_unparsedIniSections.upper_bound(key);
      if (i == confFile->m_unparsedIniSections.begin()) {
         return;
      }
      --i;
      if (i->first.isEmpty() || !key.startsWith(i->first)) {
         return;
      }
      
   } else {
      i = confFile->m_unparsedIniSections.begin();
      if (i == confFile->m_unparsedIniSections.end() || !i->first.isEmpty()) {
         return;
      }
   }
   
   if (!ConfFileSettingsPrivate::readIniSection(i->first, i->second, &confFile->m_originalKeys, m_iniCodec)) {
      setStatus(Settings::Status::FormatError);
   }
   
   confFile->m_unparsedIniSections.erase(i);
}

} // internal

using internal::SettingsGroup;

Settings::Settings(const String &organization, const String &application, Object *parent)
   : Object(*SettingsPrivate::create(Format::NativeFormat, Scope::UserScope, organization, application),
            parent)
{}

Settings::Settings(Scope scope, const String &organization, const String &application,
                   Object *parent)
   : Object(*SettingsPrivate::create(Format::NativeFormat, scope, organization, application), parent)
{}

Settings::Settings(Format format, Scope scope, const String &organization,
                   const String &application, Object *parent)
   : Object(*SettingsPrivate::create(format, scope, organization, application), parent)
{}

Settings::Settings(const String &fileName, Format format, Object *parent)
   : Object(*SettingsPrivate::create(fileName, format), parent)
{
}

Settings::Settings(Object *parent)
   : Object(*SettingsPrivate::create(sg_globalDefaultFormat, Scope::UserScope,
                                     #ifdef PDK_OS_MAC
                                     CoreApplication::getOrgDomain().isEmpty()
                                     ? CoreApplication::getOrgName()
                                     : CoreApplication::getOrgDomain()
                                       #else
                                     CoreApplication::getOrgName().isEmpty()
                                     ? CoreApplication::getOrgDomain()
                                     : CoreApplication::getOrgDomain()
                                       #endif
                                       , CoreApplication::getAppName()),
            parent)
{
}

Settings::~Settings()
{
   PDK_D(Settings);
   if (implPtr->m_pendingChanges) {
      try {
         implPtr->flush();
      } catch(...) {
         ; // ok. then don't flush but at least don't throw in the destructor
      }
   }
}

void Settings::clear()
{
   PDK_D(Settings);
   implPtr->clear();
   implPtr->requestUpdate();
}

void Settings::sync()
{
   PDK_D(Settings);
   implPtr->sync();
   implPtr->m_pendingChanges = false;
}

String Settings::getFileName() const
{
   PDK_D(const Settings);
   return implPtr->getFileName();
}

Settings::Format Settings::getFormat() const
{
   PDK_D(const Settings);
   return implPtr->m_format;
}

Settings::Scope Settings::getScope() const
{
   PDK_D(const Settings);
   return implPtr->m_scope;
}

String Settings::getOrgName() const
{
   PDK_D(const Settings);
   return implPtr->m_orgName;
}

String Settings::getAppName() const
{
   PDK_D(const Settings);
   return implPtr->m_appName;
}

#ifndef PDK_NO_TEXTCODEC

void Settings::setIniCodec(TextCodec *codec)
{
   PDK_D(Settings);
   implPtr->m_iniCodec = codec;
}

void Settings::setIniCodec(const char *codecName)
{
   PDK_D(Settings);
   if (TextCodec *codec = TextCodec::codecForName(codecName)) {
      implPtr->m_iniCodec = codec;
   }
}

TextCodec *Settings::getIniCodec() const
{
   PDK_D(const Settings);
   return implPtr->m_iniCodec;
}

#endif // PDK_NO_TEXTCODEC

Settings::Status Settings::status() const
{
   PDK_D(const Settings);
   return implPtr->m_status;
}

bool Settings::isAtomicSyncRequired() const
{
   PDK_D(const Settings);
   return implPtr->m_atomicSyncOnly;
}

void Settings::setAtomicSyncRequired(bool enable)
{
   PDK_D(Settings);
   implPtr->m_atomicSyncOnly = enable;
}

void Settings::beginGroup(const String &prefix)
{
   PDK_D(Settings);
   implPtr->beginGroupOrArray(SettingsGroup(implPtr->normalizedKey(prefix)));
}

void Settings::endGroup()
{
   PDK_D(Settings);
   if (implPtr->m_groupStack.empty()) {
      warning_stream("Settings::endGroup: No matching beginGroup()");
      return;
   }
   
   SettingsGroup group = implPtr->m_groupStack.top();
   implPtr->m_groupStack.pop();
   int len = group.toString().size();
   if (len > 0)
      implPtr->m_groupPrefix.truncate(implPtr->m_groupPrefix.size() - (len + 1));
   
   if (group.isArray())
      warning_stream("Settings::endGroup: Expected endArray() instead");
}

String Settings::getGroup() const
{
   PDK_D(const Settings);
   return implPtr->m_groupPrefix.left(implPtr->m_groupPrefix.size() - 1);
}

int Settings::beginReadArray(const String &prefix)
{
   PDK_D(Settings);
   implPtr->beginGroupOrArray(SettingsGroup(implPtr->normalizedKey(prefix), false));
   return std::any_cast<int>(getValue(Latin1String("size")));
}

void Settings::beginWriteArray(const String &prefix, int size)
{
   PDK_D(Settings);
   implPtr->beginGroupOrArray(SettingsGroup(implPtr->normalizedKey(prefix), size < 0));
   if (size < 0) {
      remove(Latin1String("size"));
   } else {
      setValue(Latin1String("size"), size);
   }
}

void Settings::endArray()
{
   PDK_D(Settings);
   if (implPtr->m_groupStack.empty()) {
      warning_stream("Settings::endArray: No matching beginArray()");
      return;
   }
   SettingsGroup group = implPtr->m_groupStack.top();
   int len = group.toString().size();
   implPtr->m_groupStack.pop();
   if (len > 0) {
      implPtr->m_groupPrefix.truncate(implPtr->m_groupPrefix.size() - (len + 1));
   }
   if (group.arraySizeGuess() != -1) {
      setValue(group.getName() + Latin1String("/size"), group.arraySizeGuess());
   }
   if (!group.isArray()) {
      warning_stream("Settings::endArray: Expected endGroup() instead");
   }
}

void Settings::setArrayIndex(int i)
{
   PDK_D(Settings);
   if (implPtr->m_groupStack.empty() || !implPtr->m_groupStack.top().isArray()) {
      warning_stream("Settings::setArrayIndex: Missing beginArray()");
      return;
   }
   
   SettingsGroup &top = implPtr->m_groupStack.top();
   int len = top.toString().size();
   top.setArrayIndex(std::max(i, 0));
   implPtr->m_groupPrefix.replace(implPtr->m_groupPrefix.size() - len - 1, len, top.toString());
}

StringList Settings::getAllKeys() const
{
   PDK_D(const Settings);
   return implPtr->getChildren(implPtr->m_groupPrefix, SettingsPrivate::ChildSpec::AllKeys);
}

StringList Settings::getChildKeys() const
{
   PDK_D(const Settings);
   return implPtr->getChildren(implPtr->m_groupPrefix, SettingsPrivate::ChildSpec::ChildKeys);
}

StringList Settings::getChildGroups() const
{
   PDK_D(const Settings);
   return implPtr->getChildren(implPtr->m_groupPrefix, SettingsPrivate::ChildSpec::ChildGroups);
}

bool Settings::isWritable() const
{
   PDK_D(const Settings);
   return implPtr->isWritable();
}

void Settings::setValue(const String &key, const std::any &value)
{
   PDK_D(Settings);
   if (key.isEmpty()) {
      warning_stream("Settings::setValue: Empty key passed");
      return;
   }
   String k = implPtr->actualKey(key);
   implPtr->set(k, value);
   implPtr->requestUpdate();
}

void Settings::remove(const String &key)
{
   PDK_D(Settings);
   /*
        We cannot use actualKey(), because remove() supports empty
        keys. The code is also tricky because of slash handling.
    */
   String theKey = implPtr->normalizedKey(key);
   if (theKey.isEmpty()) {
      theKey = getGroup();
   } else {
      theKey.prepend(implPtr->m_groupPrefix);
   }
   if (theKey.isEmpty()) {
      implPtr->clear();
   } else {
      implPtr->remove(theKey);
   }
   implPtr->requestUpdate();
}

bool Settings::contains(const String &key) const
{
   PDK_D(const Settings);
   String k = implPtr->actualKey(key);
   return implPtr->get(k, 0);
}

void Settings::setFallbacksEnabled(bool b)
{
   PDK_D(Settings);
   implPtr->m_fallbacks = !!b;
}

bool Settings::fallbacksEnabled() const
{
   PDK_D(const Settings);
   return implPtr->m_fallbacks;
}

bool Settings::event(Event *event)
{
   PDK_D(Settings);
   if (event->getType() == Event::Type::UpdateRequest) {
      implPtr->update();
      return true;
   }
   return Object::event(event);
}

std::any Settings::getValue(const String &key, const std::any &defaultValue) const
{
   PDK_D(const Settings);
   if (key.isEmpty()) {
      warning_stream("Settings::value: Empty key passed");
      return std::any();
   }
   std::any result = defaultValue;
   String k = implPtr->actualKey(key);
   implPtr->get(k, &result);
   return result;
}

void Settings::setDefaultFormat(Format format)
{
   sg_globalDefaultFormat = format;
}

Settings::Format Settings::getDefaultFormat()
{
   return sg_globalDefaultFormat;
}

void Settings::setPath(Format format, Scope scope, const String &path)
{
   std::unique_lock<std::mutex> locker(sg_settingsGlobalMutex);
   PathHash *pathHash = sg_pathHashFunc();
   if (pathHash->empty()) {
      internal::init_default_paths(std::move(locker));
   }
   (*pathHash)[internal::path_hash_key(format, scope)] = Path(path + Dir::getSeparator(), true);
}

Settings::Format Settings::registerFormat(const String &extension, ReadFunc readFunc,
                                          WriteFunc writeFunc,
                                          pdk::CaseSensitivity caseSensitivity)
{
#ifdef PDK_SETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
   PDK_ASSERT(caseSensitivity == pdk::CaseSensitivity::Sensitive);
#endif
   
   std::lock_guard<std::mutex> locker(sg_settingsGlobalMutex);
   CustomFormatVector *customFormatVector = sg_customFormatVectorFunc();
   int index = customFormatVector->size();
   if (index == 16) {// the Settings::Format enum has room for 16 custom formats
      return Settings::Format::InvalidFormat;  
   }
   ConfFileCustomFormat info;
   info.m_extension = Latin1Character('.') + extension;
   info.m_readFunc = readFunc;
   info.m_writeFunc = writeFunc;
   info.m_caseSensitivity = caseSensitivity;
   customFormatVector->push_back(info);
   
   return Settings::Format((int)Settings::Format::CustomFormat1 + index);
}

} // fs
} // io
} // pdk

PDK_DECLARE_TYPEINFO(pdk::io::fs::ConfFileCustomFormat, PDK_MOVABLE_TYPE);
PDK_DECLARE_TYPEINFO(pdk::io::fs::internal::SettingsIniKey, PDK_MOVABLE_TYPE);
PDK_DECLARE_TYPEINFO(pdk::io::fs::internal::SettingsIniSection, PDK_MOVABLE_TYPE);

#endif // PDK_NO_SETTINGS

