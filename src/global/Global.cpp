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
// Created by softboy on 2017/11/13.

#include "pdk/global/Global.h"
#include "pdk/utils/Funcs.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/lang/String.h"
#include "pdk/global/SysInfo.h"
#include <iostream>
#include <mutex>
#include <string>
#include <exception>
#include <cstdlib>
#include <climits>
#include <cstdarg>
#include <cstring>

#if defined(PDK_OS_SOLARIS)
#  include <sys/systeminfo.h>
#endif

#ifdef PDK_OS_UNIX
#include <sys/utsname.h>
#include "pdk/kernel/internal/CoreUnixPrivate.h"
#endif

#ifdef PDK_OS_BSD4
#include <sys/sysctl.h>
#endif

namespace pdk
{

using pdk::ds::ByteArray;
using pdk::ds::VarLengthArray;
using pdk::lang::Latin1Character;
using pdk::lang::String;

void pdk_assert(const char *assertion, const char *file, 
                int line) noexcept
{
   std::string errMsg(assertion);
   errMsg += " is fail";
   std::cerr << errMsg << std::endl;
   PDK_UNUSED(line);
   PDK_UNUSED(file);
}

void pdk_assert_x(const char *where, const char *what, 
                  const char *file, int line) noexcept
{
   std::cerr << what << std::endl;
   PDK_UNUSED(where);
   PDK_UNUSED(file);
   PDK_UNUSED(line);
}

static std::mutex sg_envMutex;

ByteArray pdk_getenv(const char *varName)
{
   std::lock_guard locker(sg_envMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
   size_t requiredSize = 0;
   ByteArray buffer;
   getenv_s(&requiredSize, 0, 0, varName);
   if (requiredSize == 0)
      return buffer;
   buffer.resize(int(requiredSize));
   getenv_s(&requiredSize, buffer.getRawData(), requiredSize, varName);
   // requiredSize includes the terminating null, which we don't want.
   PDK_ASSERT(buffer.endsWith('\0'));
   buffer.chop(1);
   return buffer;
#else
   return ByteArray(std::getenv(varName));
#endif
}

String pdk_env_var(const char *varName, const String &defaultValue)
{
#if defined(PDK_OS_WIN)
   std::lock_guard locker(sg_envMutex);
   VarLengthArray<wchar_t, 32> wname(int(strlen(varName)) + 1);
   for (int i = 0; i < wname.size(); ++i) { 
      // wname.size() is correct: will copy terminating null
      wname[i] = uchar(varName[i]);
   }
   size_t requiredSize = 0;
   String buffer;
   _wgetenv_s(&requiredSize, 0, 0, wname.getRawData());
   if (requiredSize == 0) {
      return defaultValue;
   }
   buffer.resize(int(requiredSize));
   _wgetenv_s(&requiredSize, reinterpret_cast<wchar_t *>(buffer.getRawData()), requiredSize,
              wname.getRawData());
   // requiredSize includes the terminating null, which we don't want.
   PDK_ASSERT(buffer.endsWith(Latin1Character('\0')));
   buffer.chop(1);
   return buffer;
#else
   ByteArray value = pdk_getenv(varName);
   if (value.isNull()) {
      return defaultValue;
   }
#if defined(PDK_OS_DARWIN)
   return String::fromUtf8(value).normalized(String::NormalizationForm::Form_C);
#else // other Unix
   return String::fromLocal8Bit(value);
#endif
#endif
}

String pdk_env_var(const char *varName)
{
   return pdk_env_var(varName, String());
}

bool env_var_is_empty(const char *varName) noexcept
{
   std::lock_guard locker(sg_envMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
   // we provide a buffer that can only hold the empty string, so
   // when the env.var isn't empty, we'll get an ERANGE error (buffer
   // too small):
   size_t dummy;
   char buffer = '\0';
   return getenv_s(&dummy, &buffer, 1, varName) != ERANGE;
#else
   const char * const value = std::getenv(varName);
   return !value || !*value;
#endif
}

int env_var_intval(const char *varName, bool *ok) noexcept
{
   static const int NumBinaryDigitsPerOctalDigit = 3;
   static const int MaxDigitsForOctalInt =
         (std::numeric_limits<uint>::digits + NumBinaryDigitsPerOctalDigit - 1) / NumBinaryDigitsPerOctalDigit;
   
   std::lock_guard locker(sg_envMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
   // we provide a buffer that can hold any int value:
   char buffer[MaxDigitsForOctalInt + 2]; // +1 for NUL +1 for optional '-'
   size_t dummy;
   if (getenv_s(&dummy, buffer, sizeof buffer, varName) != 0) {
      if (ok) {
         *ok = false;
      }
      return 0;
   }
#else
   const char * const buffer = std::getenv(varName);
   if (!buffer || strlen(buffer) > MaxDigitsForOctalInt + 2) {
      if (ok) {
         *ok = false;
      }
      return 0;
   }
#endif
   bool ok_ = true;
   char *endptr;
   const pdk::plonglong value = std::strtoll(buffer, &endptr, 0);
   if (errno == ERANGE){
      ok_ = false;
      errno = 0;
   }
   if (int(value) != value || *endptr != '\0') { // this is the check in ByteArray::toInt(), keep it in sync
      if (ok) {
         *ok = false;
      }
      return 0;
   } else if (ok) {
      *ok = ok_;
   }
   return static_cast<int>(value);
}

bool env_var_isset(const char *varName) noexcept
{
   std::lock_guard locker(sg_envMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
   size_t requiredSize = 0;
   (void)getenv_s(&requiredSize, 0, 0, varName);
   return requiredSize != 0;
#else
   return std::getenv(varName) != 0;
#endif
}

bool pdk_putenv(const char *varName, const ByteArray& value)
{
   std::lock_guard locker(sg_envMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
   return _putenv_s(varName, value.constData()) == 0;
#elif (defined(_POSIX_VERSION) && (_POSIX_VERSION-0) >= 200112L)
   // POSIX.1-2001 has setenv
   return setenv(varName, value.getRawData(), true) == 0;
#else
   ByteArray buffer(varName);
   buffer += '=';
   buffer += value;
   char* envVar = qstrdup(buffer.getConstRawData());
   int result = putenv(envVar);
   if (result != 0) // error. we have to delete the string.
      delete[] envVar;
   return result == 0;
#endif
}

bool pdk_unsetenv(const char *varName)
{
   std::lock_guard locker(sg_envMutex);
#if defined(_MSC_VER) && _MSC_VER >= 1400
   return _putenv_s(varName, "") == 0;
#elif (defined(_POSIX_VERSION) && (_POSIX_VERSION-0) >= 200112L) || defined(PDK_OS_BSD4)
   // POSIX.1-2001, BSD and Haiku have unsetenv
   return unsetenv(varName) == 0;
#elif defined(PDK_CC_MINGW)
   // On mingw, putenv("var=") removes "var" from the environment
   ByteArray buffer(varName);
   buffer += '=';
   return putenv(buffer.getConstRawData()) == 0;
#else
   // Fallback to putenv("var=") which will insert an empty var into the
   // environment and leak it
   ByteArray buffer(varName);
   buffer += '=';
   char *envVar = qstrdup(buffer.getConstRawData());
   return putenv(envVar) == 0;
#endif
}

String SysInfo::getMachineHostName()
{
#if defined(PDK_OS_LINUX)
   // gethostname(3) on Linux just calls uname(2), so do it ourselves
   // and avoid a memcpy
   struct utsname u;
   if (uname(&u) == 0) {
      return String::fromLocal8Bit(u.nodename);
   }
#else
#  ifdef PDK_OS_WIN
   // Important: pdk Network depends on machineHostName() initializing ws2_32.dll
   winsockInit();
#  endif   
   char hostName[512];
   if (gethostname(hostName, sizeof(hostName)) == -1) {
      return String();
   }
   hostName[sizeof(hostName) - 1] = '\0';
   return String::fromLocal8Bit(hostName);
#endif
   return String();
}

} // pdk
