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
// Created by softboy on 2018/02/25.

#include "pdk/base/io/fs/Settings.h"

#ifndef PDK_NO_SETTINGS

#include "pdk/base/io/fs/internal/SettingsPrivate.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/kernel/internal/CoreMacPrivate.h"
#include "pdk/kernel/CoreApplication.h"
#include <any>
#include <map>

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::kernel::CFType;
using pdk::kernel::CFString;
using pdk::io::fs::Settings;
using pdk::kernel::CoreApplication;
using pdk::io::fs::internal::SettingsPrivate;
using pdk::ds::StringList;
using pdk::lang::Latin1Character;
using pdk::ds::VarLengthArray;
using pdk::time::DateTime;

static const CFStringRef sg_hostNames[2] = { kCFPreferencesCurrentHost, kCFPreferencesAnyHost };
static const int sg_numHostNames = 2;

enum RotateShift { Macify = 1, Pdkify = 2 };

namespace {

String rotate_slashes_dots_and_middots(const String &key, int shift)
{
   static const int NumKnights = 3;
   static const char knightsOfTheRoundTable[NumKnights] = { '/', '.', '\xb7' };
   String result = key;
   for (int i = 0; i < result.size(); ++i) {
      for (int j = 0; j < NumKnights; ++j) {
         if (result.at(i) == Latin1Character(knightsOfTheRoundTable[j])) {
            result[i] = Latin1Character(knightsOfTheRoundTable[(j + shift) % NumKnights]).unicode();
            break;
         }
      }
   }
   return result;
}

CFType<CFStringRef> mac_key(const String &key)
{
   return rotate_slashes_dots_and_middots(key, Macify).toCFString();
}

String pdk_key(CFStringRef cfkey)
{
   return rotate_slashes_dots_and_middots(String::fromCFString(cfkey), Pdkify);
}

CFType<CFPropertyListRef> mac_value(const std::any &value);

CFArrayRef mac_list(const std::list<std::any> &list)
{
   int n = list.size();
   VarLengthArray<CFType<CFPropertyListRef> > cfvalues(n);
   int i = 0;
   for (const std::any & item : list) {
      cfvalues[i++] = mac_value(item);
   }
   return CFArrayCreate(kCFAllocatorDefault, reinterpret_cast<const void **>(cfvalues.getRawData()),
                        CFIndex(n), &kCFTypeArrayCallBacks);
}

CFType<CFPropertyListRef> mac_value(const std::any &value)
{
   CFPropertyListRef result = 0;
   size_t valueType = value.type().hash_code();
   if (valueType == typeid(ByteArray).hash_code()) {
      ByteArray ba = std::any_cast<ByteArray>(value);
      result = CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(ba.getRawData()),
                            CFIndex(ba.size()));
   } else if (valueType == typeid(std::list<std::any>).hash_code() ||
              valueType == typeid(StringList).hash_code()) {
      result = mac_list(std::any_cast<std::list<std::any>>(value));
   } else if (valueType == typeid(std::map<String, std::any>).hash_code()) {
      // std::map<String, std::any> is potentially a multimap,
      // whereas CFDictionary is a single-valued map. To allow
      // for multiple values with the same key, we store
      // multiple values in a CFArray. To avoid ambiguities,
      // we also wrap lists in a CFArray singleton.
      std::map<String, std::any> map = std::any_cast<std::map<String, std::any>>(value);
      
      std::map<String, std::any>::const_iterator iter = map.cbegin();
      
      int maxUniqueKeys = map.size();
      int numUniqueKeys = 0;
      VarLengthArray<CFType<CFPropertyListRef> > cfkeys(maxUniqueKeys);
      VarLengthArray<CFType<CFPropertyListRef> > cfvalues(maxUniqueKeys);
      
      while (iter != map.cend()) {
         const String &key = iter->first;
         std::list<std::any> values;
         
         do {
            values.push_back(iter->second);
            ++iter;
         } while (iter != map.cend() && iter->first == key);
         
         bool singleton = (values.size() == 1);
         if (singleton) {
            size_t firstItemType = values.front().type().hash_code();
            if (firstItemType == typeid(std::list<std::any>).hash_code() ||
                firstItemType == typeid(StringList).hash_code()) {
               singleton = false;
            }
         }
         
         cfkeys[numUniqueKeys] = key.toCFString();
         cfvalues[numUniqueKeys] = singleton ? mac_value(values.front()) : mac_list(values);
         ++numUniqueKeys;
      }
      
      result = CFDictionaryCreate(kCFAllocatorDefault,
                                  reinterpret_cast<const void **>(cfkeys.getRawData()),
                                  reinterpret_cast<const void **>(cfvalues.getRawData()),
                                  CFIndex(numUniqueKeys),
                                  &kCFTypeDictionaryKeyCallBacks,
                                  &kCFTypeDictionaryValueCallBacks);
   } else if (valueType == typeid(DateTime).hash_code()) {
      DateTime dateTime = std::any_cast<DateTime>(value);
      // CFDate, unlike DateTime, doesn't store timezone information
      if (dateTime.getTimeSpec() == pdk::TimeSpec::LocalTime) {
         result = dateTime.toCFDate();
      } else {
         goto string_case;
      }
   } else if (valueType == typeid(bool).hash_code()) {
      result = std::any_cast<bool>(value) ? kCFBooleanTrue : kCFBooleanFalse;
   } else if (valueType == typeid(int).hash_code() ||
              valueType == typeid(uint).hash_code()) {
      int n = std::any_cast<int>(value);
      result = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &n);
   } else if (valueType == typeid(double).hash_code()) {
      double n = std::any_cast<double>(value);
      result = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &n);
   } else if (valueType == typeid(long).hash_code() ||
              valueType == typeid(ulong).hash_code()) {
      pdk::pint64 n = std::any_cast<pdk::plonglong>(value);
      result = CFNumberCreate(0, kCFNumberLongLongType, &n);
   } else if (valueType == typeid(String).hash_code()) {
string_case:
      String string = SettingsPrivate::anyToString(value);
      if (string.contains(Character::Null))
         result = std::move(string).toUtf8().toCFData();
      else
         result = string.toCFString();
   }
   
   return result;
}

