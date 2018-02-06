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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_FILE_SYSTEM_META_DATA_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_FILE_SYSTEM_META_DATA_PRIVATE_H

#include "pdk/global/PlatformDefs.h"
#include "pdk/global/Global.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/time/Date.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"

// Platform-specific includes
#ifdef PDK_OS_WIN
#  include "pdk/global/Windows.h"
#  ifndef IO_REPARSE_TAG_SYMLINK
#     define IO_REPARSE_TAG_SYMLINK (0xA000000CL)
#  endif
#endif

#ifdef PDK_OS_UNIX
struct statx;
#endif

namespace pdk {
namespace io {
namespace fs {
namespace internal {

// forward declare class
class FileSystemEngine;

class PDK_UNITTEST_EXPORT FileSystemMetaData
{
public:
   FileSystemMetaData()
      : m_knownFlagsMask(0),
        m_size(-1)
   {
   }
   
   enum class MetaDataFlag : uint
   {
      // Permissions, overlaps with File::Permissions
      OtherReadPermission = 0x00000004,   OtherWritePermission = 0x00000002,  OtherExecutePermission = 0x00000001,
      GroupReadPermission = 0x00000040,   GroupWritePermission = 0x00000020,  GroupExecutePermission = 0x00000010,
      UserReadPermission  = 0x00000400,   UserWritePermission  = 0x00000200,  UserExecutePermission  = 0x00000100,
      OwnerReadPermission = 0x00004000,   OwnerWritePermission = 0x00002000,  OwnerExecutePermission = 0x00001000,
      
      OtherPermissions    = OtherReadPermission | OtherWritePermission | OtherExecutePermission,
      GroupPermissions    = GroupReadPermission | GroupWritePermission | GroupExecutePermission,
      UserPermissions     = UserReadPermission  | UserWritePermission  | UserExecutePermission,
      OwnerPermissions    = OwnerReadPermission | OwnerWritePermission | OwnerExecutePermission,
      
      ReadPermissions     = OtherReadPermission | GroupReadPermission | UserReadPermission | OwnerReadPermission,
      WritePermissions    = OtherWritePermission | GroupWritePermission | UserWritePermission | OwnerWritePermission,
      ExecutePermissions  = OtherExecutePermission | GroupExecutePermission | UserExecutePermission | OwnerExecutePermission,
      
      Permissions         = OtherPermissions | GroupPermissions | UserPermissions | OwnerPermissions,
      
      // Type
      LinkType            = 0x00010000,
      FileType            = 0x00020000,
      DirectoryType       = 0x00040000,
#if defined(PDK_OS_DARWIN)
      BundleType          = 0x00080000,
      AliasType           = 0x08000000,
#else
      BundleType          =        0x0,
      AliasType           =        0x0,
#endif
#if defined(PDK_OS_WIN)
      WinLnkType          = 0x08000000,   // Note: Uses the same position for AliasType on Mac
#else
      WinLnkType          =        0x0,
#endif
      SequentialType      = 0x00800000,   // Note: overlaps with AbstractFileEngine::RootFlag
      
      LegacyLinkType      = LinkType | AliasType | WinLnkType,
      
      Type                = LinkType | FileType | DirectoryType | BundleType | SequentialType | AliasType,
      
      // Attributes
      HiddenAttribute     = 0x00100000,
      SizeAttribute       = 0x00200000,   // Note: overlaps with AbstractFileEngine::LocalDiskFlag
      ExistsAttribute     = 0x00400000,   // For historical reasons, indicates existence of data, not the file
#if defined(PDK_OS_WIN)
      WasDeletedAttribute =        0x0,
#else
      WasDeletedAttribute = 0x40000000,   // Indicates the file was deleted
#endif
      
      Attributes          = HiddenAttribute | SizeAttribute | ExistsAttribute | WasDeletedAttribute,
      
      // Times - if we know one of them, we know them all
      AccessTime          = 0x02000000,
      BirthTime           = 0x02000000,
      MetadataChangeTime  = 0x02000000,
      ModificationTime    = 0x02000000,
      
      Times               = AccessTime | BirthTime | MetadataChangeTime | ModificationTime,
      
      // Owner IDs
      UserId              = 0x10000000,
      GroupId             = 0x20000000,
      
      OwnerIds            = UserId | GroupId,
      
      PosixStatFlags      = MetaDataFlag::OtherPermissions
      | MetaDataFlag::GroupPermissions
      | MetaDataFlag::OwnerPermissions
      | MetaDataFlag::FileType
      | MetaDataFlag::DirectoryType
      | MetaDataFlag::SequentialType
      | MetaDataFlag::SizeAttribute
      | MetaDataFlag::WasDeletedAttribute
      | MetaDataFlag::Times
      | MetaDataFlag::OwnerIds,
      
#if defined(PDK_OS_WIN)
      WinStatFlags        = MetaDataFlag::FileType
      | MetaDataFlag::DirectoryType
      | MetaDataFlag::HiddenAttribute
      | MetaDataFlag::ExistsAttribute
      | MetaDataFlag::SizeAttribute
      | MetaDataFlag::Times,
#endif
      
