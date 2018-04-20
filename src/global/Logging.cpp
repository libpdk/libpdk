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

#include "pdk/global/Global.h"
#include "pdk/global/Logging.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/LoggingCategory.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/io/internal/LoggingRegisteryPrivate.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/ElapsedTimer.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/pal/kernel/Simd.h"
#include "pdk/global/GlobalStatic.h"
#ifdef PDK_OS_WIN
#include "pdk/global/Windows.h"
#endif
#if PDK_CONFIG(slog2)
#include <sys/slog2.h>
#endif
#if PDK_HAS_INCLUDE(<paths.h>)
#include <paths.h>
#endif
#if PDK_CONFIG(journald)
# define SD_JOURNAL_SUPPRESS_LOCATION
# include <systemd/sd-journal.h>
# include <syslog.h>
#endif
#if PDK_CONFIG(syslog)
# include <syslog.h>
#endif
#ifdef PDK_OS_UNIX
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include "pdk/kernel/internal/CoreUnixPrivate.h"
#endif

#if !defined PDK_NO_REGULAREXPRESSION
#  ifdef __UCLIBC__
#    if __UCLIBC_HAS_BACKTRACE__
#      define PDK_LOGGING_HAVE_BACKTRACE
#    endif
#  elif (defined(__GLIBC__) && defined(__GLIBCXX__)) || (PDK_HAS_INCLUDE(<cxxabi.h>) && PDK_HAS_INCLUDE(<execinfo.h>))
#    define PDK_LOGGING_HAVE_BACKTRACE
#  endif
#endif

#if PDK_CONFIG(slog2)
extern char *__progname;
#endif

#if defined(PDK_OS_LINUX) && (defined(__GLIBC__) || PDK_HAS_INCLUDE(<sys/syscall.h>))
#  include <sys/syscall.h>

static long pdk_gettid()
{
   // no error handling
   // this syscall has existed since Linux 2.4.11 and cannot fail
   return syscall();
}
#elif defined(PDK_OS_DARWIN)
#  include <pthread.h>
static int pdk_gettid()
{
   // no error handling: this call cannot fail
   __uint64_t tid;
   pthread_threadid_np(NULL, &tid);
   return tid;
}
#elif defined(PDK_OS_FREEBSD_KERNEL) && defined(__FreeBSD_version) && __FreeBSD_version >= 900031
#  include <pthread_np.h>
static int pdk_gettid()
{
   return pthread_getthreadid_np();
}
#else
static pdk::pint64 pdk_gettid()
{
   return pdk::pintptr(pdk::os::thread::Thread::getCurrentThreadId());
}
#endif

#ifdef LOGGING_HAVE_BACKTRACE
//#  include <qregularexpression.h>
#  include <cxxabi.h>
#  include <execinfo.h>
#endif

#include <cstdlib>
#include <stdio.h>

namespace pdk {

using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Character;
using pdk::lang::Latin1Character;
using pdk::ds::ByteArray;
using pdk::os::thread::AtomicInt;
using pdk::os::thread::BasicAtomicPointer;
using pdk::ds::VarLengthArray;
using pdk::io::LoggingCategory;
using pdk::io::Debug;
using pdk::io::NoDebug;
using pdk::kernel::ElapsedTimer;
using pdk::kernel::CoreApplication;
using pdk::os::thread::Thread;
using pdk::time::DateTime;

namespace {
#if !defined(PDK_CC_MSVC)
PDK_NORETURN
#endif
void pdk_message_fatal(pdk::MsgType, const MessageLogContext &context, const String &message);
void pdk_message_print(pdk::MsgType, const MessageLogContext &context, const String &message);

int checked_var_value(const char *varname)
{
   // qEnvironmentVariableIntValue returns 0 on both parsing failure and on
   // empty, but we need to distinguish between the two for backwards
   // compatibility reasons.
   ByteArray str = pdk::pdk_getenv(varname);
   if (str.isEmpty()) {
      return 0;
   }
   bool ok;
   int value = str.toInt(&ok, 0);
   return ok ? value : 1;
}

static bool is_fatal(pdk::MsgType msgType)
{
   if (msgType == pdk::MsgType::FatalMsg) {
      return true;
   }
   if (msgType == pdk::MsgType::CriticalMsg) {
      static AtomicInt fatalCriticals = checked_var_value("PDK_FATAL_CRITICALS");
      
      // it's fatal if the current value is exactly 1,
      // otherwise decrement if it's non-zero
      return fatalCriticals.load() && fatalCriticals.fetchAndAddRelaxed(-1) == 1;
   }
   if (msgType == pdk::MsgType::WarningMsg || msgType == pdk::MsgType::CriticalMsg) {
      static AtomicInt fatalWarnings = checked_var_value("PDK_FATAL_WARNINGS");
      // it's fatal if the current value is exactly 1,
      // otherwise decrement if it's non-zero
      return fatalWarnings.load() && fatalWarnings.fetchAndAddRelaxed(-1) == 1;
   }
   
   return false;
}

bool will_log_to_console()
{
   // rules to determine if we'll log preferably to the console:
   //  1) if PDK_LOGGING_TO_CONSOLE is set, it determines behavior:
   //    - if it's set to 0, we will not log to console
   //    - if it's set to 1, we will log to console
   //  2) otherwise, we will log to console if we have a console window (Windows)
   //     or a controlling TTY (Unix). This is done even if stderr was redirected
   //     to the blackhole device (NUL or /dev/null).
   
   bool ok = true;
   uint envcontrol = pdk::pdk_getenv("PDK_LOGGING_TO_CONSOLE").toUInt(&ok);
   if (ok) {
      return envcontrol;
   }
#  ifdef PDK_OS_WIN
   return GetConsoleWindow();
#  elif defined(PDK_OS_UNIX)
#    ifndef _PATH_TTY
#    define _PATH_TTY "/dev/tty"
#    endif
   // if /dev/tty exists, we can only open it if we have a controlling TTY
   int devtty = pdk::kernel::safe_open(_PATH_TTY, O_RDONLY);
   if (devtty == -1 && (errno == ENOENT || errno == EPERM || errno == ENXIO)) {
      // no /dev/tty, fall back to isatty on stderr
      return isatty(STDERR_FILENO);
   } else if (devtty != -1) {
      // there is a /dev/tty and we could open it: we have a controlling TTY
      pdk::kernel::safe_close(devtty);
      return true;
   }
   
   // no controlling TTY
   return false;
#  else
#    error "Not Unix and not Windows?"
#  endif
}

} // anonymous namespace

PDK_CORE_EXPORT bool pdk_logging_to_console()
{
   static const bool logToConsole = will_log_to_console();
   return logToConsole;
}

namespace {

#if defined(PDK_CC_MSVC) && defined(PDK_DEBUG) && defined(_DEBUG) && defined(_CRT_ERROR)
inline void convert_to_wchar_t_elided(wchar_t *d, size_t space, const char *s) PDK_DECL_NOEXCEPT
{
   size_t len = qstrlen(s);
   if (len + 1 > space) {
      const size_t skip = len - space + 4; // 4 for "..." + '\0'
      s += skip;
      len -= skip;
      for (int i = 0; i < 3; ++i){
         *d++ = L'.';
      }
   }
   while (len--) {
      *d++ = *s++;
   }
   *d++ = 0;
}
#endif

PDK_NEVER_INLINE
String pdk_message(pdk::MsgType msgType, const MessageLogContext &context, const char *msg, va_list ap)
{
   String buf = String::vasprintf(msg, ap);
   pdk_message_print(msgType, context, buf);
   return buf;
}

} // anonymous namespace

void MessageLogger::debug(const char *msg, ...) const
{
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::DebugMsg, m_context, msg, ap);
   va_end(ap);
   if (is_fatal(pdk::MsgType::DebugMsg)) {
      pdk_message_fatal(pdk::MsgType::DebugMsg, m_context, message);
   }
}

