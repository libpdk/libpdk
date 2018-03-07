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
#include "pdk/dll/Library.h"
#include "pdk/dll/internal/FactoryLoaderPrivate.h"
#include "pdk/dll/internal/LibraryPrivate.h"
#include "pdk/dll/internal/ElfParserPrivate.h"
#include "pdk/dll/internal/MachParserPrivate.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/kernel/internal/SystemErrorPrivate.h"

#ifdef PDK_OS_MAC
#include "pdk/kernel/internal/CoreMacPrivate.h"
#endif

#ifndef NO_ERRNO_H
#include <errno.h>
#endif // NO_ERROR_H

#include "pdk/base/io/Debug.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/global/Endian.h"
#include "pdk/base/utils/json/JsonDocument.h"
#include "pdk/base/utils/json/JsonValue.h"

#include <vector>
#include <mutex>
#include <map>

namespace pdk {
namespace dll {

#ifdef PDK_NO_DEBUG
#  define LIBRARY_AS_DEBUG false
#else
#  define LIBRARY_AS_DEBUG true
#endif

#if defined(PDK_OS_UNIX)
// We don't use separate debug and release libs on UNIX, so we want
// to allow loading plugins, regardless of how they were built.
#  define PDK_NO_DEBUG_PLUGIN_CHECK
#endif

using internal::LibraryPrivate;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::io::fs::Dir;
using pdk::io::IoDevice;
using pdk::ds::ByteArray;
using pdk::ds::StringList;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::kernel::internal::SystemError;
using pdk::utils::json::JsonDocument;
using pdk::utils::json::JsonValue;

using PdkPluginQueryVerificationDataFunc = const char *(*)();

namespace internal {

namespace {
long pdk_find_pattern(const char *s, ulong slen,
                      const char *pattern, ulong plen)
{
   /*
      we search from the end of the file because on the supported
      systems, the read-only data/text segments are placed at the end
      of the file.  HOWEVER, when building with debugging enabled, all
      the debug symbols are placed AFTER the data/text segments.
      
      what does this mean?  when building in release mode, the search
      is fast because the data we are looking for is at the end of the
      file... when building in debug mode, the search is slower
      because we have to skip over all the debugging symbols first
    */
   if (! s || ! pattern || plen > slen) {
      return -1;
   }
   ulong i, hs = 0, hp = 0, delta = slen - plen;
   for (i = 0; i < plen; ++i) {
      hs += s[delta + i];
      hp += pattern[i];
   }
   i = delta;
   for (;;) {
      if (hs == hp && pdk::strncmp(s + i, pattern, plen) == 0) {
         return i;
      }
      if (i == 0) {
         break;
      }
      --i;
      hs -= s[i + plen];
      hs += s[i];
   }
   return -1;
}

/*
  This opens the specified library, mmaps it into memory, and searches
  for the PDK_PLUGIN_VERIFICATION_DATA.  The advantage of this approach is that
  we can get the verification data without have to actually load the library.
  This lets us detect mismatches more safely.
  
  Returns \c false if version information is not present, or if the
                information could not be read.
  Returns  true if version information is present and successfully read.
*/
bool find_pattern_unloaded(const String &library, LibraryPrivate *lib)
{
   File file(library);
   if (!file.open(IoDevice::OpenMode::ReadOnly)) {
      if (lib)
         lib->m_errorString = file.getErrorString();
      if (pdk_debug_component()) {
         warning_stream("%s: %s", File::encodeName(library).getConstRawData(),
                        pdk_printable(SystemError::getStdString()));
      }
      return false;
   }
   ByteArray data;
   ulong fdlen = file.getSize();
   const char *filedata = reinterpret_cast<char *>(file.map(0, fdlen));
   if (filedata == 0) {
      if (uchar *mapdata = file.map(0, 1)) {
         file.unmap(mapdata);
         // Mapping is supported, but failed for the entire file, likely due to OOM.
         // Return false, as readAll() would cause a bad_alloc and terminate the process.
         if (lib) {
            lib->m_errorString = Library::tr("Out of memory while loading plugin '%1'.").arg(library);
         }
         if (pdk_debug_component()) {
            warning_stream("%s: %s", File::encodeName(library).getConstRawData(),
                           pdk_printable(SystemError::getStdString(ENOMEM)));
         }
         return false;
      } else {
         // Try reading the data into memory instead.
         data = file.readAll();
         filedata = data.getConstRawData();
         fdlen = data.size();
      }
   }
   /*
       ELF and Mach-O binaries with GCC have .pdkplugin sections.
    */
   bool hasMetaData = false;
   long pos = 0;
   char pattern[] = "pTMETADATA  ";
   pattern[0] = 'P'; // Ensure the pattern "PDKMETADATA" is not found in this library should PluginLoader ever encounter it.
   const ulong plen = pdk::strlen(pattern);
#if defined (PDK_OF_ELF) && defined(PDK_CC_GNU)
   int r = ElfParser().parse(filedata, fdlen, library, lib, &pos, &fdlen);
   if (r == ElfParser::Corrupt || r == ElfParser::NotElf) {
      if (lib && pdk_debug_component()) {
         warning_stream("ElfParser: %s",pdk_printable(lib->m_errorString));
      }
      return false;
   } else if (r == ElfParser::PdkMetaDataSection) {
      long rel = pdk_find_pattern(filedata + pos, fdlen, pattern, plen);
      if (rel < 0) {
         pos = -1;
      } else {
         pos += rel;
      }
      hasMetaData = true;
   }
#elif defined (PDK_OF_MACH_O)
   {
      String errorString;
      int r = MachOParser::parse(filedata, fdlen, library, &errorString, &pos, &fdlen);
      if (r == MachOParser::NotSuitable) {
         if (pdk_debug_component()) {
            warning_stream("MachOParser: %s", pdk_printable(errorString));
         }
         if (lib) {
            lib->m_errorString = errorString;
         }
         return false;
      }
      // even if the metadata section was not found, the Mach-O parser will
      // at least return the boundaries of the right architecture
      long rel = pdk_find_pattern(filedata + pos, fdlen, pattern, plen);
      if (rel < 0) {
         pos = -1;
      } else {
         pos += rel;
      }
      hasMetaData = true;
   }
#else
   pos = pdk_find_pattern(filedata, fdlen, pattern, plen);
   if (pos > 0) {
      hasMetaData = true;
   }
#endif // defined(PDK_OF_ELF) && defined(PDK_CC_GNU)
   bool ret = false;
   if (pos >= 0) {
      if (hasMetaData) {
         const char *data = filedata + pos;
         JsonDocument doc = json_from_raw_library_meta_data(data);
         lib->m_metaData = doc.getObject();
         if (pdk_debug_component()) {
            warning_stream("Found metadata in lib %s, metadata=\n%s\n",
                           library.toLocal8Bit().getConstRawData(), doc.toJson().getConstRawData());
         }
         ret = !doc.isNull();
      }
   }
   if (!ret && lib) {
      lib->m_errorString = Library::tr("Failed to extract plugin meta data from '%1'").arg(library);
   }
   file.close();
   return ret;
}

static void install_coverage_tool(LibraryPrivate *libPrivate)
{
#ifdef __COVERAGESCANNER__
   /*
      __COVERAGESCANNER__ is defined when Qt has been instrumented for code
      coverage by TestCocoon. CoverageScanner is the name of the tool that
      generates the code instrumentation.
      This code is required here when code coverage analysis with TestCocoon
      is enabled in order to allow the loading application to register the plugin
      and then store its execution report. The execution report gathers information
      about each part of the plugin's code that has been used when
      the plugin was loaded by the launching application.
      The execution report for the plugin will go to the same execution report
      as the one defined for the application loading it.
    */
   
   int ret = __coveragescanner_register_library(libPrivate->m_fileName.toLocal8Bit());
   
   if (pdk_debug_component()) {
      if (ret >= 0) {
         debug_stream("coverage data for %s registered",
                      pdk_printable(libPrivate->m_fileName));
      } else {
         warning_stream("could not register %s: error %d; coverage data may be incomplete",
                        pdk_printable(libPrivate->m_fileName),
                        ret);
      }
   }
#else
   PDK_UNUSED(libPrivate);
#endif
}
} // anonymous namespace

class LibraryStore
{
public:
   inline ~LibraryStore();
   static inline LibraryPrivate *findOrCreate(const String &fileName, const String &version, Library::LoadHints loadHints);
   static inline void releaseLibrary(LibraryPrivate *lib);
   static inline void cleanup();
private:
   static inline LibraryStore *getInstance();
   
