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
// Created by softboy on 2018/02/09.

#include "pdk/global/Global.h"
#include "pdk/global/PlatformDefs.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/internal/FileInfoPrivate.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemMetaDataPrivate.h"
#include "pdk/base/io/Debug.h"

namespace pdk {
namespace io {
namespace fs {

using internal::FileSystemEngine;
using internal::AbstractFileEngine;
using internal::FileSystemMetaData;
using internal::FileSystemEntry;
using pdk::io::Debug;
using pdk::io::DebugStateSaver;

namespace internal {

String FileInfoPrivate::getFileName(AbstractFileEngine::FileName name) const
{
   if (m_cacheEnabled && !m_fileNames[pdk::as_integer<AbstractFileEngine::FileName>(name)].isNull()) {
      return m_fileNames[pdk::as_integer<AbstractFileEngine::FileName>(name)];
   }
   String ret;
   if (m_fileEngine == nullptr) { // local file; use the FileSystemEngine directly
      switch (name) {
      case AbstractFileEngine::FileName::CanonicalName:
      case AbstractFileEngine::FileName::CanonicalPathName: {
         FileSystemEntry entry = FileSystemEngine::getCanonicalName(m_fileEntry, m_metaData);
         if (m_cacheEnabled) { // be smart and store both
            m_fileNames[pdk::as_integer<AbstractFileEngine::FileName>(AbstractFileEngine::FileName::CanonicalName)] = entry.getFilePath();
            m_fileNames[pdk::as_integer<AbstractFileEngine::FileName>(AbstractFileEngine::FileName::CanonicalPathName)] = entry.getPath();
         }
         if (name == AbstractFileEngine::FileName::CanonicalName) {
            ret = entry.getFilePath();
         } else {
            ret = entry.getPath();
         }
         break;
      }
      case AbstractFileEngine::FileName::LinkName:
         ret = FileSystemEngine::getLinkTarget(m_fileEntry, m_metaData).getFilePath();
         break;
      case AbstractFileEngine::FileName::BundleName:
         ret = FileSystemEngine::getBundleName(m_fileEntry);
         break;
      case AbstractFileEngine::FileName::AbsoluteName:
      case AbstractFileEngine::FileName::AbsolutePathName: {
         FileSystemEntry entry = FileSystemEngine::getAbsoluteName(m_fileEntry);
         if (m_cacheEnabled) { // be smart and store both
            m_fileNames[pdk::as_integer<AbstractFileEngine::FileName>(AbstractFileEngine::FileName::AbsoluteName)] = entry.getFilePath();
            m_fileNames[pdk::as_integer<AbstractFileEngine::FileName>(AbstractFileEngine::FileName::AbsolutePathName)] = entry.getPath();
         }
         if (name == AbstractFileEngine::FileName::AbsoluteName) {
            ret = entry.getFilePath();
         } else {
            ret = entry.getPath();
         }
         break;
      }
      default: break;
      }
   } else {
      ret = m_fileEngine->getFileName(name);
   }
   if (ret.isNull()) {
      ret = Latin1String("");
   }
   if (m_cacheEnabled) {
      m_fileNames[pdk::as_integer<AbstractFileEngine::FileName>(name)] = ret;
   }
   return ret;
}

String FileInfoPrivate::getFileOwner(AbstractFileEngine::FileOwner own) const
{
   typename std::underlying_type<AbstractFileEngine::FileOwner>::type ownIdx 
         = pdk::as_integer<AbstractFileEngine::FileOwner>(own);
   if (m_cacheEnabled && !m_fileOwners[ownIdx].isNull()) {
      return m_fileOwners[ownIdx];
   }
   String ret;
   if (m_fileEngine == nullptr) {
      switch (own) {
      case AbstractFileEngine::FileOwner::OwnerUser:
         ret = FileSystemEngine::resolveUserName(m_fileEntry, m_metaData);
         break;
      case AbstractFileEngine::FileOwner::OwnerGroup:
         ret = FileSystemEngine::resolveGroupName(m_fileEntry, m_metaData);
         break;
      }
   } else {
      ret = m_fileEngine->getOwner(own);
   }
   if (ret.isNull()) {
      ret = Latin1String("");
   }
   if (m_cacheEnabled) {
      m_fileOwners[ownIdx] = ret;
   }
   return ret;
}

uint FileInfoPrivate::getFileFlags(AbstractFileEngine::FileFlags request) const
{
   PDK_ASSERT(m_fileEngine); // should never be called when using the native FS
   // We split the testing into tests for for LinkType, BundleType, PermsMask
   // and the rest.
   // Tests for file permissions on Windows can be slow, expecially on network
   // paths and NTFS drives.
   // In order to determine if a file is a symlink or not, we have to lstat().
   // If we're not interested in that information, we might as well avoid one
   // extra syscall. Bundle detecton on Mac can be slow, expecially on network
   // paths, so we separate out that as well.
   
   AbstractFileEngine::FileFlags req = 0;
   uint cachedFlags = 0;
   
   if (request & (AbstractFileEngine::FileFlag::FlagsMask | AbstractFileEngine::FileFlag::TypesMask)) {
      if (!getCachedFlag(pdk::as_integer<CacheFlag>(CacheFlag::CachedFileFlags))) {
         req |= AbstractFileEngine::FileFlag::FlagsMask;
         req |= AbstractFileEngine::FileFlag::TypesMask;
         req &= (~pdk::as_integer<AbstractFileEngine::FileFlag>(AbstractFileEngine::FileFlag::LinkType));
         req &= (~pdk::as_integer<AbstractFileEngine::FileFlag>(AbstractFileEngine::FileFlag::BundleType));
         
         cachedFlags |= pdk::as_integer<CacheFlag>(CacheFlag::CachedFileFlags);
      }
      
      if (request & AbstractFileEngine::FileFlag::LinkType) {
         if (!getCachedFlag(pdk::as_integer<CacheFlag>(CacheFlag::CachedLinkTypeFlag))) {
            req |= AbstractFileEngine::FileFlag::LinkType;
            cachedFlags |= pdk::as_integer<CacheFlag>(CacheFlag::CachedLinkTypeFlag);
         }
      }
      
      if (request & AbstractFileEngine::FileFlag::BundleType) {
         if (!getCachedFlag(pdk::as_integer<CacheFlag>(CacheFlag::CachedBundleTypeFlag))) {
            req |= AbstractFileEngine::FileFlag::BundleType;
            cachedFlags |= pdk::as_integer<CacheFlag>(CacheFlag::CachedBundleTypeFlag);
         }
      }
   }
   
   if (request & AbstractFileEngine::FileFlag::PermsMask) {
      if (!getCachedFlag(pdk::as_integer<CacheFlag>(CacheFlag::CachedPerms))) {
         req |= AbstractFileEngine::FileFlag::PermsMask;
         cachedFlags |= pdk::as_integer<CacheFlag>(CacheFlag::CachedPerms);
      }
   }
   
   if (req) {
      if (m_cacheEnabled) {
         req &= (~pdk::as_integer<AbstractFileEngine::FileFlag>(AbstractFileEngine::FileFlag::Refresh));
      } else {
         req |= AbstractFileEngine::FileFlag::Refresh;
      }
      AbstractFileEngine::FileFlags flags = m_fileEngine->getFileFlags(req);
      m_fileFlags |= uint(flags);
      setCachedFlag(cachedFlags);
   }
   return m_fileFlags & request;
}

DateTime &FileInfoPrivate::getFileTime(AbstractFileEngine::FileTime request) const
{
   PDK_ASSERT(m_fileEngine); // should never be called when using the native FS
   if (!m_cacheEnabled) {
      clearFlags();
   }
   uint cf = 0;
   typename std::underlying_type<AbstractFileEngine::FileTime>::type requestIdx 
         = pdk::as_integer<AbstractFileEngine::FileTime>(request);
   switch (request) {
   case AbstractFileEngine::FileTime::AccessTime:
      cf = pdk::as_integer<CacheFlag>(CacheFlag::CachedATime);
      break;
   case AbstractFileEngine::FileTime::BirthTime:
      cf = pdk::as_integer<CacheFlag>(CacheFlag::CachedBTime);
      break;
   case AbstractFileEngine::FileTime::MetadataChangeTime:
      cf = pdk::as_integer<CacheFlag>(CacheFlag::CachedMCTime);
      break;
   case AbstractFileEngine::FileTime::ModificationTime:
      cf = pdk::as_integer<CacheFlag>(CacheFlag::CachedMTime);
      break;
   }
   
   if (!getCachedFlag(cf)) {
      m_fileTimes[requestIdx] = m_fileEngine->getFileTime(request);
      setCachedFlag(cf);
   }
   return m_fileTimes[requestIdx];
}

} // internal

FileInfo::FileInfo(FileInfoPrivate *d)
   : m_implPtr(d)
{}

FileInfo::FileInfo() 
   : m_implPtr(new FileInfoPrivate())
{}

FileInfo::FileInfo(const String &file) 
   : m_implPtr(new FileInfoPrivate(file))
{
}

FileInfo::FileInfo(const File &file) 
   : m_implPtr(new FileInfoPrivate(file.getFileName()))
{
}

FileInfo::FileInfo(const Dir &dir, const String &file)
   : m_implPtr(new FileInfoPrivate(dir.getFilePath(file)))
{
}

FileInfo::FileInfo(const FileInfo &fileinfo)
   : m_implPtr(fileinfo.m_implPtr)
{
   
}

FileInfo::~FileInfo()
{
}

bool FileInfo::operator==(const FileInfo &fileinfo) const
{
   PDK_D(const FileInfo);
   // ### Qt 5: understand long and short file names on Windows
   // ### (GetFullPathName()).
   if (fileinfo.m_implPtr == m_implPtr)
      return true;
   if (implPtr->m_isDefaultConstructed || fileinfo.m_implPtr->m_isDefaultConstructed) {
      return false;
   }
   // Assume files are the same if path is the same
   if (implPtr->m_fileEntry.getFilePath() == fileinfo.m_implPtr->m_fileEntry.getFilePath())
      return true;
   
   pdk::CaseSensitivity sensitive;
   if (implPtr->m_fileEngine == nullptr || fileinfo.m_implPtr->m_fileEngine == nullptr) {
      if (implPtr->m_fileEngine != fileinfo.m_implPtr->m_fileEngine) {
         // one is native, the other is a custom file-engine
         return false;
      }
      sensitive = FileSystemEngine::isCaseSensitive() 
            ? pdk::CaseSensitivity::Sensitive 
            : pdk::CaseSensitivity::Insensitive;
   } else {
      if (implPtr->m_fileEngine->caseSensitive() != fileinfo.m_implPtr->m_fileEngine->caseSensitive()) {
         return false;
      }
      sensitive = implPtr->m_fileEngine->caseSensitive()
            ? pdk::CaseSensitivity::Sensitive
            : pdk::CaseSensitivity::Insensitive;
   }
   
   // Fallback to expensive canonical path computation
   return getCanonicalFilePath().compare(fileinfo.getCanonicalFilePath(), sensitive) == 0;
}

FileInfo &FileInfo::operator=(const FileInfo &fileinfo)
{
   m_implPtr = fileinfo.m_implPtr;
   return *this;
}

void FileInfo::setFile(const String &file)
{
   bool caching = m_implPtr.constData()->m_cacheEnabled;
   *this = FileInfo(file);
   m_implPtr->m_cacheEnabled = caching;
}

void FileInfo::setFile(const File &file)
{
   setFile(file.getFileName());
}

void FileInfo::setFile(const Dir &dir, const String &file)
{
   setFile(dir.getFilePath(file));
}

String FileInfo::getAbsoluteFilePath() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->getFileName(AbstractFileEngine::FileName::AbsoluteName);
}

String FileInfo::getCanonicalFilePath() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->getFileName(AbstractFileEngine::FileName::CanonicalName);
}

String FileInfo::getAbsolutePath() const
{
   PDK_D(const FileInfo);
   
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->getFileName(AbstractFileEngine::FileName::AbsolutePathName);
}

String FileInfo::getCanonicalPath() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed)
      return Latin1String("");
   return implPtr->getFileName(AbstractFileEngine::FileName::CanonicalPathName);
}

