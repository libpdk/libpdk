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

#ifndef PDK_GLOBAL_LIBRARY_INFO_H
#define PDK_GLOBAL_LIBRARY_INFO_H

#include "pdk/base/lang/String.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/utils/VersionNumber.h"

namespace pdk {

// forward declare class with namespace
namespace ds {
class StringList;
} // ds

using pdk::utils::VersionNumber;
using pdk::ds::StringList;

class PDK_CORE_EXPORT LibraryInfo
{
public:
   static const char * build() noexcept;
   static bool isDebugBuild();
   
   static VersionNumber getVersion() noexcept PDK_DECL_CONST_FUNCTION;
   
   enum class LibraryLocation
   {
      PrefixPath = 0,
      DocumentationPath,
      HeadersPath,
      LibrariesPath,
      LibraryExecutablesPath,
      BinariesPath,
      PluginsPath,
      ImportsPath,
      ArchDataPath,
      DataPath,
      TranslationsPath,
      ExamplesPath,
      TestsPath,
      // Insert new values above this line
      // Please read the comments in LibraryInfo.cpp before adding
      // These are not subject to binary compatibility constraints
      SysrootPath,
      SysrootifyPrefixPath,
      HostBinariesPath,
      HostLibrariesPath,
      HostDataPath,
      TargetSpecPath,
      HostSpecPath,
      HostPrefixPath,
      LastHostPath = HostPrefixPath,
      SettingsPath = 100
   };
   static String getPath(LibraryLocation);
   enum class PathGroup
   {
      FinalPaths,
      EffectivePaths,
      EffectiveSourcePaths,
      DevicePaths
   };
   
   static String getRawLocation(LibraryLocation, PathGroup);
   static void reload();
   static StringList platformPluginArguments(const String &platformName);
   
private:
   LibraryInfo();
};

} // pdk 

#endif // PDK_GLOBAL_LIBRARY_INFO_H
