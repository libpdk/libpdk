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

#ifndef PDK_DLL_LIBRARY_H
#define PDK_DLL_LIBRARY_H

#include "pdk/kernel/Object.h"

PDK_REQUIRE_CONFIG(library);

namespace pdk {

// forward declare class with namespace
namespace lang {
class String;
} // String

namespace dll {

// forward declare class with namespace
namespace internal {
class LibraryPrivate;
} // internal

using pdk::kernel::Object;
using internal::LibraryPrivate;
using pdk::lang::String;

class PDK_CORE_EXPORT Library : public Object
{
public:
   enum class LoadHint
   {
      ResolveAllSymbolsHint = 0x01,
      ExportExternalSymbolsHint = 0x02,
      LoadArchiveMemberHint = 0x04,
      PreventUnloadHint = 0x08,
      DeepBindHint = 0x10
   };
   PDK_DECLARE_FLAGS(LoadHints, LoadHint);
   
   explicit Library(Object *parent = nullptr);
   explicit Library(const String& fileName, Object *parent = nullptr);
   explicit Library(const String& fileName, int verNum, Object *parent = nullptr);
   explicit Library(const String& fileName, const String &version, Object *parent = nullptr);
   ~Library();
   
   pdk::FuncPointer resolve(const char *symbol);
   static pdk::FuncPointer resolve(const String &fileName, const char *symbol);
   static pdk::FuncPointer resolve(const String &fileName, int verNum, const char *symbol);
   static pdk::FuncPointer resolve(const String &fileName, const String &version, const char *symbol);
   
   bool load();
   bool unload();
   bool isLoaded() const;
   
   static bool isLibrary(const String &fileName);
   
   void setFileName(const String &fileName);
   String getFileName() const;
   
   void setFileNameAndVersion(const String &fileName, int verNum);
   void setFileNameAndVersion(const String &fileName, const String &version);
   String getErrorString() const;
   
   void setLoadHints(LoadHints hints);
   LoadHints getLoadHints() const;
private:
   LibraryPrivate *m_implPtr;
   bool m_didLoad;
   PDK_DISABLE_COPY(Library);
};

} // dll
} // pdk

PDK_DECLARE_OPERATORS_FOR_FLAGS(pdk::dll::Library::LoadHints)

#endif // PDK_DLL_LIBRARY_H
