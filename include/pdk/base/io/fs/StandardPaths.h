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

#ifndef PDK_M_BASE_IO_FS_STANDARD_PATHS_H
#define PDK_M_BASE_IO_FS_STANDARD_PATHS_H

#include "pdk/base/ds/StringList.h"

namespace pdk {
namespace io {
namespace fs {

#ifndef PDK_NO_STANDARDPATHS

using pdk::lang::String;
using pdk::ds::StringList;

class PDK_CORE_EXPORT StandardPaths
{
public:
   // Do not re-order, must match DesktopServices
   enum class StandardLocation {
      DesktopLocation,
      DocumentsLocation,
      FontsLocation,
      ApplicationsLocation,
      MusicLocation,
      MoviesLocation,
      PicturesLocation,
      TempLocation,
      HomeLocation,
      DataLocation,
      CacheLocation,
      GenericDataLocation,
      RuntimeLocation,
      ConfigLocation,
      DownloadLocation,
      GenericCacheLocation,
      GenericConfigLocation,
      AppDataLocation,
      AppConfigLocation,
      AppLocalDataLocation = DataLocation
   };
   
   static String writableLocation(StandardLocation type);
   static StringList standardLocations(StandardLocation type);
   
   enum class LocateOption
   {
      LocateFile = 0x0,
      LocateDirectory = 0x1
   };
   PDK_DECLARE_FLAGS(LocateOptions, LocateOption);
   
   static String locate(StandardLocation type, const String &fileName, LocateOptions options = LocateOption::LocateFile);
   static StringList locateAll(StandardLocation type, const String &fileName, LocateOptions options = LocateOption::LocateFile);
   static String displayName(StandardLocation type);
   static String findExecutable(const String &executableName, const StringList &paths = StringList());
   
   static void setTestModeEnabled(bool testMode);
   static bool isTestModeEnabled();
   
private:
   // prevent construction
   StandardPaths();
   ~StandardPaths();
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(StandardPaths::LocateOptions)

#endif // PDK_NO_STANDARDPATHS

} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_STANDARD_PATHS_H
