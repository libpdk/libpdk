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

#include "pdk/dll/internal/ElfParserPrivate.h"

//#if defined (PDK_OF_ELF) && defined(PDK_CC_GNU)

#include "pdk/dll/internal/LibraryPrivate.h"
#include "pdk/dll/Library.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/lang/String.h"
#include "pdk/global/Logging.h"
#include "pdk/base/ds/ByteArray.h"

namespace pdk {
namespace dll {
namespace internal {

using pdk::dll::Library;
using pdk::ds::ByteArray;

const char *ElfParser::parseSectionHeader(const char *data, ElfSectionHeader *sh)
{
   sh->m_name = read<pelfword_t>(data);
   data += sizeof(pelfword_t); // sh_name
   sh->m_type = read<pelfword_t>(data);
   data += sizeof(pelfword_t)  // sh_type
         + sizeof(pelfaddr_t)   // sh_flags
         + sizeof(pelfaddr_t);  // sh_addr
   sh->m_offset = read<pelfoff_t>(data);
   data += sizeof(pelfoff_t);  // sh_offset
   sh->m_size = read<pelfoff_t>(data);
   data += sizeof(pelfoff_t);  // sh_size
   return data;
}

int ElfParser::parse(const char *dataStart, ulong fdlen, const String &library, LibraryPrivate *lib, long *pos, ulong *sectionlen)
{
#if defined(ELF_PARSER_DEBUG)
   debug_stream() << "ElfParser::parse " << library;
#endif
   if (fdlen < 64){
      if (lib) {
         lib->m_errorString = Library::tr("'%1' is not an ELF object (%2)").arg(library, Library::tr("file too small"));
      }
      return NotElf;
   }
   const char *data = dataStart;
   if (pdk::strncmp(data, "\177ELF", 4) != 0) {
      if (lib) {
         lib->m_errorString = Library::tr("'%1' is not an ELF object").arg(library);
      }
      return NotElf;
   }
   // 32 or 64 bit
   if (data[4] != 1 && data[4] != 2) {
      if (lib) {
         lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)").arg(library, Library::tr("odd cpu architecture"));
      }
      return Corrupt;
   }
   m_bits = (data[4] << 5);
   /*  If you remove this check, to read ELF objects of a different arch, please make sure you modify the typedefs
        to match the _plugin_ architecture.
    */
   if ((sizeof(void*) == 4 && m_bits != 32) || (sizeof(void*) == 8 && m_bits != 64)) {
      if (lib) {
         lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)").arg(library, Library::tr("wrong cpu architecture"));
      }
      return Corrupt;
   }
   // endian
   if (data[5] == 0) {
      if (lib) {
         lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)").arg(library, Library::tr("odd endianness"));
      }
      return Corrupt;
   }
   m_endian = (data[5] == 1 ? ElfLittleEndian : ElfBigEndian);
   
   data += 16                  // e_ident
         +  sizeof(pelfhalf_t)  // e_type
         +  sizeof(pelfhalf_t)  // e_machine
         +  sizeof(pelfword_t)  // e_version
         +  sizeof(pelfaddr_t)  // e_entry
         +  sizeof(pelfoff_t);  // e_phoff
   
   pelfoff_t e_shoff = read<pelfoff_t> (data);
   data += sizeof(pelfoff_t)    // e_shoff
         +  sizeof(pelfword_t);  // e_flags
   
   pelfhalf_t e_shsize = read<pelfhalf_t> (data);
   