void MessageLogger::info(const char *msg, ...) const
{
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::InfoMsg, m_context, msg, ap);
   va_end(ap);
   if (is_fatal(pdk::MsgType::InfoMsg)) {
      pdk_message_fatal(pdk::MsgType::InfoMsg, m_context, message);
   }
}

void MessageLogger::debug(const LoggingCategory &category, const char *msg, ...) const
{
   if (!category.isDebugEnabled()) {
      return;
   }
   MessageLogContext ctxt;
   ctxt.copy(m_context);
   ctxt.m_category = category.getCategoryName();
   
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::DebugMsg, ctxt, msg, ap);
   va_end(ap);
   if (is_fatal(pdk::MsgType::DebugMsg)) {
      pdk_message_fatal(pdk::MsgType::DebugMsg, ctxt, message);
   }
}

void MessageLogger::debug(MessageLogger::CategoryFunction catFunc,
                          const char *msg, ...) const
{
   const LoggingCategory &cat = (*catFunc)();
   if (!cat.isDebugEnabled()) {
      return;
   }
   MessageLogContext ctxt;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::DebugMsg, ctxt, msg, ap);
   va_end(ap);
   if (is_fatal(pdk::MsgType::DebugMsg)) {
      pdk_message_fatal(pdk::MsgType::DebugMsg, ctxt, message);
   }   
}

#ifndef PDK_NO_DEBUG_STREAM

Debug MessageLogger::debug() const
{
   Debug dbg = Debug(pdk::MsgType::DebugMsg);
   MessageLogContext &ctxt = dbg.m_stream->m_context;
   ctxt.copy(m_context);
   return dbg;
}

Debug MessageLogger::debug(const LoggingCategory &cat) const
{
   Debug dbg = Debug(pdk::MsgType::DebugMsg);
   if (!cat.isDebugEnabled()) {
      dbg.m_stream->m_messageOutput = false;
   }
   MessageLogContext &ctxt = dbg.m_stream->m_context;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   return dbg;
}

Debug MessageLogger::debug(MessageLogger::CategoryFunction catFunc) const
{
   return debug((*catFunc)());
}

NoDebug MessageLogger::noDebug() const noexcept
{
   return NoDebug();
}

#endif // PDK_NO_DEBUG_STREAM

void MessageLogger::info(const LoggingCategory &cat, const char *msg, ...) const
{
   if (!cat.isInfoEnabled())
      return;
   
   MessageLogContext ctxt;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::InfoMsg, ctxt, msg, ap);
   va_end(ap);
   
   if (is_fatal(pdk::MsgType::InfoMsg)) {
      pdk_message_fatal(pdk::MsgType::InfoMsg, ctxt, message);
   }
}

void MessageLogger::info(MessageLogger::CategoryFunction catFunc,
                         const char *msg, ...) const
{
   const LoggingCategory &cat = (*catFunc)();
   if (!cat.isInfoEnabled()) {
      return;
   }
   MessageLogContext ctxt;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::InfoMsg, ctxt, msg, ap);
   va_end(ap);
   
   if (is_fatal(pdk::MsgType::InfoMsg)) {
      pdk_message_fatal(pdk::MsgType::InfoMsg, ctxt, message);
   }
}

#ifndef PDK_NO_DEBUG_STREAM

Debug MessageLogger::info() const
{
   Debug dbg = Debug(pdk::MsgType::InfoMsg);
   MessageLogContext &ctxt = dbg.m_stream->m_context;
   ctxt.copy(m_context);
   return dbg;
}

Debug MessageLogger::info(const LoggingCategory &cat) const
{
   Debug dbg = Debug(pdk::MsgType::InfoMsg);
   if (!cat.isInfoEnabled()) {
      dbg.m_stream->m_messageOutput = false;
   }
   MessageLogContext &ctxt = dbg.m_stream->m_context;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   
   return dbg;
}

