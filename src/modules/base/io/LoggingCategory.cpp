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

#include "pdk/base/io/LoggingCategory.h"
#include "pdk/base/io/internal/LoggingRegisteryPrivate.h"
#include "pdk/global/GlobalStatic.h"

namespace pdk {
namespace io {

using pdk::os::thread::BasicAtomicInt;
using pdk::io::internal::LoggingRegistry;

const char pdkDefaultCategoryName[] = "default";

PDK_GLOBAL_STATIC_WITH_ARGS(LoggingCategory, pdkDefaultCategory, (pdkDefaultCategoryName));

#ifndef PDK_ATOMIC_INT8_IS_SUPPORTED
namespace {

void set_bool_lane(BasicAtomicInt *atomic, bool enable, int shift)
{
   const int bit = 1 << shift;
   if (enable) {
      atomic->fetchAndOrRelaxed(bit);
   } else {
      atomic->fetchAndAndRelaxed(~bit);
   }
}

} // anonymous namespace
#endif

LoggingCategory::LoggingCategory(const char *category)
   : m_data(nullptr),
     m_name(nullptr)
{
   init(category, pdk::MsgType::DebugMsg);
}

LoggingCategory::LoggingCategory(const char *category, pdk::MsgType enableForLevel)
   : m_data(nullptr),
     m_name(nullptr)
{
   init(category, enableForLevel);
}

void LoggingCategory::init(const char *category, pdk::MsgType severityLevel)
{
   m_enabled.store(0x01010101);   // m_enabledDebug = m_enabledWarning = m_enabledCritical = true;
   if (category) {
      m_name = category;
   } else {
      m_name = pdkDefaultCategoryName;
   }
   if (LoggingRegistry *reg = LoggingRegistry::getInstance()) {
      reg->registerCategory(this, severityLevel);
   }
}

LoggingCategory::~LoggingCategory()
{
   if (LoggingRegistry *reg = LoggingRegistry::getInstance()) {
      reg->unregisterCategory(this);
   }
}

bool LoggingCategory::isEnabled(pdk::MsgType msgtype) const
{
   switch (msgtype) {
   case pdk::MsgType::DebugMsg:
      return isDebugEnabled();
   case pdk::MsgType::InfoMsg:
      return isInfoEnabled();
   case pdk::MsgType::WarningMsg: 
      return isWarningEnabled();
   case pdk::MsgType::CriticalMsg:
      return isCriticalEnabled();
   case pdk::MsgType::FatalMsg:
      return true;
   }
   return false;
}

void LoggingCategory::setEnabled(pdk::MsgType type, bool enable)
{
   switch (type) {
#ifdef PDK_ATOMIC_INT8_IS_SUPPORTED
   case pdk::MsgType::DebugMsg:
      m_bools.m_enabledDebug.store(enable);
      break;
   case pdk::MsgType::InfoMsg:
      m_bools.m_enabledInfo.store(enable);
      break;
   case pdk::MsgType::WarningMsg:
      m_bools.m_enabledWarning.store(enable);
      break;
   case pdk::MsgType::CriticalMsg:
      m_bools.m_enabledCritical.store(enable);
      break;
#else
   case pdk::MsgType::DebugMsg:
      set_bool_lane(&m_enabled, enable, DebugShift);
      break;
   case pdk::MsgType::InfoMsg:
      set_bool_lane(&m_enabled, enable, InfoShift); 
      break;
   case pdk::MsgType::WarningMsg: 
      set_bool_lane(&m_enabled, enable, WarningShift); 
      break;
   case pdk::MsgType::CriticalMsg: 
      set_bool_lane(&m_enabled, enable, CriticalShift);
      break;
#endif
   case pdk::MsgType::FatalMsg:
      break;
   }
}

LoggingCategory *LoggingCategory::getDefaultCategory()
{
   return pdkDefaultCategory();
}

LoggingCategory::CategoryFilter
LoggingCategory::installFilter(LoggingCategory::CategoryFilter filter)
{
   return LoggingRegistry::getInstance()->installFilter(filter);
}

void LoggingCategory::setFilterRules(const String &rules)
{
   LoggingRegistry::getInstance()->setApiRules(rules);
}

} // io
} // pdk
