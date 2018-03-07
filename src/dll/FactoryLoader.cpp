//// @copyright 2017-2018 zzu_softboy <zzu_softboy@163.com>
////
//// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
//// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
//// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////
//// Created by softboy on 2018/03/07.

//#include "pdk/dll/internal/FactoryLoaderPrivate.h"
//#include "pdk/dll/FactoryInterface.h"
//#include "pdk/dll/Plugin.h"
//#include "pdk/dll/PluginLoader.h"
//#include "pdk/base/io/fs/Dir.h"
//#include "pdk/base/io/Debug.h"
//#include "pdk/kernel/internal/ObjectPrivate.h"
//#include "pdk/kernel/internal/CoreApplicationPrivate.h"
//#include "pdk/base/utils/json/JsonDocument.h"
//#include "pdk/base/utils/json/JsonValue.h"
//#include "pdk/base/utils/json/JsonObject.h"
//#include "pdk/base/utils/json/JsonArray.h"
//#include "pdk/global/GlobalStatic.h"

//#include <list>
//#include <map>

//namespace pdk {
//namespace dll {
//namespace internal {

//using pdk::kernel::internal::ObjectPrivate;
//using pdk::ds::ByteArray;
//using pdk::lang::String;
//using pdk::lang::Latin1String;
//using pdk::lang::Latin1Character;
//using pdk::kernel::CoreApplication;
//using pdk::kernel::internal::CoreApplicationPrivate;
//using pdk::io::fs::Dir;
//using pdk::io::fs::FileInfo;
//using pdk::utils::json::JsonArray;
//using pdk::dll::Library;

//class FactoryLoaderPrivate : public ObjectPrivate
//{
//   PDK_DECLARE_PUBLIC(FactoryLoader);
//public:
//   FactoryLoaderPrivate()
//   {}
   
//   ByteArray m_iid;
//#if PDK_CONFIG(library)
//   ~FactoryLoaderPrivate();
//   mutable std::mutex m_mutex;
//   std::list<LibraryPrivate *> m_libraryList;
//   std::map<String, LibraryPrivate*> m_keyMap;
//   String m_suffix;
//   pdk::CaseSensitivity m_cs;
//   StringList m_loadedPaths;
//#endif
//};

//#if PDK_CONFIG(library)

//PDK_GLOBAL_STATIC(std::list<FactoryLoader *>, sg_factoryLoaders);
//PDK_GLOBAL_STATIC(std::recursive_mutex, sg_factoryloaderMutex);

//FactoryLoaderPrivate::~FactoryLoaderPrivate()
//{
//   for (size_t i = 0; i < m_libraryList.size(); ++i) {
//      auto iter = m_libraryList.begin();
//      std::advance(iter, i);
//      LibraryPrivate *library = *iter;
//      library->unload();
//      library->release();
//   }
//}

//void FactoryLoader::update()
//{
//#ifdef PDK_SHARED
//   PDK_D(FactoryLoader);
//   StringList paths = CoreApplication::getLibraryPaths();
//   for (size_t i = 0; i < paths.size(); ++i) {
//      const String &pluginDir = paths.at(i);
//      // Already loaded, skip it...
//      if (implPtr->m_loadedPaths.contains(pluginDir)) {
//         continue;
//      }
//      implPtr->m_loadedPaths.push_back(pluginDir);
//      String path = pluginDir + implPtr->m_suffix;
//      if (pdk_debug_component()) {
//         debug_stream() << "FactoryLoader::FactoryLoader() checking directory path" << path << "...";
//      }
//      if (!Dir(path).exists(Latin1String("."))) {
//         continue;
//      }
      
      
//      StringList plugins = Dir(path).entryList(
//         #ifdef PDK_OS_WIN
//               StringList(StringLiteral("*.dll")),
//         #endif
//               Dir::Filter::Files);
//      LibraryPrivate *library = nullptr;
//#ifdef PDK_OS_MAC
//      // Loading both the debug and release version of the cocoa plugins causes the objective-c runtime
//      // to print "duplicate class definitions" warnings. Detect if FactoryLoader is about to load both,
//      // skip one of them (below).
//      //
//      // ### FIXME find a proper solution
//      //
//      const bool isLoadingDebugAndReleaseCocoa = plugins.contains(Latin1String("libqcocoa_debug.dylib"))
//            && plugins.contains(Latin1String("libqcocoa.dylib"));
//#endif
//      for (size_t j = 0; j < plugins.size(); ++j) {
//         String fileName = Dir::cleanPath(path + Latin1Character('/') + plugins.at(j));
         