String FileInfo::getPath() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->m_fileEntry.getPath();
}

bool FileInfo::isRelative() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return true;
   }
   if (implPtr->m_fileEngine == nullptr) {
      return implPtr->m_fileEntry.isRelative();
   }
   return implPtr->m_fileEngine->isRelativePath();
}

bool FileInfo::makeAbsolute()
{
   if (m_implPtr.constData()->m_isDefaultConstructed
       || !m_implPtr.constData()->m_fileEntry.isRelative()) {
      return false;
   }
   setFile(getAbsoluteFilePath());
   return true;
}

bool FileInfo::exists() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return false;
   }
   if (implPtr->m_fileEngine == nullptr) {
      if (!implPtr->m_cacheEnabled || !implPtr->m_metaData.hasFlags(FileSystemMetaData::MetaDataFlag::ExistsAttribute)) {
         FileSystemEngine::fillMetaData(implPtr->m_fileEntry, implPtr->m_metaData, FileSystemMetaData::MetaDataFlag::ExistsAttribute);
      }
      return implPtr->m_metaData.exists();
   }
   return implPtr->getFileFlags(AbstractFileEngine::FileFlag::ExistsFlag);
}

bool FileInfo::exists(const String &file)
{
   FileSystemEntry entry(file);
   FileSystemMetaData data;
   AbstractFileEngine *engine =
         FileSystemEngine::resolveEntryAndCreateLegacyEngine(entry, data);
   // Expensive fallback to non-FileSystemEngine implementation
   if (engine) {
      return FileInfo(new FileInfoPrivate(entry, data, engine)).exists();
   }
   FileSystemEngine::fillMetaData(entry, data, FileSystemMetaData::MetaDataFlag::ExistsAttribute);
   return data.exists();
}

