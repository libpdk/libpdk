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
// Created by softboy on 2018/02/08.

#ifndef PDK_GLOBAL_LOGGING_H
#define PDK_GLOBAL_LOGGING_H

#include "pdk/global/Global.h"

namespace pdk {

// forward declare class with namespace
namespace io {
class Debug;
class NoDebug;
class LoggingCategory;
} // io

// forward declare class with namespace
namespace lang {
class String;
} // lang

using io::Debug;
using io::NoDebug;
using lang::String;
using io::LoggingCategory;

class MessageLogContext
{
   PDK_DISABLE_COPY(MessageLogContext);
public:
   constexpr MessageLogContext()
      : m_version(2),
        m_line(0),
        m_file(nullptr),
        m_function(nullptr),
        m_category(nullptr) {}
   
   constexpr MessageLogContext(const char *fileName, int lineNumber, const char *functionName, 
                               const char *categoryName)
      : m_version(2), 
        m_line(lineNumber), 
        m_file(fileName), 
        m_function(functionName), 
        m_category(categoryName) 
   {}
   
   void copy(const MessageLogContext &logContext);
   
   int m_version;
   int m_line;
   const char *m_file;
   const char *m_function;
   const char *m_category;
   
private:
   friend class MessageLogger;
   friend class Debug;
};

class PDK_CORE_EXPORT MessageLogger
{
   PDK_DISABLE_COPY(MessageLogger);
public:
   constexpr MessageLogger()
      : m_context()
   {}
   
   constexpr MessageLogger(const char *file, int line, const char *function)
      : m_context(file, line, function, "default")
   {}
   
   constexpr MessageLogger(const char *file, int line, const char *function, const char *category)
      : m_context(file, line, function, category)
   {}
   
   void debug(const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(2, 3);
   void noDebug(const char *, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(2, 3)
   {}
   void info(const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(2, 3);
   void warning(const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(2, 3);
   void critical(const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(2, 3);
   
   typedef const LoggingCategory &(*CategoryFunction)();
   
   void debug(const LoggingCategory &cat, const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(3, 4);
   void debug(CategoryFunction catFunc, const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(3, 4);
   void info(const LoggingCategory &cat, const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(3, 4);
   void info(CategoryFunction catFunc, const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(3, 4);
   void warning(const LoggingCategory &cat, const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(3, 4);
   void warning(CategoryFunction catFunc, const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(3, 4);
   void critical(const LoggingCategory &cat, const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(3, 4);
   void critical(CategoryFunction catFunc, const char *msg, ...) const PDK_ATTRIBUTE_FORMAT_PRINTF(3, 4);
   
#ifndef PDK_CC_MSVC
   PDK_NORETURN
#endif
   void fatal(const char *msg, ...) const noexcept PDK_ATTRIBUTE_FORMAT_PRINTF(2, 3);
   
#ifndef PDK_NO_DEBUG_STREAM
   Debug debug() const;
   Debug debug(const LoggingCategory &cat) const;
   Debug debug(CategoryFunction catFunc) const;
   Debug info() const;
   Debug info(const LoggingCategory &cat) const;
   Debug info(CategoryFunction catFunc) const;
   Debug warning() const;
   Debug warning(const LoggingCategory &cat) const;
   Debug warning(CategoryFunction catFunc) const;
   Debug critical() const;
   Debug critical(const LoggingCategory &cat) const;
   Debug critical(CategoryFunction catFunc) const;
   
   NoDebug noDebug() const noexcept;
#endif // PDK_NO_DEBUG_STREAM
   
private:
   MessageLogContext m_context;
};

#if !defined(PDK_MESSAGELOGCONTEXT) && !defined(PDK_NO_MESSAGELOGCONTEXT)
#  if defined(PDK_NO_DEBUG)
#    define PDK_NO_MESSAGELOGCONTEXT
#  else
#    define PDK_MESSAGELOGCONTEXT
#  endif
#endif

#ifdef PDK_MESSAGELOGCONTEXT
#define PDK_MESSAGELOG_FILE __FILE__
#define PDK_MESSAGELOG_LINE __LINE__
#define PDK_MESSAGELOG_FUNC PDK_FUNC_INFO
#else
#define PDK_MESSAGELOG_FILE nullptr
#define PDK_MESSAGELOG_LINE 0
#define PDK_MESSAGELOG_FUNC nullptr
#endif

#define debug_stream MessageLogger(PDK_MESSAGELOG_FILE, PDK_MESSAGELOG_LINE, PDK_MESSAGELOG_FUNC).debug
#define info_stream MessageLogger(PDK_MESSAGELOG_FILE, PDK_MESSAGELOG_LINE, PDK_MESSAGELOG_FUNC).info
#define warning_stream MessageLogger(PDK_MESSAGELOG_FILE, PDK_MESSAGELOG_LINE, PDK_MESSAGELOG_FUNC).warning
#define critical_stream MessageLogger(PDK_MESSAGELOG_FILE, PDK_MESSAGELOG_LINE, PDK_MESSAGELOG_FUNC).critical
#define fatal_stream MessageLogger(PDK_MESSAGELOG_FILE, PDK_MESSAGELOG_LINE, PDK_MESSAGELOG_FUNC).fatal

#define PDK_NO_DEBUG_MACRO while (false) MessageLogger().noDebug

#if defined(PDK_NO_DEBUG_OUTPUT)
#  undef debug_stream
#  define debug_stream PDK_NO_DEBUG_MACRO
#endif
#if defined(PDK_NO_INFO_OUTPUT)
#  undef info_stream
#  define info_stream PDK_NO_DEBUG_MACRO
#endif
#if defined(PDK_NO_WARNING_OUTPUT)
#  undef warning_stream
#  define warning_stream PDK_NO_DEBUG_MACRO
#endif

PDK_CORE_EXPORT void message_output(pdk::MsgType, const MessageLogContext &context,
                                    const String &message);

PDK_CORE_EXPORT void errno_warning(int code, const char *msg, ...);
PDK_CORE_EXPORT void errno_warning(const char *msg, ...);

typedef void (*PdkMessageHandler)(pdk::MsgType, const MessageLogContext&, const String &);
PDK_CORE_EXPORT PdkMessageHandler install_message_handler(PdkMessageHandler);

PDK_CORE_EXPORT void set_message_pattern(const String &messagePattern);
PDK_CORE_EXPORT String format_log_message(pdk::MsgType type, const MessageLogContext &context,
                                          const String &buf);

} // pdk

#endif // PDK_GLOBAL_LOGGING_H
