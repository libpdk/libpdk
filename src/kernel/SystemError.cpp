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
// Created by softboy on 2018/02/06.

#include "pdk/global/Global.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"
#include "pdk/base/lang/String.h"
#include <cerrno>
#if defined(PDK_CC_MSVC)
#  include <crtdbg.h>
#endif
#ifdef PDK_OS_WIN
#  include "pdk/global/Windows.h"
#endif

using pdk::lang::Latin1Character;
using pdk::lang::Latin1String;
using pdk::lang::String;

namespace
{
#if !defined(PDK_OS_WIN) && !defined(PDK_NO_THREAD) && !defined(PDK_OS_INTEGRITY) && \
   defined(_POSIX_THREAD_SAFE_FUNCTIONS) && _POSIX_VERSION >= 200112L

// There are two incompatible versions of strerror_r:
// a) the XSI/POSIX.1 version, which returns an int,
//    indicating success or not
// b) the GNU version, which returns a char*, which may or may not
//    be the beginning of the buffer we used
// The GNU libc manpage for strerror_r says you should use the XSI
// version in portable code. However, it's impossible to do that if
// _GNU_SOURCE is defined so we use C++ overloading to decide what to do
// depending on the return type
inline PDK_DECL_UNUSED String from_strerror_helper(int, const ByteArray &buf)
{
   return String::fromLocal8Bit(buf);
}
inline PDK_DECL_UNUSED String fromstrerror_helper(const char *str, const ByteArray &)
{
   return String::fromLocal8Bit(str);
}

#endif

#ifdef PDK_OS_WIN
String windows_error_string(int errorCode)
{
   String ret;
   wchar_t *string = 0;
   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL,
                 errorCode,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPWSTR)&string,
                 0,
                 NULL);
   ret = String::fromWCharArray(string);
   LocalFree((HLOCAL)string);
   
   if (ret.isEmpty() && errorCode == ERROR_MOD_NOT_FOUND) {
      ret = String::fromLatin1("The specified module could not be found.");
   }
   if (ret.endsWith(Latin1String("\r\n"))) {
      ret.chop(2);
   }
   if (ret.isEmpty()) {
      ret = String::fromLatin1("Unknown error 0x%1.")
            .arg(unsigned(errorCode), 8, 16, Latin1Character('0'));
   }
   return ret;
}
#endif

String standard_library_error_string(int errorCode)
{
   const char *s = 0;
   String ret;
   switch (errorCode) {
   case 0:
      break;
   case EACCES:
      s = PDK_TRANSLATE_NOOP("IoDevice", "Permission denied");
      break;
   case EMFILE:
      s = PDK_TRANSLATE_NOOP("IoDevice", "Too many open files");
      break;
   case ENOENT:
      s = PDK_TRANSLATE_NOOP("IoDevice", "No such file or directory");
      break;
   case ENOSPC:
      s = PDK_TRANSLATE_NOOP("IoDevice", "No space left on device");
      break;
   default: {
#if !defined(PDK_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && _POSIX_VERSION >= 200112L && !defined(PDK_OS_INTEGRITY)
      ByteArray buf(1024, Qt::Uninitialized);
      ret = fromstrerror_helper(strerror_r(errorCode, buf.getRawData(), buf.size()), buf);
#else
      ret = String::fromLocal8Bit(strerror(errorCode));
#endif
      break; }
   }
   if (s) {
      // ######## this breaks moc build currently
      // ret = CoreApplication::translate("IoDevice", s);
      ret = String::fromLatin1(s);
   }
   return ret.trimmed();
}

} // anonymous namespace

namespace pdk {
namespace kernel {
namespace internal {

String SystemError::getString(ErrorScope errorScope, int errorCode)
{
   switch(errorScope) {
   case ErrorScope::NativeError:
#if defined (PDK_OS_WIN)
      return windowsErrorString(errorCode);
#else
      //unix: fall through as native and standard library are the same
      PDK_FALLTHROUGH();
#endif
   case ErrorScope::StandardLibraryError:
      return standard_library_error_string(errorCode);
   case ErrorScope::NoError:
      return Latin1String("No error");
   }
}

String SystemError::getStdString(int errorCode)
{
   return standard_library_error_string(errorCode == -1 ? errno : errorCode);
}

} // internal
} // kernel

#ifdef PDK_OS_WIN
String SystemError::getWindowsString(int errorCode)
{
   return windows_error_string(errorCode == -1 ? GetLastError() : errorCode);
}

String pdk_error_string(int code)
{
   return windowsErrorString(code == -1 ? GetLastError() : code);
}
#else
String pdk_error_string(int code)
{
   return standard_library_error_string(code == -1 ? errno : code);
}
#endif

} // pdk


