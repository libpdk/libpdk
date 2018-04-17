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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_FILESYSTEM_ENGINE_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_FILESYSTEM_ENGINE_PRIVATE_H

#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemMetaDataPrivate.h"

namespace pdk {

// forward declare class with namespace
namespace kernel {
namespace internal {
class SystemError;
} // internal
} // kernel

// forward declare class with namespace
namespace time {
class DateTime;
} // time

namespace io {
namespace fs {
namespace internal {

using pdk::kernel::internal::SystemError;
using pdk::time::DateTime;

class FileSystemEngine
{
public:
   static bool isCaseSensitive()
   {
#ifndef PDK_OS_WIN
      return true;
#else
      return false;
#endif
   }
   
   static FileSystemEntry getLinkTarget(const FileSystemEntry &link, FileSystemMetaData &data);
   static FileSystemEntry getCanonicalName(const FileSystemEntry &entry, FileSystemMetaData &data);
   static FileSystemEntry getAbsoluteName(const FileSystemEntry &entry);
   static ByteArray getId(const FileSystemEntry &entry);
   static String resolveUserName(const FileSystemEntry &entry, FileSystemMetaData &data);
   static String resolveGroupName(const FileSystemEntry &entry, FileSystemMetaData &data);
   
#if defined(PDK_OS_UNIX)
   static String resolveUserName(uint userId);
   static String resolveGroupName(uint groupId);
#endif
#if defined(PDK_OS_DARWIN)
    static String getBundleName(const FileSystemEntry &entry);
#else
    static String getBundleName(const FileSystemEntry &entry)
    {
       PDK_UNUSED(entry);
       return String();
    }
#endif
   static bool fillMetaData(const FileSystemEntry &entry, FileSystemMetaData &data,
                            FileSystemMetaData::MetaDataFlags what);
#if defined(PDK_OS_UNIX)
   static bool cloneFile(int srcfd, int dstfd, const FileSystemMetaData &knownData);
   static bool fillMetaData(int fd, FileSystemMetaData &data); // what = PosixStatFlags
   static ByteArray getId(int fd);
   static bool setFileTime(int fd, const DateTime &newDate,
                           AbstractFileEngine::FileTime whatTime, SystemError &error);
   static bool setPermissions(int fd, File::Permissions permissions, SystemError &error,
                              FileSystemMetaData *data = nullptr);
#endif
#if defined(PDK_OS_WIN)
   
   static bool uncListSharesOnServer(const String &server, StringList *list); //Used also by FileEngineIterator::hasNext()
   static bool fillMetaData(int fd, FileSystemMetaData &data,
                            FileSystemMetaData::MetaDataFlags what);
   static bool fillMetaData(HANDLE fHandle, FileSystemMetaData &data,
                            FileSystemMetaData::MetaDataFlags what);
   static bool fillPermissions(const FileSystemEntry &entry, FileSystemMetaData &data,
                               FileSystemMetaData::MetaDataFlags what);
   static ByteArray id(HANDLE fHandle);
   static bool setFileTime(HANDLE fHandle, const DateTime &newDate,
                           AbstractFileEngine::FileTime whatTime, SystemError &error);
   static String owner(const FileSystemEntry &entry, AbstractFileEngine::FileOwner own);
   static String nativeAbsoluteFilePath(const String &path);
#endif
   //homePath, rootPath and tempPath shall return clean paths
   static String getHomePath();
   static String getRootPath();
   static String getTempPath();
   
   static bool createDirectory(const FileSystemEntry &entry, bool createParents);
   static bool removeDirectory(const FileSystemEntry &entry, bool removeEmptyParents);
   
   static bool createLink(const FileSystemEntry &source, const FileSystemEntry &target, SystemError &error);
   
   static bool copyFile(const FileSystemEntry &source, const FileSystemEntry &target, SystemError &error);
   static bool renameFile(const FileSystemEntry &source, const FileSystemEntry &target, SystemError &error);
   static bool renameOverwriteFile(const FileSystemEntry &source, const FileSystemEntry &target, SystemError &error);
   static bool removeFile(const FileSystemEntry &entry, SystemError &error);
   
   static bool setPermissions(const FileSystemEntry &entry, File::Permissions permissions, SystemError &error,
                              FileSystemMetaData *data = nullptr);
   
   // unused, therefore not implemented
   static bool setFileTime(const FileSystemEntry &entry, const DateTime &newDate,
                           AbstractFileEngine::FileTime whatTime, SystemError &error);
   
   static bool setCurrentPath(const FileSystemEntry &entry);
   static FileSystemEntry getCurrentPath();
   
   static AbstractFileEngine *resolveEntryAndCreateLegacyEngine(FileSystemEntry &entry,
                                                                FileSystemMetaData &data);
private:
   static String slowCanonicalized(const String &path);
#if defined(PDK_OS_WIN)
   static void clearWinStatData(FileSystemMetaData &data);
#endif
};

} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_FILESYSTEM_ENGINE_PRIVATE_H