void FileInfo::refresh()
{
   PDK_D(FileInfo);
   implPtr->clear();
}

String FileInfo::getFilePath() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->m_fileEntry.getFilePath();
}

String FileInfo::getFileName() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->m_fileEntry.getFileName();
}

String FileInfo::getBaseName() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->m_fileEntry.getBaseName();
}

String FileInfo::getCompleteBaseName() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->m_fileEntry.getCompleteBaseName();
}

String FileInfo::getCompleteSuffix() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   } 
   return implPtr->m_fileEntry.getCompleteSuffix();
}

String FileInfo::getSuffix() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->m_fileEntry.getSuffix();
}

String FileInfo::getBundleName() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->getFileName(AbstractFileEngine::FileName::BundleName);
}

Dir FileInfo::getDir() const
{
   PDK_D(const FileInfo);
   // ### Maybe rename this to parentDirectory(), considering what it actually does?
   return Dir(implPtr->m_fileEntry.getPath());
}

Dir FileInfo::getAbsoluteDir() const
{
   return Dir(getAbsolutePath());
}

bool FileInfo::isReadable() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute<bool>(
            FileSystemMetaData::MetaDataFlag::UserReadPermission,
            [implPtr]() 
   { return (implPtr->m_metaData.permissions() & File::Permission::ReadUser) != 0; },
   [implPtr]() { return implPtr->getFileFlags(AbstractFileEngine::FileFlag::ReadUserPerm); });
}

