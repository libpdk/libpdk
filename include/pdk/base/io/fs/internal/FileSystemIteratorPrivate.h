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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_FILE_SYSTEM_ITERATOR_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_FILE_SYSTEM_ITERATOR_PRIVATE_H

#ifndef PDK_NO_FILESYSTEMITERATOR

#include "pdk/global/Global.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/DirIterator.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemMetaDataPrivate.h"
#include "pdk/base/ds/StringList.h"

// Platform-specific headers
#if !defined(PDK_OS_WIN)
#include "pdk/utils/ScopedPointer.h"
#endif

namespace pdk {
namespace io {
namespace fs {
namespace internal {

class FileSystemIterator
{
public:
   FileSystemIterator(const FileSystemEntry &entry, Dir::Filters filters,
                      const StringList &nameFilters, DirIterator::IteratorFlags flags
                      = DirIterator::FollowSymlinks | DirIterator::Subdirectories);
   ~FileSystemIterator();
   
   bool advance(FileSystemEntry &fileEntry, FileSystemMetaData &metaData);
   
private:
   FileSystemEntry::NativePath m_nativePath;
   
   // Platform-specific data
#if defined(PDK_OS_WIN)
   String m_dirPath;
   HANDLE m_findFileHandle;
   StringList m_uncShares;
   bool m_uncFallback;
   int m_uncShareIndex;
   bool m_onlyDirs;
#else
   PDK_DIR *m_dir;
   PDK_DIRENT *m_dirEntry;
   int m_lastError;
#endif
   
   PDK_DISABLE_COPY(FileSystemIterator);
};

} // internal
} // fs
} // io
} // pdk

#endif // PDK_NO_FILESYSTEMITERATOR

#endif // PDK_M_BASE_IO_FS_INTERNAL_FILE_SYSTEM_ITERATOR_PRIVATE_H
