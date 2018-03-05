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

#ifndef PDK_DLL_INTERNAL_ELF_PARSER_PRIVATE_H
#define PDK_DLL_INTERNAL_ELF_PARSER_PRIVATE_H

#include "pdk/global/Endian.h"
#include "pdk/global/Global.h"

PDK_REQUIRE_CONFIG(library);

//#if defined (PDK_OF_ELF) && defined(PDK_CC_GNU)

namespace pdk {

// forward declare class with namespace
namespace lang {
class String;
} // lang

namespace dll {
namespace internal {

using pdk::lang::String;
class LibraryPrivate;

typedef pdk::puint16  pelfhalf_t;
typedef pdk::puint32  pelfword_t;
typedef pdk::uintptr pelfoff_t;
typedef pdk::uintptr pelfaddr_t;

class ElfParser
{
public:
   enum { PdkMetaDataSection, NoPdkSection, NotElf, Corrupt };
   enum {ElfLittleEndian = 0, ElfBigEndian = 1};
   
   struct ElfSectionHeader
   {
      pelfword_t m_name;
      pelfword_t m_type;
      pelfoff_t  m_offset;
      pelfoff_t  m_size;
   };
   
   int m_endian;
   int m_bits;
   int m_stringTableFileOffset;
   
   template <typename T>
   T read(const char *s)
   {
      if (m_endian == ElfBigEndian) {
         return pdk::from_big_endian<T>(s);
      } else {
         return pdk::from_little_endian<T>(s);
      }
   }
   
   const char *parseSectionHeader(const char* s, ElfSectionHeader *sh);
   int parse(const char *m_s, ulong fdlen, const String &library, LibraryPrivate *lib, long *pos, ulong *sectionlen);
};

} // internal
} // dll
} // pdk

//#endif // defined(PDK_OF_ELF) && defined(PDK_CC_GNU)

#endif // PDK_DLL_INTERNAL_ELF_PARSER_PRIVATE_H