bool FileInfo::isWritable() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute<bool>(
            FileSystemMetaData::MetaDataFlag::UserWritePermission,
            [implPtr]() { return (implPtr->m_metaData.permissions() & File::Permission::WriteUser) != 0; },
   [implPtr]() { return implPtr->getFileFlags(AbstractFileEngine::FileFlag::WriteUserPerm); });
}

bool FileInfo::isExecutable() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute<bool>(
            FileSystemMetaData::MetaDataFlag::UserExecutePermission,
            [implPtr]() { return (implPtr->m_metaData.permissions() & File::Permission::ExeUser) != 0; },
   [implPtr]() { return implPtr->getFileFlags(AbstractFileEngine::FileFlag::ExeUserPerm); });
}

bool FileInfo::isHidden() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute<bool>(
            FileSystemMetaData::MetaDataFlag::HiddenAttribute,
            [implPtr]() { return implPtr->m_metaData.isHidden(); },
   [implPtr]() { return implPtr->getFileFlags(AbstractFileEngine::FileFlag::HiddenFlag); });
}

bool FileInfo::isNativePath() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return false;
   }
   if (implPtr->m_fileEngine == nullptr) {
      return true;
   }      
   return implPtr->getFileFlags(AbstractFileEngine::FileFlag::LocalDiskFlag);
}

