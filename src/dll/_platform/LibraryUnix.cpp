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
#include "pdk/base/io/fs/File.h"
#include "pdk/dll/internal/LibraryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"

#ifdef PDK_OS_MAC
#include "pdk/kernel/internal/CoreMacPrivate.h"
#endif

#if !defined(PDK_HPUX_LD)
#include <dlfcn.h>
#endif

namespace pdk {
namespace dll {
namespace internal {

using pdk::lang::Latin1Character;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::io::fs::internal::FileSystemEntry;
using pdk::io::fs::File;
using pdk::ds::ByteArray;
using pdk::kernel::CFType;

namespace {

String pdk_dlerror()
{
#if !defined(PDK_HPUX_LD)
   const char *err = dlerror();
#else
   const char *err = strerror(errno);
#endif
   return err ? Latin1Character('(') + String::fromLocal8Bit(err) + Latin1Character(')'): String();
}

} // anonymous namespace

StringList LibraryPrivate::suffixesSys(const String &fullVersion)
{
   StringList suffixes;
#if defined(PDK_OS_HPUX)
   // according to
   // http://docs.hp.com/en/B2355-90968/linkerdifferencesiapa.htm
   
   // In PA-RISC (PA-32 and PA-64) shared libraries are suffixed
   // with .sl. In IPF (32-bit and 64-bit), the shared libraries
   // are suffixed with .so. For compatibility, the IPF linker
   // also supports the .sl suffix.
   
   // But since we don't know if we are built on HPUX or HPUXi,
   // we support both .sl (and .<version>) and .so suffixes but
   // .so is preferred.
# if defined(__ia64)
   if (!fullVersion.isEmpty()) {
      suffixes.push_back(String::fromLatin1(".so.%1").arg(fullVersion));
   } else {
      suffixes.push_back(Latin1String(".so"));
   }
# endif
   if (!fullVersion.isEmpty()) {
      suffixes.push_back(String::fromLatin1(".sl.%1").arg(fullVersion));
      suffixes.push_back(String::fromLatin1(".%1").arg(fullVersion));
   } else {
      suffixes.push_back(Latin1String(".sl"));
   }
#elif defined(PDK_OS_AIX)
   suffixes.push_back(".a");
   
#else
   if (!fullVersion.isEmpty()) {
      suffixes.push_back(String::fromLatin1(".so.%1").arg(fullVersion));
   } else {
      suffixes.push_back(Latin1String(".so"));
   }
#endif
# ifdef PDK_OS_MAC
   if (!fullVersion.isEmpty()) {
      suffixes.push_back(String::fromLatin1(".%1.bundle").arg(fullVersion));
      suffixes.push_back(String::fromLatin1(".%1.dylib").arg(fullVersion));
   } else {
      suffixes.push_back(Latin1String(".bundle"));
      suffixes.push_back(Latin1String(".dylib"));
   }
#endif
   return suffixes;
}

StringList LibraryPrivate::prefixesSys()
{
   StringList ret;
   ret.push_back(Latin1String("lib"));
   return ret;
}

bool LibraryPrivate::loadSys()
{
   String attempt;
   FileSystemEntry fsEntry(m_fileName);
   String path = fsEntry.getPath();
   String name = fsEntry.getFileName();
   if (path == Latin1String(".") && !m_fileName.startsWith(path)) {
      path.clear();
   } else {
      path += Latin1Character('/');
   }
   StringList suffixes;
   StringList prefixes;
   if (m_pluginState != IsAPlugin) {
      prefixes = prefixesSys();
      suffixes = suffixesSys(m_fullVersion);
   }
   int dlFlags = 0;
#if defined(PDK_HPUX_LD)
   dlFlags = DYNAMIC_PATH | BIND_NONFATAL;
   if (loadHints & Library::LoadHint::ResolveAllSymbolsHint) {
      dlFlags |= BIND_IMMEDIATE;
   } else {
      dlFlags |= BIND_DEFERRED;
   }
#else
   Library::LoadHints loadHints = getLoadHints();
   if (loadHints & Library::LoadHint::ResolveAllSymbolsHint) {
      dlFlags |= RTLD_NOW;
   } else {
      dlFlags |= RTLD_LAZY;
   }
   if (loadHints & Library::LoadHint::ExportExternalSymbolsHint) {
      dlFlags |= RTLD_GLOBAL;
   }
#if !defined(PDK_OS_CYGWIN)
   else {
      dlFlags |= RTLD_LOCAL;
   }
#endif
#if defined(RTLD_DEEPBIND)
   if (loadHints & Library::LoadHint::DeepBindHint)
      dlFlags |= RTLD_DEEPBIND;
#endif
   
   // Provide access to RTLD_NODELETE flag on Unix
   // From GNU documentation on RTLD_NODELETE:
   // Do not unload the library during dlclose(). Consequently, the
   // library's specific static variables are not reinitialized if the
   // library is reloaded with dlopen() at a later time.
#ifdef RTLD_NODELETE
   if (loadHints & Library::LoadHint::PreventUnloadHint) {
      dlFlags |= RTLD_NODELETE;
   }
#endif
#if defined(PDK_OS_AIX)   // Not sure if any other platform actually support this thing.
   if (loadHints & Library::LoadHint::LoadArchiveMemberHint) {
      dlFlags |= RTLD_MEMBER;
   }
#endif
#endif // PDK_HPUX_LD
   
   // If the filename is an absolute path then we want to try that first as it is most likely
   // what the callee wants. If we have been given a non-absolute path then lets try the
   // native library name first to avoid unnecessary calls to dlopen().
   if (fsEntry.isAbsolute()) {
      suffixes.push_front(String());
      prefixes.push_front(String());
   } else {
      suffixes.push_back(String());
      prefixes.push_back(String());
   }
   bool retry = true;
   for(size_t prefix = 0; retry && !m_handle && prefix < prefixes.size(); prefix++) {
      for(size_t suffix = 0; retry && !m_handle && suffix < suffixes.size(); suffix++) {
         if (!prefixes.at(prefix).isEmpty() && name.startsWith(prefixes.at(prefix)))
            continue;
         if (!suffixes.at(suffix).isEmpty() && name.endsWith(suffixes.at(suffix)))
            continue;
         if (loadHints & Library::LoadHint::LoadArchiveMemberHint) {
            attempt = name;
            int lparen = attempt.indexOf(Latin1Character('('));
            if (lparen == -1) {
               lparen = attempt.count();
            }
            attempt = path + prefixes.at(prefix) + attempt.insert(lparen, suffixes.at(suffix));
         } else {
            attempt = path + prefixes.at(prefix) + name + suffixes.at(suffix);
         }
#if defined(PDK_HPUX_LD)
         m_handle = (void*)shl_load(File::encodeName(attempt), dlFlags, 0);
#else
         m_handle = dlopen(File::encodeName(attempt), dlFlags);
#endif
         if (!m_handle && m_fileName.startsWith(Latin1Character('/')) && File::exists(attempt)) {
            // We only want to continue if dlopen failed due to that the shared library did not exist.
            // However, we are only able to apply this check for absolute filenames (since they are
            // not influenced by the content of LD_LIBRARY_PATH, /etc/ld.so.cache, DT_RPATH etc...)
            // This is all because dlerror is flawed and cannot tell us the reason why it failed.
            retry = false;
         }
      }
   }
   
#ifdef PDK_OS_MAC
   if (!m_handle) {
      ByteArray utf8Bundle = m_fileName.toUtf8();
      CFType<CFURLRef> bundleUrl = CFURLCreateFromFileSystemRepresentation(NULL, reinterpret_cast<const UInt8*>(utf8Bundle.getRawData()), utf8Bundle.length(), true);
      CFType<CFBundleRef> bundle = CFBundleCreate(NULL, bundleUrl);
      if(bundle) {
         CFType<CFURLRef> url = CFBundleCopyExecutableURL(bundle);
         char executableFile[FILENAME_MAX];
         CFURLGetFileSystemRepresentation(url, true, reinterpret_cast<UInt8*>(executableFile), FILENAME_MAX);
         attempt = String::fromUtf8(executableFile);
         m_handle = dlopen(File::encodeName(attempt), dlFlags);
      }
   }
#endif
   if (!m_handle) {
      m_errorString = Library::tr("Cannot load library %1: %2").arg(m_fileName, pdk_dlerror());
   }
   if (m_handle) {
      m_qualifiedFileName = attempt;
      m_errorString.clear();
   }
   return (m_handle != nullptr);
}

bool LibraryPrivate::unloadSys()
{
#if defined(PDK_HPUX_LD)
   if (shl_unload((shl_t)m_handle)) {
#else
   if (dlclose(m_handle)) {
#endif
#if defined (PDK_OS_QNX)                // Workaround until fixed in QNX; fixes crash in
      char *error = dlerror();      // Declarative auto test "qmlenginecleanup" for instance
      if (!pdk::strcmp(error, "Shared objects still referenced")) { // On QNX that's only "informative"
         return true;
      }
      m_errorString = Library::tr("Cannot unload library %1: %2").arg(fileName,
                                                                      Latin1String(error));
#else
      m_errorString = Library::tr("Cannot unload library %1: %2").arg(m_fileName, pdk_dlerror());
#endif
      return false;
   }
   m_errorString.clear();
   return true;
}

#if defined(PDK_OS_LINUX)
PDK_CORE_EXPORT FuncPointer pdk_linux_find_symbol_sys(const char *symbol)
{
   return FuncPointer(dlsym(RTLD_DEFAULT, symbol));
}
#endif

#ifdef PDK_OS_MAC
PDK_CORE_EXPORT FuncPointer pdk_mac_resolve_sys(void *handle, const char *symbol)
{
   return FuncPointer(dlsym(handle, symbol));
}
#endif

FuncPointer LibraryPrivate::resolveSys(const char* symbol)
{
#if defined(PDK_HPUX_LD)
   FuncPointer address = nullptr;
   if (shl_findsym((shl_t*)&m_handle, symbol, TYPE_UNDEFINED, &address) < 0) {
      address = nullptr;
   }
#else
   FuncPointer address = FuncPointer(dlsym(m_handle, symbol));
#endif
   if (!address) {
      m_errorString = Library::tr("Cannot resolve symbol \"%1\" in %2: %3").arg(
               String::fromLatin1(symbol), m_fileName, pdk_dlerror());
   } else {
      m_errorString.clear();
   }
   return address;
}

} // internal
} // dll
} // pdk
