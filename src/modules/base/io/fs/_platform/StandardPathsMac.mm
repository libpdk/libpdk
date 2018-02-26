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

#ifndef PDK_NO_STANDARDPATHS

#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/kernel/internal/CoreMacPrivate.h"
#include "pdk/kernel/CoreApplication.h"
#include <Foundation/Foundation.h>

namespace pdk {
namespace io {
namespace fs {

using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::kernel::CoreApplication;
using pdk::kernel::CFString;
using pdk::kernel::CFType;
using pdk::io::fs::FileInfo;

namespace {

String path_for_directory(NSSearchPathDirectory directory,
                          NSSearchPathDomainMask mask)
{
   return String::fromNSString(
            [NSSearchPathForDirectoriesInDomains(directory, mask, YES) lastObject]);
}

NSSearchPathDirectory search_path_directory(StandardPaths::StandardLocation type)
{
   switch (type) {
   case StandardPaths::StandardLocation::DesktopLocation:
      return NSDesktopDirectory;
   case StandardPaths::StandardLocation::DocumentsLocation:
      return NSDocumentDirectory;
   case StandardPaths::StandardLocation::ApplicationsLocation:
      return NSApplicationDirectory;
   case StandardPaths::StandardLocation::MusicLocation:
      return NSMusicDirectory;
   case StandardPaths::StandardLocation::MoviesLocation:
      return NSMoviesDirectory;
   case StandardPaths::StandardLocation::PicturesLocation:
      return NSPicturesDirectory;
   case StandardPaths::StandardLocation::GenericDataLocation:
   case StandardPaths::StandardLocation::RuntimeLocation:
   case StandardPaths::StandardLocation::AppDataLocation:
   case StandardPaths::StandardLocation::AppLocalDataLocation:
      return NSApplicationSupportDirectory;
   case StandardPaths::StandardLocation::GenericCacheLocation:
   case StandardPaths::StandardLocation::CacheLocation:
      return NSCachesDirectory;
   case StandardPaths::StandardLocation::DownloadLocation:
      return NSDownloadsDirectory;
   default:
      return (NSSearchPathDirectory)0;
   }
}

void append_organization_and_app(String &path)
{
   const String org = CoreApplication::getOrgName();
   if (!org.isEmpty()) {
      path += Latin1Character('/') + org;
   }
   const String appName = CoreApplication::getAppName();
   if (!appName.isEmpty()) {
      path += Latin1Character('/') + appName;
   }
   
}

String base_writable_location(StandardPaths::StandardLocation type,
                              NSSearchPathDomainMask mask = NSUserDomainMask,
                              bool appendOrgAndApp = false)
{
   String path;
   const NSSearchPathDirectory dir = search_path_directory(type);
   switch (type) {
   case StandardPaths::StandardLocation::HomeLocation:
      path = Dir::getHomePath();
      break;
   case StandardPaths::StandardLocation::TempLocation:
      path = Dir::getTempPath();
      break;
#if defined(PDK_PLATFORM_UIKIT)
      // These locations point to non-existing write-protected paths. Use sensible fallbacks.
   case StandardPaths::StandardLocation::MusicLocation:
      path = path_for_directory(NSDocumentDirectory, mask) + Latin1String("/Music");
      break;
   case StandardPaths::StandardLocation::MoviesLocation:
      path = path_for_directory(NSDocumentDirectory, mask) + Latin1String("/Movies");
      break;
   case StandardPaths::StandardLocation::PicturesLocation:
      path = path_for_directory(NSDocumentDirectory, mask) + Latin1String("/Pictures");
      break;
   case StandardPaths::StandardLocation::DownloadLocation:
      path = path_for_directory(NSDocumentDirectory, mask) + Latin1String("/Downloads");
      break;
   case StandardPaths::StandardLocation::DesktopLocation:
      path = path_for_directory(NSDocumentDirectory, mask) + Latin1String("/Desktop");
      break;
   case StandardPaths::StandardLocation::ApplicationsLocation:
      break;
#endif
   case StandardPaths::StandardLocation::FontsLocation:
      path = path_for_directory(NSLibraryDirectory, mask) + Latin1String("/Fonts");
      break;
   case StandardPaths::StandardLocation::ConfigLocation:
   case StandardPaths::StandardLocation::GenericConfigLocation:
   case StandardPaths::StandardLocation::AppConfigLocation:
      path = path_for_directory(NSLibraryDirectory, mask) + Latin1String("/Preferences");
      break;
   default:
      path = path_for_directory(dir, mask);
      break;
   }
   if (appendOrgAndApp) {
      switch (type) {
      case StandardPaths::StandardLocation::AppDataLocation:
      case StandardPaths::StandardLocation::AppLocalDataLocation:
      case StandardPaths::StandardLocation::AppConfigLocation:
      case StandardPaths::StandardLocation::CacheLocation:
         append_organization_and_app(path);
         break;
      default:
         break;
      }
   }
   return path;
}

} // anonymous namespace

String StandardPaths::writableLocation(StandardLocation type)
{
   String location = base_writable_location(type, NSUserDomainMask, true);
   if (isTestModeEnabled()) {
      location = location.replace(Dir::getHomePath(), Dir::getHomePath() + Latin1String("/.qttest"));
   }
   return location;
}

StringList StandardPaths::standardLocations(StandardLocation type)
{
   StringList dirs;
   
#if defined(PDK_PLATFORM_UIKIT)
   if (type == PicturesLocation) {
      dirs.push_back(writableLocation(StandardLocation::PicturesLocation));
      dirs.push_back(Latin1String("assets-library://"));
   }
#endif
   
   if (type == StandardLocation::GenericDataLocation || 
       type == StandardLocation::FontsLocation || 
       type == StandardLocation::ApplicationsLocation ||
       type == StandardLocation::AppDataLocation ||
       type == StandardLocation::AppLocalDataLocation ||
       type == StandardLocation::GenericCacheLocation || 
       type == StandardLocation::CacheLocation) {
      std::list<NSSearchPathDomainMask> masks;
      masks.push_back(NSLocalDomainMask);
      if (type == StandardLocation::FontsLocation || type == StandardLocation::GenericCacheLocation) {
         masks.push_back(NSSystemDomainMask);
      }
      for (std::list<NSSearchPathDomainMask>::const_iterator iter = masks.begin();
           iter != masks.end(); ++iter) {
         const String path = base_writable_location(type, *iter, true);
         if (!path.isEmpty() && !dirs.contains(path)) {
            dirs.push_back(path);
         }
      }
   }
   
   if (type == StandardLocation::AppDataLocation || type == StandardLocation::AppLocalDataLocation) {
      CFBundleRef mainBundle = CFBundleGetMainBundle();
      if (mainBundle) {
         CFURLRef bundleUrl = CFBundleCopyBundleURL(mainBundle);
         CFStringRef cfBundlePath = CFURLCopyFileSystemPath(bundleUrl, kCFURLPOSIXPathStyle);
         String bundlePath = String::fromCFString(cfBundlePath);
         CFRelease(cfBundlePath);
         CFRelease(bundleUrl);
         
         CFURLRef resourcesUrl = CFBundleCopyResourcesDirectoryURL(mainBundle);
         CFStringRef cfResourcesPath = CFURLCopyFileSystemPath(resourcesUrl,
                                                               kCFURLPOSIXPathStyle);
         String resourcesPath = String::fromCFString(cfResourcesPath);
         CFRelease(cfResourcesPath);
         CFRelease(resourcesUrl);
         
         // Handle bundled vs unbundled executables. CFBundleGetMainBundle() returns
         // a valid bundle in both cases. CFBundleCopyResourcesDirectoryURL() returns
         // an absolute path for unbundled executables.
         if (resourcesPath.startsWith(Latin1Character('/'))) {
            dirs.push_back(resourcesPath);
         } else {
            dirs.push_back(bundlePath + resourcesPath);
         }
      }
   }
   const String localDir = writableLocation(type);
   if (!localDir.isEmpty()) {
      dirs.insert(dirs.begin(), localDir);
   }
   return dirs;
}

String StandardPaths::displayName(StandardLocation type)
{
   // Use "Home" instead of the user's Unix username
   if (StandardLocation::HomeLocation == type) {
      return CoreApplication::translate("StandardPaths", "Home");
   }
   // The temporary directory returned by the old Carbon APIs is ~/Library/Caches/TemporaryItems,
   // the display name of which ("TemporaryItems") isn't translated by the system. The standard
   // temporary directory has no reasonable display name either, so use something more sensible.
   if (StandardLocation::TempLocation == type) {
      return CoreApplication::translate("StandardPaths", "Temporary Items");
   }
   // standardLocations() may return an empty list on some platforms
   if (StandardLocation::ApplicationsLocation == type) {
      return CoreApplication::translate("StandardPaths", "Applications");
   }
   const CFString fsPath(*standardLocations(type).cbegin());
   if (CFType<CFURLRef> url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                            fsPath, kCFURLPOSIXPathStyle, true)) {
      CFString name;
      CFURLCopyResourcePropertyForKey(url, kCFURLLocalizedNameKey, &name, NULL);
      if (name && CFStringGetLength(name))
         return String::fromCFString(name);
   }
   
   return FileInfo(base_writable_location(type)).getFileName();
}

} // fs
} // io
} // pdk


#endif // PDK_NO_STANDARDPATHS