std::any pdk_value(CFPropertyListRef cfvalue)
{
   if (!cfvalue) {
      return std::any();
   }
   CFTypeID typeId = CFGetTypeID(cfvalue);
   
   // Sorted grossly from most to least frequent type.
   
   if (typeId == CFStringGetTypeID()) {
      return SettingsPrivate::stringToAny(String::fromCFString(static_cast<CFStringRef>(cfvalue)));
   } else if (typeId == CFNumberGetTypeID()) {
      CFNumberRef cfnumber = static_cast<CFNumberRef>(cfvalue);
      if (CFNumberIsFloatType(cfnumber)) {
         double d;
         CFNumberGetValue(cfnumber, kCFNumberDoubleType, &d);
         return d;
      } else {
         int i;
         pdk::pint64 ll;
         
         if (CFNumberGetType(cfnumber) == kCFNumberIntType) {
            CFNumberGetValue(cfnumber, kCFNumberIntType, &i);
            return i;
         }
         CFNumberGetValue(cfnumber, kCFNumberLongLongType, &ll);
         return ll;
      }
   } else if (typeId == CFArrayGetTypeID()) {
      CFArrayRef cfarray = static_cast<CFArrayRef>(cfvalue);
      std::list<std::any> list;
      CFIndex size = CFArrayGetCount(cfarray);
      bool metNonString = false;
      for (CFIndex i = 0; i < size; ++i) {
         std::any value = pdk_value(CFArrayGetValueAtIndex(cfarray, i));
         if (value.type().hash_code() != typeid(String).hash_code()) {
            metNonString = true;
         }
         list.push_back(value);
      }
      if (metNonString) {
         return list;
      } else {
         StringList ret;
         for (auto &item : list) {
            ret.push_back(std::any_cast<String>(item));
         }
         return ret;
      }
   } else if (typeId == CFBooleanGetTypeID()) {
      return (bool)CFBooleanGetValue(static_cast<CFBooleanRef>(cfvalue));
   } else if (typeId == CFDataGetTypeID()) {
      ByteArray byteArray = ByteArray::fromRawCFData(static_cast<CFDataRef>(cfvalue));
      
      // Fast-path for QByteArray, so that we don't have to go
      // though the expensive and lossy conversion via UTF-8.
      if (!byteArray.startsWith('@')) {
         byteArray.detach();
         return byteArray;
      }
      
      const String str = String::fromUtf8(byteArray.getConstRawData(), byteArray.size());
      return SettingsPrivate::stringToAny(str);
   } else if (typeId == CFDictionaryGetTypeID()) {
      CFDictionaryRef cfdict = static_cast<CFDictionaryRef>(cfvalue);
      CFTypeID arrayTypeId = CFArrayGetTypeID();
      int size = (int)CFDictionaryGetCount(cfdict);
      VarLengthArray<CFPropertyListRef> keys(size);
      VarLengthArray<CFPropertyListRef> values(size);
      CFDictionaryGetKeysAndValues(cfdict, keys.getRawData(), values.getRawData());
      
      std::multimap<String, std::any> map;
      for (int i = 0; i < size; ++i) {
         String key = String::fromCFString(static_cast<CFStringRef>(keys[i]));
         if (CFGetTypeID(values[i]) == arrayTypeId) {
            CFArrayRef cfarray = static_cast<CFArrayRef>(values[i]);
            CFIndex arraySize = CFArrayGetCount(cfarray);
            for (CFIndex j = arraySize - 1; j >= 0; --j)
               map.insert(std::make_pair(key, pdk_value(CFArrayGetValueAtIndex(cfarray, j))));
         } else {
            map.insert(std::make_pair(key, pdk_value(values[i])));
         }
      }
      return map;
   } else if (typeId == CFDateGetTypeID()) {
      return DateTime::fromCFDate(static_cast<CFDateRef>(cfvalue));
   }
   return std::any();
}

