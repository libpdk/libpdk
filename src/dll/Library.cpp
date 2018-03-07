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
using pdk::kernel::internal::SystemError;
using pdk::utils::json::JsonDocument;
using pdk::utils::json::JsonValue;

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
  for the QT_PLUGIN_VERIFICATION_DATA.  The advantage of this approach is that
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
       ELF and Mach-O binaries with GCC have .qplugin sections.
    */
   bool hasMetaData = false;
   long pos = 0;
   char pattern[] = "pTMETADATA  ";
   pattern[0] = 'P'; // Ensure the pattern "QTMETADATA" is not found in this library should PluginLoader ever encounter it.
   const ulong plen = pdk::strlen(pattern);
#if defined (PDK_OF_ELF) && defined(PDK_CC_GNU)
   int r = ElfParser().parse(filedata, fdlen, library, lib, &pos, &fdlen);
   if (r == QElfParser::Corrupt || r == QElfParser::NotElf) {
      if (lib && qt_debug_component()) {
         qWarning("QElfParser: %s",qPrintable(lib->errorString));
      }
      return false;
   } else if (r == QElfParser::QtMetaDataSection) {
      long rel = qt_find_pattern(filedata + pos, fdlen, pattern, plen);
      if (rel < 0)
         pos = -1;
      else
         pos += rel;
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
         JsonDocument doc = qJsonFromRawLibraryMetaData(data);
         lib->metaData = doc.object();
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

static void installCoverageTool(LibraryPrivate *libPrivate)
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
   
   int ret = __coveragescanner_register_library(libPrivate->fileName.toLocal8Bit());
   
   if (qt_debug_component()) {
      if (ret >= 0) {
         qDebug("coverage data for %s registered",
                qPrintable(libPrivate->fileName));
      } else {
         qWarning("could not register %s: error %d; coverage data may be incomplete",
                  qPrintable(libPrivate->fileName),
                  ret);
      }
   }
#else
   PDK_UNUSED(libPrivate);
#endif
}
} // anonymous namespace
} // internal

} // dll
} // pdk