Debug MessageLogger::info(MessageLogger::CategoryFunction catFunc) const
{
   return info((*catFunc)());
}

#endif

void MessageLogger::warning(const char *msg, ...) const
{
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::WarningMsg, m_context, msg, ap);
   va_end(ap);
   if (is_fatal(pdk::MsgType::WarningMsg)) {
      pdk_message_fatal(pdk::MsgType::WarningMsg, m_context, message);
   }
}

void MessageLogger::warning(const LoggingCategory &cat, const char *msg, ...) const
{
   if (!cat.isWarningEnabled()) {
      return;
   }
   
   MessageLogContext ctxt;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::WarningMsg, ctxt, msg, ap);
   va_end(ap);
   
   if (is_fatal(pdk::MsgType::WarningMsg)) {
      pdk_message_fatal(pdk::MsgType::WarningMsg, ctxt, message);
   }
}

void MessageLogger::warning(MessageLogger::CategoryFunction catFunc,
                            const char *msg, ...) const
{
   const LoggingCategory &cat = (*catFunc)();
   if (!cat.isWarningEnabled())
      return;
   
   MessageLogContext ctxt;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::WarningMsg, ctxt, msg, ap);
   va_end(ap);
   
   if (is_fatal(pdk::MsgType::WarningMsg)) {
      pdk_message_fatal(pdk::MsgType::WarningMsg, ctxt, message);
   }
}

#ifndef PDK_NO_DEBUG_STREAM

Debug MessageLogger::warning() const
{
   Debug dbg = Debug(pdk::MsgType::WarningMsg);
   MessageLogContext &ctxt = dbg.m_stream->m_context;
   ctxt.copy(m_context);
   return dbg;
}

Debug MessageLogger::warning(const LoggingCategory &cat) const
{
   Debug dbg = Debug(pdk::MsgType::WarningMsg);
   if (!cat.isWarningEnabled()) {
      dbg.m_stream->m_messageOutput = false;
   }
   MessageLogContext &ctxt = dbg.m_stream->m_context;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   return dbg;
}

Debug MessageLogger::warning(MessageLogger::CategoryFunction catFunc) const
{
   return warning((*catFunc)());
}

#endif

void MessageLogger::critical(const char *msg, ...) const
{
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::CriticalMsg, m_context, msg, ap);
   va_end(ap);
   if (is_fatal(pdk::MsgType::CriticalMsg)) {
      pdk_message_fatal(pdk::MsgType::CriticalMsg, m_context, message);
   }
}

void MessageLogger::critical(const LoggingCategory &cat, const char *msg, ...) const
{
   if (!cat.isCriticalEnabled()) {
      return;
   }
   MessageLogContext ctxt;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::CriticalMsg, ctxt, msg, ap);
   va_end(ap);
   
   if (is_fatal(pdk::MsgType::CriticalMsg)) {
      pdk_message_fatal(pdk::MsgType::CriticalMsg, ctxt, message);
   }
}


void MessageLogger::critical(MessageLogger::CategoryFunction catFunc,
                             const char *msg, ...) const
{
   const LoggingCategory &cat = (*catFunc)();
   if (!cat.isCriticalEnabled()) {
      return;
   }
   MessageLogContext ctxt;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   va_list ap;
   va_start(ap, msg); // use variable arg list
   const String message = pdk_message(pdk::MsgType::CriticalMsg, ctxt, msg, ap);
   va_end(ap);
   
   if (is_fatal(pdk::MsgType::CriticalMsg)) {
      pdk_message_fatal(pdk::MsgType::CriticalMsg, ctxt, message);
   }
}

#ifndef PDK_NO_DEBUG_STREAM

Debug MessageLogger::critical() const
{
   Debug dbg = Debug(pdk::MsgType::CriticalMsg);
   MessageLogContext &ctxt = dbg.m_stream->m_context;
   ctxt.copy(m_context);
   return dbg;
}

Debug MessageLogger::critical(const LoggingCategory &cat) const
{
   Debug dbg = Debug(pdk::MsgType::CriticalMsg);
   if (!cat.isCriticalEnabled()) {
      dbg.m_stream->m_messageOutput = false;
   }
   MessageLogContext &ctxt = dbg.m_stream->m_context;
   ctxt.copy(m_context);
   ctxt.m_category = cat.getCategoryName();
   return dbg;
}

Debug MessageLogger::critical(MessageLogger::CategoryFunction catFunc) const
{
   return critical((*catFunc)());
}

#endif

void MessageLogger::fatal(const char *msg, ...) const noexcept
{
   String message;
   va_list ap;
   va_start(ap, msg); // use variable arg list
   PDK_TERMINATE_ON_EXCEPTION(message = pdk_message(pdk::MsgType::FatalMsg, m_context, msg, ap));
   va_end(ap);
   pdk_message_fatal(pdk::MsgType::FatalMsg, m_context, message);
}