   // all members and instance() are protected by sg_libraryMutex
   using LibraryMap = std::map<String, LibraryPrivate*>;
   LibraryMap m_libraryMap;
};

static std::mutex sg_libraryMutex;
static LibraryStore *sg_libraryData = nullptr;
static bool sg_libraryDataOnce;

LibraryStore::~LibraryStore()
{
   sg_libraryData = nullptr;
}

inline void LibraryStore::cleanup()
{
   LibraryStore *data = sg_libraryData;
   if (!data) {
      return;
   }
   // find any libraries that are still loaded but have a no one attached to them
   LibraryMap::iterator iter = data->m_libraryMap.begin();
   for (; iter != data->m_libraryMap.end(); ++iter) {
      LibraryPrivate *lib = iter->second;
      if (lib->m_libraryRefCount.load() == 1) {
         if (lib->m_libraryUnloadCount.load() > 0) {
            PDK_ASSERT(lib->m_handle);
            lib->m_libraryUnloadCount.store(1);
#ifdef __GLIBC__
            // glibc has a bug in unloading from global destructors
            // see https://bugzilla.novell.com/show_bug.cgi?id=622977
            // and http://sourceware.org/bugzilla/show_bug.cgi?id=11941
            lib->unload(LibraryPrivate::UnloadFlag::NoUnloadSys);
#else
            lib->unload();
#endif
         }
         delete lib;
         iter->second = 0;
      }
   }
   
   if (pdk_debug_component()) {
      // dump all objects that remain
      for (auto &iter : std::as_const(data->m_libraryMap)) {
         const LibraryPrivate *lib = iter.second;
         if (lib) {
            debug_stream() << "On pdk core unload," << lib->m_fileName << "was leaked, with"
                           << lib->m_libraryRefCount.load() << "users";
         }
      }
   }
   delete data;
}

namespace {
void library_cleanup()
{
   LibraryStore::cleanup();
}
} // anonymous namespace

PDK_DESTRUCTOR_FUNCTION(library_cleanup)

// must be called with a locked mutex
LibraryStore *LibraryStore::getInstance()
{
   if (PDK_UNLIKELY(!sg_libraryDataOnce && !sg_libraryData)) {
      // only create once per process lifetime
      sg_libraryData = new LibraryStore;
      sg_libraryDataOnce = true;
   }
   return sg_libraryData;
}

inline LibraryPrivate *LibraryStore::findOrCreate(const String &fileName, const String &version,
                                                  Library::LoadHints loadHints)
{
   std::lock_guard<std::mutex> locker(sg_libraryMutex);
   LibraryStore *data = getInstance();
   
   // check if this library is already loaded
   LibraryPrivate *lib = 0;
   if (PDK_LIKELY(data)) {
      lib = data->m_libraryMap.at(fileName);
      if (lib) {
         lib->mergeLoadHints(loadHints);
      }
   }
   if (!lib) {
      lib = new LibraryPrivate(fileName, version, loadHints);
   }
   // track this library
   if (PDK_LIKELY(data) && !fileName.isEmpty()) {
      data->m_libraryMap[fileName] = lib;
   }
   lib->m_libraryRefCount.ref();
   return lib;
}

inline void LibraryStore::releaseLibrary(LibraryPrivate *lib)
{
   std::lock_guard<std::mutex> locker(sg_libraryMutex);
   LibraryStore *data = getInstance();
   if (lib->m_libraryRefCount.deref()) {
      // still in use
      return;
   }
   // no one else is using
   PDK_ASSERT(lib->m_libraryUnloadCount.load() == 0);
   if (PDK_LIKELY(data) && !lib->m_fileName.isEmpty()) {
      LibraryPrivate *that = data->m_libraryMap.at(lib->m_fileName);
      data->m_libraryMap.erase(lib->m_fileName);
      PDK_ASSERT(lib == that);
      PDK_UNUSED(that);
   }
   delete lib;
}

LibraryPrivate::LibraryPrivate(const String &canonicalFileName, const String &version, Library::LoadHints loadHints)
   : m_handle(0),
     m_fileName(canonicalFileName),
     m_fullVersion(version),
     m_instance(0),
     m_libraryRefCount(0),
     m_libraryUnloadCount(0),
     m_pluginState(MightBeAPlugin)
{
   m_loadHintsInt.store(loadHints);
   if (canonicalFileName.isEmpty()) {
      m_errorString = Library::tr("The shared library was not found.");
   }
}

LibraryPrivate *LibraryPrivate::findOrCreate(const String &fileName, const String &version,
                                             Library::LoadHints loadHints)
{
   return LibraryStore::findOrCreate(fileName, version, loadHints);
}

LibraryPrivate::~LibraryPrivate()
{}

void LibraryPrivate::mergeLoadHints(Library::LoadHints lh)
{
   // if the library is already loaded, we can't change the load hints
   if (m_handle) {
      return;
   }
   m_loadHintsInt.store(lh);
}

FuncPointer LibraryPrivate::resolve(const char *symbol)
{
   if (!m_handle) {
      return nullptr;
   }
   return resolveSys(symbol);
}

void LibraryPrivate::setLoadHints(Library::LoadHints lh)
{
   // this locks a global mutex
   std::lock_guard<std::mutex> lock(sg_libraryMutex);
   mergeLoadHints(lh);
}

bool LibraryPrivate::load()
{
   if (m_handle) {
      m_libraryUnloadCount.ref();
      return true;
   }
   if (m_fileName.isEmpty()) {
      return false;      
   }
   bool ret = loadSys();
   if (pdk_debug_component()) {
      if (ret) {
         debug_stream() << "loaded library" << m_fileName;
      } else {
         debug_stream() << pdk_utf8_printable(m_errorString);
      }
   }
   if (ret) {
      //when loading a library we add a reference to it so that the LibraryPrivate won't get deleted
      //this allows to unload the library at a later time
      m_libraryUnloadCount.ref();
      m_libraryRefCount.ref();
      install_coverage_tool(this);
   }
   return ret;
}

bool LibraryPrivate::unload(UnloadFlag flag)
{
   if (!m_handle) {
      return false;
   }
   if (m_libraryUnloadCount.load() > 0 && !m_libraryUnloadCount.deref()) { // only unload if ALL Library instance wanted to
      delete m_inst.getData();
      if (flag == UnloadFlag::NoUnloadSys || unloadSys()) {
         if (pdk_debug_component()) {
            warning_stream() << "LibraryPrivate::unload succeeded on" << m_fileName
                             << (flag == UnloadFlag::NoUnloadSys ? "(faked)" : "");
         }
         //when the library is unloaded, we release the reference on it so that 'this'
         //can get deleted
         m_libraryRefCount.deref();
         m_handle = nullptr;
         m_instance = nullptr;
      }
   }
   return (m_handle == nullptr);
}

void LibraryPrivate::release()
{
   LibraryStore::releaseLibrary(this);
}

bool LibraryPrivate::loadPlugin()
{
   if (m_instance) {
      m_libraryUnloadCount.ref();
      return true;
   }
   if (m_pluginState == IsNotAPlugin)
      return false;
   if (load()) {
      m_instance = (PdkPluginInstanceFunc)resolve("pdk_plugin_instance");
      return m_instance;
   }
   if (pdk_debug_component()) {
      warning_stream() << "LibraryPrivate::loadPlugin failed on" << m_fileName << ":" << m_errorString;
   }
   m_pluginState = IsNotAPlugin;
   return false;
}

namespace {
bool pdk_get_metadata(PdkPluginQueryVerificationDataFunc pfn, LibraryPrivate *priv)
{
   const char *szData = 0;
   if (!pfn) {
      return false;
   }
   szData = pfn();
   if (!szData) {
      return false;
   }
   JsonDocument doc = json_from_raw_library_meta_data(szData);
   if (doc.isNull()) {
      return false;
   }
   priv->m_metaData = doc.getObject();
   return true;
}
} // anonymous namespace

bool LibraryPrivate::isPlugin()
{
   if (m_pluginState == MightBeAPlugin) {
      updatePluginState();
   }
   return m_pluginState == IsAPlugin;
}

void LibraryPrivate::updatePluginState()
{
   m_errorString.clear();
   if (m_pluginState != MightBeAPlugin) {
      return;
   }
   bool success = false;
   
#if defined(PDK_OS_UNIX) && !defined(PDK_OS_MAC)
   if (m_fileName.endsWith(Latin1String(".debug"))) {
      // refuse to load a file that ends in .debug
      // these are the debug symbols from the libraries
      // the problem is that they are valid shared library files
      // and dlopen is known to crash while opening them
      // pretend we didn't see the file
      m_errorString = Library::tr("The shared library was not found.");
      m_pluginState = IsNotAPlugin;
      return;
   }
#endif
   if (!m_handle) {
      // scan for the plugin metadata without loading
      success = find_pattern_unloaded(m_fileName, this);
   } else {
      // library is already loaded (probably via Library)
      // simply get the target function and call it.
      PdkPluginQueryVerificationDataFunc getMetaData = NULL;
      getMetaData = (PdkPluginQueryVerificationDataFunc) resolve("pdk_plugin_query_metadata");
      success = pdk_get_metadata(getMetaData, this);
   }
   if (!success) {
      if (m_errorString.isEmpty()){
         if (m_fileName.isEmpty()) {
            m_errorString = Library::tr("The shared library was not found.");
         } else {
            m_errorString = Library::tr("The file '%1' is not a valid pdk plugin.").arg(m_fileName);
         }
      }
      m_pluginState = IsNotAPlugin;
      return;
   }
   m_pluginState = IsNotAPlugin; // be pessimistic
   uint pdkVersion = (uint)m_metaData.getValue(Latin1String("version")).toDouble();
   bool debug = m_metaData.getValue(Latin1String("debug")).toBool();
   if ((pdkVersion & 0x00ff00) > (PDK_VERSION & 0x00ff00) || (pdkVersion & 0xff0000) != (PDK_VERSION & 0xff0000)) {
      if (pdk_debug_component()) {
         warning_stream("In %s:\n"
                        "  Plugin uses incompatible Qt library (%d.%d.%d) [%s]",
                        File::encodeName(m_fileName).getConstRawData(),
                        (pdkVersion&0xff0000) >> 16, (pdkVersion & 0xff00) >> 8, pdkVersion & 0xff,
                        debug ? "debug" : "release");
      }
      m_errorString = Library::tr("The plugin '%1' uses incompatible pdk library. (%2.%3.%4) [%5]")
            .arg(m_fileName)
            .arg((pdkVersion & 0xff0000) >> 16)
            .arg((pdkVersion & 0xff00) >> 8)
            .arg(pdkVersion & 0xff)
            .arg(debug ? Latin1String("debug") : Latin1String("release"));
#ifndef PDK_NO_DEBUG_PLUGIN_CHECK
   } else if(debug != LIBRARY_AS_DEBUG) {
      //don't issue a qWarning since we will hopefully find a non-debug? --Sam
      m_errorString = Library::tr("The plugin '%1' uses incompatible pdk library."
                                  " (Cannot mix debug and release libraries.)").arg(fileName);
#endif
   } else {
      m_pluginState = IsAPlugin;
   }
}

} // internal

bool Library::isLibrary(const String &fileName)
{
#if defined(PDK_OS_WIN)
   return fileName.endsWith(Latin1String(".dll"), pdk::CaseSensitivity::Insensitive);
#else // Generic Unix
   String completeSuffix = FileInfo(fileName).getCompleteSuffix();
   if (completeSuffix.isEmpty()) {
      return false;
   }
   const std::vector<StringRef> suffixes = completeSuffix.splitRef(Latin1Character('.'));
   StringList validSuffixList;
   
# if defined(PDK_OS_HPUX)
   /*
    See "HP-UX Linker and Libraries User's Guide", section "Link-time Differences between PA-RISC and IPF":
    "In PA-RISC (PA-32 and PA-64) shared libraries are suffixed with .sl. In IPF (32-bit and 64-bit),
    the shared libraries are suffixed with .so. For compatibility, the IPF linker also supports the .sl suffix."
 */
   validSuffixList.push_back(Latin1String("sl"));
#  if defined __ia64
   validSuffixList.push_back(Latin1String("so"));
#  endif
# elif defined(PDK_OS_AIX)
   validSuffixList.push_back(Latin1String("a"));
   validSuffixList.push_back(Latin1String("so"));
# elif defined(PDK_OS_DARWIN)
   // On Apple platforms, dylib look like libmylib.1.0.0.dylib
   if (suffixes.back() == Latin1String("dylib")) {
      return true;
   }
   validSuffixList.push_back(Latin1String("so"));
   validSuffixList.push_back(Latin1String("bundle"));
# elif defined(PDK_OS_UNIX)
   validSuffixList.push_back(Latin1String("so"));
# endif
   // Examples of valid library names:
   //  libfoo.so
   //  libfoo.so.0
   //  libfoo.so.0.3
   //  libfoo-0.3.so
   //  libfoo-0.3.so.0.3.0
   
   int suffix;
   int suffixPos = -1;
   for (suffix = 0; static_cast<size_t>(suffix) < validSuffixList.size() && suffixPos == -1; ++suffix) {
      auto iter = std::find_if(suffixes.begin(), suffixes.end(), [&validSuffixList, &suffix](const StringRef &item) -> bool {
         return item == StringRef(&validSuffixList.at(suffix));
      });
      if (iter == suffixes.end()) {
         suffixPos = -1;
      } else {
         suffixPos = std::distance(suffixes.begin(), iter);
      }
   }
   bool valid = suffixPos != -1;
   for (size_t i = suffixPos + 1; i < suffixes.size() && valid; ++i) {
      if (i != static_cast<size_t>(suffixPos)) {
         suffixes.at(i).toInt(&valid);
      }
   }
   return valid;
#endif
}

bool Library::load()
{
   if (!m_implPtr) {
      return false;
   }
   if (m_didLoad) {
      return m_implPtr->m_handle;
   }
   m_didLoad = true;
   return m_implPtr->load();
}

bool Library::unload()
{
   if (m_didLoad) {
      m_didLoad = false;
      return m_implPtr->unload();
   }
   return false;
}

bool Library::isLoaded() const
{
   return m_implPtr && m_implPtr->m_handle;
}

Library::Library(Object *parent)
   : Object(parent),
     m_implPtr(nullptr),
     m_didLoad(false)
{}

Library::Library(const String &fileName, Object *parent)
   : Object(parent),
     m_implPtr(nullptr),
     m_didLoad(false)
{
   setFileName(fileName);
}

Library::Library(const String& fileName, int verNum, Object *parent)
   : Object(parent),
     m_implPtr(nullptr),
     m_didLoad(false)
{
   setFileNameAndVersion(fileName, verNum);
}

Library::Library(const String& fileName, const String &version, Object *parent)
   : Object(parent),
     m_implPtr(nullptr),
     m_didLoad(false)
{
   setFileNameAndVersion(fileName, version);
}

Library::~Library()
{
   if (m_implPtr) {
      m_implPtr->release();
   }
}

void Library::setFileName(const String &fileName)
{
   Library::LoadHints lh;
   if (m_implPtr) {
      lh = m_implPtr->getLoadHints();
      m_implPtr->release();
      m_implPtr = nullptr;
      m_didLoad = false;
   }
   m_implPtr = LibraryPrivate::findOrCreate(fileName, String(), lh);
}

String Library::getFileName() const
{
   if (m_implPtr) {
      return m_implPtr->m_qualifiedFileName.isEmpty() 
            ? m_implPtr->m_fileName 
            : m_implPtr->m_qualifiedFileName;
   }
   return String();
}

void Library::setFileNameAndVersion(const String &fileName, int verNum)
{
   Library::LoadHints lh;
   if (m_implPtr) {
      lh = m_implPtr->getLoadHints();
      m_implPtr->release();
      m_implPtr = nullptr;
      m_didLoad = false;
   }
   m_implPtr = LibraryPrivate::findOrCreate(fileName, verNum >= 0 ? String::number(verNum) : String(), lh);
}

void Library::setFileNameAndVersion(const String &fileName, const String &version)
{
   Library::LoadHints lh;
   if (m_implPtr) {
      lh = m_implPtr->getLoadHints();
      m_implPtr->release();
      m_implPtr = nullptr;
      m_didLoad = false;
   }
   m_implPtr = LibraryPrivate::findOrCreate(fileName, version, lh);
}

FuncPointer Library::resolve(const char *symbol)
{
   if (!isLoaded() && !load()) {
      return nullptr;
   }
   return m_implPtr->resolve(symbol);
}

FuncPointer Library::resolve(const String &fileName, const char *symbol)
{
   Library library(fileName);
   return library.resolve(symbol);
}

FuncPointer Library::resolve(const String &fileName, int verNum, const char *symbol)
{
   Library library(fileName, verNum);
   return library.resolve(symbol);
}

FuncPointer Library::resolve(const String &fileName, const String &version, const char *symbol)
{
   Library library(fileName, version);
   return library.resolve(symbol);
}

String Library::getErrorString() const
{
   return (!m_implPtr || m_implPtr->m_errorString.isEmpty()) 
         ? tr("Unknown error")
         : m_implPtr->m_errorString;
}

void Library::setLoadHints(LoadHints hints)
{
   if (!m_implPtr) {
      m_implPtr = LibraryPrivate::findOrCreate(String());   // ugly, but we need a d-ptr
      m_implPtr->m_errorString.clear();
   }
   m_implPtr->setLoadHints(hints);
}

Library::LoadHints Library::getLoadHints() const
{
   return m_implPtr ? m_implPtr->getLoadHints() : Library::LoadHints();
}

namespace internal {
bool pdk_debug_component()
{
   static int debug_env = pdk::env_var_intval("PDK_DEBUG_PLUGINS");
   return debug_env != 0;
}
} // internal

} // dll
} // pdk