   if (e_shsize > fdlen) {
      if (lib) {
         lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)").arg(library, Library::tr("unexpected e_shsize"));
      }
      return Corrupt;
   }
   
   data += sizeof(pelfhalf_t)  // e_ehsize
         +  sizeof(pelfhalf_t)  // e_phentsize
         +  sizeof(pelfhalf_t); // e_phnum
   
   pelfhalf_t e_shentsize = read<pelfhalf_t> (data);
   
   if (e_shentsize % 4){
      if (lib) {
         lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)").arg(library, Library::tr("unexpected e_shentsize"));
      }
      return Corrupt;
   }
   data += sizeof(pelfhalf_t); // e_shentsize
   pelfhalf_t e_shnum     = read<pelfhalf_t> (data);
   data += sizeof(pelfhalf_t); // e_shnum
   pelfhalf_t e_shtrndx   = read<pelfhalf_t> (data);
   data += sizeof(pelfhalf_t); // e_shtrndx
   
   if ((pdk::puint32)(e_shnum * e_shentsize) > fdlen) {
      if (lib) {
         const String message =
               Library::tr("announced %n section(s), each %1 byte(s), exceed file size",
                           nullptr, int(e_shnum)).arg(e_shentsize);
         lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)").arg(library, message);
      }
      return Corrupt;
   }
   
#if defined(ELF_PARSER_DEBUG)
   debug_stream() << e_shnum << "sections starting at " << ("0x" + ByteArray::number(e_shoff, 16)).data() << "each" << e_shentsize << "bytes";
#endif
   
   ElfSectionHeader strtab;
   pdk::pulonglong soff = e_shoff + pelfword_t(e_shentsize) * pelfword_t(e_shtrndx);
   
   if ((soff + e_shentsize) > fdlen || soff % 4 || soff == 0) {
      if (lib) {
         lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)")
               .arg(library, Library::tr("shstrtab section header seems to be at %1")
                    .arg(String::number(soff, 16)));
      }
      return Corrupt;
   }
   
   parseSectionHeader(dataStart + soff, &strtab);
   m_stringTableFileOffset = strtab.m_offset;
   
   if ((pdk::puint32)(m_stringTableFileOffset + e_shentsize) >= fdlen || m_stringTableFileOffset == 0) {
      if (lib) {
         lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)")
               .arg(library, Library::tr("string table seems to be at %1")
                    .arg(String::number(soff, 16)));
      }
      return Corrupt;
   }
#if defined(ELF_PARSER_DEBUG)
   debug_stream(".shstrtab at 0x%s", ByteArray::number(m_stringTableFileOffset, 16).data());
#endif
   
   const char *s = dataStart + e_shoff;
   for (int i = 0; i < e_shnum; ++i) {
      ElfSectionHeader sh;
      parseSectionHeader(s, &sh);
      if (sh.m_name == 0) {
         s += e_shentsize;
         continue;
      }
      const char *shnam = dataStart + m_stringTableFileOffset + sh.m_name;
      
      if (m_stringTableFileOffset + sh.m_name > fdlen) {
         if (lib)
            lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)")
                  .arg(library, Library::tr("section name %1 of %2 behind end of file")
                       .arg(i).arg(e_shnum));
         return Corrupt;
      }
      
#if defined(ELF_PARSER_DEBUG)
      debug_stream() << "++++" << i << shnam;
#endif
      
      if (pdk::strcmp(shnam, ".pdkmetadata") == 0 || pdk::strcmp(shnam, ".rodata") == 0) {
         if (!(sh.m_type & 0x1)) {
            if (shnam[1] == 'r') {
               if (lib) {
                  lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)")
                        .arg(library, Library::tr("empty .rodata. not a library."));
               }
               return Corrupt;
            }
#if defined(ELF_PARSER_DEBUG)
            debug_stream()<<"section is not program data. skipped.";
#endif
            s += e_shentsize;
            continue;
         }
         if (sh.m_offset == 0 || (sh.m_offset + sh.m_size) > fdlen || sh.m_size < 1) {
            if (lib)
               lib->m_errorString = Library::tr("'%1' is an invalid ELF object (%2)")
                     .arg(library, Library::tr("missing section data. This is not a library."));
            return Corrupt;
         }
         *pos = sh.m_offset;
         *sectionlen = sh.m_size - 1;
         if (shnam[1] == 'q')
            return PdkMetaDataSection;
      }
      s += e_shentsize;
   }
   return NoPdkSection;
}

} // internal
} // dll
} // pdk

//#endif // defined(PDK_OF_ELF) && defined(PDK_CC_GNU)
