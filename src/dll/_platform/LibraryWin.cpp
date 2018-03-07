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

#include "pdk/global/PlatformDefs.h"
#include "pdk/global/Windows.h"
#include "pdk/dll/internal/LibraryPrivate.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"

namespace pdk {
namespace dll {
namespace internal {

using pdk::ds::StringList;
using pdk::lang::Latin1String;
using pdk::lang::String;
using pdk::lang::Latin1Character;
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::io::fs::internal::FileSystemEntry;

StringList LibraryPrivate::suffixesSys(const String& fullVersion)
{
   PDK_UNUSED(fullVersion);
   return StringList(StringLiteral(".dll"));
}

StringList LibraryPrivate::prefixesSys()
{
   return StringList();
}

bool LibraryPrivate::loadSys()
{
   // We make the following attempts at locating the library:
   //
   // Windows
   // if (absolute)
   //     fileName
   //     fileName + ".dll"
   // else
   //     fileName + ".dll"
   //     fileName
   //
   // NB If it's a plugin we do not ever try the ".dll" extension
   StringList attempts;
   if (m_pluginState != IsAPlugin) {
      attempts.push_back(m_fileName + Latin1String(".dll"));
   }
   // If the fileName is an absolute path we try that first, otherwise we
   // use the system-specific suffix first
   FileSystemEntry fsEntry(m_fileName);
   if (fsEntry.isAbsolute()) {
      attempts.push_front(m_fileName);
   } else {
      attempts.push_back(m_fileName);
   }
   for (const String &attempt : std::as_const(attempts)) {
      m_handle = LoadLibrary((wchar_t*)Dir::toNativeSeparators(attempt).utf16());
      // If we have a handle or the last error is something other than "unable
      // to find the module", then bail out
      if (m_handle || ::GetLastError() != ERROR_MOD_NOT_FOUND) {
         break;
      }
   }
   SetErrorMode(oldmode);
   if (!m_handle) {
      m_errorString = Library::tr("Cannot load library %1: %2").arg(
               Dir::toNativeSeparators(m_fileName), pdk::error_string());
   } else {
      // Query the actual name of the library that was loaded
      m_errorString.clear();
      wchar_t buffer[MAX_PATH];
      ::GetModuleFileName(m_handle, buffer, MAX_PATH);
      
      String moduleFileName = String::fromWCharArray(buffer);
      moduleFileName.remove(0, 1 + moduleFileName.lastIndexOf(Latin1Character('\\')));
      const Dir dir(fsEntry.path());
      if (dir.getPath() == Latin1String(".")) {
         m_qualifiedFileName = moduleFileName;
      } else {
         m_qualifiedFileName = dir.filePath(moduleFileName);
      }
      if (getLoadHints() & Library::LoadHint::PreventUnloadHint) {
         // prevent the unloading of this component
         HMODULE hmod;
         bool ok = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_PIN |
                                     GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                                     reinterpret_cast<const wchar_t *>(m_handle),
                                     &hmod);
         PDK_ASSERT(!ok || hmod == m_handle);
         PDK_UNUSED(ok);
      }
   }
   return (m_handle != nullptr);
}

bool LibraryPrivate::unloadSys()
{
   if (!FreeLibrary(m_handle)) {
      m_errorString = Library::tr("Cannot unload library %1: %2").arg(
               Dir::toNativeSeparators(fileName),  pdk::error_string());
      return false;
   }
   m_errorString.clear();
   return true;
}

FuncPointer LibraryPrivate::resolveSys(const char* symbol)
{
   FARPROC address = GetProcAddress(m_handle, symbol);
   if (!address) {
      m_errorString = Library::tr("Cannot resolve symbol \"%1\" in %2: %3").arg(
               String::fromLatin1(symbol), Dir::toNativeSeparators(m_fileName), pdk::error_string());
   } else {
      m_errorString.clear();
   }
   return FuncPointer(address);
}

} // internal
} // dll
} // pdk
