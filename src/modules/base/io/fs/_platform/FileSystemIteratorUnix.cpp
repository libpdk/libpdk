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
// Created by softboy on 2018/02/07.

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/io/fs/internal/FileSystemIteratorPrivate.h"

#ifndef PDK_NO_FILESYSTEMITERATOR

#include <cstdlib>
#include <cerrno>

namespace pdk {
namespace io {
namespace fs {
namespace internal {

FileSystemIterator::FileSystemIterator(const FileSystemEntry &entry, Dir::Filters filters,
                                       const StringList &nameFilters, DirIterator::IteratorFlags flags)
   : m_nativePath(entry.getNativeFilePath()),
     m_dir(0),
     m_dirEntry(0),
     m_lastError(0)
{
   PDK_UNUSED(filters);
   PDK_UNUSED(nameFilters);
   PDK_UNUSED(flags);
   
   if ((m_dir = PDK_OPENDIR(m_nativePath.getConstRawData())) == 0) {
      m_lastError = errno;
   } else {
      if (!m_nativePath.endsWith('/')) {
         m_nativePath.append('/');
      }
   }
}

FileSystemIterator::~FileSystemIterator()
{
   if (m_dir) {
      PDK_CLOSEDIR(m_dir);
   }
}

bool FileSystemIterator::advance(FileSystemEntry &fileEntry, FileSystemMetaData &metaData)
{
   if (!m_dir) {
      return false;
   }
   m_dirEntry = PDK_READDIR(m_dir);
   if (m_dirEntry) {
      fileEntry = FileSystemEntry(m_nativePath + ByteArray(m_dirEntry->d_name), FileSystemEntry::FromNativePath());
      metaData.fillFromDirEnt(*m_dirEntry);
      return true;
   }
   m_lastError = errno;
   return false;
}

} // internal
} // fs
} // io
} // pdk

#endif // PDK_NO_FILESYSTEMITERATOR
