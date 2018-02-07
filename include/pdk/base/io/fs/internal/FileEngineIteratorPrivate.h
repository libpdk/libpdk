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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_FILE_ENGINE_ITERATOR_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_FILE_ENGINE_ITERATOR_PRIVATE_H

#ifndef PDK_NO_FILESYSTEMITERATOR

#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemIteratorPrivate.h"
#include "pdk/base/io/fs/Dir.h"

namespace pdk {
namespace io {
namespace fs {
namespace internal {

// forward declare class
class FileEngineIteratorPrivate;
class FileEngineIteratorPlatformSpecificData;

class FileEngineIterator : public AbstractFileEngineIterator
{
public:
    FileEngineIterator(Dir::Filters filters, const StringList &filterNames);
    ~FileEngineIterator();

    String next() override;
    bool hasNext() const override;

    String getCurrentFileName() const override;
    FileInfo getCurrentFileInfo() const override;

private:
    void advance() const;
    mutable pdk::utils::ScopedPointer<FileSystemIterator> m_nativeIterator;
    mutable FileInfo m_currentInfo;
    mutable FileInfo m_nextInfo;
    mutable bool m_done;
};

} // internal
} // fs
} // io
} // pdk

#endif // PDK_NO_FILESYSTEMITERATOR

#endif // PDK_M_BASE_IO_FS_INTERNAL_FILE_ENGINE_ITERATOR_PRIVATE_H