PDK_UNITTEST_EXPORT ByteArray pdk_cleanup_func_info(ByteArray info)
{
   // Strip the function info down to the base function name
   // note that this throws away the template definitions,
   // the parameter types (overloads) and any const/volatile qualifiers.
   if (info.isEmpty()) {
      return info;
   }
   int pos;
   // Skip trailing [with XXX] for templates (gcc), but make
   // sure to not affect Objective-C message names.
   pos = info.size() - 1;
   if (info.endsWith(']') && !(info.startsWith('+') || info.startsWith('-'))) {
      while (--pos) {
         if (info.at(pos) == '[') {
            info.truncate(pos);
         }
      }
   }
   // operator names with '(', ')', '<', '>' in it
   static const char operator_call[] = "operator()";
   static const char operator_lessThan[] = "operator<";
   static const char operator_greaterThan[] = "operator>";
   static const char operator_lessThanEqual[] = "operator<=";
   static const char operator_greaterThanEqual[] = "operator>=";
   
   // canonize operator names
   info.replace("operator ", "operator");
   
   // remove argument list
   while(true) {
      int parencount = 0;
      pos = info.lastIndexOf(')');
      if (pos == -1) {
         // Don't know how to parse this function name
         return info;
      }
      // find the beginning of the argument list
      --pos;
      ++parencount;
      while (pos && parencount) {
         if (info.at(pos) == ')') {
            ++parencount;
         } else if (info.at(pos) == '(') {
            --parencount;
         }
         --pos;
      }
      if (parencount != 0) {
         return info;
      }
      info.truncate(++pos);
      if (info.at(pos - 1) == ')') {
         if (info.indexOf(operator_call) == pos - (int)strlen(operator_call)) {
            break;
         }
         // this function returns a pointer to a function
         // and we matched the arguments of the return type's parameter list
         // try again
         info.remove(0, info.indexOf('('));
         info.chop(1);
         continue;
      } else {
         break;
      }
   }
   
   // find the beginning of the function name
   int parencount = 0;
   int templatecount = 0;
   --pos;
   
   // make sure special characters in operator names are kept
   if (pos > -1) {
      switch (info.at(pos)) {
      case ')':
         if (info.indexOf(operator_call) == pos - (int)strlen(operator_call) + 1) {
            pos -= 2;
         }
         break;
      case '<':
         if (info.indexOf(operator_lessThan) == pos - (int)strlen(operator_lessThan) + 1) {
            --pos;
         }
         break;
      case '>':
         if (info.indexOf(operator_greaterThan) == pos - (int)strlen(operator_greaterThan) + 1) {
            --pos;
         } 
         break;
      case '=': {
         int operatorLength = (int)strlen(operator_lessThanEqual);
         if (info.indexOf(operator_lessThanEqual) == pos - operatorLength + 1) {
            pos -= 2;
         } else if (info.indexOf(operator_greaterThanEqual) == pos - operatorLength + 1) {
            pos -= 2;
         }
         break;
      }
      default:
         break;
      }
   }
   
   while (pos > -1) {
      if (parencount < 0 || templatecount < 0) {
         return info;
      }
      char c = info.at(pos);
      if (c == ')') {
         ++parencount;
      } else if (c == '(') {
         --parencount;
      } else if (c == '>') {
         ++templatecount;
      } else if (c == '<') {
         --templatecount;
      } else if (c == ' ' && templatecount == 0 && parencount == 0) {
         break;
      }
      --pos;
   }
   info = info.mid(pos + 1);
   // remove trailing '*', '&' that are part of the return argument
   while ((info.at(0) == '*')
          || (info.at(0) == '&')) {
      info = info.mid(1);
   }
   // we have the full function name now.
   // clean up the templates
   while ((pos = info.lastIndexOf('>')) != -1) {
      if (!info.contains('<')) {
         break;
      }
      // find the matching close
      int end = pos;
      templatecount = 1;
      --pos;
      while (pos && templatecount) {
         char c = info.at(pos);
         if (c == '>') {
            ++templatecount;
         } else if (c == '<') {
            --templatecount;
         }
         --pos;
      }
      ++pos;
      info.remove(pos, end - pos + 1);
   }
   return info;
}

// tokens as recognized in PDK_MESSAGE_PATTERN
static const char sg_categoryTokenC[] = "%{category}";
static const char sg_typeTokenC[] = "%{type}";
static const char sg_messageTokenC[] = "%{message}";
static const char sg_fileTokenC[] = "%{file}";
static const char sg_lineTokenC[] = "%{line}";
static const char sg_functionTokenC[] = "%{function}";
static const char sg_pidTokenC[] = "%{pid}";
static const char sg_appnameTokenC[] = "%{appname}";
static const char sg_threadidTokenC[] = "%{threadid}";
static const char sg_qthreadptrTokenC[] = "%{qthreadptr}";
static const char sg_timeTokenC[] = "%{time"; //not a typo: this command has arguments
static const char sg_backtraceTokenC[] = "%{backtrace"; //ditto
static const char sg_ifCategoryTokenC[] = "%{if-category}";
static const char sg_ifDebugTokenC[] = "%{if-debug}";
static const char sg_ifInfoTokenC[] = "%{if-info}";
static const char sg_ifWarningTokenC[] = "%{if-warning}";
static const char sg_ifCriticalTokenC[] = "%{if-critical}";
static const char sg_ifFatalTokenC[] = "%{if-fatal}";
static const char sg_endifTokenC[] = "%{endif}";
static const char sg_emptyTokenC[] = "";
static const char sg_defaultPattern[] = "%{if-category}%{category}: %{endif}%{message}";

struct MessagePattern
{
   MessagePattern();
   ~MessagePattern();
   
   void setPattern(const String &pattern);
   
   // 0 terminated arrays of literal tokens / literal or placeholder tokens
   const char **m_literals;
   const char **m_tokens;
   std::list<String> m_timeArgs;   // timeFormats in sequence of %{time
   ElapsedTimer m_timer;
#ifdef LOGGING_HAVE_BACKTRACE
   struct BacktraceParams {
      String m_backtraceSeparator;
      int m_backtraceDepth;
   };
   std::vector<BacktraceParams> m_backtraceArgs; // backtrace argumens in sequence of %{backtrace
#endif
   bool m_fromEnvironment;
   static std::mutex sm_mutex;
};

std::mutex MessagePattern::sm_mutex;

MessagePattern::MessagePattern()
   : m_literals(0),
     m_tokens(0),
     m_fromEnvironment(false)
{
   m_timer.start();
   const String envPattern = String::fromLocal8Bit(pdk::pdk_getenv("PDK_MESSAGE_PATTERN"));
   if (envPattern.isEmpty()) {
      setPattern(Latin1String(sg_defaultPattern));
   } else {
      setPattern(envPattern);
      m_fromEnvironment = true;
   }
}