bool FileInfo::isFile() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute<bool>(
            FileSystemMetaData::MetaDataFlag::FileType,
            [implPtr]() { return implPtr->m_metaData.isFile(); },
   [implPtr]() { return implPtr->getFileFlags(AbstractFileEngine::FileFlag::FileType); });
}

bool FileInfo::isDir() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute<bool>(
            FileSystemMetaData::MetaDataFlag::DirectoryType,
            [implPtr]() { return implPtr->m_metaData.isDirectory(); },
   [implPtr]() { return implPtr->getFileFlags(AbstractFileEngine::FileFlag::DirectoryType); });
}

bool FileInfo::isSymLink() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute<bool>(
            FileSystemMetaData::MetaDataFlag::LegacyLinkType,
            [implPtr]() { return implPtr->m_metaData.isLegacyLink(); },
   [implPtr]() { return implPtr->getFileFlags(AbstractFileEngine::FileFlag::LinkType); });
}

bool FileInfo::isRoot() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed)
      return false;
   if (implPtr->m_fileEngine == nullptr) {
      if (implPtr->m_fileEntry.isRoot()) {
#if defined(PDK_OS_WIN)
         //the path is a drive root, but the drive may not exist
         //for backward compatibility, return true only if the drive exists
         if (!implPtr->m_cacheEnabled || !implPtr->m_metaData.hasFlags(FileSystemMetaData::MetaDataFlag::ExistsAttribute))
            FileSystemEngine::fillMetaData(implPtr->m_fileEntry, implPtr->m_metaData, FileSystemMetaData::MetaDataFlag::ExistsAttribute);
         return implPtr->m_metaData.exists();
#else
         return true;
#endif
      }
      return false;
   }
   return implPtr->getFileFlags(AbstractFileEngine::FileFlag::RootFlag);
}

bool FileInfo::isBundle() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute<bool>(
            FileSystemMetaData::MetaDataFlag::BundleType,
            [implPtr]() { return implPtr->m_metaData.isBundle(); },
   [implPtr]() { return implPtr->getFileFlags(AbstractFileEngine::FileFlag::BundleType); });
}

String FileInfo::getSymLinkTarget() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->getFileName(AbstractFileEngine::FileName::LinkName);
}

String FileInfo::getOwner() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->getFileOwner(AbstractFileEngine::FileOwner::OwnerUser);
}

uint FileInfo::getOwnerId() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute(
            uint(-2),
            FileSystemMetaData::MetaDataFlag::UserId,
            [implPtr]() { return implPtr->m_metaData.getUserId(); },
   [implPtr]() { return implPtr->m_fileEngine->getOwnerId(AbstractFileEngine::FileOwner::OwnerUser); });
}

String FileInfo::getGroup() const
{
   PDK_D(const FileInfo);
   if (implPtr->m_isDefaultConstructed) {
      return Latin1String("");
   }
   return implPtr->getFileOwner(AbstractFileEngine::FileOwner::OwnerGroup);
}

uint FileInfo::getGroupId() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute(
            uint(-2),
            FileSystemMetaData::MetaDataFlag::GroupId,
            [implPtr]() { return implPtr->m_metaData.getGroupId(); },
   [implPtr]() { return implPtr->m_fileEngine->getOwnerId(AbstractFileEngine::FileOwner::OwnerGroup); });
}

bool FileInfo::getPermission(File::Permissions permissions) const
{
   PDK_D(const FileInfo);
   // the FileSystemMetaData::MetaDataFlag and File::Permissions overlap, so just cast.
   auto fseFlags = FileSystemMetaData::MetaDataFlag(int(permissions));
   auto feFlags = AbstractFileEngine::FileFlags(int(permissions));
   return implPtr->checkAttribute<bool>(
            fseFlags,
            [=]() { return (implPtr->m_metaData.permissions() & permissions) == permissions; },
   [=]() {
      return implPtr->getFileFlags(feFlags) == permissions.getUnderData();
   });
}