//#ifdef PDK_OS_MAC
//         if (isLoadingDebugAndReleaseCocoa) {
//#ifdef PDK_DEBUG
//            if (fileName.contains(Latin1String("libqcocoa.dylib"))) {
//               continue;    // Skip release plugin in debug mode
//            }
//#else
//            if (fileName.contains(Latin1String("libqcocoa_debug.dylib"))) {
//               continue;    // Skip debug plugin in release mode
//            }   
//#endif
//         }
//#endif
//         if (pdk_debug_component()) {
//            debug_stream() << "FactoryLoader::FactoryLoader() looking at" << fileName;
//         }
//         library = LibraryPrivate::findOrCreate(FileInfo(fileName).getCanonicalFilePath());
//         if (!library->isPlugin()) {
//            if (pdk_debug_component()) {
//               debug_stream() << library->m_errorString << pdk::io::endl
//                              << "         not a plugin";
//            }
//            library->release();
//            continue;
//         }
         
//         StringList keys;
//         bool metaDataOk = false;
         
//         String iid = library->m_metaData.getValue(Latin1String("IID")).toString();
//         if (iid == Latin1String(implPtr->m_iid.getConstRawData(), implPtr->m_iid.size())) {
//            JsonObject object = library->m_metaData.getValue(Latin1String("MetaData")).toObject();
//            metaDataOk = true;
//            JsonArray k = object.getValue(Latin1String("Keys")).toArray();
//            for (int i = 0; i < k.getSize(); ++i) {
//               keys += implPtr->m_cs == pdk::CaseSensitivity::Sensitive? k.at(i).toString() : k.at(i).toString().toLower();
//            }
//         }
//         if (pdk_debug_component()) {
//            debug_stream() << "Got keys from plugin meta data" << keys;
//         }
//         if (!metaDataOk) {
//            library->release();
//            continue;
//         }
//         int keyUsageCount = 0;
//         for (size_t k = 0; k < keys.size(); ++k) {
//            // first come first serve, unless the first
//            // library was built with a future pdk version,
//            // whereas the new one has a pdk version that fits
//            // better
//            const String &key = keys.at(k);
//            LibraryPrivate *previous = implPtr->m_keyMap.at(key);
//            int prevPdkVersion = 0;
//            if (previous) {
//               prevPdkVersion = (int)previous->m_metaData.getValue(Latin1String("version")).toDouble();
//            }
//            int pdkVersion = (int)library->m_metaData.getValue(Latin1String("version")).toDouble();
//            if (!previous || (prevPdkVersion > PDK_VERSION && pdkVersion <= PDK_VERSION)) {
//               implPtr->m_keyMap[key] = library;
//               ++keyUsageCount;
//            }
//         }
//         if (keyUsageCount || keys.empty()) {
//            library->setLoadHints(Library::LoadHint::PreventUnloadHint); // once loaded, don't unload
//            implPtr->m_libraryList.push_back(library);
//         } else {
//            library->release();
//         }
//      }
//   }
//#else
//   PDK_D(FactoryLoader);
//   if (pdk_debug_component()) {
//      debug_stream() << "FactoryLoader::FactoryLoader() ignoring" << implPtr->m_iid
//                     << "since plugins are disabled in static builds";
//   }
//#endif
//}

//FactoryLoader::~FactoryLoader()
//{
//   std::lock_guard<std::recursive_mutex> locker(*sg_factoryloaderMutex());
//   sg_factoryLoaders()->remove(this);
//}

//#if defined(PDK_OS_UNIX) && !defined (PDK_OS_MAC)
//LibraryPrivate *FactoryLoader::library(const String &key) const
//{
//   PDK_D(const FactoryLoader);
//   return implPtr->m_keyMap.at(implPtr->m_cs ? key : key.toLower());
//}
//#endif

//void FactoryLoader::refreshAll()
//{
//   std::lock_guard<std::recursive_mutex> locker(*sg_factoryloaderMutex());
//   std::list<FactoryLoader *> *loaders = sg_factoryLoaders();
//   for (std::list<FactoryLoader *>::const_iterator iter = loaders->cbegin();
//        iter != loaders->cend(); ++iter) {
//      (*iter)->update();
//   }
//}

