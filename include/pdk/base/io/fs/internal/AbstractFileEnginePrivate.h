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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_ABSTRACT_FILE_ENGINE_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_ABSTRACT_FILE_ENGINE_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/utils/ScopedPointer.h"

#ifdef open
#error pdk/io/fs/AbstractFileEnginePrivate.h must be included before any header file that defines open
#endif

namespace pdk {

// forward declare class with namespace
namespace lang {
class String;
} // lang

// forward declare class with namespace
namespace ds {
class StringList;
} // ds

namespace io {
namespace fs {

// forward declare class
class AbstractFileEngineIterator;
class DirIterator;

namespace internal {
// forward  declare class
class AbstractFileEnginePrivate;
using pdk::lang::String;
using pdk::ds::StringList;

class PDK_CORE_EXPORT AbstractFileEngine
{
public:
   enum class FileFlag
   {
      //perms (overlaps the File::Permission)
      ReadOwnerPerm = 0x4000, WriteOwnerPerm = 0x2000, ExeOwnerPerm = 0x1000,
      ReadUserPerm  = 0x0400, WriteUserPerm  = 0x0200, ExeUserPerm  = 0x0100,
      ReadGroupPerm = 0x0040, WriteGroupPerm = 0x0020, ExeGroupPerm = 0x0010,
      ReadOtherPerm = 0x0004, WriteOtherPerm = 0x0002, ExeOtherPerm = 0x0001,
      
      //types
      LinkType      = 0x10000,
      FileType      = 0x20000,
      DirectoryType = 0x40000,
      BundleType    = 0x80000,
      
      //flags
      HiddenFlag     = 0x0100000,
      LocalDiskFlag  = 0x0200000,
      ExistsFlag     = 0x0400000,
      RootFlag       = 0x0800000,
      Refresh        = 0x1000000,
      
      //masks
      PermsMask  = 0x0000FFFF,
      TypesMask  = 0x000F0000,
      FlagsMask  = 0x0FF00000,
      FileInfoAll = FlagsMask | PermsMask | TypesMask
   };
   PDK_DECLARE_FLAGS(FileFlags, FileFlag);
   
   enum class FileName
   {
      DefaultName,
      BaseName,
      PathName,
      AbsoluteName,
      AbsolutePathName,
      LinkName,
      CanonicalName,
      CanonicalPathName,
      BundleName,
      NFileNames = 9
   };
   
   enum class FileOwner
   {
      OwnerUser,
      OwnerGroup
   };
   
   enum class FileTime
   {
      AccessTime,
      BirthTime,
      MetadataChangeTime,
      ModificationTime
   };
   
   virtual ~AbstractFileEngine();
   
   virtual bool open(IoDevice::OpenMode openMode);
   virtual bool close();
   virtual bool flush();
   virtual bool syncToDisk();
   virtual pdk::pint64 size() const;
   virtual pdk::pint64 pos() const;
   virtual bool seek(pdk::pint64 pos);
   virtual bool isSequential() const;
   virtual bool remove();
   virtual bool copy(const String &newName);
   virtual bool rename(const String &newName);
   virtual bool renameOverwrite(const String &newName);
   virtual bool link(const String &newName);
   virtual bool mkdir(const String &dirName, bool createParentDirectories) const;
   virtual bool rmdir(const String &dirName, bool recurseParentDirectories) const;
   virtual bool setSize(pdk::pint64 size);
   virtual bool caseSensitive() const;
   virtual bool isRelativePath() const;
   virtual StringList entryList(Dir::Filters filters, const StringList &filterNames) const;
   virtual FileFlags fileFlags(FileFlags type=FileInfoAll) const;
   virtual bool setPermissions(uint perms);
   virtual ByteArray id() const;
   virtual String fileName(FileName file=DefaultName) const;
   virtual uint ownerId(FileOwner) const;
   virtual String owner(FileOwner) const;
   virtual bool setFileTime(const DateTime &newDate, FileTime time);
   virtual DateTime fileTime(FileTime time) const;
   virtual void setFileName(const String &file);
   virtual int handle() const;
   virtual bool cloneTo(AbstractFileEngine *target);
   bool atEnd() const;
   uchar *map(pdk::pint64 offset, pdk::pint64 size, File::MemoryMapFlags flags);
   bool unmap(uchar *ptr);
   
