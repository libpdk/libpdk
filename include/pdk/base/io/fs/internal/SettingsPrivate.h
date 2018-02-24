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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_SETTINGS_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_SETTINGS_PRIVATE_H

#include "pdk/base/time/DateTime.h"
#include "pdk/base/io/IoDevice.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/base/io/fs/Settings.h"
#include "pdk/base/text/codecs/TextCodec.h"

#include <map>
#include <any>
#include <mutex>
#include <list>
#include <stack>

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::lang::Latin1Character;
using pdk::ds::ByteArray;
using pdk::time::DateTime;
using pdk::os::thread::AtomicInt;
using pdk::kernel::internal::ObjectPrivate;
using pdk::text::codecs::TextCodec;
using pdk::io::IoDevice;

#ifndef PDK_OS_WIN
#define PDK_Settings_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
#endif

// used in testing framework
#define SETTINGS_P_H_VERSION 3

#ifdef PDK_Settings_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
static const pdk::CaseSensitivity IniCaseSensitivity = pdk::CaseSensitivity::Sensitive;

class SettingsKey : public String
{
public:
   inline SettingsKey(const String &key, pdk::CaseSensitivity cs, int /* position */ = -1)
      : String(key)
   {
      PDK_ASSERT(cs == pdk::CaseSensitivity::Sensitive); 
      PDK_UNUSED(cs);
   }
   
   inline String getOriginalCaseKey() const
   {
      return *this;
   }
   
   inline int getOriginalKeyPosition() const
   {
      return -1;
   }
};
#else
static const pdk::CaseSensitivity IniCaseSensitivity = pdk::CaseSensitivity::Sensitive;

class SettingsKey : public String
{
public:
   inline SettingsKey(const String &key, pdk::CaseSensitivity cs, int position = -1)
      : String(key), 
        m_theOriginalKey(key),
        m_theOriginalKeyPosition(position)
   {
      if (cs == pdk::CaseSensitivity::Sensitive) {
         String::operator=(toLower());
      }
   }
   
   inline String getOriginalCaseKey() const
   {
      return m_theOriginalKey;
   }
   
   inline int getOriginalKeyPosition() const
   {
      return m_theOriginalKeyPosition;
   }
   
private:
   String m_theOriginalKey;
   int m_theOriginalKeyPosition;
};
#endif

using UnparsedSettingsMap = std::map<SettingsKey, ByteArray>;
using ParsedSettingsMap = std::map<SettingsKey, std::any>;

class SettingsGroup
{
public:
   inline SettingsGroup()
      : m_num(-1),
        m_maxNum(-1)
   {}
   
   inline SettingsGroup(const String &s)
      : m_str(s), 
        m_num(-1), 
        m_maxNum(-1) 
   {}
   
   inline SettingsGroup(const String &s, bool guessArraySize)
      : m_str(s),
        m_num(0),
        m_maxNum(guessArraySize ? 0 : -1)
   {}
   
   inline String getName() const
   {
      return m_str;
   }
   inline String toString() const;
   inline bool isArray() const 
   {
      return m_num != -1;
   }
   inline int arraySizeGuess() const
   {
      return m_maxNum;
   }
   
   inline void setArrayIndex(int i)
   {
      m_num = i + 1;
      if (m_maxNum != -1 && m_num > m_maxNum) {
         m_maxNum = m_num;
      }
   }
   
   String m_str;
   int m_num;
   int m_maxNum;
};

inline String SettingsGroup::toString() const
{
   String result;
   result = m_str;
   if (m_num > 0) {
      result += Latin1Character('/');
      result += String::number(m_num);
   }
   return result;
}

class PDK_UNITTEST_EXPORT ConfFile
{
public:
   ~ConfFile();
   
   ParsedSettingsMap getMergedKeyMap() const;
   bool isWritable() const;
   
   static ConfFile *fromName(const String &name, bool _userPerms);
   static void clearCache();
   
   String m_name;
   DateTime m_timeStamp;
   pdk::pint64 m_size;
   UnparsedSettingsMap m_unparsedIniSections;
   ParsedSettingsMap m_originalKeys;
   ParsedSettingsMap m_addedKeys;
   ParsedSettingsMap m_removedKeys;
   AtomicInt m_ref;
   std::mutex m_mutex;
   bool m_userPerms;
   
private:
#ifdef PDK_DISABLE_COPY
   ConfFile(const ConfFile &) = delete;
   ConfFile &operator=(const ConfFile &) = delete;
#endif
   ConfFile(const String &name, bool userPerms);
   
   friend class ConfFile_createsItself; // silences compiler warning
};

using pdk::io::fs::Settings;