MessagePattern::~MessagePattern()
{
   for (int i = 0; m_literals[i]; ++i) {
      delete [] m_literals[i];
   }
   delete [] m_literals;
   m_literals = nullptr;
   delete [] m_tokens;
   m_tokens = nullptr;
}

void MessagePattern::setPattern(const String &pattern)
{
   if (m_literals) {
      for (int i = 0; m_literals[i]; ++i) {
         delete [] m_literals[i];
      }
      delete [] m_literals;
   }
   delete [] m_tokens;
   m_timeArgs.clear();
#ifdef LOGGING_HAVE_BACKTRACE
   m_backtraceArgs.clear();
#endif
   
   // scanner
   std::list<String> lexemes;
   String lexeme;
   bool inPlaceholder = false;
   for (int i = 0; i < pattern.size(); ++i) {
      const Character c = pattern.at(i);
      if ((c == Latin1Character('%'))
          && !inPlaceholder) {
         if ((i + 1 < pattern.size())
             && pattern.at(i + 1) == Latin1Character('{')) {
            // beginning of placeholder
            if (!lexeme.isEmpty()) {
               lexemes.push_back(lexeme);
               lexeme.clear();
            }
            inPlaceholder = true;
         }
      }
      lexeme.append(c);
      if ((c == Latin1Character('}') && inPlaceholder)) {
         // end of placeholder
         lexemes.push_back(lexeme);
         lexeme.clear();
         inPlaceholder = false;
      }
   }
   if (!lexeme.isEmpty()) {
      lexemes.push_back(lexeme);
   } 
   
   // tokenizer
   VarLengthArray<const char*> literalsVar;
   m_tokens = new const char*[lexemes.size() + 1];
   m_tokens[lexemes.size()] = 0;
   
   bool nestedIfError = false;
   bool inIf = false;
   String error;
   
   for (int i = 0; static_cast<size_t>(i) < lexemes.size(); ++i) {
      auto iter = lexemes.begin();
      std::advance(iter, i);
      const String lexeme = *iter;
      if (lexeme.startsWith(Latin1String("%{"))
          && lexeme.endsWith(Latin1Character('}'))) {
         // placeholder
         if (lexeme == Latin1String(sg_typeTokenC)) {
            m_tokens[i] = sg_typeTokenC;
         } else if (lexeme == Latin1String(sg_categoryTokenC)) {
            m_tokens[i] = sg_categoryTokenC;
         } else if (lexeme == Latin1String(sg_messageTokenC)) {
            m_tokens[i] = sg_messageTokenC;
         } else if (lexeme == Latin1String(sg_fileTokenC)) {
            m_tokens[i] = sg_fileTokenC;
         } else if (lexeme == Latin1String(sg_lineTokenC)) {
            m_tokens[i] = sg_lineTokenC;
         } else if (lexeme == Latin1String(sg_functionTokenC)) {
            m_tokens[i] = sg_functionTokenC;
         } else if (lexeme == Latin1String(sg_pidTokenC)) {
            m_tokens[i] = sg_pidTokenC;
         } else if (lexeme == Latin1String(sg_appnameTokenC)) {
            m_tokens[i] = sg_appnameTokenC;
         } else if (lexeme == Latin1String(sg_threadidTokenC)) {
            m_tokens[i] = sg_threadidTokenC;
         } else if (lexeme == Latin1String(sg_qthreadptrTokenC)) {
            m_tokens[i] = sg_qthreadptrTokenC;
         } else if (lexeme.startsWith(Latin1String(sg_timeTokenC))) {
            m_tokens[i] = sg_timeTokenC;
            int spaceIdx = lexeme.indexOf(Character::fromLatin1(' '));
            if (spaceIdx > 0) {
               m_timeArgs.push_back(lexeme.substring(spaceIdx + 1, lexeme.length() - spaceIdx - 2));
            } else {
               m_timeArgs.push_back(String());
            }
            
         } else if (lexeme.startsWith(Latin1String(sg_backtraceTokenC))) {
#ifdef LOGGING_HAVE_BACKTRACE
            //            tokens[i] = backtraceTokenC;
            //            String backtraceSeparator = StringLiteral("|");
            //            int backtraceDepth = 5;
            //            QRegularExpression depthRx(StringLiteral(" depth=(?|\"([^\"]*)\"|([^ }]*))"));
            //            QRegularExpression separatorRx(StringLiteral(" separator=(?|\"([^\"]*)\"|([^ }]*))"));
            //            QRegularExpressionMatch m = depthRx.match(lexeme);
            //            if (m.hasMatch()) {
            //               int depth = m.capturedRef(1).toInt();
            //               if (depth <= 0)
            //                  error += Latin1String("PDK_MESSAGE_PATTERN: %{backtrace} depth must be a number greater than 0\n");
            //               else
            //                  backtraceDepth = depth;
            //            }
            //            m = separatorRx.match(lexeme);
            //            if (m.hasMatch())
            //               backtraceSeparator = m.captured(1);
            //            BacktraceParams backtraceParams;
            //            backtraceParams.backtraceDepth = backtraceDepth;
            //            backtraceParams.backtraceSeparator = backtraceSeparator;
            //            backtraceArgs.append(backtraceParams);
#else
            error += Latin1String("PDK_MESSAGE_PATTERN: %{backtrace} is not supported by this pdk build\n");
            m_tokens[i] = "";
#endif
         }
         
#define IF_TOKEN(LEVEL) \
   else if (lexeme == Latin1String(LEVEL)) { \
   if (inIf) \
   nestedIfError = true; \
   m_tokens[i] = LEVEL; \
   inIf = true; \
      }
         IF_TOKEN(sg_ifCategoryTokenC)
               IF_TOKEN(sg_ifDebugTokenC)
               IF_TOKEN(sg_ifInfoTokenC)
               IF_TOKEN(sg_ifWarningTokenC)
               IF_TOKEN(sg_ifCriticalTokenC)
               IF_TOKEN(sg_ifFatalTokenC)
      #undef IF_TOKEN
               else if (lexeme == Latin1String(sg_endifTokenC)) {
            m_tokens[i] = sg_endifTokenC;
            if (!inIf && !nestedIfError) {
               error += Latin1String("PDK_MESSAGE_PATTERN: %{endif} without an %{if-*}\n");
            }
            inIf = false;
         } else {
            m_tokens[i] = sg_emptyTokenC;
            error += StringLiteral("PDK_MESSAGE_PATTERN: Unknown placeholder %1\n")
                  .arg(lexeme);
         }
      } else {
         char *literal = new char[lexeme.size() + 1];
         std::strncpy(literal, lexeme.toLatin1().getConstRawData(), lexeme.size());
         literal[lexeme.size()] = '\0';
         literalsVar.append(literal);
         m_tokens[i] = literal;
      }
   }
   if (nestedIfError) {
      error += Latin1String("PDK_MESSAGE_PATTERN: %{if-*} cannot be nested\n");
   } else if (inIf) {
      error += Latin1String("PDK_MESSAGE_PATTERN: missing %{endif}\n");
   }
   if (!error.isEmpty()) {
#if defined(PDK_OS_WIN)
      if (!pdk_logging_to_console()) {
         OutputDebugString(reinterpret_cast<const wchar_t*>(error.utf16()));
      } else
#endif
      {
         fprintf(stderr, "%s", error.toLocal8Bit().getConstRawData());
         fflush(stderr);
      }
   }
   m_literals = new const char*[literalsVar.size() + 1];
   m_literals[literalsVar.size()] = 0;
   std::memcpy(m_literals, literalsVar.getConstRawData(), literalsVar.size() * sizeof(const char*));
}

