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

#ifndef PDK_DLL_INTERNAL_LIBRARY_PRIVATE_H
#define PDK_DLL_INTERNAL_LIBRARY_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/dll/Library.h"
#include "pdk/dll/Plugin.h"
#include "pdk/utils/SharedPointer.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/utils/json/JsonObject.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/kernel/Pointer.h"

#ifdef PDK_OS_WIN
#  include "pdk/global/Windows.h"
#endif

PDK_REQUIRE_CONFIG(library);

namespace pdk {
namespace dll {
namespace internal {

using pdk::utils::json::JsonObject;
using pdk::os::thread::AtomicInt;
using pdk::kernel::Pointer;

bool pdk_debug_component();

class LibraryStore;
class LibraryPrivate
{
public:
   
#ifdef PDK_OS_WIN
   HINSTANCE
#else
   void *
#endif
   m_handle;
   enum class UnloadFlag
   {
      UnloadSys,
      NoUnloadSys
   };
   String m_fileName;
   String m_qualifiedFileName;
   String m_fullVersion;
   bool load();
   bool loadPlugin(); // loads and resolves instance
   bool unload(UnloadFlag flag = UnloadFlag::UnloadSys);
   void release();
   FuncPointer resolve(const char *);
   Library::LoadHints getLoadHints() const
   {
      return Library::LoadHints(m_loadHintsInt.load());
   }
   
   void setLoadHints(Library::LoadHints lh);
   
   static LibraryPrivate *findOrCreate(const String &fileName, const String &version = String(),
                                       Library::LoadHints loadHints = 0);
   static StringList suffixesSys(const String &fullVersion);
   static StringList prefixesSys();
   
   Pointer<Object> m_inst;
   PdkPluginInstanceFunc m_instance;
   JsonObject m_metaData;
   
   String m_errorString;
   
   void updatePluginState();
   bool isPlugin();
   
private:
   explicit LibraryPrivate(const String &canonicalFileName, const String &version, Library::LoadHints loadHints);
   ~LibraryPrivate();
   void mergeLoadHints(Library::LoadHints loadHints);
   bool loadSys();
   bool unloadSys();
   FuncPointer resolveSys(const char *);
   AtomicInt m_loadHintsInt;
   /// counts how many Library or PluginLoader are attached to us, plus 1 if it's loaded
   AtomicInt m_libraryRefCount;
   /// counts how many times load() or loadPlugin() were called
   AtomicInt m_libraryUnloadCount;
   enum { IsAPlugin, IsNotAPlugin, MightBeAPlugin } m_pluginState;
   friend class LibraryStore;
};

} // internal
} // dll
} // pdk

#endif // PDK_DLL_INTERNAL_LIBRARY_PRIVATE_H
