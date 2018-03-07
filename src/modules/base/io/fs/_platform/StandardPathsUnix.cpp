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
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/TextStream.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/kernel/CoreApplication.h"
#include <cerrno>
#include <cstdlib>
#include <map>

#ifndef PDK_NO_STANDARDPATHS

namespace pdk {
namespace io {
namespace fs {

using pdk::kernel::CoreApplication;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::fs::internal::FileSystemEngine;
using pdk::io::IoDevice;

namespace {

void append_organization_and_app(String &path)
{
#ifndef PDK_BOOTSTRAPPED
   const String org = CoreApplication::getOrgName();
   if (!org.isEmpty()) {
      path += Latin1Character('/') + org;
   }
   const String appName = CoreApplication::getAppName();
   if (!appName.isEmpty()) {
      path += Latin1Character('/') + appName;
   }
#else
   PDK_UNUSED(path);
#endif
}

} // anonymous namespace

String StandardPaths::writableLocation(StandardLocation type)
{
   switch (type) {
   case StandardLocation::HomeLocation:
      return Dir::getHomePath();
   case StandardLocation::TempLocation:
      return Dir::getTempPath();
   case StandardLocation::CacheLocation:
   case StandardLocation::GenericCacheLocation:
   {
      // http://standards.freedesktop.org/basedir-spec/basedir-spec-0.6.html
      String xdgCacheHome = File::decodeName(pdk::pdk_getenv("XDG_CACHE_HOME"));
      if (isTestModeEnabled()) {
         xdgCacheHome = Dir::getHomePath() + Latin1String("/.pdktest/cache");
      }
      if (xdgCacheHome.isEmpty()) {
         xdgCacheHome = Dir::getHomePath() + Latin1String("/.cache");
      }
      if (type == StandardLocation::CacheLocation) {
         append_organization_and_app(xdgCacheHome);
      }
      return xdgCacheHome;
   }
   case StandardLocation::AppDataLocation:
   case StandardLocation::AppLocalDataLocation:
   case StandardLocation::GenericDataLocation:
   {
      String xdgDataHome = File::decodeName(pdk::pdk_getenv("XDG_DATA_HOME"));
      if (isTestModeEnabled()) {
         xdgDataHome = Dir::getHomePath() + Latin1String("/.pdktest/share");
      }
      if (xdgDataHome.isEmpty()) {
         xdgDataHome = Dir::getHomePath() + Latin1String("/.local/share");
      }
      if (type == StandardLocation::AppDataLocation || type == StandardLocation::AppLocalDataLocation) {
         append_organization_and_app(xdgDataHome);
      }
      return xdgDataHome;
   }
   case StandardLocation::ConfigLocation:
   case StandardLocation::GenericConfigLocation:
   case StandardLocation::AppConfigLocation:
   {
      // http://standards.freedesktop.org/basedir-spec/latest/
      String xdgConfigHome = File::decodeName(pdk::pdk_getenv("XDG_CONFIG_HOME"));
      if (isTestModeEnabled()) {
         xdgConfigHome = Dir::getHomePath() + Latin1String("/.pdktest/config");
      }
      if (xdgConfigHome.isEmpty()) {
         xdgConfigHome = Dir::getHomePath() + Latin1String("/.config");
      }
      if (type == StandardLocation::AppConfigLocation) {
         append_organization_and_app(xdgConfigHome);
      }
      return xdgConfigHome;
   }
   case StandardLocation::RuntimeLocation:
   {
      const uint myUid = uint(geteuid());
      // http://standards.freedesktop.org/basedir-spec/latest/
      FileInfo fileInfo;
      String xdgRuntimeDir = File::decodeName(pdk::pdk_getenv("XDG_RUNTIME_DIR"));
      if (xdgRuntimeDir.isEmpty()) {
         const String userName = FileSystemEngine::resolveUserName(myUid);
         xdgRuntimeDir = Dir::getTempPath() + Latin1String("/runtime-") + userName;
         fileInfo.setFile(xdgRuntimeDir);
         if (!fileInfo.isDir()) {
            if (!Dir().mkdir(xdgRuntimeDir)) {
               warning_stream("StandardPaths: error creating runtime directory %s: %s", 
                              pdk_printable(xdgRuntimeDir), 
                              pdk_printable(pdk::pdk::error_string(errno)));
               return String();
            }
         }
         warning_stream("StandardPaths: XDG_RUNTIME_DIR not set, defaulting to '%s'", pdk_printable(xdgRuntimeDir));
      } else {
         fileInfo.setFile(xdgRuntimeDir);
         if (!fileInfo.exists()) {
            warning_stream("StandardPaths: XDG_RUNTIME_DIR points to non-existing path '%s', "
                           "please create it with 0700 permissions.", pdk_printable(xdgRuntimeDir));
            return String();
         }
         if (!fileInfo.isDir()) {
            warning_stream("StandardPaths: XDG_RUNTIME_DIR points to '%s' which is not a directory",
                           pdk_printable(xdgRuntimeDir));
            return String();
         }
      }
      // "The directory MUST be owned by the user"
      if (fileInfo.getOwnerId() != myUid) {
         warning_stream("StandardPaths: wrong ownership on runtime directory %s, %d instead of %d", pdk_printable(xdgRuntimeDir),
                        fileInfo.getOwnerId(), myUid);
         return String();
      }
      // "and he MUST be the only one having read and write access to it. Its Unix access mode MUST be 0700."
      // since the current user is the owner, set both xxxUser and xxxOwner
      const File::Permissions wantedPerms = File::Permissions(
               pdk::as_integer<File::Permission>(File::Permission::ReadUser) | 
               pdk::as_integer<File::Permission>(File::Permission::WriteUser) | 
               pdk::as_integer<File::Permission>(File::Permission::ExeUser) |
               pdk::as_integer<File::Permission>(File::Permission::ReadOwner) |
               pdk::as_integer<File::Permission>(File::Permission::WriteOwner) |
               pdk::as_integer<File::Permission>(File::Permission::ExeOwner));
      if (fileInfo.permissions() != wantedPerms) {
         File file(xdgRuntimeDir);
         if (!file.setPermissions(wantedPerms)) {
            warning_stream("StandardPaths: could not set correct permissions on runtime directory %s: %s",
                           pdk_printable(xdgRuntimeDir), pdk_printable(file.getErrorString()));
            return String();
         }
      }
      return xdgRuntimeDir;
   }
   default:
      break;
   }
   
   // http://www.freedesktop.org/wiki/Software/xdg-user-dirs
   String xdgConfigHome = File::decodeName(pdk::pdk_getenv("XDG_CONFIG_HOME"));
   if (xdgConfigHome.isEmpty()) {
      xdgConfigHome = Dir::getHomePath() + Latin1String("/.config");
   }
   File file(xdgConfigHome + Latin1String("/user-dirs.dirs"));
   if (!isTestModeEnabled() && file.open(IoDevice::OpenMode::ReadOnly)) {
      std::map<String, String> lines;
      TextStream stream(&file);
      // Only look for lines like: XDG_DESKTOP_DIR="$HOME/Desktop"
      //      QRegularExpression exp(Latin1String("^XDG_(.*)_DIR=(.*)$"));
      //      while (!stream.atEnd()) {
      //         const String &line = stream.readLine();
      //         QRegularExpressionMatch match = exp.match(line);
      //         if (match.hasMatch()) {
      //            const StringList lst = match.capturedTexts();
      //            const String key = lst.at(1);
      //            String value = lst.at(2);
      //            if (value.length() > 2
      //                && value.startsWith(QLatin1Char('\"'))
      //                && value.endsWith(QLatin1Char('\"')))
      //               value = value.mid(1, value.length() - 2);
      //            // Store the key and value: "DESKTOP", "$HOME/Desktop"
      //            lines[key] = value;
      //         }
      //      }
      
      String key;
      switch (type) {
      case StandardLocation::DesktopLocation:
         key = Latin1String("DESKTOP");
         break;
      case StandardLocation::DocumentsLocation:
         key = Latin1String("DOCUMENTS");
         break;
      case StandardLocation::PicturesLocation:
         key = Latin1String("PICTURES");
         break;
      case StandardLocation::MusicLocation:
         key = Latin1String("MUSIC");
         break;
      case StandardLocation::MoviesLocation:
         key = Latin1String("VIDEOS");
         break;
      case StandardLocation::DownloadLocation:
         key = Latin1String("DOWNLOAD");
         break;
      default:
         break;
      }
      if (!key.isEmpty()) {
         String value = lines.at(key);
         if (!value.isEmpty()) {
            // value can start with $HOME
            if (value.startsWith(Latin1String("$HOME"))) {
               value = Dir::getHomePath() + value.substringRef(5);
            }     
            if (value.length() > 1 && value.endsWith(Latin1Character('/'))) {
               value.chop(1);
            }
            return value;
         }
      }
   }
   
   String path;
   switch (type) {
   case StandardLocation::DesktopLocation:
      path = Dir::getHomePath() + Latin1String("/Desktop");
      break;
   case StandardLocation::DocumentsLocation:
      path = Dir::getHomePath() + Latin1String("/Documents");
      break;
   case StandardLocation::PicturesLocation:
      path = Dir::getHomePath() + Latin1String("/Pictures");
      break;
      
   case StandardLocation::FontsLocation:
      path = writableLocation(StandardLocation::GenericDataLocation) + Latin1String("/fonts");
      break;
      
   case StandardLocation::MusicLocation:
      path = Dir::getHomePath() + Latin1String("/Music");
      break;
      
   case StandardLocation::MoviesLocation:
      path = Dir::getHomePath() + Latin1String("/Videos");
      break;
   case StandardLocation::DownloadLocation:
      path = Dir::getHomePath() + Latin1String("/Downloads");
      break;
   case StandardLocation::ApplicationsLocation:
      path = writableLocation(StandardLocation::GenericDataLocation) + Latin1String("/applications");
      break;
      
   default:
      break;
   }
   
   return path;
}

namespace {

StringList xdg_data_dirs()
{
   StringList dirs;
   // http://standards.freedesktop.org/basedir-spec/latest/
   String xdgDataDirsEnv = File::decodeName(pdk::pdk_getenv("XDG_DATA_DIRS"));
   if (xdgDataDirsEnv.isEmpty()) {
      dirs.push_back(String::fromLatin1("/usr/local/share"));
      dirs.push_back(String::fromLatin1("/usr/share"));
   } else {
      dirs = xdgDataDirsEnv.split(Latin1Character(':'), String::SplitBehavior::SkipEmptyParts);
      // Normalize paths, skip relative paths
      StringList::iterator iter = dirs.begin();
      StringList::iterator end = dirs.end();
      while (iter != end) {
         const String &dir = *iter;
         if (!dir.startsWith(Latin1Character('/'))) {
            dirs.erase(iter);
         } else {
            *iter = Dir::cleanPath(dir);
         } 
         ++iter;
      }
      // Remove duplicates from the list, there's no use for duplicated
      // paths in XDG_DATA_DIRS - if it's not found in the given
      // directory the first time, it won't be there the second time.
      // Plus duplicate paths causes problems for example for mimetypes,
      // where duplicate paths here lead to duplicated mime types returned
      // for a file, eg "text/plain,text/plain" instead of "text/plain"
      dirs.removeDuplicates();
   }
   return dirs;
}

StringList xdg_config_dirs()
{
   StringList dirs;
   // http://standards.freedesktop.org/basedir-spec/latest/
   const String xdgConfigDirs = File::decodeName(pdk::pdk_getenv("XDG_CONFIG_DIRS"));
   if (xdgConfigDirs.isEmpty()) {
      dirs.push_back(String::fromLatin1("/etc/xdg"));
   } else {
      dirs = xdgConfigDirs.split(Latin1Character(':'));
   } 
   return dirs;
}

} // anonymous namespace

StringList StandardPaths::standardLocations(StandardLocation type)
{
   StringList dirs;
   switch (type) {
   case StandardLocation::ConfigLocation:
   case StandardLocation::GenericConfigLocation:
      dirs = xdg_config_dirs();
      break;
   case StandardLocation::AppConfigLocation:
      dirs = xdg_config_dirs();
      for (size_t i = 0; i < dirs.size(); ++i) {
         append_organization_and_app(dirs[i]);
      }
      break;
   case StandardLocation::GenericDataLocation:
      dirs = xdg_data_dirs();
      break;
   case StandardLocation::ApplicationsLocation:
      dirs = xdg_data_dirs();
      for (size_t i = 0; i < dirs.size(); ++i)
         dirs[i].append(Latin1String("/applications"));
      break;
   case StandardLocation::AppDataLocation:
   case StandardLocation::AppLocalDataLocation:
      dirs = xdg_data_dirs();
      for (size_t i = 0; i < dirs.size(); ++i) {
         append_organization_and_app(dirs[i]);
      }
      break;
   case StandardLocation::FontsLocation:
      dirs += Dir::getHomePath() + Latin1String("/.fonts");
      break;
   default:
      break;
   }
   const String localDir = writableLocation(type);
   dirs.insert(dirs.begin(), localDir);
   return dirs;
}

} // fs
} // io
} // pdk

#endif // PDK_NO_STANDARDPATHS