#if PDK_CONFIG(slog2)
#ifndef PDK_LOG_CODE
#define PDK_LOG_CODE 9000
#endif

static void slog2_default_handler(pdk::MsgType msgType, const char *message)
{
   if (slog2_set_default_buffer((slog2_buffer_t)-1) == 0) {
      slog2_buffer_set_config_t buffer_config;
      slog2_buffer_t buffer_handle;
      
      buffer_config.buffer_set_name = __progname;
      buffer_config.num_buffers = 1;
      buffer_config.verbosity_level = SLOG2_DEBUG1;
      buffer_config.buffer_config[0].buffer_name = "default";
      buffer_config.buffer_config[0].num_pages = 8;
      
      if (slog2_register(&buffer_config, &buffer_handle, 0) == -1) {
         fprintf(stderr, "Error registering slogger2 buffer!\n");
         fprintf(stderr, "%s", message);
         fflush(stderr);
         return;
      }
      
      // Set as the default buffer
      slog2_set_default_buffer(buffer_handle);
   }
   int severity;
   //Determines the severity level
   switch (msgType) {
   case pdk::MsgType::DebugMsg:
      severity = SLOG2_DEBUG1;
      break;
   case pdk::MsgType::InfoMsg:
      severity = SLOG2_INFO;
      break;
   case pdk::MsgType::WarningMsg:
      severity = SLOG2_NOTICE;
      break;
   case pdk::MsgType::CriticalMsg:
      severity = SLOG2_WARNING;
      break;
   case pdk::MsgType::FatalMsg:
      severity = SLOG2_ERROR;
      break;
   }
   //writes to the slog2 buffer
   slog2c(NULL, PDK_LOG_CODE, severity, message);
}
#endif // slog2

PDK_GLOBAL_STATIC(MessagePattern, sg_messagePattern);