   typedef AbstractFileEngineIterator Iterator;
   virtual Iterator *beginEntryList(Dir::Filters filters, const StringList &filterNames);
   virtual Iterator *endEntryList();
   
   virtual pdk::pint64 read(char *data, pdk::pint64 maxlen);
   virtual pdk::pint64 readLine(char *data, pdk::pint64 maxlen);
   virtual pdk::pint64 write(const char *data, pdk::pint64 len);
   
   File::FileError error() const;
   String errorString() const;
   
   enum Extension {
      AtEndExtension,
      FastReadLineExtension,
      MapExtension,
      UnMapExtension
   };
   class ExtensionOption
   {};
   class ExtensionReturn
   {};
   
   class MapExtensionOption : public ExtensionOption
   {
   public:
      pdk::pint64 offset;
      pdk::pint64 size;
      File::MemoryMapFlags flags;
   };
   class MapExtensionReturn : public ExtensionReturn
   {
   public:
      uchar *address;
   };
   
   class UnMapExtensionOption : public ExtensionOption
   {
   public:
      uchar *address;
   };
   
   virtual bool extension(Extension extension, const ExtensionOption *option = nullptr, ExtensionReturn *output = nullptr);
   virtual bool supportsExtension(Extension extension) const;
   
   // Factory
   static AbstractFileEngine *create(const String &fileName);
   
protected:
   void setError(File::FileError error, const String &str);
   
   AbstractFileEngine();
   AbstractFileEngine(AbstractFileEnginePrivate &);
   
   pdk::utils::ScopedPointer<AbstractFileEnginePrivate> m_implPtr;
private:
   PDK_DECLARE_PRIVATE(AbstractFileEngine);
   PDK_DISABLE_COPY(AbstractFileEngine);
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(AbstractFileEngine::FileFlags)

class PDK_CORE_EXPORT AbstractFileEngineHandler
{
   public:
   AbstractFileEngineHandler();
   virtual ~AbstractFileEngineHandler();
   virtual AbstractFileEngine *create(const String &fileName) const = 0;
};

class AbstractFileEngineIteratorPrivate;
class PDK_CORE_EXPORT AbstractFileEngineIterator
{
public:
   AbstractFileEngineIterator(Dir::Filters filters, const StringList &nameFilters);
   virtual ~AbstractFileEngineIterator();
   
   virtual String next() = 0;
   virtual bool hasNext() const = 0;
   
   String path() const;
   StringList nameFilters() const;
   Dir::Filters filters() const;
   
   virtual String currentFileName() const = 0;
   virtual FileInfo currentFileInfo() const;
   String currentFilePath() const;
   
protected:
   enum EntryInfoType
   {
   };
   virtual std::any entryInfo(EntryInfoType type) const;
   
private:
   PDK_DISABLE_COPY(AbstractFileEngineIterator);
   friend class DirIterator;
   friend class DirIteratorPrivate;
   void setPath(const String &path);
   pdk::utils::ScopedPointer<AbstractFileEngineIteratorPrivate> m_implPtr;
};

class AbstractFileEnginePrivate
{
public:
   inline AbstractFileEnginePrivate()
      : fileError(File::FileError::UnspecifiedError)
   {
   }
   inline virtual ~AbstractFileEnginePrivate() { }
   
   File::FileError m_fileError;
   String m_errorString;
   
   AbstractFileEngine *m_apiPtr;
   PDK_DECLARE_PUBLIC(AbstractFileEngine);
};

AbstractFileEngine *pdk_custom_file_engine_handler_create(const String &path);


} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_ABSTRACT_FILE_ENGINE_PRIVATE_H
