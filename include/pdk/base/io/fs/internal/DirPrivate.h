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
// Created by softboy on 2018/02/06.

#ifndef PDK_M_BASE_IO_FS_INTERNAL_DIR_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_DIR_PRIVATE_H

#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemMetaDataPrivate.h"
#include "pdk/utils/SharedData.h"
#include "pdk/base/ds/StringList.h"

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::utils::SharedData;

class DirPrivate : public SharedData
{
public:
   explicit DirPrivate(const String &path, const StringList &nameFilters = StringList(),
                       Dir::SortFlags sort = Dir::SortFlags(pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::Name) | 
                                                            pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::IgnoreCase)),
                       Dir::Filters filters = Dir::Filter::AllEntries);
   
   explicit DirPrivate(const DirPrivate &copy);
   
   bool exists() const;
   
   void initFileEngine();
   void initFileLists(const Dir &dir) const;
   
   static void sortFileList(Dir::SortFlags, FileInfoList &, StringList *, FileInfoList *);
   static inline Character getFilterSepChar(const String &nameFilter);
   static inline StringList splitFilters(const String &nameFilter, Character sep = 0);
   void setPath(const String &path);
   void clearFileLists();
   void resolveAbsoluteEntry() const;
   
   mutable bool m_fileListsInitialized;
   mutable StringList m_files;
   mutable FileInfoList m_fileInfos;
   
   StringList m_nameFilters;
   Dir::SortFlags m_sort;
   Dir::Filters m_filters;
   
   pdk::utils::ScopedPointer<AbstractFileEngine> m_fileEngine;
   
   FileSystemEntry m_dirEntry;
   mutable FileSystemEntry m_absoluteDirEntry;
   mutable FileSystemMetaData m_metaData;
};

} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_DIR_PRIVATE_H