String format_log_message(pdk::MsgType type, const MessageLogContext &context, const String &str)
{
   String message;
   std::lock_guard<std::mutex> lock(MessagePattern::sm_mutex);
   MessagePattern *pattern = sg_messagePattern();
   if (!pattern) {
      // after destruction of static MessagePattern instance
      message.append(str);
      return message;
   }
   bool skip = false;
   int timeArgsIdx = 0;
#ifdef LOGGING_HAVE_BACKTRACE
   int backtraceArgsIdx = 0;
#endif
   
   // we do not convert file, function, line literals to local encoding due to overhead
   for (int i = 0; pattern->m_tokens[i] != 0; ++i) {
      const char *token = pattern->m_tokens[i];
      if (token == sg_endifTokenC) {
         skip = false;
      } else if (skip) {
         // we skip adding messages, but we have to iterate over
         // timeArgsIdx and backtraceArgsIdx anyway
         if (token == sg_timeTokenC)
            timeArgsIdx++;
#ifdef LOGGING_HAVE_BACKTRACE
         else if (token == sg_backtraceTokenC)
            backtraceArgsIdx++;
#endif
      } else if (token == sg_messageTokenC) {
         message.append(str);
      } else if (token == sg_categoryTokenC) {
         message.append(Latin1String(context.m_category));
      } else if (token == sg_typeTokenC) {
         switch (type) {
         case pdk::MsgType::DebugMsg:   message.append(Latin1String("debug")); break;
         case pdk::MsgType::InfoMsg:    message.append(Latin1String("info")); break;
         case pdk::MsgType::WarningMsg: message.append(Latin1String("warning")); break;
         case pdk::MsgType::CriticalMsg:message.append(Latin1String("critical")); break;
         case pdk::MsgType::FatalMsg:   message.append(Latin1String("fatal")); break;
         }
      } else if (token == sg_fileTokenC) {
         if (context.m_file) {
            message.append(Latin1String(context.m_file));
         } else {
            message.append(Latin1String("unknown"));
         }
      } else if (token == sg_lineTokenC) {
         message.append(String::number(context.m_line));
      } else if (token == sg_functionTokenC) {
         if (context.m_function) {
            message.append(String::fromLatin1(pdk_cleanup_func_info(context.m_function)));
         } else
            message.append(Latin1String("unknown"));
      } else if (token == sg_pidTokenC) {
         message.append(String::number(CoreApplication::getAppPid()));
      } else if (token == sg_appnameTokenC) {
         message.append(CoreApplication::getAppName());
      } else if (token == sg_threadidTokenC) {
         // print the TID as decimal
         message.append(String::number(pdk_gettid()));
      } else if (token == sg_qthreadptrTokenC) {
         message.append(Latin1String("0x"));
         message.append(String::number(pdk::plonglong(Thread::getCurrentThread()->getCurrentThread()), 16));
#ifdef LOGGING_HAVE_BACKTRACE
         //      } else if (token == sg_backtraceTokenC) {
         //         MessagePattern::BacktraceParams backtraceParams = pattern->m_backtraceArgs.at(backtraceArgsIdx);
         //         backtraceArgsIdx++;
         //         message.append(formatBacktraceForLogMessage(backtraceParams, context.function));
#endif
      } else if (token == sg_timeTokenC) {
         auto iter = pattern->m_timeArgs.begin();
         std::advance(iter, timeArgsIdx);
         String timeFormat = *iter;
         timeArgsIdx++;
         if (timeFormat == Latin1String("process")) {
            pdk::puint64 ms = pattern->m_timer.getElapsed();
            message.append(String::asprintf("%6d.%03d", uint(ms / 1000), uint(ms % 1000)));
         } else if (timeFormat ==  Latin1String("boot")) {
            // just print the milliseconds since the elapsed timer reference
            // like the Linux kernel does
            ElapsedTimer now;
            now.start();
            uint ms = now.getMsecsSinceReference();
            message.append(String::asprintf("%6d.%03d", uint(ms / 1000), uint(ms % 1000)));
#if PDK_CONFIG(datestring)
         } else if (timeFormat.isEmpty()) {
            message.append(DateTime::getCurrentDateTime().toString(pdk::DateFormat::ISODate));
         } else {
            message.append(DateTime::getCurrentDateTime().toString(timeFormat));
#endif // PDK_CONFIG(datestring)
         }
      } else if (token == sg_ifCategoryTokenC) {
         if (!context.m_category || (strcmp(context.m_category, "default") == 0)) {
            skip = true;
         }
      } else if (token == sg_ifDebugTokenC) {
         skip = type != pdk::MsgType::DebugMsg;
      } else if (token == sg_ifInfoTokenC) {
         skip = type != pdk::MsgType::InfoMsg;
      } else if (token == sg_ifCriticalTokenC) {
         skip = type != pdk::MsgType::CriticalMsg;
      } else if (token == sg_ifFatalTokenC) {
         skip = type != pdk::MsgType::FatalMsg;
      } else {
         message.append(Latin1String(token));
      }
   }
   return message;
}

namespace {

void pdk_default_msg_handler(pdk::MsgType type, const char *buf);
void pdk_default_message_handler(pdk::MsgType type, const MessageLogContext &context, const String &buf);

} // anonymous namespace

// pointer to MsgHandler debug handler (without context)
static BasicAtomicPointer<void (pdk::MsgType, const char*)> sg_msgHandler = PDK_BASIC_ATOMIC_INITIALIZER(pdk_default_msg_handler);
// pointer to QtMessageHandler debug handler (with context)
static BasicAtomicPointer<void (pdk::MsgType, const MessageLogContext &, const String &)> sg_messageHandler = PDK_BASIC_ATOMIC_INITIALIZER(pdk_default_message_handler);

#if PDK_CONFIG(journald)
void systemd_default_message_handler(pdk::MsgType type,
                                            const MessageLogContext &context,
                                            const String &message)
{
   int priority = LOG_INFO; // Informational
   switch (type) {
   case pdk::MsgType::DebugMsg:
      priority = LOG_DEBUG; // Debug-level messages
      break;
   case pdk::MsgType::InfoMsg:
      priority = LOG_INFO; // Informational conditions
      break;
   case pdk::MsgType::WarningMsg:
      priority = LOG_WARNING; // Warning conditions
      break;
   case pdk::MsgType::CriticalMsg:
      priority = LOG_CRIT; // Critical conditions
      break;
   case pdk::MsgType::FatalMsg:
      priority = LOG_ALERT; // Action must be taken immediately
      break;
   }
   
   sd_journal_send("MESSAGE=%s",     message.toUtf8().constData(),
                   "PRIORITY=%i",    priority,
                   "CODE_FUNC=%s",   context.function ? context.function : "unknown",
                   "CODE_LINE=%d",   context.line,
                   "CODE_FILE=%s",   context.file ? context.file : "unknown",
                   "PDK_CATEGORY=%s", context.category ? context.category : "unknown",
                   NULL);
}
#endif

#if PDK_CONFIG(syslog)
void syslog_default_message_handler(pdk::MsgType type, const char *message)
{
   int priority = LOG_INFO; // Informational
   switch (type) {
   case pdk::MsgType::DebugMsg:
      priority = LOG_DEBUG; // Debug-level messages
      break;
   case pdk::MsgType::InfoMsg:
      priority = LOG_INFO; // Informational conditions
      break;
   case pdk::MsgType::WarningMsg:
      priority = LOG_WARNING; // Warning conditions
      break;
   case pdk::MsgType::CriticalMsg:
      priority = LOG_CRIT; // Critical conditions
      break;
   case pdk::MsgType::FatalMsg:
      priority = LOG_ALERT; // Action must be taken immediately
      break;
   }
   syslog(priority, "%s", message);
}
#endif

