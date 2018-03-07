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

#include "pdk/dll/internal/MachParserPrivate.h"

#if defined(PDK_OF_MACH_O)

#include "pdk/global/Endian.h"
#include "pdk/dll/internal/LibraryPrivate.h"

#include <mach-o/loader.h>
#include <mach-o/fat.h>

namespace pdk {
namespace dll {
namespace internal {

using pdk::lang::Latin1Character;

#if defined(PDK_PROCESSOR_X86_64)
#  define MACHO64
static const cpu_type_t sg_myCpuType = CPU_TYPE_X86_64;
#elif defined(PDK_PROCESSOR_X86_32)
static const cpu_type_t sg_myCpuType = CPU_TYPE_X86;
#elif defined(PDK_PROCESSOR_POWER_64)
#  define MACHO64
static const cpu_type_t sg_myCpuType = CPU_TYPE_POWERPC64;
#elif defined(PDK_PROCESSOR_POWER_32)
static const cpu_type_t sg_myCpuType = CPU_TYPE_POWERPC;
#elif defined(PDK_PROCESSOR_ARM_64)
#  define MACHO64
static const cpu_type_t sg_myCpuType = CPU_TYPE_ARM64;
#elif defined(PDK_PROCESSOR_ARM)
static const cpu_type_t sg_myCpuType = CPU_TYPE_ARM;
#else
#  error "Unknown CPU type"
#endif

#ifdef MACHO64
#  undef MACHO64
using my_mach_header = mach_header_64;
using my_segment_command = segment_command_64;
using my_section = section_64;
static const uint32_t sg_myMagic = MH_MAGIC_64;
#else
using my_mach_header = mach_header;
using my_segment_command = segment_command;
typedef section my_section;
static const uint32_t sg_myMagic = MH_MAGIC;
#endif

namespace {

int ns(const String &reason, const String &library, String *errorString)
{
   if (errorString) {
      *errorString = Library::tr("'%1' is not a valid Mach-O binary (%2)")
            .arg(library, reason.isEmpty() ? Library::tr("file is corrupt") : reason);
   }
   return MachOParser::NotSuitable;
}

} // anonymous namespace

int MachOParser::parse(const char *ms, ulong fdlen, const String &library, String *errorString, long *pos, ulong *sectionlen)
{
   // The minimum size of a Mach-O binary we're interested in.
   // It must have a full Mach header, at least one segment and at least one
   // section. It's probably useless with just the "qtmetadata" section, but
   // it's valid nonetheless.
   // A fat binary must have this plus the fat header, of course.
   static const size_t MinFileSize = sizeof(my_mach_header) + sizeof(my_segment_command) + sizeof(my_section);
   static const size_t MinFatHeaderSize = sizeof(fat_header) + 2 * sizeof(fat_arch);
   if (PDK_UNLIKELY(fdlen < MinFileSize)) {
      return ns(Library::tr("file too small"), library, errorString);
   }
   // find out if this is a fat Mach-O binary first
   const my_mach_header *header = 0;
   const fat_header *fat = reinterpret_cast<const fat_header *>(ms);
   if (fat->magic == pdk::to_big_endian(FAT_MAGIC)) {
      // find our architecture in the binary
      const fat_arch *arch = reinterpret_cast<const fat_arch *>(fat + 1);
      if (PDK_UNLIKELY(fdlen < MinFatHeaderSize)) {
         return ns(Library::tr("file too small"), library, errorString);
      }
      int count = pdk::from_big_endian(fat->nfat_arch);
      if (PDK_UNLIKELY(fdlen < sizeof(*fat) + sizeof(*arch) * count)) {
         return ns(String(), library, errorString);
      }
      for (int i = 0; i < count; ++i) {
         if (arch[i].cputype == pdk::to_big_endian(sg_myCpuType)) {
            // ### should we check the CPU subtype? Maybe on ARM?
            uint32_t size = pdk::from_big_endian(arch[i].size);
            uint32_t offset = pdk::from_big_endian(arch[i].offset);
            if (PDK_UNLIKELY(size > fdlen) || PDK_UNLIKELY(offset > fdlen)
                || PDK_UNLIKELY(size + offset > fdlen) || PDK_UNLIKELY(size < MinFileSize)) {
               return ns(String(), library, errorString);
            }
            header = reinterpret_cast<const my_mach_header *>(ms + offset);
            fdlen = size;
            break;
         }
      }
      if (!header) {
         return ns(Library::tr("no suitable architecture in fat binary"), library, errorString);
      }
      // check the magic again
      if (PDK_UNLIKELY(header->magic != sg_myMagic)) {
         return ns(String(), library, errorString);
      }
   } else {
      header = reinterpret_cast<const my_mach_header *>(ms);
      fat = 0;
      // check magic
      if (header->magic != sg_myMagic)
         return ns(Library::tr("invalid magic %1").arg(pdk::from_big_endian(header->magic), 8, 16, Latin1Character('0')),
                   library, errorString);
   }
   
   // from this point on, fdlen is specific to this architecture
   // from this point on, everything is in host byte order
   *pos = reinterpret_cast<const char *>(header) - ms;
   // (re-)check the CPU type
   // ### should we check the CPU subtype? Maybe on ARM?
   if (header->cputype != sg_myCpuType) {
      if (fat) {
         return ns(String(), library, errorString);
      }
      return ns(Library::tr("wrong architecture"), library, errorString);
   }
   // check the file type
   if (PDK_UNLIKELY(header->filetype != MH_BUNDLE && header->filetype != MH_DYLIB))
      return ns(Library::tr("not a dynamic library"), library, errorString);
   
   // find the __TEXT segment, "qtmetadata" section
   const my_segment_command *seg = reinterpret_cast<const my_segment_command *>(header + 1);
   ulong minsize = sizeof(*header);
   for (uint i = 0; i < header->ncmds; ++i,
        seg = reinterpret_cast<const my_segment_command *>(reinterpret_cast<const char *>(seg) + seg->cmdsize)) {
      // We're sure that the file size includes at least one load command
      // but we have to check anyway if we're past the first
      if (PDK_UNLIKELY(fdlen < minsize + sizeof(load_command))) {
         return ns(String(), library, errorString);
      }
      // cmdsize can't be trusted until validated
      // so check it against fdlen anyway
      // (these are unsigned operations, with overflow behavior specified in the standard)
      minsize += seg->cmdsize;
      if (PDK_UNLIKELY(fdlen < minsize) || PDK_UNLIKELY(fdlen < seg->cmdsize)) {
         return ns(String(), library, errorString);
      }
      const uint32_t MyLoadCommand = sizeof(void *) > 4 ? LC_SEGMENT_64 : LC_SEGMENT;
      if (seg->cmd != MyLoadCommand) {
         continue;
      }
      // is this the __TEXT segment?
      if (strcmp(seg->segname, "__TEXT") == 0) {
         const my_section *sect = reinterpret_cast<const my_section *>(seg + 1);
         for (uint j = 0; j < seg->nsects; ++j) {
            // is this the "pdkmetadata" section?
            if (strcmp(sect[j].sectname, "pdkmetadata") != 0) {
               continue;
            }
            // found it!
            if (PDK_UNLIKELY(fdlen < sect[j].offset) || PDK_UNLIKELY(fdlen < sect[j].size)
                || PDK_UNLIKELY(fdlen < sect[j].offset + sect[j].size)) {
               return ns(String(), library, errorString);
            }
            *pos += sect[j].offset;
            *sectionlen = sect[j].size;
            return PdkMetaDataSection;
         }
      }
      
      // other type of segment
      seg = reinterpret_cast<const my_segment_command *>(reinterpret_cast<const char *>(seg) + seg->cmdsize);
   }
   
   //    // No pdk section was found, but at least we know that where the proper architecture's boundaries are
   //    return NoPdkSection;
   if (errorString) {
      *errorString = Library::tr("'%1' is not a Qt plugin").arg(library);
   }
   return NotSuitable;
}


} // internal
} // dll
} // pdk

#endif // PDK_OF_MACH_O
