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

#ifndef PDK_M_BASE_IO_FS_DIR_ITERATOR_H
#define PDK_M_BASE_IO_FS_DIR_ITERATOR_H

#include "pdk/base/io/fs/Dir.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/base/lang/String.h"

namespace pdk {
namespace io {
namespace fs {

// forward declare class with namespace
namespace internal {
class DirIteratorPrivate;
} // iterator

using pdk::lang::String;
using internal::DirIteratorPrivate;

class PDK_CORE_EXPORT DirIterator
{
public:
   enum class IteratorFlag
   {
      NoIteratorFlags = 0x0,
      FollowSymlinks = 0x1,
      Subdirectories = 0x2
   };
   PDK_DECLARE_FLAGS(IteratorFlags, IteratorFlag);
   
   DirIterator(const Dir &dir, IteratorFlags flags = IteratorFlag::NoIteratorFlags);
   DirIterator(const String &path,
               IteratorFlags flags = IteratorFlag::NoIteratorFlags);
   DirIterator(const String &path,
               Dir::Filters filter,
               IteratorFlags flags = IteratorFlag::NoIteratorFlags);
   DirIterator(const String &path,
               const StringList &nameFilters,
               Dir::Filters filters = Dir::Filter::NoFilter,
               IteratorFlags flags = IteratorFlag::NoIteratorFlags);
   
   ~DirIterator();
   
   String next();
   bool hasNext() const;
   
   String getFileName() const;
   String getFilePath() const;
   FileInfo getFileInfo() const;
   String getPath() const;
   
private:
   PDK_DISABLE_COPY(DirIterator);
   
   pdk::utils::ScopedPointer<DirIteratorPrivate> m_implPtr;
   friend class Dir;
};

} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_DIR_ITERATOR_H