      AllMetaDataFlags    = 0xFFFFFFFF
      
   };
   PDK_DECLARE_FLAGS(MetaDataFlags, MetaDataFlag);
   
   bool hasFlags(MetaDataFlags flags) const
   {
      return ((m_knownFlagsMask & flags) == flags);
   }
   
   MetaDataFlags missingFlags(MetaDataFlags flags)
   {
      return flags & ~m_knownFlagsMask;
   }
   
   void clear()
   {
      m_knownFlagsMask = 0;
   }
   
   void clearFlags(MetaDataFlags flags = MetaDataFlag::AllMetaDataFlags)
   {
      m_knownFlagsMask &= ~flags;
   }
   
   bool exists() const
   {
      return (m_entryFlags & MetaDataFlag::ExistsAttribute);
   }
   
   bool isLink() const
   {
      return  (m_entryFlags & MetaDataFlag::LinkType);
   }
   
   bool isFile() const
   {
      return (m_entryFlags & MetaDataFlag::FileType);
   }
   
   bool isDirectory() const
   {
      return (m_entryFlags & MetaDataFlag::DirectoryType);
   }
   
   bool isBundle() const;
   bool isAlias() const;
   bool isLegacyLink() const
   {
      return (m_entryFlags & MetaDataFlag::LegacyLinkType);
   }
   
   bool isSequential() const
   {
      return (m_entryFlags & MetaDataFlag::SequentialType);
   }
   
   bool isHidden() const
   {
      return (m_entryFlags & MetaDataFlag::HiddenAttribute);
   }
   
   bool wasDeleted() const
   {
      return (m_entryFlags & MetaDataFlag::WasDeletedAttribute);
   }
   
#if defined(PDK_OS_WIN)
   bool isLnkFile() const
   {
      return (m_entryFlags & MetaDataFlag::WinLnkType);
   }
#else
   bool isLnkFile() const
   {
      return false;
   }
#endif
   
   pdk::pint64 getSize() const
   {
      return m_size;
   }
   
   File::Permissions permissions() const
   {
      return File::Permissions((m_entryFlags & MetaDataFlag::Permissions).getUnderData());
   }
   
   DateTime getAccessTime() const;
   DateTime getBirthTime() const;
   DateTime getMetadataChangeTime() const;
   DateTime getModificationTime() const;
   
   DateTime fileTime(AbstractFileEngine::FileTime time) const;
   uint userId() const;
   uint groupId() const;
   uint ownerId(AbstractFileEngine::FileOwner owner) const;
   
#ifdef PDK_OS_UNIX
   void fillFromStatxBuf(const struct statx &statBuffer);
   void fillFromStatBuf(const PDK_STATBUF &statBuffer);
   void fillFromDirEnt(const PDK_DIRENT &statBuffer);
#endif
   
#if defined(PDK_OS_WIN)
   inline void fillFromFileAttribute(DWORD fileAttribute, bool isDriveRoot = false);
   inline void fillFromFindData(WIN32_FIND_DATA &findData, bool setLinkType = false, bool isDriveRoot = false);
   inline void fillFromFindInfo(BY_HANDLE_FILE_INFORMATION &fileInfo);
#endif
private:
   friend class FileSystemEngine;
   
   MetaDataFlags m_knownFlagsMask;
   MetaDataFlags m_entryFlags;
   
   pdk::pint64 m_size;
   
   // Platform-specific data goes here:
#if defined(PDK_OS_WIN)
   DWORD m_fileAttribute;
   FILETIME m_birthTime;
   FILETIME m_changeTime;
   FILETIME m_lastAccessTime;
   FILETIME m_lastWriteTime;
#else
   // msec precision
   pdk::pint64 m_accessTime;
   pdk::pint64 m_birthTime;
   pdk::pint64 m_metadataChangeTime;
   pdk::pint64 m_modificationTime;
   