File::Permissions FileInfo::getPermissions() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute<File::Permissions>(
            FileSystemMetaData::MetaDataFlag::Permissions,
            [implPtr]() { return implPtr->m_metaData.permissions(); },
   [implPtr]() {
      return File::Permissions(implPtr->getFileFlags(AbstractFileEngine::FileFlag::PermsMask) & 
                               pdk::as_integer<AbstractFileEngine::FileFlag>(AbstractFileEngine::FileFlag::PermsMask));
   });
}

pdk::pint64 FileInfo::getSize() const
{
   PDK_D(const FileInfo);
   return implPtr->checkAttribute<pdk::pint64>(
            FileSystemMetaData::MetaDataFlag::SizeAttribute,
            [implPtr]() { return implPtr->m_metaData.getSize(); },
   [implPtr]() {
      if (!implPtr->getCachedFlag(pdk::as_integer<FileInfoPrivate::CacheFlag>(FileInfoPrivate::CacheFlag::CachedSize))) {
         implPtr->setCachedFlag(pdk::as_integer<FileInfoPrivate::CacheFlag>(FileInfoPrivate::CacheFlag::CachedSize));
         implPtr->m_fileSize = implPtr->m_fileEngine->getSize();
      }
      return implPtr->m_fileSize;
   });
}

DateTime FileInfo::getBirthTime() const
{
   return getFileTime(File::FileTime::FileBirthTime);
}

DateTime FileInfo::getMetadataChangeTime() const
{
   return getFileTime(File::FileTime::FileMetadataChangeTime);
}

DateTime FileInfo::getLastModified() const
{
   return getFileTime(File::FileTime::FileModificationTime);
}

DateTime FileInfo::getLastRead() const
{
   return getFileTime(File::FileTime::FileAccessTime);
}

DateTime FileInfo::getFileTime(File::FileTime time) const
{
   PDK_STATIC_ASSERT(int(File::FileTime::FileAccessTime) == int(AbstractFileEngine::FileTime::AccessTime));
   PDK_STATIC_ASSERT(int(File::FileTime::FileBirthTime) == int(AbstractFileEngine::FileTime::BirthTime));
   PDK_STATIC_ASSERT(int(File::FileTime::FileMetadataChangeTime) == int(AbstractFileEngine::FileTime::MetadataChangeTime));
   PDK_STATIC_ASSERT(int(File::FileTime::FileModificationTime) == int(AbstractFileEngine::FileTime::ModificationTime));
   
   PDK_D(const FileInfo);
   auto fetime = AbstractFileEngine::FileTime(time);
   FileSystemMetaData::MetaDataFlags flag;
   switch (time) {
   case File::FileTime::FileAccessTime:
      flag = FileSystemMetaData::MetaDataFlag::AccessTime;
      break;
   case File::FileTime::FileBirthTime:
      flag = FileSystemMetaData::MetaDataFlag::BirthTime;
      break;
   case File::FileTime::FileMetadataChangeTime:
      flag = FileSystemMetaData::MetaDataFlag::MetadataChangeTime;
      break;
   case File::FileTime::FileModificationTime:
      flag = FileSystemMetaData::MetaDataFlag::ModificationTime;
      break;
   }
   
   return implPtr->checkAttribute<DateTime>(
            flag,
            [=]() { return implPtr->m_metaData.getFileTime(fetime).toLocalTime(); },
   [=]() { return implPtr->getFileTime(fetime).toLocalTime(); });
}

FileInfoPrivate* FileInfo::getImplPtr()
{
   return m_implPtr.data();
}

bool FileInfo::getCaching() const
{
   PDK_D(const FileInfo);
   return implPtr->m_cacheEnabled;
}

void FileInfo::setCaching(bool enable)
{
   PDK_D(FileInfo);
   implPtr->m_cacheEnabled = enable;
}

#ifndef PDK_NO_DEBUG_STREAM
Debug operator<<(Debug dbg, const FileInfo &fileInfo)
{
   DebugStateSaver saver(dbg);
   dbg.nospace();
   dbg.noquote();
   dbg << "FileInfo(" << Dir::toNativeSeparators(fileInfo.getFilePath()) << ')';
   return dbg;
}
#endif

} // fs
} // io
} // pdk

