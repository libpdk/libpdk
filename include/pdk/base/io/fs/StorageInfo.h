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
// Created by softboy on 2018/04/16.

#ifndef PDK_M_BASE_IO_FS_STORAGE_INFO_H
#define PDK_M_BASE_IO_FS_STORAGE_INFO_H

#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/lang/String.h"
#include "pdk/utils/SharedData.h"

namespace pdk {
namespace io {

// forwward declare class
class Debug;

namespace fs {

// forward declare class with namespace
namespace internal {
class StorageInfoPrivate;
} // internal

using pdk::io::Debug;
using internal::StorageInfoPrivate;
using pdk::utils::ExplicitlySharedDataPointer;

class PDK_CORE_EXPORT StorageInfo
{
public:
   StorageInfo();
   explicit StorageInfo(const String &path);
   explicit StorageInfo(const Dir &dir);
   StorageInfo(const StorageInfo &other);
   ~StorageInfo();
   
   StorageInfo &operator=(const StorageInfo &other);
   StorageInfo &operator=(StorageInfo &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   inline void swap(StorageInfo &other) noexcept
   {
      std::swap(m_implPtr, other.m_implPtr);
   }
   
   void setPath(const String &path);
   
   String getRootPath() const;
   ByteArray getDevice() const;
   ByteArray getSubvolume() const;
   ByteArray getFileSystemType() const;
   String getName() const;
   String getDisplayName() const;
   
   pdk::pint64 getBytesTotal() const;
   pdk::pint64 getBytesFree() const;
   pdk::pint64 getBytesAvailable() const;
   int getBlockSize() const;
   
   inline bool isRoot() const;
   bool isReadOnly() const;
   bool isReady() const;
   bool isValid() const;
   
   void refresh();
   
   static std::list<StorageInfo> getMountedVolumes();
   static StorageInfo getRoot();
   
private:
   friend class StorageInfoPrivate;
   friend bool operator==(const StorageInfo &first, const StorageInfo &second);
   friend PDK_CORE_EXPORT Debug operator<<(Debug, const StorageInfo &);
   ExplicitlySharedDataPointer<StorageInfoPrivate> m_implPtr;
};

inline bool operator==(const StorageInfo &lhs, const StorageInfo &rhs)
{
   if (lhs.m_implPtr == rhs.m_implPtr) {
      return true;
   }
   
   return lhs.getDevice() == rhs.getDevice() && lhs.getRootPath() == rhs.getRootPath();
}

inline bool operator!=(const StorageInfo &lhs, const StorageInfo &rhs)
{
   return !(lhs == rhs);
}

inline bool StorageInfo::isRoot() const
{
   return *this == StorageInfo::getRoot();
}

#ifndef PDK_NO_DEBUG_STREAM
PDK_CORE_EXPORT Debug operator<<(Debug debug, const StorageInfo &);
#endif

} // fs
} // io
} // pdk

PDK_DECLARE_SHARED(pdk::io::fs::StorageInfo)

#endif // PDK_M_BASE_IO_FS_STORAGE_INFO_H
