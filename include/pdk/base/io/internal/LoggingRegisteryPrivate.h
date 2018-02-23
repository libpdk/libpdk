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

#ifndef PDK_M_BASE_IO_INTERNAL_LOGGING_REGISTER_PRIVATE_H
#define PDK_M_BASE_IO_INTERNAL_LOGGING_REGISTER_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/io/LoggingCategory.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/io/TextStream.h"

#include <map>
#include <mutex>
#include <vector>

namespace pdk {
namespace io {
namespace internal {

using pdk::lang::StringRef;
using pdk::lang::String;
using pdk::io::TextStream;

class PDK_UNITTEST_EXPORT LoggingRule
{
public:
   LoggingRule();
   LoggingRule(const StringRef &pattern, bool enabled);
   int pass(const String &categoryName, pdk::MsgType type) const;
   
   enum PatternFlag
   {
      FullText = 0x1,
      LeftFilter = 0x2,
      RightFilter = 0x4,
      MidFilter = LeftFilter |  RightFilter
   };
   PDK_DECLARE_FLAGS(PatternFlags, PatternFlag);
   
   String m_category;
   int m_messageType;
   PatternFlags m_flags;
   bool m_enabled;
   
private:
   void parse(const StringRef &pattern);
};

class PDK_UNITTEST_EXPORT LoggingSettingsParser
{
public:
   void setImplicitRulesSection(bool inRulesSection) 
   {
      m_inRulesSection = inRulesSection;
   }
   
   void setContent(const String &content);
   void setContent(TextStream &stream);
   
   std::vector<LoggingRule> getRules() const
   {
      return m_rules;
   }
   
private:
   void parseNextLine(StringRef line);
   
private:
   bool m_inRulesSection = false;
   std::vector<LoggingRule> m_rules;
};

class PDK_UNITTEST_EXPORT LoggingRegistry
{
public:
   LoggingRegistry();
   
   void initializeRules();
   
   void registerCategory(LoggingCategory *category, pdk::MsgType enableForLevel);
   void unregisterCategory(LoggingCategory *category);
   
   void setApiRules(const String &content);
   
   LoggingCategory::CategoryFilter
   installFilter(LoggingCategory::CategoryFilter filter);
   
   static LoggingRegistry *getInstance();
   
private:
   void updateRules();
   
   static void defaultCategoryFilter(LoggingCategory *category);
   
   enum RuleSet {
      // sorted by order in which defaultCategoryFilter considers them:
      PdkConfigRules,
      ConfigRules,
      ApiRules,
      EnvironmentRules,
      NumRuleSets
   };
   
   std::mutex m_registryMutex;
   
   // protected by mutex:
   std::vector<LoggingRule> m_ruleSets[NumRuleSets];
   std::map<LoggingCategory*, pdk::MsgType> m_categories;
   LoggingCategory::CategoryFilter m_categoryFilter;
};

} // internal
} // io
} // pdk

PDK_DECLARE_OPERATORS_FOR_FLAGS(pdk::io::internal::LoggingRule::PatternFlags)
PDK_DECLARE_TYPEINFO(pdk::io::internal::LoggingRule, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_IO_INTERNAL_LOGGING_REGISTER_PRIVATE_H
