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

#ifndef PDK_DLL_INTERNAL_SYSTEM_LIBRARY_PRIVATE_H
#define PDK_DLL_INTERNAL_SYSTEM_LIBRARY_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/String.h"

//#ifdef PDK_OS_WIN

namespace pdk {
namespace dll {
namespace internal {

using pdk::lang::String;

class SystemLibrary
{
public:
   explicit SystemLibrary(const String &libraryName)
   {
      m_libraryName = libraryName;
      m_handle = 0;
      m_didLoad = false;
   }
   
   explicit SystemLibrary(const wchar_t *libraryName)
   {
      m_libraryName = String::fromWCharArray(libraryName);
      m_handle = 0;
      m_didLoad = false;
   }
   
   bool load(bool onlySystemDirectory = true)
   {
      m_handle = load((const wchar_t *)m_libraryName.utf16(), onlySystemDirectory);
      m_didLoad = true;
      return (m_handle != 0);
   }
   
   bool isLoaded()
   {
      return (m_handle != 0);
   }
   
   FuncPointer resolve(const char *symbol)
   {
      if (!m_didLoad) {
         load();
      }
      if (!m_handle) {
         return nullptr;
      }
      return FuncPointer(GetProcAddress(m_handle, symbol));
   }
   
   static FuncPointer resolve(const String &libraryName, const char *symbol)
   {
      return SystemLibrary(libraryName).resolve(symbol);
   }
   
   static PDK_CORE_EXPORT HINSTANCE load(const wchar_t *lpFileName, bool onlySystemDirectory = true);
private:
   HINSTANCE m_handle;
   String m_libraryName;
   bool m_didLoad;
};

} // internal
} // dll
} // pdk

//#endif // PDK_OS_WIN

#endif // PDK_DLL_INTERNAL_SYSTEM_LIBRARY_PRIVATE_H
