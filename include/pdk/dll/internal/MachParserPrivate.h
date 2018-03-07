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

#ifndef PDK_DLL_INTERNAL_MACH_PARSER_PRIVATE_H
#define PDK_DLL_INTERNAL_MACH_PARSER_PRIVATE_H

#include "pdk/global/Endian.h"
#include "pdk/global/Global.h"

PDK_REQUIRE_CONFIG(library);

#if defined(PDK_OF_MACH_O)

namespace pdk {

// forward declare class with namespace
namespace lang {
class String;
} // String

namespace dll {
namespace internal {

class LibraryPrivate;
using pdk::lang::String;

class PDK_UNITTEST_EXPORT MachOParser
{
public:
   enum { 
      PdkMetaDataSection, 
      NoPdkSection, 
      NotSuitable
   };
   static int parse(const char *ms, ulong fdlen, const String &library, 
                    String *errorString, long *pos, ulong *sectionlen);
};

} // internal
} // dll
} // pdk

#endif // PDK_OF_MACH_O

#endif // PDK_DLL_INTERNAL_MACH_PARSER_PRIVATE_H
