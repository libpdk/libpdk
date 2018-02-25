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
// Created by softboy on 2018/02/24.

#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/Settings.h"
#include "pdk/global/LibraryInfo.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/global/GlobalStatic.h"

#ifdef PDK_OS_DARWIN
#include "pdk/kernel/internal/CoreMacPrivate.h"
#endif

#include "pdk/global/internal/ArchDetechPrivate.h"

namespace pdk {

// forward declare class with namespace
namespace pal {
namespace kernel {
void dump_cpu_features(); // // in pdk/pal/kernel/Simd.cpp
} // kernel
} // pal

#ifndef PDK_NO_SETTINGS

using pdk::io::fs::Settings;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::lang::StringList;

namespace internal {
struct LibrarySettings
{
   LibrarySettings();
   void load();
   pdk::utils::ScopedPointer<Settings> m_settings;
   bool m_haveDevicePaths;
   bool m_haveEffectiveSourcePaths;
   bool m_haveEffectivePaths;
   bool m_havePaths;
};

PDK_GLOBAL_STATIC(LibrarySettings, sg_librarySettings);

class LibraryInfoPrivate
{
public:
   static Settings *findConfiguration();
   static void reload()
   {
      if (sg_librarySettings.exists()) {
         sg_librarySettings->load();
      } 
   }
   static bool haveGroup(LibraryInfo::PathGroup group)
   {
   }
   
   static Settings *configuration()
   {
   }
};

static const char sg_platformsSection[] = "Platforms";

LibrarySettings::LibrarySettings()
{
   load();
}

void LibrarySettings::load()
{
}

Settings *LibraryInfoPrivate::findConfiguration()
{
}

#endif // PDK_NO_SETTINGS

} // internal

LibraryInfo::LibraryInfo()
{}

#if defined(PDK_CC_INTEL) // must be before GNU, Clang and MSVC because ICC/ICL claim to be them
#  ifdef __INTEL_CLANG_COMPILER
#    define ICC_COMPAT "Clang"
#  elif defined(__INTEL_MS_COMPAT_LEVEL)
#    define ICC_COMPAT "Microsoft"
#  elif defined(__GNUC__)
#    define ICC_COMPAT "GCC"
#  else
#    define ICC_COMPAT "no"
#  endif
#  if __INTEL_COMPILER == 1300
#    define ICC_VERSION "13.0"
#  elif __INTEL_COMPILER == 1310
#    define ICC_VERSION "13.1"
#  elif __INTEL_COMPILER == 1400
#    define ICC_VERSION "14.0"
#  elif __INTEL_COMPILER == 1500
#    define ICC_VERSION "15.0"
#  else
#    define ICC_VERSION PDK_STRINGIFY(__INTEL_COMPILER)
#  endif
#  ifdef __INTEL_COMPILER_UPDATE
#    define COMPILER_STRING "Intel(R) C++ " ICC_VERSION "." PDK_STRINGIFY(__INTEL_COMPILER_UPDATE) \
   " build " PDK_STRINGIFY(__INTEL_COMPILER_BUILD_DATE) " [" \
   ICC_COMPAT " compatibility]"
#  else
#    define COMPILER_STRING "Intel(R) C++ " ICC_VERSION \
   " build " PDK_STRINGIFY(__INTEL_COMPILER_BUILD_DATE) " [" \
   ICC_COMPAT " compatibility]"
#  endif
#elif defined(PDK_CC_CLANG) // must be before GNU, because clang claims to be GNU too
#  ifdef __apple_build_version__ // Apple clang has other version numbers
#    define COMPILER_STRING "Clang " __clang_version__ " (Apple)"
#  else
#    define COMPILER_STRING "Clang " __clang_version__
#  endif
#elif defined(PDK_CC_GHS)
#  define COMPILER_STRING "GHS " PDK_STRINGIFY(__GHS_VERSION_NUMBER)
#elif defined(PDK_CC_GNU)
#  define COMPILER_STRING "GCC " __VERSION__
#elif defined(PDK_CC_MSVC)
#  if _MSC_VER < 1600
#    define COMPILER_STRING "MSVC 2008"
#  elif _MSC_VER < 1700
#    define COMPILER_STRING "MSVC 2010"
#  elif _MSC_VER < 1800
#    define COMPILER_STRING "MSVC 2012"
#  elif _MSC_VER < 1900
#    define COMPILER_STRING "MSVC 2013"
#  elif _MSC_VER < 1910
#    define COMPILER_STRING "MSVC 2015"
#  elif _MSC_VER < 2000
#    define COMPILER_STRING "MSVC 2017"
#  else
#    define COMPILER_STRING "MSVC _MSC_VER " PDK_STRINGIFY(_MSC_VER)
#  endif
#else
#  define COMPILER_STRING "<unknown compiler>"
#endif
#ifdef PDK_NO_DEBUG
#  define DEBUG_STRING " release"
#else
#  define DEBUG_STRING " debug"
#endif
#ifdef PDK_SHARED
#  define SHARED_STRING " shared (dynamic)"
#else
#  define SHARED_STRING " static"
#endif
#define PDK_BUILD_STR "pdk " PDK_VERSION_STR " (" ARCH_FULL SHARED_STRING DEBUG_STRING " build; by " COMPILER_STRING ")"

const char *LibraryInfo::build() noexcept
{
   return PDK_BUILD_STR;
}

bool LibraryInfo::isDebugBuild()
{
#ifdef PDK_DEBUG
   return true;
#else
   return false;
#endif
}

VersionNumber LibraryInfo::getVersion() noexcept
{
   return VersionNumber();
}

static const struct {
   char key[19], value[13];
} sg_pdkConfEntries[] = {
{ "Prefix", "." },
{ "Documentation", "doc" }, // should be ${Data}/doc
{ "Headers", "include" },
{ "Libraries", "lib" },
#ifdef PDK_OS_WIN
{ "LibraryExecutables", "bin" },
#else
{ "LibraryExecutables", "libexec" }, // should be ${ArchData}/libexec
#endif
{ "Binaries", "bin" },
{ "Plugins", "plugins" }, // should be ${ArchData}/plugins
{ "Imports", "imports" }, // should be ${ArchData}/imports
{ "ArchData", "." },
{ "Data", "." },
{ "Translations", "translations" }, // should be ${Data}/translations
{ "Examples", "examples" },
{ "Tests", "tests" },
{ "Sysroot", "" },
{ "SysrootifyPrefix", "" },
{ "HostBinaries", "bin" },
{ "HostLibraries", "lib" },
{ "HostData", "." },
{ "TargetSpec", "" },
{ "HostSpec", "" },
{ "HostPrefix", "" },
};

void LibraryInfo::reload()
{}

String LibraryInfo::getPath(LibraryLocation loc)
{}


String LibraryInfo::getRawLocation(LibraryLocation loc, PathGroup group)
{}

StringList LibraryInfo::platformPluginArguments(const String &platformName)
{
}

} // pdk
