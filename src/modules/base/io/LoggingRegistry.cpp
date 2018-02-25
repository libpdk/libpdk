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

#include "pdk/base/io/internal/LoggingRegisteryPrivate.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/StandardPaths.h"
#include "pdk/base/io/TextStream.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/global/Logging.h"
#include "pdk/global/GlobalStatic.h"
#include "pdk/global/LibraryInfo.h"
#include <vector>
#include <mutex>

namespace pdk {
namespace io {
namespace internal {

// We can't use the default macros because this would lead to recursion.
// Instead let's define our own one that unconditionally logs...
#define debugMsg pdk::MessageLogger(__FILE__, __LINE__, __FUNCTION__, "pdk.core.logging").debug
#define warnMsg pdk::MessageLogger(__FILE__, __LINE__, __FUNCTION__, "pdk.core.logging").warning

PDK_GLOBAL_STATIC(LoggingRegistry, sg_LoggingRegistry);

using pdk::lang::StringRef;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::ds::ByteArray;
using pdk::io::TextStream;
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::fs::StandardPaths;
using pdk::LibraryInfo;

LoggingRule::LoggingRule() :
   m_enabled(false)
{}
LoggingRule::LoggingRule(const StringRef &pattern, bool enabled) :
   m_messageType(-1),
   m_enabled(enabled)
{
   parse(pattern);
}

int LoggingRule::pass(const String &cat, pdk::MsgType msgType) const
{
   // check message type
   if (m_messageType > -1 && m_messageType != pdk::as_integer<pdk::MsgType>(msgType)) {
      return 0;
   }
   if (m_flags == FullText) {
      // full match
      if (m_category == cat) {
         return (m_enabled ? 1 : -1);
      } else {
         return 0;
      }
   }
   const int idx = cat.indexOf(m_category);
   if (idx >= 0) {
      if (m_flags == MidFilter) {
         // matches somewhere
         if (idx >= 0) {
            return (m_enabled ? 1 : -1);
         }
      } else if (m_flags == LeftFilter) {
         // matches left
         if (idx == 0) {
            return (m_enabled ? 1 : -1);
         }
      } else if (m_flags == RightFilter) {
         // matches right
         if (idx == (cat.count() - m_category.count())) {
            return (m_enabled ? 1 : -1);
         }
      }
   }
   return 0;
}

void LoggingRule::parse(const StringRef &pattern)
{
   StringRef p;
   // strip trailing ".messagetype"
   if (pattern.endsWith(Latin1String(".debug"))) {
      p = StringRef(pattern.getStr(), pattern.getPosition(),
                    pattern.length() - 6); // strlen(".debug")
      m_messageType = pdk::as_integer<pdk::MsgType>(pdk::MsgType::DebugMsg);
   } else if (pattern.endsWith(Latin1String(".info"))) {
      p = StringRef(pattern.getStr(), pattern.getPosition(),
                    pattern.length() - 5); // strlen(".info")
      m_messageType = pdk::as_integer<pdk::MsgType>(pdk::MsgType::InfoMsg);
   } else if (pattern.endsWith(Latin1String(".warning"))) {
      p = StringRef(pattern.getStr(), pattern.getPosition(),
                    pattern.length() - 8); // strlen(".warning")
      m_messageType = pdk::as_integer<pdk::MsgType>(pdk::MsgType::WarningMsg);
   } else if (pattern.endsWith(Latin1String(".critical"))) {
      p = StringRef(pattern.getStr(), pattern.getPosition(),
                    pattern.length() - 9); // strlen(".critical")
      m_messageType = pdk::as_integer<pdk::MsgType>(pdk::MsgType::CriticalMsg);
   } else {
      p = pattern;
   }
   
   if (!p.contains(Latin1Character('*'))) {
      m_flags = FullText;
   } else {
      if (p.endsWith(Latin1Character('*'))) {
         m_flags |= LeftFilter;
         p = StringRef(p.getStr(), p.getPosition(), p.length() - 1);
      }
      if (p.startsWith(Latin1Character('*'))) {
         m_flags |= RightFilter;
         p = StringRef(p.getStr(), p.getPosition() + 1, p.length() - 1);
      }
      if (p.contains(Latin1Character('*'))) {// '*' only supported at start/end
         m_flags = 0;
      }
   }
   
   m_category = p.toString();
}

void LoggingSettingsParser::setContent(const String &content)
{
   m_rules.clear();
   const auto lines = content.splitRef(Latin1Character('\n'));
   for (const auto &line : lines) {
      parseNextLine(line);
   }
}

void LoggingSettingsParser::setContent(TextStream &stream)
{
   m_rules.clear();
   String line;
   while (stream.readLineInto(&line)) {
      parseNextLine(StringRef(&line));
   }
}

void LoggingSettingsParser::parseNextLine(StringRef line)
{
   // Remove whitespace at start and end of line:
   line = line.trimmed();
   // comment
   if (line.startsWith(Latin1Character(';'))) {
      return;
   }
   if (line.startsWith(Latin1Character('[')) && line.endsWith(Latin1Character(']'))) {
      // new section
      auto sectionName = line.substring(1, line.size() - 2).trimmed();
      m_inRulesSection = sectionName.compare(Latin1String("rules"), pdk::CaseSensitivity::Insensitive) == 0;
      return;
   }
   if (m_inRulesSection) {
      int equalPos = line.indexOf(Latin1Character('='));
      if (equalPos != -1) {
         if (line.lastIndexOf(Latin1Character('=')) == equalPos) {
            const auto pattern = line.left(equalPos).trimmed();
            const auto valueStr = line.substring(equalPos + 1).trimmed();
            int value = -1;
            if (valueStr == Latin1String("true")) {
               value = 1;
            } else if (valueStr == Latin1String("false")) {
               value = 0;
            }
            LoggingRule rule(pattern, (value == 1));
            if (rule.m_flags != 0 && (value != -1)) {
               m_rules.push_back(rule);
            } else {
               warnMsg("Ignoring malformed logging rule: '%s'", line.toUtf8().getConstRawData());
            }
         } else {
            warnMsg("Ignoring malformed logging rule: '%s'", line.toUtf8().getConstRawData());
         }
      }
   }
}

LoggingRegistry::LoggingRegistry()
   : m_categoryFilter(defaultCategoryFilter)
{
   initializeRules(); // Init on first use
}

namespace {
bool logging_debug()
{
   static const bool debugEnv = pdk::env_var_isset("PDK_LOGGING_DEBUG");
   return debugEnv;
}

std::vector<LoggingRule> load_rules_from_file(const String &filePath)
{
   File file(filePath);
   if (file.open(IoDevice::OpenMode::ReadOnly | IoDevice::OpenMode::Text)) {
      if (logging_debug())
         debugMsg("Loading \"%s\" ...",
                  Dir::toNativeSeparators(file.getFileName()).toUtf8().getConstRawData());
      TextStream stream(&file);
      LoggingSettingsParser parser;
      parser.setContent(stream);
      return parser.getRules();
   }
   return std::vector<LoggingRule>();
}
} // anonymous namespace


void LoggingRegistry::initializeRules()
{
   std::vector<LoggingRule> er, qr, cr;
   // get rules from environment
   const ByteArray rulesFilePath = pdk::pdk_getenv("PDK_LOGGING_CONF");
   if (!rulesFilePath.isEmpty()) {
      er = load_rules_from_file(File::decodeName(rulesFilePath));
   }
   const ByteArray rulesSrc = pdk::pdk_getenv("PDK_LOGGING_RULES").replace(';', '\n');
   if (!rulesSrc.isEmpty()) {
      TextStream stream(rulesSrc);
      LoggingSettingsParser parser;
      parser.setImplicitRulesSection(true);
      parser.setContent(stream);
      for (const LoggingRule &item : parser.getRules()) {
         er.push_back(item);
      }
   }
   const String configFileName = StringLiteral("pdklogging.ini");
   // get rules from pdk data configuration path
   const String pdkConfigPath
         = Dir(LibraryInfo::getPath(LibraryInfo::LibraryLocation::DataPath)).getAbsoluteFilePath(configFileName);
   qr = load_rules_from_file(pdkConfigPath);
   // get rules from user's/system configuration
   const String envPath = StandardPaths::locate(StandardPaths::StandardLocation::GenericConfigLocation,
                                                String::fromLatin1("PdkProject/") + configFileName);
   if (!envPath.isEmpty()) {
      cr = load_rules_from_file(envPath);
   }
   const std::lock_guard<std::mutex> locker(m_registryMutex);
   m_ruleSets[EnvironmentRules] = std::move(er);
   m_ruleSets[PdkConfigRules] = std::move(qr);
   m_ruleSets[ConfigRules] = std::move(cr);
   if (!m_ruleSets[EnvironmentRules].empty() || !m_ruleSets[PdkConfigRules].empty() || !m_ruleSets[ConfigRules].empty()) {
      updateRules();
   }
}

void LoggingRegistry::registerCategory(LoggingCategory *category, pdk::MsgType enableForLevel)
{
   std::lock_guard<std::mutex> locker(m_registryMutex);
   if (m_categories.find(category) == m_categories.end()) {
      m_categories[category] = enableForLevel;
      (*m_categoryFilter)(category);
   }
}

void LoggingRegistry::unregisterCategory(LoggingCategory *cat)
{
   std::lock_guard<std::mutex> locker(m_registryMutex);
   m_categories.erase(cat);
}

void LoggingRegistry::setApiRules(const String &content)
{
   LoggingSettingsParser parser;
   parser.setImplicitRulesSection(true);
   parser.setContent(content);
   if (logging_debug()) {
      debugMsg("Loading logging rules set by LoggingCategory::setFilterRules ...");
   }
   const std::lock_guard<std::mutex> locker(m_registryMutex);
   m_ruleSets[ApiRules] = parser.getRules();
   updateRules();
}

void LoggingRegistry::updateRules()
{
   for (auto iter = m_categories.begin(), end = m_categories.end(); iter != end; ++iter)
      (*m_categoryFilter)(iter->first);
}

LoggingCategory::CategoryFilter
LoggingRegistry::installFilter(LoggingCategory::CategoryFilter filter)
{
   std::lock_guard<std::mutex> locker(m_registryMutex);
   if (filter == nullptr) {
      filter = defaultCategoryFilter;      
   }
   LoggingCategory::CategoryFilter old = m_categoryFilter;
   m_categoryFilter = filter;
   updateRules();
   return old;
}

LoggingRegistry *LoggingRegistry::getInstance()
{
   return sg_LoggingRegistry();
}

void LoggingRegistry::defaultCategoryFilter(LoggingCategory *category)
{
   const LoggingRegistry *reg = LoggingRegistry::getInstance();
   PDK_ASSERT(reg->m_categories.find(category) != reg->m_categories.end());
   pdk::MsgType enableForLevel = reg->m_categories.at(category);
   // NB: note that the numeric values of the *Msg constants are
   //     not in severity order.
   bool debug = (enableForLevel == pdk::MsgType::DebugMsg);
   bool info = debug || (enableForLevel ==  pdk::MsgType::InfoMsg);
   bool warning = info || (enableForLevel ==  pdk::MsgType::WarningMsg);
   bool critical = warning || (enableForLevel ==  pdk::MsgType::CriticalMsg);

   // hard-wired implementation of
   //   pdk.*.debug=false
   //   pdk.debug=false
   if (const char *categoryName = category->getCategoryName()) {
      // == "pdk" or startsWith("pdk.")
      if (strcmp(categoryName, "pdk") == 0 || strncmp(categoryName, "pdk.", 4) == 0) {
         debug = false;
      }  
   }

   String categoryName = Latin1String(category->getCategoryName());

   for (const auto &ruleSet : reg->m_ruleSets) {
      for (const auto &rule : ruleSet) {
         int filterpass = rule.pass(categoryName, pdk::MsgType::DebugMsg);
         if (filterpass != 0) {
            debug = (filterpass > 0);
         }
         filterpass = rule.pass(categoryName, pdk::MsgType::InfoMsg);
         if (filterpass != 0) {
            info = (filterpass > 0);
         }
         filterpass = rule.pass(categoryName, pdk::MsgType::WarningMsg);
         if (filterpass != 0) {
            warning = (filterpass > 0);
         }
         filterpass = rule.pass(categoryName, pdk::MsgType::CriticalMsg);
         if (filterpass != 0) {
            critical = (filterpass > 0);
         }
      }
   }
   category->setEnabled(pdk::MsgType::DebugMsg, debug);
   category->setEnabled(pdk::MsgType::InfoMsg, info);
   category->setEnabled(pdk::MsgType::WarningMsg, warning);
   category->setEnabled(pdk::MsgType::CriticalMsg, critical);
}

} // internal
} // io
} // pdk