   uint m_userId;
   uint m_groupId;
#endif
   
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(FileSystemMetaData::MetaDataFlags)

#if defined(PDK_OS_DARWIN)
inline bool FileSystemMetaData::isBundle() const
{
   return (m_entryFlags & MetaDataFlag::BundleType);
}

inline bool FileSystemMetaData::isAlias() const
{
   return (m_entryFlags & MetaDataFlag::AliasType);
}

#else
inline bool FileSystemMetaData::isBundle() const
{
   return false;
}

inline bool FileSystemMetaData::isAlias() const
{
   return false;
}
#endif

#if defined(PDK_OS_UNIX) || defined (PDK_OS_WIN)
inline DateTime FileSystemMetaData::fileTime(AbstractFileEngine::FileTime time) const
{
   switch (time) {
   case AbstractFileEngine::FileTime::ModificationTime:
      return getModificationTime();
      
   case AbstractFileEngine::FileTime::AccessTime:
      return getAccessTime();
      
   case AbstractFileEngine::FileTime::BirthTime:
      return getBirthTime();
      
   case AbstractFileEngine::FileTime::MetadataChangeTime:
      return getMetadataChangeTime();
   }
}
#endif

#if defined(PDK_OS_UNIX)
inline DateTime FileSystemMetaData::getBirthTime() const
{
   return m_birthTime ? DateTime::fromMSecsSinceEpoch(m_birthTime) : DateTime();
}

inline DateTime FileSystemMetaData::getMetadataChangeTime() const
{
   return m_metadataChangeTime ? DateTime::fromMSecsSinceEpoch(m_metadataChangeTime) : DateTime();
}

inline DateTime FileSystemMetaData::getModificationTime() const
{
   return m_modificationTime ? DateTime::fromMSecsSinceEpoch(m_modificationTime) : DateTime();
}

inline DateTime FileSystemMetaData::getAccessTime() const
{
   return m_accessTime ? DateTime::fromMSecsSinceEpoch(m_accessTime) : DateTime();
}

inline uint FileSystemMetaData::userId() const
{
   return m_userId;
}

inline uint FileSystemMetaData::groupId() const
{
   return m_groupId;
}

inline uint FileSystemMetaData::ownerId(AbstractFileEngine::FileOwner owner) const
{
   if (owner == AbstractFileEngine::FileOwner::OwnerUser) {
      return userId();
   } else {
      return groupId();
   }
}
#endif

#if defined(PDK_OS_WIN)
inline uint FileSystemMetaData::userId() const
{
   return (uint) -2;
}

inline uint FileSystemMetaData::groupId() const
{
   return (uint) -2;
}

inline uint FileSystemMetaData::ownerId(AbstractFileEngine::FileOwner owner) const
{
   if (owner == AbstractFileEngine::FileOwner::OwnerUser) {
      return userId();
   } else {
      return groupId();
   }
}

inline void FileSystemMetaData::fillFromFileAttribute(DWORD fileAttribute,bool isDriveRoot)
{
   m_fileAttribute = fileAttribute;
   // Ignore the hidden attribute for drives.
   if (!isDriveRoot && (m_fileAttribute & FILE_ATTRIBUTE_HIDDEN)) {
      entryFlags |= HiddenAttribute;
   }
   entryFlags |= ((fileAttribute & FILE_ATTRIBUTE_DIRECTORY) ? DirectoryType: FileType);
   entryFlags |= ExistsAttribute;
   knownFlagsMask |= FileType | DirectoryType | HiddenAttribute | ExistsAttribute;
}

inline void FileSystemMetaData::fillFromFindData(WIN32_FIND_DATA &findData, bool setLinkType, bool isDriveRoot)
{
   fillFromFileAttribute(findData.dwFileAttributes, isDriveRoot);
   m_birthTime = findData.ftCreationTime;
   lastm_accessTime = findData.ftLastAccessTime;
   m_changeTime = m_lastWriteTime = findData.ftLastWriteTime;
   if (m_fileAttribute & FILE_ATTRIBUTE_DIRECTORY) {
      m_size = 0;
   } else {
      m_size = findData.nFileSizeHigh;
      m_size <<= 32;
      m_size += findData.nFileSizeLow;
   }
   knownFlagsMask |=  Times | SizeAttribute;
   if (setLinkType) {
      knownFlagsMask |=  LinkType;
      entryFlags &= ~LinkType;
      if ((m_fileAttribute & FILE_ATTRIBUTE_REPARSE_POINT)
          && (findData.dwReserved0 == IO_REPARSE_TAG_SYMLINK)) {
         entryFlags |= LinkType;
      }
   }
}

inline void FileSystemMetaData::fillFromFindInfo(BY_HANDLE_FILE_INFORMATION &fileInfo)
{
   fillFromFileAttribute(fileInfo.dwFileAttributes);
   m_birthTime = fileInfo.ftCreationTime;
   lastm_accessTime = fileInfo.ftLastAccessTime;
   m_changeTime = m_lastWriteTime = fileInfo.ftLastWriteTime;
   if (m_fileAttribute & FILE_ATTRIBUTE_DIRECTORY) {
      m_size = 0;
   } else {
      m_size = fileInfo.nFileSizeHigh;
      m_size <<= 32;
      m_size += fileInfo.nFileSizeLow;
   }
   knownFlagsMask |=  Times | SizeAttribute;
}

#endif // PDK_OS_WIN

} // internal
} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_INTERNAL_FILE_SYSTEM_META_DATA_PRIVATE_H
