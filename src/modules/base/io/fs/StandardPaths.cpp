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
// Created by softboy on 2018/02/26.

#include "pdk/base/io/fs/StandardPaths.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/kernel/Object.h"
#include "pdk/kernel/CoreApplication.h"

#if PDK_HAS_INCLUDE(<paths.h>)
#include <paths.h>
#endif

#ifdef PDK_OS_UNIX
#include <unistd.h>
#endif

#ifndef PDK_NO_STANDARDPATHS

namespace pdk {
namespace io {
namespace fs {

using pdk::lang::String;
using pdk::lang::StringList;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::ds::ByteArray;
using pdk::kernel::CoreApplication;

namespace {

bool exists_as_specified(const String &path, StandardPaths::LocateOptions options)
{
   if (options & StandardPaths::LocateOption::LocateDirectory) {
      return Dir(path).exists();
   }
   return FileInfo(path).isFile();
}

String check_executable(const String &path)
{
   const FileInfo info(path);
   if (info.isBundle()) {
      return info.getBundleName();
   }
   if (info.isFile() && info.isExecutable()) {
      return Dir::cleanPath(path);
   }
   return String();
}

#ifdef PDK_OS_WIN
StringList executable_extensions()
{
   // If %PATHEXT% does not contain .exe, it is either empty, malformed, or distorted in ways that we cannot support, anyway.
   const StringList pathExt = String::fromLocal8Bit(pdk::pdk_getenv("PATHEXT")).toLower().split(Latin1Character(';'));
   return pathExt.contains(Latin1String(".exe"), pdk::CaseSensitivity::Insensitive) ?
            pathExt :
            StringList() << Latin1String(".exe") << Latin1String(".com")
                         << Latin1String(".bat") << Latin1String(".cmd");
}

// Find executable appending candidate suffixes, used for suffix-less executables
// on Windows.
inline String search_executable_append_suffix(const StringList &searchPaths,
                                              const String &executableName,
                                              const StringList &suffixes)
{
   const Dir currentDir = Dir::getCurrent();
   for (const String &searchPath : searchPaths) {
      const String candidateRoot = currentDir.getAbsoluteFilePath(searchPath + Latin1Character('/') + executableName);
      for (const String &suffix : suffixes) {
         const String absPath = check_executable(candidateRoot + suffix);
         if (!absPath.isEmpty()) {
            return absPath;
         }
      }
   }
   return String();
}
#endif

inline String searchExecutable(const StringList &searchPaths,
                               const String &executableName)
{
   const Dir currentDir = Dir::getCurrent();
   for (const String &searchPath : searchPaths) {
      const String candidate = currentDir.getAbsoluteFilePath(searchPath + Latin1Character('/') + executableName);
      const String absPath = check_executable(candidate);
      if (!absPath.isEmpty()) {
         return absPath;
      }
   }
   return String();
}

} // anonymous namespace

String StandardPaths::locate(StandardLocation type, const String &fileName, LocateOptions options)
{
   const StringList &dirs = standardLocations(type);
   for (StringList::const_iterator dir = dirs.cbegin(); dir != dirs.cend(); ++dir) {
      const String path = *dir + Latin1Character('/') + fileName;
      if (exists_as_specified(path, options)) {
         return path;
      } 
   }
   return String();
}

StringList StandardPaths::locateAll(StandardLocation type, const String &fileName, LocateOptions options)
{
   const StringList &dirs = standardLocations(type);
   StringList result;
   for (StringList::const_iterator dir = dirs.cbegin(); dir != dirs.cend(); ++dir) {
      const String path = *dir + Latin1Character('/') + fileName;
      if (exists_as_specified(path, options)) {
         result.push_back(path);
      }
   }
   return result;
}

String StandardPaths::findExecutable(const String &executableName, const StringList &paths)
{
   if (FileInfo(executableName).isAbsolute()) {
      return check_executable(executableName);
   }
   StringList searchPaths = paths;
   if (paths.empty()) {
      ByteArray pEnv = pdk::pdk_getenv("PATH");
      if (PDK_UNLIKELY(pEnv.isNull())) {
         // Get a default path. POSIX.1 does not actually require this, but
         // most Unix libc fall back to confstr(_CS_PATH) if the PATH
         // environment variable isn't set. Let's try to do the same.
#if defined(_PATH_DEFPATH)
         // BSD API.
         pEnv = _PATH_DEFPATH;
#elif defined(_CS_PATH)
         // POSIX API.
         size_t n = confstr(_CS_PATH, nullptr, 0);
         if (n) {
            pEnv.resize(n);
            // size()+1 is ok because ByteArray always has an extra NUL-terminator
            confstr(_CS_PATH, pEnv.getRawData(), pEnv.size() + 1);
         }
#else
         // Windows SDK's execvpe() does not have a fallback, so we won't
         // apply one either.
#endif
      }
      
      // Remove trailing slashes, which occur on Windows.
      const StringList rawPaths = String::fromLocal8Bit(pEnv.getConstRawData()).split(Dir::getListSeparator(),                                                                           String::SplitBehavior::SkipEmptyParts);
      searchPaths.resize(rawPaths.size());
      for (const String &rawPath : rawPaths) {
         String cleanPath = Dir::cleanPath(rawPath);
         if (cleanPath.size() > 1 && cleanPath.endsWith(Latin1Character('/'))) {
            cleanPath.truncate(cleanPath.size() - 1);
         }
         searchPaths.push_back(cleanPath);
      }
   }
   
#ifdef PDK_OS_WIN
   // On Windows, if the name does not have a suffix or a suffix not
   // in PATHEXT ("xx.foo"), append suffixes from PATHEXT.
   static const StringList executableExtensions = executable_extensions();
   if (executableName.contains(Latin1Character('.'))) {
      const String suffix = FileInfo(executableName).suffix();
      if (suffix.isEmpty() || !executableExtensions.contains(Latin1Character('.') + suffix, pdk::CaseSensitivity::Insensitive))
         return search_executable_append_suffix(searchPaths, executableName, executableExtensions);
   } else {
      return search_executable_append_suffix(searchPaths, executableName, executableExtensions);
   }
#endif
   return searchExecutable(searchPaths, executableName);
}

#if !defined(PDK_OS_MAC)
String StandardPaths::displayName(StandardLocation type)
{
   switch (type) {
   case StandardLocation::DesktopLocation:
      return CoreApplication::translate("StandardPaths", "Desktop");
   case StandardLocation::DocumentsLocation:
      return CoreApplication::translate("StandardPaths", "Documents");
   case StandardLocation::FontsLocation:
      return CoreApplication::translate("StandardPaths", "Fonts");
   case StandardLocation::ApplicationsLocation:
      return CoreApplication::translate("StandardPaths", "Applications");
   case StandardLocation::MusicLocation:
      return CoreApplication::translate("StandardPaths", "Music");
   case StandardLocation::MoviesLocation:
      return CoreApplication::translate("StandardPaths", "Movies");
   case StandardLocation::PicturesLocation:
      return CoreApplication::translate("StandardPaths", "Pictures");
   case StandardLocation::TempLocation:
      return CoreApplication::translate("StandardPaths", "Temporary Directory");
   case StandardLocation::HomeLocation:
      return CoreApplication::translate("StandardPaths", "Home");
   case StandardLocation::CacheLocation:
      return CoreApplication::translate("StandardPaths", "Cache");
   case StandardLocation::GenericDataLocation:
      return CoreApplication::translate("StandardPaths", "Shared Data");
   case StandardLocation::RuntimeLocation:
      return CoreApplication::translate("StandardPaths", "Runtime");
   case StandardLocation::ConfigLocation:
      return CoreApplication::translate("StandardPaths", "Configuration");
   case StandardLocation::GenericConfigLocation:
      return CoreApplication::translate("StandardPaths", "Shared Configuration");
   case StandardLocation::GenericCacheLocation:
      return CoreApplication::translate("StandardPaths", "Shared Cache");
   case StandardLocation::DownloadLocation:
      return CoreApplication::translate("StandardPaths", "Download");
   case StandardLocation::AppDataLocation:
   case StandardLocation::AppLocalDataLocation:
      return CoreApplication::translate("StandardPaths", "Application Data");
   case StandardLocation::AppConfigLocation:
      return CoreApplication::translate("StandardPaths", "Application Configuration");
   }
   // not reached
   return String();
}
#endif

static bool sg_testMode = false;

void StandardPaths::setTestModeEnabled(bool testMode)
{
   sg_testMode = testMode;
}

bool StandardPaths::isTestModeEnabled()
{
   return sg_testMode;
}

} // fs
} // io
} // pdk

#endif // PDK_NO_STANDARDPATHS
