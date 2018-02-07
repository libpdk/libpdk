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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_FILE_ENGINE_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_FILE_ENGINE_PRIVATE_H

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemMetaDataPrivate.h"
#include <map>

namespace pdk {
namespace io {
namespace fs {
namespace internal {

class FileEnginePrivate;
class PDK_CORE_EXPORT FileEngine : public AbstractFileEngine
{
   PDK_DECLARE_PRIVATE(FileEngine);
public:
   FileEngine();
   explicit FileEngine(const String &file);
   ~FileEngine();
   
   bool open(IoDevice::OpenModes openMode) override;
   bool open(IoDevice::OpenModes flags, FILE *fh);
   bool close() override;
   bool flush() override;
   bool syncToDisk() override;
   pdk::pint64 getSize() const override;
   pdk::pint64 getPosition() const override;
   bool seek(pdk::pint64) override;
   bool isSequential() const override;
   bool remove() override;
   bool copy(const String &newName) override;
   bool rename(const String &newName) override;
   bool renameOverwrite(const String &newName) override;
   bool link(const String &newName) override;
   bool mkdir(const String &dirName, bool createParentDirectories) const override;
   bool rmdir(const String &dirName, bool recurseParentDirectories) const override;
   bool setSize(pdk::pint64 size) override;
   bool caseSensitive() const override;
   bool isRelativePath() const override;
   StringList entryList(Dir::Filters filters, const StringList &filterNames) const override;
   FileFlags fileFlags(FileFlags type) const override;
   bool setPermissions(uint perms) override;
   ByteArray id() const override;
   String fileName(FileName file) const override;
   uint ownerId(FileOwner) const override;
   String owner(FileOwner) const override;
   bool setFileTime(const DateTime &newDate, FileTime time) override;
   DateTime fileTime(FileTime time) const override;
   void setFileName(const String &file) override;
   int handle() const override;
   
#ifndef PDK_NO_FILESYSTEMITERATOR
   Iterator *beginEntryList(Dir::Filters filters, const StringList &filterNames) override;
   Iterator *endEntryList() override;
#endif
   
   pdk::pint64 read(char *data, pdk::pint64 maxlen) override;
   pdk::pint64 readLine(char *data, pdk::pint64 maxlen) override;
   pdk::pint64 write(const char *data, pdk::pint64 len) override;
   bool cloneTo(AbstractFileEngine *target) override;
   
   virtual bool isUnnamedFile() const
   { return false; }
   
   bool extension(Extension extension, const ExtensionOption *option = 0, ExtensionReturn *output = 0) override;
   bool supportsExtension(Extension extension) const override;
   
   //FS only!!
   bool open(IoDevice::OpenModes flags, int fd);
   bool open(IoDevice::OpenModes flags, int fd, File::FileHandleFlags handleFlags);
   bool open(IoDevice::OpenModes flags, FILE *fh, File::FileHandleFlags handleFlags);
   static bool setCurrentPath(const String &path);
   static String getCurrentPath(const String &path = String());
   static String getHomePath();
   static String getRootPath();
   static String getTempPath();
   static FileInfoList getDrives();
   
protected:
   FileEngine(FileEnginePrivate &dd);
};

class PDK_UNITTEST_EXPORT FileEnginePrivate : public AbstractFileEnginePrivate
{
   PDK_DECLARE_PUBLIC(FileEngine);
   
public:
#ifdef PDK_OS_WIN
   static String longFileName(const String &path);
#endif
   
   FileSystemEntry m_fileEntry;
   IoDevice::OpenModes m_openMode;
   
   bool nativeOpen(IoDevice::OpenModes openMode);
   bool openFh(IoDevice::OpenModes flags, FILE *fh);
   bool openFd(IoDevice::OpenModes flags, int fd);
   bool nativeClose();
   bool closeFdFh();
   bool nativeFlush();
   bool nativeSyncToDisk();
   bool flushFh();
   pdk::pint64 nativeSize() const;
#ifndef PDK_OS_WIN
   pdk::pint64 sizeFdFh() const;
#endif
   pdk::pint64 getNativePos() const;
   pdk::pint64 getPosFdFh() const;
   bool nativeSeek(pdk::pint64);
   bool seekFdFh(pdk::pint64);
   pdk::pint64 nativeRead(char *data, pdk::pint64 maxlen);
   pdk::pint64 readFdFh(char *data, pdk::pint64 maxlen);
   pdk::pint64 nativeReadLine(char *data, pdk::pint64 maxlen);
   pdk::pint64 readLineFdFh(char *data, pdk::pint64 maxlen);
   pdk::pint64 nativeWrite(const char *data, pdk::pint64 len);
   pdk::pint64 writeFdFh(const char *data, pdk::pint64 len);
   int getNativeHandle() const;
   bool getNativeIsSequential() const;
#ifndef PDK_OS_WIN
   bool isSequentialFdFh() const;
#endif
   
   uchar *map(pdk::pint64 offset, pdk::pint64 size, File::MemoryMapFlags flags);
   bool unmap(uchar *ptr);
   void unmapAll();
   
   mutable FileSystemMetaData m_metaData;
   
   FILE *m_fh;
   
#ifdef PDK_OS_WIN
   HANDLE m_fileHandle;
   HANDLE m_mapHandle;
   std::map<uchar *, DWORD /* offset % AllocationGranularity */> m_maps;
   
   mutable int m_cachedFd;
   mutable DWORD m_fileAttrib;
#else
   std::map<uchar *, std::pair<int /*offset % PageSize*/, size_t /*length + offset % PageSize*/>> m_maps;
#endif
   int m_fd;
   
   enum class LastIOCommand
   {
      IOFlushCommand,
      IOReadCommand,
      IOWriteCommand
   };
   LastIOCommand m_lastIOCommand;
   bool m_lastFlushFailed;
   bool m_closeFileHandle;
   
   mutable uint m_isSequential : 2;
   mutable uint m_triedStat : 1;
   mutable uint m_needLstat : 1;
   mutable uint m_isLink : 1;
   
#if defined(PDK_OS_WIN)
   bool doStat(FileSystemMetaData::MetaDataFlags flags) const;
#else
   bool doStat(FileSystemMetaData::MetaDataFlags flags = FileSystemMetaData::MetaDataFlag::PosixStatFlags) const;
#endif
   bool isSymlink() const;
   
#if defined(PDK_OS_WIN32)
   int sysOpen(const String &, int flags);
#endif
   
protected:
   FileEnginePrivate();
   
   void init();
   
   AbstractFileEngine::FileFlags getPermissions(AbstractFileEngine::FileFlags type) const;
};


} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_FILE_ENGINE_PRIVATE_H
