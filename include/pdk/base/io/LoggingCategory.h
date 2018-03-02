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

#ifndef PDK_M_BASE_IO_LOGGING_CATEGORY_H
#define PDK_M_BASE_IO_LOGGING_CATEGORY_H

#include "pdk/global/Global.h"
#include "pdk/base/io/Debug.h"

namespace pdk {
namespace io {

using pdk::os::thread::BasicAtomicInteger;
using pdk::os::thread::BasicAtomicInt;

class PDK_CORE_EXPORT LoggingCategory
{
   PDK_DISABLE_COPY(LoggingCategory);
public:
   explicit LoggingCategory(const char *category);
   LoggingCategory(const char *category, pdk::MsgType severityLevel);
   ~LoggingCategory();
   bool isEnabled(pdk::MsgType type) const;
   void setEnabled(pdk::MsgType type, bool enable);
#ifdef PDK_ATOMIC_INT8_IS_SUPPORTED
   bool isDebugEnabled() const
   {
      return m_bools.m_enabledDebug.load();
   }
   
   bool isInfoEnabled() const
   {
      return m_bools.m_enabledInfo.load();
   }
   
   bool isWarningEnabled() const
   {
      return m_bools.m_enabledWarning.load();
   }
   
   bool isCriticalEnabled() const 
   {
      return m_bools.m_enabledCritical.load();
   }
#else
   bool isDebugEnabled() const 
   {
      return m_enabled.load() >> DebugShift & 1;
   }
   
   bool isInfoEnabled() const
   {
      return enabled.load() >> InfoShift & 1;
   }
   
   bool isWarningEnabled() const
   {
      return enabled.load() >> WarningShift & 1;
   }
   
   bool isCriticalEnabled() const
   {
      return enabled.load() >> CriticalShift & 1;
   }
#endif
   const char *getCategoryName() const
   {
      return m_name;
   }
   
   // allows usage of both factory method and variable in qCX macros
   LoggingCategory &operator()()
   {
      return *this;
   }
   
   const LoggingCategory &operator()() const
   {
      return *this;
   }
   
   static LoggingCategory *getDefaultCategory();
   using CategoryFilter = void (*)(LoggingCategory*);
   static CategoryFilter installFilter(CategoryFilter);
   static void setFilterRules(const String &rules);
private:
   void init(const char *category, pdk::MsgType severityLevel);
   PDK_DECL_UNUSED_MEMBER void *m_data; // reserved for future use
   const char *m_name;
#ifdef PDK_BIG_ENDIAN
   enum {
      DebugShift = 0,
      WarningShift = 8,
      CriticalShift = 16,
      InfoShift = 24
   };
#else
   enum {
      DebugShift = 24,
      WarningShift = 16,
      CriticalShift = 8,
      InfoShift = 0
   };
#endif
   struct AtomicBools {
#ifdef PDK_ATOMIC_INT8_IS_SUPPORTED
      BasicAtomicInteger<bool> m_enabledDebug;
      BasicAtomicInteger<bool> m_enabledWarning;
      BasicAtomicInteger<bool> m_enabledCritical;
      BasicAtomicInteger<bool> m_enabledInfo;
#endif
   };
   union {
      AtomicBools m_bools;
      BasicAtomicInt m_enabled;
   };
   PDK_DECL_UNUSED_MEMBER bool m_placeholder[4]; // reserved for future use
};

#define PDK_DECLARE_LOGGING_CATEGORY(name) \
   extern const pdk::io::LoggingCategory &name();

#define PDK_LOGGING_CATEGORY(name, ...) \
   const pdk::LoggingCategory &name() \
{ \
   static const pdk::LoggingCategory category(__VA_ARGS__); \
   return category; \
}

#define cdebug_stream(category, ...) \
   for (bool pdkCategoryEnabled = category().isDebugEnabled(); pdkCategoryEnabled; pdkCategoryEnabled = false) \
   pdk::MessageLogger(PDK_MESSAGELOG_FILE, PDK_MESSAGELOG_LINE, PDK_MESSAGELOG_FUNC, category().getCategoryName()).debug(__VA_ARGS__)

#define cinfo_stream(category, ...) \
   for (bool pdkCategoryEnabled = category().isInfoEnabled(); pdkCategoryEnabled; pdkCategoryEnabled = false) \
   pdk::MessageLogger(PDK_MESSAGELOG_FILE, PDK_MESSAGELOG_LINE, PDK_MESSAGELOG_FUNC, category().getCategoryName()).info(__VA_ARGS__)

#define cwarning_stream(category, ...) \
   for (bool pdkCategoryEnabled = category().isWarningEnabled(); pdkCategoryEnabled; pdkCategoryEnabled = false) \
   pdk::MessageLogger(PDK_MESSAGELOG_FILE, PDK_MESSAGELOG_LINE, PDK_MESSAGELOG_FUNC, category().getCategoryName()).warning(__VA_ARGS__)

#define ccritical_stream(category, ...) \
   for (bool pdkCategoryEnabled = category().isCriticalEnabled(); pdkCategoryEnabled; pdkCategoryEnabled = false) \
   pdk::MessageLogger(PDK_MESSAGELOG_FILE, PDK_MESSAGELOG_LINE, PDK_MESSAGELOG_FUNC, category().getCategoryName()).critical(__VA_ARGS__)

#if defined(PDK_NO_DEBUG_OUTPUT)
#  undef cdebug_stream
#  define cdebug_stream(category) PDK_NO_DEBUG_MACRO()
#endif
#if defined(PDK_NO_INFO_OUTPUT)
#  undef cinfo_stream
#  define cinfo_stream(category) PDK_NO_DEBUG_MACRO()
#endif
#if defined(PDK_NO_WARNING_OUTPUT)
#  undef cwarning_stream
#  define cwarning_stream(category) PDK_NO_DEBUG_MACRO()
#endif

} // io
} // pdk

#endif // PDK_M_BASE_IO_LOGGING_CATEGORY_H