namespace {

void pdk_default_message_handler(pdk::MsgType type, const MessageLogContext &context,
                                 const String &buf)
{
   String logMessage = format_log_message(type, context, buf);   
   // print nothing if message pattern didn't apply / was empty.
   // (still print empty lines, e.g. because message itself was empty)
   if (logMessage.isNull()) {
      return;
   }
   if (!pdk_logging_to_console()) {
#if defined(PDK_OS_WIN)
      logMessage.append(Latin1Character('\n'));
      OutputDebugString(reinterpret_cast<const wchar_t *>(logMessage.utf16()));
      return;
#elif PDK_CONFIG(slog2)
      logMessage.append(Latin1Character('\n'));
      slog2_default_handler(type, logMessage.toLocal8Bit().getConstRawData());
      return;
#elif PDK_CONFIG(journald)
      systemd_default_message_handler(type, context, logMessage);
      return;
#elif PDK_CONFIG(syslog)
      syslog_default_message_handler(type, logMessage.toUtf8().getConstRawData());
      return;
#endif
   }
   fprintf(stderr, "%s\n", logMessage.toLocal8Bit().getConstRawData());
   fflush(stderr);
}

void pdk_default_msg_handler(pdk::MsgType type, const char *buf)
{
   MessageLogContext emptyContext;
   pdk_default_message_handler(type, emptyContext, String::fromLocal8Bit(buf));
}

static thread_local bool sg_msgHandlerGrabbed = false;

bool grab_message_handler()
{
   if (sg_msgHandlerGrabbed) {
      return false;
   }
   sg_msgHandlerGrabbed = true;
   return true;
}

void ungrab_message_handler()
{
   sg_msgHandlerGrabbed = false;
}

void pdk_message_print(pdk::MsgType msgType, const MessageLogContext &context, const String &message)
{
   // debug_stream, warning_stream, ... macros do not check whether category is enabled
   if (!context.m_category || (strcmp(context.m_category, "default") == 0)) {
      if (LoggingCategory *defaultCategory = LoggingCategory::getDefaultCategory()) {
         if (!defaultCategory->isEnabled(msgType)) {
            return;
         }  
      }
   }
   // prevent recursion in case the message handler generates messages
   // itself, e.g. by using Qt API
   if (grab_message_handler()) {
      // prefer new message handler over the old one
      if (sg_msgHandler.load() == pdk_default_msg_handler
          || sg_messageHandler.load() != pdk_default_message_handler) {
         (*sg_messageHandler.load())(msgType, context, message);
      } else {
         (*sg_msgHandler.load())(msgType, message.toLocal8Bit().getConstRawData());
      }
      ungrab_message_handler();
   } else {
      fprintf(stderr, "%s\n", message.toLocal8Bit().getConstRawData());
   }
}

void pdk_message_fatal(pdk::MsgType, const MessageLogContext &context, const String &message)
{
#if defined(PDK_CC_MSVC) && defined(PDK_DEBUG) && defined(_DEBUG) && defined(_CRT_ERROR)
   wchar_t contextFileL[256];
   // we probably should let the compiler do this for us, by declaring QMessageLogContext::file to
   // be const wchar_t * in the first place, but the #ifdefery above is very complex  and we
   // wouldn't be able to change it later on...
   convert_to_wchar_t_elided(contextFileL, sizeof contextFileL / sizeof *contextFileL,
                             context.m_file);
   // get the current report mode
   int reportMode = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
   _CrtSetReportMode(_CRT_ERROR, reportMode);
   
   int ret = _CrtDbgReportW(_CRT_ERROR, contextFileL, context.m_line, _CRT_WIDE(PDK_VERSION_STR),
                            reinterpret_cast<const wchar_t *>(message.utf16()));
   if ((ret == 0) && (reportMode & _CRTDBG_MODE_WNDW))
      return; // ignore
   else if (ret == 1)
      _CrtDbgBreak();
#else
   PDK_UNUSED(context);
   PDK_UNUSED(message);
#endif
   std::abort();
}

} // anonymous namespace

void message_output(pdk::MsgType msgType, const MessageLogContext &context, const String &message)
{
   pdk_message_print(msgType, context, message);
   if (is_fatal(msgType)) {
      pdk_message_fatal(msgType, context, message);
   }  
}

void errno_warning(const char *msg, ...)
{
   // pdk::error_string() will allocate anyway, so we don't have
   // to be careful here (like we do in plain warning_stream())
   va_list ap;
   va_start(ap, msg);
   String buf = String::vasprintf(msg, ap);
   va_end(ap);
   buf += Latin1String(" (") + pdk::error_string(-1) + Latin1Character(')');
   MessageLogContext context;
   message_output(pdk::MsgType::CriticalMsg, context, buf);
}

void errno_warning(int code, const char *msg, ...)
{
   // pdk::error_string() will allocate anyway, so we don't have
   // to be careful here (like we do in plain warning_stream())
   va_list ap;
   va_start(ap, msg);
   String buf = String::vasprintf(msg, ap);
   va_end(ap);
   buf += Latin1String(" (") + pdk::error_string(code) + Latin1Character(')');
   MessageLogContext context;
   message_output(pdk::MsgType::CriticalMsg, context, buf);
}

PdkMessageHandler install_message_handler(PdkMessageHandler handler)
{
   if (!handler) {
      handler = pdk_default_message_handler;
   }
   //set 'h' and return old message handler
   return sg_messageHandler.fetchAndStoreRelaxed(handler);
}

void pdk_set_message_pattern(const String &pattern)
{
   std::lock_guard<std::mutex> lock(MessagePattern::sm_mutex);
   if (!sg_messagePattern()->m_fromEnvironment) {
      sg_messagePattern()->setPattern(pattern);
   }
}

void MessageLogContext::copy(const MessageLogContext &logContext)
{
   this->m_category = logContext.m_category;
   this->m_file = logContext.m_file;
   this->m_line = logContext.m_line;
   this->m_function = logContext.m_function;
}

} // pdk

#ifdef LOGGING_HAVE_BACKTRACE
PDK_DECLARE_TYPEINFO(pdk::MessagePattern::BacktraceParams, PDK_MOVABLE_TYPE);
#endif
