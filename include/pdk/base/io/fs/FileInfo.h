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

#ifndef PDK_M_BASE_IO_FS_FILE_INFO_H
#define PDK_M_BASE_IO_FS_FILE_INFO_H

#include "pdk/base/io/fs/File.h"
#include "pdk/utils/SharedData.h"
#include "pdk/base/io/Debug.h"

namespace pdk {

// forward declare class with namespace
namespace time {
class DateTime;
} // time

namespace io {
namespace fs {

// forward declare class with namespace
namespace internal {
class DirIteratorPrivate;
class FileInfoPrivate;
} // internal

class Dir;
using pdk::time::DateTime;
using internal::DirIteratorPrivate;
using internal::FileInfoPrivate;

class PDK_CORE_EXPORT FileInfo
{
   friend class DirIteratorPrivate;
public:
   explicit FileInfo(FileInfoPrivate *d);
   
   FileInfo();
   FileInfo(const String &file);
   FileInfo(const File &file);
   FileInfo(const Dir &dir, const String &file);
   FileInfo(const FileInfo &fileinfo);
   ~FileInfo();
   
   FileInfo &operator=(const FileInfo &fileinfo);
   FileInfo &operator=(FileInfo &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   void swap(FileInfo &other) noexcept
   {
      std::swap(m_implPtr, other.m_implPtr);
   }
   
   bool operator==(const FileInfo &fileinfo) const;
   inline bool operator!=(const FileInfo &fileinfo) const
   {
      return !(operator==(fileinfo));
   }
   
   void setFile(const String &file);
   void setFile(const File &file);
   void setFile(const Dir &dir, const String &file);
   bool exists() const;
   static bool exists(const String &file);
   void refresh();
   
   String getFilePath() const;
   String getAbsoluteFilePath() const;
   String getCanonicalFilePath() const;
   String getFileName() const;
   String getBaseName() const;
   String getCompleteBaseName() const;
   String getSuffix() const;
   String getBundleName() const;
   String getCompleteSuffix() const;
   
   String getPath() const;
   String getAbsolutePath() const;
   String getCanonicalPath() const;
   Dir getDir() const;
   Dir getAbsoluteDir() const;
   
   bool isReadable() const;
   bool isWritable() const;
   bool isExecutable() const;
   bool isHidden() const;
   bool isNativePath() const;
   
   bool isRelative() const;
   inline bool isAbsolute() const
   {
      return !isRelative();
   }
   
   bool makeAbsolute();
   
   bool isFile() const;
   bool isDir() const;
   bool isSymLink() const;
   bool isRoot() const;
   
   String getSymLinkTarget() const;
   
   String getOwner() const;
   uint getOwnerId() const;
   String getGroup() const;
   uint getGroupId() const;
   
   bool permission(File::Permissions permissions) const;
   File::Permissions permissions() const;
   
   pdk::pint64 getSize() const;
   
   // @TODO review
   // ### inline these functions
   DateTime getBirthTime() const;
   DateTime getMetadataChangeTime() const;
   DateTime getLastModified() const;
   DateTime getLastRead() const;
   DateTime getFileTime(File::FileTime time) const;
   
   bool getCaching() const;
   void setCaching(bool on);
   
protected:
   pdk::utils::SharedDataPointer<FileInfoPrivate> m_implPtr;
   
private:
   FileInfoPrivate* getImplPtr();
   inline const FileInfoPrivate* getImplPtr() const
   {
      return m_implPtr.constData();
   }
};

using FileInfoList = std::list<FileInfo>;

#ifndef PDK_NO_DEBUG_STREAM
PDK_CORE_EXPORT Debug operator<<(Debug, const FileInfo &);
#endif

} // fs
} // io
} // pdk

PDK_DECLARE_SHARED(pdk::io::fs::FileInfo)

#endif // PDK_M_BASE_IO_FS_FILE_INFO_H