String comify(const String &organization)
{
   for (int i = organization.size() - 1; i >= 0; --i) {
      Character ch = organization.at(i);
      if (ch == Latin1Character('.') || ch == Character(0x3002) || ch == Character(0xff0e)
          || ch == Character(0xff61)) {
         String suffix = organization.substring(i + 1).toLower();
         if (suffix.size() == 2 || suffix == Latin1String("com")
             || suffix == Latin1String("org") || suffix == Latin1String("net")
             || suffix == Latin1String("edu") || suffix == Latin1String("gov")
             || suffix == Latin1String("mil") || suffix == Latin1String("biz")
             || suffix == Latin1String("info") || suffix == Latin1String("name")
             || suffix == Latin1String("pro") || suffix == Latin1String("aero")
             || suffix == Latin1String("coop") || suffix == Latin1String("museum")) {
            String result = organization;
            result.replace(Latin1Character('/'), Latin1Character(' '));
            return result;
         }
         break;
      }
      int uc = ch.unicode();
      if ((uc < 'a' || uc > 'z') && (uc < 'A' || uc > 'Z')) {
         break;
      }
   }
   
   String domain;
   for (int i = 0; i < organization.size(); ++i) {
      Character ch = organization.at(i);
      int uc = ch.unicode();
      if ((uc >= 'a' && uc <= 'z') || (uc >= '0' && uc <= '9')) {
         domain += ch;
      } else if (uc >= 'A' && uc <= 'Z') {
         domain += ch.toLower();
      } else {
         domain += Latin1Character(' ');
      }
   }
   domain = domain.simplified();
   domain.replace(Latin1Character(' '), Latin1Character('-'));
   if (!domain.isEmpty()) {
      domain.append(Latin1String(".com"));
   }
   return domain;
}

} // anonymous namespace

class MacSettingsPrivate : public SettingsPrivate
{
public:
   MacSettingsPrivate(Settings::Scope scope, const String &organization,
                      const String &application);
   ~MacSettingsPrivate();
   