class PDK_UNITTEST_EXPORT SettingsPrivate : public ObjectPrivate
{
   PDK_DECLARE_PUBLIC(Settings);
   
public:
   SettingsPrivate(Settings::Format format);
   SettingsPrivate(Settings::Format format, Settings::Scope scope,
                   const String &organization, const String &application);
   virtual ~SettingsPrivate();
   
   virtual void remove(const String &key) = 0;
   virtual void set(const String &key, const std::any &value) = 0;
   virtual bool get(const String &key, std::any *value) const = 0;
   
   enum class ChildSpec { AllKeys, ChildKeys, ChildGroups };
   virtual StringList getChildren(const String &prefix, ChildSpec spec) const = 0;
   
   virtual void clear() = 0;
   virtual void sync() = 0;
   virtual void flush() = 0;
   virtual bool isWritable() const = 0;
   virtual String fileName() const = 0;
   
   String actualKey(const String &key) const;
   void beginGroupOrArray(const SettingsGroup &group);
   void setStatus(Settings::Status status) const;
   void requestUpdate();
   void update();
   
   static String normalizedKey(const String &key);
   static SettingsPrivate *create(Settings::Format format, Settings::Scope scope,
                                  const String &organization, const String &application);
   static SettingsPrivate *create(const String &fileName, Settings::Format format);
   
   static void processChild(StringRef key, ChildSpec spec, StringList &result);
   
   // Variant streaming functions
   static StringList anyListToStringList(const std::list<std::any> &l);
   static std::any stringListToVariantList(const StringList &l);
   
   // parser functions
   static String variantToString(const std::any &v);
   static std::any stringToVariant(const String &s);
   static void iniEscapedKey(const String &key, ByteArray &result);
   static bool iniUnescapedKey(const ByteArray &key, int from, int to, String &result);
   static void iniEscapedString(const String &str, ByteArray &result, TextCodec *codec);
   static void iniEscapedStringList(const StringList &strs, ByteArray &result, TextCodec *codec);
   static bool iniUnescapedStringList(const ByteArray &str, int from, int to,
                                      String &stringResult, StringList &stringListResult,
                                      TextCodec *codec);
   static StringList splitArgs(const String &s, int idx);
   
   Settings::Format m_format;
   Settings::Scope m_scope;
   String m_orgName;
   String m_appName;
   TextCodec *m_iniCodec;
   
protected:
   std::stack<SettingsGroup> m_groupStack;
   String m_groupPrefix;
   bool m_fallbacks;
   bool m_pendingChanges;
   bool m_atomicSyncOnly = true;
   mutable Settings::Status m_status;
};

class ConfFileSettingsPrivate : public SettingsPrivate
{
public:
   ConfFileSettingsPrivate(Settings::Format format, Settings::Scope scope,
                           const String &organization, const String &application);
   ConfFileSettingsPrivate(const String &fileName, Settings::Format format);
   ~ConfFileSettingsPrivate();
   
   void remove(const String &key) override;
   void set(const String &key, const std::any &value) override;
   bool get(const String &key, std::any *value) const override;
   
   StringList getChildren(const String &prefix, ChildSpec spec) const override;
   
   void clear() override;
   void sync() override;
   void flush() override;
   bool isWritable() const override;
   String fileName() const override;
   
   bool readIniFile(const ByteArray &data, UnparsedSettingsMap *unparsedIniSections);
   static bool readIniSection(const SettingsKey &section, const ByteArray &data,
                              ParsedSettingsMap *settingsMap, TextCodec *codec);
   static bool readIniLine(const ByteArray &data, int &dataPos, int &lineStart, int &lineLen,
                           int &equalsPos);
   
private:
   void initFormat();
   void initAccess();
   void syncConfFile(ConfFile *confFile);
   bool writeIniFile(IoDevice &device, const ParsedSettingsMap &map);
#ifdef PDK_OS_MAC
   bool readPlistFile(const ByteArray &data, ParsedSettingsMap *map) const;
   bool writePlistFile(IoDevice &file, const ParsedSettingsMap &map) const;
#endif
   void ensureAllSectionsParsed(ConfFile *confFile) const;
   void ensureSectionParsed(ConfFile *confFile, const SettingsKey &key) const;
   
   std::vector<ConfFile *> m_confFiles;
   Settings::ReadFunc m_readFunc;
   Settings::WriteFunc m_writeFunc;
   String m_extension;
   pdk::CaseSensitivity m_caseSensitivity;
   int m_nextPosition;
};

} // internal
} // fs
} // io
} // pdk

PDK_DECLARE_TYPEINFO(pdk::io::fs::internal::SettingsKey, PDK_MOVABLE_TYPE);
PDK_DECLARE_TYPEINFO(pdk::io::fs::internal::SettingsGroup, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_IO_FS_INTERNAL_SETTINGS_PRIVATE_H
