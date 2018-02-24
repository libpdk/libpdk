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

#ifndef PDK_M_BASE_IO_FS_SETTINGS_H
#define PDK_M_BASE_IO_FS_SETTINGS_H

#include "pdk/kernel/Object.h"
#include "pdk/base/lang/String.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include <any>

#ifndef PDK_NO_SETTINGS
#include <ctype.h>
#endif // PDK_NO_SETTINGS

namespace pdk {
namespace io {

// forward declare class
class IoDevice;

namespace fs {

#ifdef Status // we seem to pick up a macro Status --> int somewhere
#undef Status
#endif

// forward declare class with namespace
namespace internal
{
class SettingsPrivate;
} // internal

using internal::SettingsPrivate;
using pdk::kernel::Object;
using pdk::kernel::Event;
using pdk::lang::String;
using pdk::ds::StringList;
using pdk::text::codecs::TextCodec;
using pdk::io::IoDevice;

class PDK_CORE_EXPORT Settings : public Object
{
   pdk::utils::ScopedPointer<SettingsPrivate> m_implPtr;
   PDK_DECLARE_PRIVATE(Settings);
   
public:
   enum class Status
   {
      NoError = 0,
      AccessError,
      FormatError
   };
   
   enum class Format {
      NativeFormat,
      IniFormat,
      
#ifdef PDK_OS_WIN
      Registry32Format,
      Registry64Format,
#endif
      
      InvalidFormat = 16,
      CustomFormat1,
      CustomFormat2,
      CustomFormat3,
      CustomFormat4,
      CustomFormat5,
      CustomFormat6,
      CustomFormat7,
      CustomFormat8,
      CustomFormat9,
      CustomFormat10,
      CustomFormat11,
      CustomFormat12,
      CustomFormat13,
      CustomFormat14,
      CustomFormat15,
      CustomFormat16
   };
   
   enum class Scope
   {
      UserScope,
      SystemScope
   };
   
   explicit Settings(const String &organization,
                     const String &application = String(), Object *parent = nullptr);
   Settings(Scope scope, const String &organization,
            const String &application = String(), Object *parent = nullptr);
   Settings(Format format, Scope scope, const String &organization,
            const String &application = String(), Object *parent = nullptr);
   Settings(const String &fileName, Format format, Object *parent = nullptr);
   explicit Settings(Object *parent = nullptr);
   
   ~Settings();
   
   void clear();
   void sync();
   Status status() const;
   bool isAtomicSyncRequired() const;
   void setAtomicSyncRequired(bool enable);
   
   void beginGroup(const String &prefix);
   void endGroup();
   String getGroup() const;
   
   int beginReadArray(const String &prefix);
   void beginWriteArray(const String &prefix, int size = -1);
   void endArray();
   void setArrayIndex(int i);
   
   StringList getAllKeys() const;
   StringList getChildKeys() const;
   StringList getChildGroups() const;
   bool isWritable() const;
   
   void setValue(const String &key, const std::any &value);
   std::any getValue(const String &key, const std::any &defaultValue = std::any()) const;
   
   void remove(const String &key);
   bool contains(const String &key) const;
   
   void setFallbacksEnabled(bool b);
   bool fallbacksEnabled() const;
   
   String getFileName() const;
   Format getFormat() const;
   Scope getScope() const;
   String getOrgName() const;
   String getAppName() const;
   
   void setIniCodec(TextCodec *codec);
   void setIniCodec(const char *codecName);
   TextCodec *getIniCodec() const;
   
   static void setDefaultFormat(Format format);
   static Format getDefaultFormat();
   static void setPath(Format format, Scope scope, const String &path);
   
   typedef std::map<String, std::any> SettingsMap;
   typedef bool (*ReadFunc)(IoDevice &device, SettingsMap &map);
   typedef bool (*WriteFunc)(IoDevice &device, const SettingsMap &map);
   
   static Format registerFormat(const String &extension, ReadFunc readFunc, WriteFunc writeFunc,
                                pdk::CaseSensitivity caseSensitivity = pdk::CaseSensitivity::Sensitive);
   
protected:
   bool event(Event *event) override;
   
private:
   PDK_DISABLE_COPY(Settings);
};

} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_SETTINGS_H
