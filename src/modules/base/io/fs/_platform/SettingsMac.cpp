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

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::kernel::CFType;
using pdk::kernel::CFString;
using pdk::io::fs::Settings;
using pdk::io::fs::internal::SettingsPrivate;
using pdk::ds::StringList;

static const CFStringRef sg_hostNames[2] = { kCFPreferencesCurrentHost, kCFPreferencesAnyHost };
static const int sg_numHostNames = 2;

enum RotateShift { Macify = 1, Pdkify = 2 };

namespace {

String rotate_slashes_dots_and_middots(const String &key, int shift)
{
}

CFType<CFStringRef> mac_key(const String &key)
{
}

String pdk_key(CFStringRef cfkey)
{
}

CFType<CFPropertyListRef> mac_value(const std::any &value);

CFArrayRef mac_list(const std::list<std::any> &list)
{
}

CFType<CFPropertyListRef> mac_value(const std::any &value)
{
}

std::any pdk_value(CFPropertyListRef cfvalue)
{
}

String comify(const String &organization)
{
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
   
}

MacSettingsPrivate::~MacSettingsPrivate()
{
}

void MacSettingsPrivate::remove(const String &key)
{
   
}

void MacSettingsPrivate::set(const String &key, const std::any &value)
{
}

bool MacSettingsPrivate::get(const String &key, std::any *value) const
{
}

StringList MacSettingsPrivate::getChildren(const String &prefix, ChildSpec spec) const
{
}

void MacSettingsPrivate::clear()
{
}

void MacSettingsPrivate::sync()
{
}

void MacSettingsPrivate::flush()
{
}

bool MacSettingsPrivate::isWritable() const
{
}

String MacSettingsPrivate::getFileName() const
{
}

SettingsPrivate *SettingsPrivate::create(Settings::Format format,
                                         Settings::Scope scope,
                                         const String &organization,
                                         const String &application)
{
}

bool ConfFileSettingsPrivate::readPlistFile(const ByteArray &data, ParsedSettingsMap *map) const
{
   
}

bool ConfFileSettingsPrivate::writePlistFile(IoDevice &file, const ParsedSettingsMap &map) const
{
}

} // internal
} // fs
} // io
} // pdk

#endif // PDK_NO_SETTINGS
