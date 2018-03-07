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

#ifndef PDK_DLL_INTERNAL_FACTORY_LOADER_PRIVATE_H
#define PDK_DLL_INTERNAL_FACTORY_LOADER_PRIVATE_H

#include "pdk/kernel/Object.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/utils/json/JsonObject.h"
#include "pdk/base/utils/json/JsonDocument.h"
#include "pdk/global/Endian.h"
#if PDK_CONFIG(library)
#include "pdk/dll/internal/LibraryPrivate.h"
#endif

#include <map>
#include <list>

namespace pdk {
namespace dll {
namespace internal {

using pdk::utils::json::JsonDocument;
using pdk::utils::json::JsonObject;
using pdk::ds::ByteArray;
using pdk::kernel::Object;
using pdk::lang::String;

inline JsonDocument json_from_raw_library_meta_data(const char *raw)
{
   raw += strlen("PDKMETADATA  ");
   // the size of the embedded JSON object can be found 8 bytes into the data (see qjson_p.h),
   // but doesn't include the size of the header (8 bytes)
   ByteArray json(raw, pdk::from_little_endian<uint>(*(const uint *)(raw + 8)) + 8);
   return JsonDocument::fromBinaryData(json);
}

class FactoryLoaderPrivate;
class PDK_CORE_EXPORT FactoryLoader : public Object
{
   PDK_DECLARE_PRIVATE(FactoryLoader);
   
public:
   explicit FactoryLoader(const char *iid,
                          const String &suffix = String(),
                          pdk::CaseSensitivity = pdk::CaseSensitivity::Sensitive);
   
#if PDK_CONFIG(library)
   ~FactoryLoader();
   
   void update();
   static void refreshAll();
   
#if defined(PDK_OS_UNIX) && !defined (PDK_OS_MAC)
   LibraryPrivate *library(const String &key) const;
#endif // PDK_OS_UNIX && !PDK_OS_MAC
#endif // PDK_CONFIG(library)
   
   std::multimap<int, String> keyMap() const;
   int indexOf(const String &needle) const;
   
   std::list<JsonObject> getMetaData() const;
   Object *getInstance(int index) const;
};

template <class PluginInterface, class FactoryInterface, typename ...Args>
PluginInterface *pdk_load_plugin(const FactoryLoader *loader, const String &key, Args &&...args)
{
   const int index = loader->indexOf(key);
   if (index != -1) {
      Object *factoryObject = loader->getInstance(index);
      if (FactoryInterface *factory = dynamic_cast<FactoryInterface *>(factoryObject)) {
         if (PluginInterface *result = factory->create(key, std::forward<Args>(args)...)) {
            return result;
         }
      } 
   }
   return nullptr;
}

} // internal
} // dll
} // pdk

#endif // PDK_DLL_INTERNAL_FACTORY_LOADER_PRIVATE_H