//#endif // PDK_CONFIG(library)

//FactoryLoader::FactoryLoader(const char *iid,
//                             const String &suffix,
//                             pdk::CaseSensitivity cs)
//   : Object(*new FactoryLoaderPrivate)
//{
//   moveToThread(CoreApplicationPrivate::getMainThread());
//   PDK_D(FactoryLoader);
//   implPtr->m_iid = iid;
//#if PDK_CONFIG(library)
//   implPtr->m_cs = cs;
//   implPtr->m_suffix = suffix;
   
//   std::lock_guard<std::recursive_mutex> locker(*sg_factoryloaderMutex());
//   update();
//   sg_factoryLoaders()->push_back(this);
//#else
//   PDK_UNUSED(suffix);
//   PDK_UNUSED(cs);
//#endif
//}

//std::list<JsonObject> FactoryLoader::getMetaData() const
//{
//   PDK_D(const FactoryLoader);
//   std::list<JsonObject> metaData;
//#if PDK_CONFIG(library)
//   std::lock_guard<std::mutex> locker(implPtr->m_mutex);
//   for (size_t i = 0; i < implPtr->m_libraryList.size(); ++i) {
//      auto iter = implPtr->m_libraryList.begin();
//      std::advance(iter, i);
//      metaData.push_back((*iter)->m_metaData);
//   }
//#endif
//   const auto staticPlugins = PluginLoader::staticPlugins();
//   for (const QStaticPlugin &plugin : staticPlugins) {
//      const QJsonObject object = plugin.metaData();
//      if (object.value(QLatin1String("IID")) != QLatin1String(d->iid.constData(), d->iid.size()))
//         continue;
//      metaData.append(object);
//   }
//   return metaData;
//}

//QObject *FactoryLoader::instance(int index) const
//{
//   Q_D(const FactoryLoader);
//   if (index < 0)
//      return 0;
   
//#if QT_CONFIG(library)
//   QMutexLocker lock(&d->mutex);
//   if (index < d->libraryList.size()) {
//      LibraryPrivate *library = d->libraryList.at(index);
//      if (library->instance || library->loadPlugin()) {
//         if (!library->inst)
//            library->inst = library->instance();
//         QObject *obj = library->inst.data();
//         if (obj) {
//            if (!obj->parent())
//               obj->moveToThread(CoreApplicationPrivate::mainThread());
//            return obj;
//         }
//      }
//      return 0;
//   }
//   index -= d->libraryList.size();
//   lock.unlock();
//#endif
   
//   QVector<QStaticPlugin> staticPlugins = QPluginLoader::staticPlugins();
//   for (int i = 0; i < staticPlugins.count(); ++i) {
//      const QJsonObject object = staticPlugins.at(i).metaData();
//      if (object.value(QLatin1String("IID")) != QLatin1String(d->iid.constData(), d->iid.size()))
//         continue;
      
//      if (index == 0)
//         return staticPlugins.at(i).instance();
//      --index;
//   }
   
//   return 0;
//}

//QMultiMap<int, String> FactoryLoader::keyMap() const
//{
//   QMultiMap<int, String> result;
//   const QList<QJsonObject> metaDataList = metaData();
//   for (int i = 0; i < metaDataList.size(); ++i) {
//      const QJsonObject metaData = metaDataList.at(i).value(QLatin1String("MetaData")).toObject();
//      const QJsonArray keys = metaData.value(QLatin1String("Keys")).toArray();
//      const int keyCount = keys.size();
//      for (int k = 0; k < keyCount; ++k)
//         result.insert(i, keys.at(k).toString());
//   }
//   return result;
//}

//int FactoryLoader::indexOf(const String &needle) const
//{
//   const QList<QJsonObject> metaDataList = metaData();
//   for (int i = 0; i < metaDataList.size(); ++i) {
//      const QJsonObject metaData = metaDataList.at(i).value(QLatin1String("MetaData")).toObject();
//      const QJsonArray keys = metaData.value(QLatin1String("Keys")).toArray();
//      const int keyCount = keys.size();
//      for (int k = 0; k < keyCount; ++k) {
//         if (!keys.at(k).toString().compare(needle, Qt::CaseInsensitive))
//            return i;
//      }
//   }
//   return -1;
//}

//} // internal
//} // dll
//} // pdk