   void remove(const String &key);
   void set(const String &key, const std::any &value);
   bool get(const String &key, std::any *value) const;
   StringList getChildren(const String &prefix, ChildSpec spec) const;
   void clear();
   void sync();
   void flush();
   bool isWritable() const;
   String getFileName() const;
   
private:
   struct SearchDomain
   {
      CFStringRef m_userName;
      CFStringRef m_appOrSuiteId;
   };
   
   CFString m_appId;
   CFString m_suiteId;
   CFString m_hostName;
   SearchDomain m_domains[6];
   int m_numDomains;
};

MacSettingsPrivate::MacSettingsPrivate(Settings::Scope scope, const String &organization,
                                       const String &application)
   : SettingsPrivate(Settings::Format::NativeFormat, scope, organization, application)
{
   String javaPackageName;
   int curPos = 0;
   int nextDot;
   
   // attempt to use the organization parameter
   String domainName = comify(organization);
   // if not found, attempt to use the bundle identifier.
   if (domainName.isEmpty()) {
      CFBundleRef main_bundle = CFBundleGetMainBundle();
      if (main_bundle != NULL) {
         CFStringRef main_bundle_identifier = CFBundleGetIdentifier(main_bundle);
         if (main_bundle_identifier != NULL) {
            String bundle_identifier(pdk_key(main_bundle_identifier));
            // CFBundleGetIdentifier returns identifier separated by slashes rather than periods.
            StringList bundle_identifier_components = bundle_identifier.split(Latin1Character('/'));
            // pre-reverse them so that when they get reversed again below, they are in the com.company.product format.
            StringList bundle_identifier_components_reversed;
            for (size_t i = 0; i < bundle_identifier_components.size(); ++i) {
               const String &bundle_identifier_component = bundle_identifier_components.at(i);
               bundle_identifier_components_reversed.push_front(bundle_identifier_component);
            }
            domainName = bundle_identifier_components_reversed.join(Latin1Character('.'));
         }
      }
   }
   // if no bundle identifier yet. use a hard coded string.
   if (domainName.isEmpty()) {
      domainName = Latin1String("unknown-organization.trolltech.com");
   }
   
   while ((nextDot = domainName.indexOf(Latin1Character('.'), curPos)) != -1) {
      javaPackageName.prepend(domainName.substringRef(curPos, nextDot - curPos));
      javaPackageName.prepend(Latin1Character('.'));
      curPos = nextDot + 1;
   }
   javaPackageName.prepend(domainName.substringRef(curPos));
   javaPackageName = std::move(javaPackageName).toLower();
   if (curPos == 0) {
      javaPackageName.prepend(Latin1String("com."));
   }
   m_suiteId = javaPackageName;
   
   if (!application.isEmpty()) {
      javaPackageName += Latin1Character('.') + application;
      m_appId = javaPackageName;
   }
   
   m_numDomains = 0;
   for (int i = (scope == Settings::Scope::SystemScope) ? 1 : 0; i < 2; ++i) {
      for (int j = (application.isEmpty()) ? 1 : 0; j < 3; ++j) {
         SearchDomain &domain = m_domains[m_numDomains++];
         domain.m_userName = (i == 0) ? kCFPreferencesCurrentUser : kCFPreferencesAnyUser;
         if (j == 0) {
            domain.m_appOrSuiteId = m_appId;
         } else if (j == 1) {
            domain.m_appOrSuiteId = m_suiteId;
         } else {
            domain.m_appOrSuiteId = kCFPreferencesAnyApplication;
         } 
      }
   }
   
   m_hostName = (scope == Settings::Scope::SystemScope) ? kCFPreferencesCurrentHost : kCFPreferencesAnyHost;
   sync();
}

MacSettingsPrivate::~MacSettingsPrivate()
{
}

