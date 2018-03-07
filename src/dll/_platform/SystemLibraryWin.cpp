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
// Created by softboy on 2018/03/07.

#include "pdk/dll/internal/SystemLibraryPrivate.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/fs/FileInfo.h"

namespace pdk {

using pdk::lang::String;

// forward declare function with namespace
namespace kernel {
extern String retrieve_app_filename();
} // kernel

namespace dll {
namespace internal {

using pdk::ds::VarLengthArray;
using pdk::io::fs::FileInfo;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::ds::StringList;

static String system_directory()
{
   VarLengthArray<wchar_t, MAX_PATH> fullPath;
   UINT retLen = ::GetSystemDirectory(fullPath.getRawData(), MAX_PATH);
   if (retLen > MAX_PATH) {
      fullPath.resize(retLen);
      retLen = ::GetSystemDirectory(fullPath.getRawData(), retLen);
   }
   // in some rare cases retLen might be 0
   return String::fromWCharArray(fullPath.getConstRawData(), int(retLen));
}

HINSTANCE SystemLibrary::load(const wchar_t *libraryName, bool onlySystemDirectory /* = true */)
{
   StringList searchOrder;
   if (!onlySystemDirectory) {
      searchOrder << FileInfo(pdk::kernel::retrieve_app_filename()).getPath();
   }
      
   searchOrder.push_back(system_directory());
   if (!onlySystemDirectory) {
      const String PATH(Latin1String(pdk::pdk_getenv("PATH").getConstRawData()));
      searchOrder.push_back(PATH.split(Latin1Character(';'), String::SplitBehavior::SkipEmptyParts));
   }
   String fileName = String::fromWCharArray(libraryName);
   fileName.append(Latin1String(".dll"));
   
   // Start looking in the order specified
   for (int i = 0; i < searchOrder.count(); ++i) {
      String fullPathAttempt = searchOrder.at(i);
      if (!fullPathAttempt.endsWith(Latin1Character('\\'))) {
         fullPathAttempt.append(Latin1Character('\\'));
      }
      fullPathAttempt.append(fileName);
      HINSTANCE inst = ::LoadLibrary((const wchar_t *)fullPathAttempt.utf16());
      if (inst != 0)
         return inst;
   }
   return 0;
}

} // internal
} // dll
} // pdk
