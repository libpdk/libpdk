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

#ifndef PDK_DLL_PLUGIN_LOADER_H
#define PDK_DLL_PLUGIN_LOADER_H

#include "pdk/global/Global.h"
#if PDK_CONFIG(library)
#include "pdk/dll/Library.h"
#endif
#include "pdk/dll/Plugin.h"
#include "pdk/kernel/Object.h"

namespace pdk {

// forward declare class with namespace
namespace utils {
namespace json {
class JsonObject;
} // json
} // utils

namespace dll {

// forward declare class with namespace
namespace internal {
class LibraryPrivate;
} // internal

using pdk::utils::json::JsonObject;
using internal::LibraryPrivate;
using pdk::kernel::Object;
using pdk::kernel::ObjectList;

#if PDK_CONFIG(library)

class PDK_CORE_EXPORT PluginLoader : public Object
{
public:
   explicit PluginLoader(Object *parent = nullptr);
   explicit PluginLoader(const String &fileName, Object *parent = nullptr);
   ~PluginLoader();
   
   Object *getInstance();
   JsonObject getMetaData() const;
   
   static ObjectList getStaticInstances();
   static std::vector<StaticPlugin> getStaticPlugins();
   
   bool load();
   bool unload();
   bool isLoaded() const;
   
   void setFileName(const String &fileName);
   String getFileName() const;
   String getErrorString() const;
   
   void setLoadHints(Library::LoadHints loadHints);
   Library::LoadHints getLoadHints() const;
   
private:
   LibraryPrivate *m_implPtr;
   bool m_didLoad;
   PDK_DISABLE_COPY(PluginLoader);
};

#else
class PDK_CORE_EXPORT PluginLoader
{
public:
   static ObjectList getStaticInstances();
   static std::vector<StaticPlugin> getStaticPlugins();
};
#endif
} // dll
} // pdk

#endif // PDK_DLL_PLUGIN_LOADER_H