void MacSettingsPrivate::remove(const String &key)
{
   StringList keys = getChildren(key + Latin1Character('/'), ChildSpec::AllKeys);
   
   // If i == -1, then delete "key" itself.
   for (int i = -1; i < (int)keys.size(); ++i) {
      String subKey = key;
      if (i >= 0) {
         subKey += Latin1Character('/');
         subKey += keys.at(i);
      }
      CFPreferencesSetValue(mac_key(subKey), 0, m_domains[0].m_appOrSuiteId,
            m_domains[0].m_userName, m_hostName);
   }
}

void MacSettingsPrivate::set(const String &key, const std::any &value)
{
   CFPreferencesSetValue(mac_key(key), mac_value(value), m_domains[0].m_appOrSuiteId,
         m_domains[0].m_userName, m_hostName);
}

bool MacSettingsPrivate::get(const String &key, std::any *value) const
{
   CFString k = mac_key(key);
   for (int i = 0; i < m_numDomains; ++i) {
      for (int j = 0; j < sg_numHostNames; ++j) {
         CFType<CFPropertyListRef> ret =
               CFPreferencesCopyValue(k, m_domains[i].m_appOrSuiteId, m_domains[i].m_userName,
                                      sg_hostNames[j]);
         if (ret) {
            if (value) {
               *value = pdk_value(ret);
            }
            return true;
         }
      }
      if (!m_fallbacks) {
         break;
      }
   }
   return false;
}

