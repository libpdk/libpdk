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
#include "pdk/global/GlobalStatic.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/dll/Library.h"
#include "pdk/dll/Plugin.h"
#include "pdk/dll/PluginLoader.h"
#include "pdk/dll/internal/FactoryLoaderPrivate.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/Debug.h"

namespace pdk {
namespace dll {

using pdk::kernel::Object;
using pdk::lang::String;
using pdk::lang::StringRef;
using pdk::lang::Latin1Character;
using pdk::ds::StringList;
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::kernel::CoreApplication;

#if PDK_CONFIG(library)

PluginLoader::PluginLoader(Object *parent)
   : Object(parent),
     m_implPtr(nullptr),
     m_didLoad(false)
{
}

PluginLoader::PluginLoader(const String &fileName, Object *parent)
   : Object(parent),
     m_implPtr(nullptr),
     m_didLoad(false)
{
   setFileName(fileName);
   setLoadHints(Library::LoadHint::PreventUnloadHint);
}

PluginLoader::~PluginLoader()
{
   if (m_implPtr) {
      m_implPtr->release();
   }
}

Object *PluginLoader::getInstance()
{
   if (!isLoaded() && !load()) {
      return nullptr;
   }
   if (!m_implPtr->m_inst && m_implPtr->m_instance) {
      // @TODO Pointer issue
      m_implPtr->m_inst = m_implPtr->m_instance();
   }
   return m_implPtr->m_inst.getData();
}

JsonObject PluginLoader::getMetaData() const
{
   if (!m_implPtr) {
      return JsonObject();
   }
   return m_implPtr->m_metaData;
}

bool PluginLoader::load()
{
   if (!m_implPtr || m_implPtr->m_fileName.isEmpty())
      return false;
   if (m_didLoad) {
      return m_implPtr->m_handle && m_implPtr->m_instance;
   }
   if (!m_implPtr->isPlugin()) {
      return false;
   }
   m_didLoad = true;
   return m_implPtr->loadPlugin();
}

bool PluginLoader::unload()
{
   if (m_didLoad) {
      m_didLoad = false;
      return m_implPtr->unload();
   }
   if (m_implPtr) {
      m_implPtr->m_errorString = tr("The plugin was not loaded.");
   } // Ouch
   return false;
}

bool PluginLoader::isLoaded() const
{
   return m_implPtr && m_implPtr->m_handle && m_implPtr->m_instance;
}

#if defined(PDK_SHARED)
namespace {
String locate_plugin(const String &fileName)
{
   const bool isAbsolute = Dir::isAbsolutePath(fileName);
   if (isAbsolute) {
      FileInfo fi(fileName);
      if (fi.isFile()) {
         return fi.getCanonicalFilePath();
      }
   }
   StringList prefixes = LibraryPrivate::prefixesSys();
   prefixes.push_front(String());
   StringList suffixes = LibraryPrivate::suffixesSys(String());
   suffixes.push_front(String());
   // Split up "subdir/filename"
   const int slash = fileName.lastIndexOf(Latin1Character('/'));
   const StringRef baseName = fileName.substringRef(slash + 1);
   const StringRef basePath = isAbsolute ? StringRef() : fileName.leftRef(slash + 1); // keep the '/'
   const bool debug = internal::pdk_debug_component();
   StringList paths;
   if (isAbsolute) {
      paths.push_back(fileName.left(slash)); // don't include the '/'
   } else {
      paths = CoreApplication::getLibraryPaths();
      paths.push_front(StringLiteral(".")); // search in current dir first
   }
   for (const String &path : std::as_const(paths)) {
      for (const String &prefix : std::as_const(prefixes)) {
         for (const String &suffix : std::as_const(suffixes)) {
            const String fn = path + Latin1Character('/') + basePath + prefix + baseName + suffix;
            if (debug) {
               debug_stream() << "Trying..." << fn;
            }
            if (FileInfo(fn).isFile()) {
               return fn;
            }
         }
      }
   }
   if (debug) {
      debug_stream() << fileName << "not found";
   }
   return String();
}
} // anonymous namespace
#endif

void PluginLoader::setFileName(const String &fileName)
{
#if defined(PDK_SHARED)
   Library::LoadHints lh = Library::LoadHint::PreventUnloadHint;
   if (m_implPtr) {
      lh = m_implPtr->getLoadHints();
      m_implPtr->release();
      m_implPtr = nullptr;
      m_didLoad = false;
   }
   const String fn = locate_plugin(fileName);
   m_implPtr = LibraryPrivate::findOrCreate(fn, String(), lh);
   if (!fn.isEmpty()) {
      m_implPtr->updatePluginState();
   }
#else
   if (pdk_debug_component()) {
      warning_stream("Cannot load %s into a statically linked Qt library.",
                     (const char*)File::encodeName(fileName));
   }
   PDK_UNUSED(fileName);
#endif
}

String PluginLoader::getFileName() const
{
   if (m_implPtr) {
      return m_implPtr->m_fileName;
   }
   return String();
}

String PluginLoader::getErrorString() const
{
   return (!m_implPtr || m_implPtr->m_errorString.isEmpty()) ? tr("Unknown error") : m_implPtr->m_errorString;
}

void PluginLoader::setLoadHints(Library::LoadHints loadHints)
{
   if (!m_implPtr) {
      m_implPtr = LibraryPrivate::findOrCreate(String());   // ugly, but we need a d-ptr
      m_implPtr->m_errorString.clear();
   }
   m_implPtr->setLoadHints(loadHints);
}



Library::LoadHints PluginLoader::getLoadHints() const
{
   return m_implPtr ? m_implPtr->getLoadHints() : Library::LoadHints();
}

using StaticPluginList = std::vector<StaticPlugin>;
PDK_GLOBAL_STATIC(StaticPluginList, sg_staticPluginList);

void PDK_CORE_EXPORT register_static_plugin_func(StaticPlugin plugin)
{
   sg_staticPluginList()->push_back(plugin);
}

ObjectList PluginLoader::getStaticInstances()
{
   ObjectList instances;
   const StaticPluginList *plugins = sg_staticPluginList();
   if (plugins) {
      const int numPlugins = plugins->size();
      instances.resize(numPlugins);
      for (int i = 0; i < numPlugins; ++i) {
         instances.push_back(plugins->at(i).m_instance());
      }
   }
   return instances;
}

std::vector<StaticPlugin> PluginLoader::getStaticPlugins()
{
   StaticPluginList *plugins = sg_staticPluginList();
   if (plugins) {
      return *plugins;
   }
   return std::vector<StaticPlugin>();
}

JsonObject StaticPlugin::getMetaData() const
{
   return internal::json_from_raw_library_meta_data(m_rawMetaData()).getObject();
}

#endif

} // dll
} // pdk


