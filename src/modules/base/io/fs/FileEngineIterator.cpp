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

#include "pdk/base/io/fs/internal/FileEngineIteratorPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemIteratorPrivate.h"
#include "pdk/base/io/fs/internal/FileInfoPrivate.h"

#ifndef PDK_NO_FILESYSTEMITERATOR

namespace pdk {
namespace io {
namespace fs {
namespace internal {

FileEngineIterator::FileEngineIterator(Dir::Filters filters, const StringList &filterNames)
   : AbstractFileEngineIterator(filters, filterNames),
     m_done(false)
{
}

FileEngineIterator::~FileEngineIterator()
{
}

bool FileEngineIterator::hasNext() const
{
   if (!m_done && !m_nativeIterator) {
      m_nativeIterator.reset(new FileSystemIterator(FileSystemEntry(path()),
                                                  filters(), nameFilters()));
      advance();
   }
   return !m_done;
}

String FileEngineIterator::next()
{
   if (!hasNext()) {
      return String();
   }
   advance();
   return currentFilePath();
}

void FileEngineIterator::advance() const
{
   m_currentInfo = m_nextInfo;
   FileSystemEntry entry;
   FileSystemMetaData data;
   if (m_nativeIterator->advance(entry, data)) {
      m_nextInfo = FileInfo(new FileInfoPrivate(entry, data));
   } else {
      m_done = true;
      m_nativeIterator.reset();
   }
}

String FileEngineIterator::currentFileName() const
{
   return m_currentInfo.getFileName();
}

FileInfo FileEngineIterator::currentFileInfo() const
{
   return m_currentInfo;
}

} // internal
} // fs
} // io
} // pdk

#endif // PDK_NO_FILESYSTEMITERATOR