StringList MacSettingsPrivate::getChildren(const String &prefix, ChildSpec spec) const
{
   StringList result;
   int startPos = prefix.size();
   
   for (int i = 0; i < m_numDomains; ++i) {
      for (int j = 0; j < sg_numHostNames; ++j) {
         CFType<CFArrayRef> cfarray = CFPreferencesCopyKeyList(m_domains[i].m_appOrSuiteId,
                                                               m_domains[i].m_userName,
                                                               sg_hostNames[j]);
         if (cfarray) {
            CFIndex size = CFArrayGetCount(cfarray);
            for (CFIndex k = 0; k < size; ++k) {
               String currentKey =
                     pdk_key(static_cast<CFStringRef>(CFArrayGetValueAtIndex(cfarray, k)));
               if (currentKey.startsWith(prefix)) {
                  processChild(currentKey.substringRef(startPos), spec, result);
               }
            }
         }
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

void MacSettingsPrivate::clear()
{
   CFType<CFArrayRef> cfarray = CFPreferencesCopyKeyList(m_domains[0].m_appOrSuiteId,
         m_domains[0].m_userName, m_hostName);
   CFPreferencesSetMultiple(0, cfarray, m_domains[0].m_appOrSuiteId, m_domains[0].m_userName,
         m_hostName);
}

void MacSettingsPrivate::sync()
{
   for (int i = 0; i < m_numDomains; ++i) {
      for (int j = 0; j < sg_numHostNames; ++j) {
         Boolean ok = CFPreferencesSynchronize(m_domains[i].m_appOrSuiteId,
                                               m_domains[i].m_userName, sg_hostNames[j]);
         // only report failures for the primary file (the one we write to)
         if (!ok && i == 0 && sg_hostNames[j] == m_hostName && m_status == Settings::Status::NoError) {
            setStatus(Settings::Status::AccessError);
         }
      }
   }
}

void MacSettingsPrivate::flush()
{
   sync();
}

bool MacSettingsPrivate::isWritable() const
{
   MacSettingsPrivate *that = const_cast<MacSettingsPrivate *>(this);
   String impossibleKey(Latin1String("pdk_internal/"));
   
   Settings::Status oldStatus = that->m_status;
   that->m_status = Settings::Status::NoError;
   
   that->set(impossibleKey, std::any());
   that->sync();
   bool writable = (m_status == Settings::Status::NoError) && that->get(impossibleKey, 0);
   that->remove(impossibleKey);
   that->sync();
   
   that->m_status = oldStatus;
   return writable;
}

String MacSettingsPrivate::getFileName() const
{
   String result;
   if (m_scope == Settings::Scope::UserScope) {
      result = Dir::getHomePath();
   } 
   result += Latin1String("/Library/Preferences/");
   result += String::fromCFString(m_domains[0].m_appOrSuiteId);
   result += Latin1String(".plist");
   return result;
}

SettingsPrivate *SettingsPrivate::create(Settings::Format format,
                                         Settings::Scope scope,
                                         const String &organization,
                                         const String &application)
{
   // @TODO work with PDK_BOOTSTRAPPED
#ifndef PDK_BOOTSTRAPPED
   if (organization == Latin1String("pdk"))
   {
      String orgDomain = CoreApplication::getOrgDomain();
      String appName = CoreApplication::getAppName();
      
      SettingsPrivate *newSettings;
      if (format == Settings::Format::NativeFormat) {
         newSettings = new MacSettingsPrivate(scope, orgDomain, appName);
      } else {
         newSettings = new ConfFileSettingsPrivate(format, scope, orgDomain, appName);
      }
      
      newSettings->beginGroupOrArray(SettingsGroup(normalizedKey(organization)));
      if (!application.isEmpty()) {
         newSettings->beginGroupOrArray(SettingsGroup(normalizedKey(application)));
      }
      return newSettings;
   }
#endif
   if (format == Settings::Format::NativeFormat) {
      return new MacSettingsPrivate(scope, organization, application);
   } else {
      return new ConfFileSettingsPrivate(format, scope, organization, application);
   }
}

bool ConfFileSettingsPrivate::readPlistFile(const ByteArray &data, ParsedSettingsMap *map) const
{
   CFType<CFDataRef> cfData = data.toRawCFData();
   CFType<CFPropertyListRef> propertyList =
         CFPropertyListCreateWithData(kCFAllocatorDefault, cfData, kCFPropertyListImmutable, nullptr, nullptr);
   
   if (!propertyList) {
      return true;
   }
   
   if (CFGetTypeID(propertyList) != CFDictionaryGetTypeID()) {
      return false;
   }
   
   CFDictionaryRef cfdict =
         static_cast<CFDictionaryRef>(static_cast<CFPropertyListRef>(propertyList));
   int size = (int)CFDictionaryGetCount(cfdict);
   VarLengthArray<CFPropertyListRef> keys(size);
   VarLengthArray<CFPropertyListRef> values(size);
   CFDictionaryGetKeysAndValues(cfdict, keys.getRawData(), values.getRawData());
   
   for (int i = 0; i < size; ++i) {
      String key = pdk_key(static_cast<CFStringRef>(keys[i]));
      map->insert(std::make_pair(SettingsKey(key, pdk::CaseSensitivity::Sensitive), pdk_value(values[i])));
   }
   return true;
}

bool ConfFileSettingsPrivate::writePlistFile(IoDevice &file, const ParsedSettingsMap &map) const
{
   VarLengthArray<CFType<CFStringRef> > cfkeys(map.size());
   VarLengthArray<CFType<CFPropertyListRef> > cfvalues(map.size());
   int i = 0;
   ParsedSettingsMap::const_iterator j;
   for (j = map.cbegin(); j != map.cend(); ++j) {
      cfkeys[i] = mac_key(j->first);
      cfvalues[i] = mac_value(j->second);
      ++i;
   }
   
   CFType<CFDictionaryRef> propertyList =
         CFDictionaryCreate(kCFAllocatorDefault,
                            reinterpret_cast<const void **>(cfkeys.getRawData()),
                            reinterpret_cast<const void **>(cfvalues.getRawData()),
                            CFIndex(map.size()),
                            &kCFTypeDictionaryKeyCallBacks,
                            &kCFTypeDictionaryValueCallBacks);
   
   CFType<CFDataRef> xmlData = CFPropertyListCreateData(
            kCFAllocatorDefault, propertyList, kCFPropertyListXMLFormat_v1_0, 0, 0);
   
   return file.write(ByteArray::fromRawCFData(xmlData)) == CFDataGetLength(xmlData);
}

} // internal
} // fs
} // io
} // pdk

#endif // PDK_NO_SETTINGS
