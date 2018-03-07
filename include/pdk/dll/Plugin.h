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
// Created by softboy on 2018/03/05.

#ifndef PDK_DLL_PLUGIN_H
#define PDK_DLL_PLUGIN_H

#include "pdk/kernel/Object.h"
#include "pdk/kernel/Pointer.h"
#include "pdk/base/utils/json/JsonObject.h"

namespace pdk {
namespace dll {

using pdk::kernel::Object;
using pdk::kernel::Pointer;
using pdk::utils::json::JsonObject;

#ifndef PDK_EXTERN_C
#  ifdef __cplusplus
#    define PDK_EXTERN_C extern "C"
#  else
#    define PDK_EXTERN_C extern
#  endif
#endif

using PdkPluginInstanceFunc = Object *(*)();
using PdkPluginMetaDataFunc = const char *(*)();

struct PDK_CORE_EXPORT StaticPlugin
{
   // Note: This struct is initialized using an initializer list.
   // As such, it cannot have any new constructors or variables.
   PdkPluginInstanceFunc m_instance;
   PdkPluginMetaDataFunc m_rawMetaData;
   JsonObject getMetaData() const;
};

void PDK_CORE_EXPORT register_static_plugin_func(StaticPlugin staticPlugin);

#if (defined(PDK_OF_ELF) || defined(PDK_OS_WIN)) && (defined (PDK_CC_GNU) || defined(PDK_CC_CLANG))
#  define PDK_PLUGIN_METADATA_SECTION \
   __attribute__ ((section (".pdkmetadata"))) __attribute__((used))
#elif defined(PDK_OS_MAC)
// TODO: Implement section parsing on Mac
#  define PDK_PLUGIN_METADATA_SECTION \
   __attribute__ ((section ("__TEXT,pdkmetadata"))) __attribute__((used))
#elif defined(PDK_CC_MSVC)
// TODO: Implement section parsing for MSVC
#pragma section(".pdkmetadata",read,shared)
#  define pdk_PLUGIN_METADATA_SECTION \
   __declspec(allocate(".pdkmetadata"))
#else
#  define PDK_PLUGIN_VERIFICATION_SECTION
#  define PDK_PLUGIN_METADATA_SECTION
#endif

#define PDK_IMPORT_PLUGIN(PLUGIN) \
   extern const pdk::dll::StaticPlugin pdk_static_plugin_##PLUGIN(); \
   class Static##PLUGIN##PluginInstance{ \
   public: \
   Static##PLUGIN##PluginInstance() { \
   register_static_plugin_func(pdk_static_plugin_##PLUGIN()); \
} \
}; \
   static Static##PLUGIN##PluginInstance static##PLUGIN##Instance;

#define PDK_PLUGIN_INSTANCE(IMPLEMENTATION) \
{ \
   static pdk::kernel::Pointer<pdk::kernel::Object> _instance; \
   if (!_instance)      \
   _instance = new IMPLEMENTATION; \
   return _instance; \
}

#if defined(PDK_STATICPLUGIN)
#  define PDK_EXPORT_PLUGIN(PLUGINCLASS, PLUGINCLASSNAME) \
   static pdk::kernel::Object *pdk_plugin_instance_##PLUGINCLASSNAME() \
   PDK_PLUGIN_INSTANCE(PLUGINCLASS) \
   static const char *pdk_plugin_query_metadata_##PLUGINCLASSNAME() { return reinterpret_cast<const char *>(pdk_pluginMetaData); } \
   const pdk::dll::StaticPlugin pdk_static_plugin_##PLUGINCLASSNAME() { \
   pdk::dll::StaticPlugin plugin = { pdk_plugin_instance_##PLUGINCLASSNAME, pdk_plugin_query_metadata_##PLUGINCLASSNAME}; \
   return plugin; \
}
#else
#  define PDK_EXPORT_PLUGIN(PLUGINCLASS, PLUGINCLASSNAME)      \
   PDK_EXTERN_C PDK_DECL_EXPORT \
   const char *pdk_plugin_query_metadata() \
{ return reinterpret_cast<const char *>(pdk_pluginMetaData); } \
   PDK_EXTERN_C PDK_DECL_EXPORT pdk::kernel::Object *pdk_plugin_instance() \
   PDK_PLUGIN_INSTANCE(PLUGINCLASS)
#endif

} // dll
} // pdk

PDK_DECLARE_TYPEINFO(pdk::dll::StaticPlugin, PDK_PRIMITIVE_TYPE);

#endif // PDK_DLL_PLUGIN_H
