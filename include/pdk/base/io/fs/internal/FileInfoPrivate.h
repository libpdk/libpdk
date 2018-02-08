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

#ifndef PDK_M_BASE_IO_FS_INTERNAL_FILEINFO_PRIVATE_H
#define PDK_M_BASE_IO_FS_INTERNAL_FILEINFO_PRIVATE_H

#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/os/thread/Atomic.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/utils/SharedData.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemMetaDataPrivate.h"

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::utils::SharedData;

class FileInfoPrivate : public SharedData
{
public:
   enum
   {
      // note: m_cachedFlags is only 30-bits wide
      Cachedm_fileFlags         = 0x01,
      CachedLinkTypeFlag      = 0x02,
      CachedBundleTypeFlag    = 0x04,
      CachedSize              = 0x08,
      CachedATime             = 0x10,
      CachedBTime             = 0x20,
      CachedMCTime            = 0x40,
      CachedMTime             = 0x80,
      CachedPerms             = 0x100
   };
   
   inline FileInfoPrivate()
      : SharedData(), m_fileEngine(0),
        m_cachedFlags(0),
        m_isDefaultConstructed(true),
        m_cacheEnabled(true),
        m_fileFlags(0),
        m_fileSize(0)
   {}
   inline FileInfoPrivate(const FileInfoPrivate &copy)
      : SharedData(copy),
        m_fileEntry(copy.m_fileEntry),
        m_metaData(copy.m_metaData),
        m_fileEngine(FileSystemEngine::resolveEntryAndCreateLegacyEngine(m_fileEntry, m_metaData)),
        m_cachedFlags(0),
     #ifndef PDK_NO_FSFILEENGINE
        m_isDefaultConstructed(false),
     #else
        m_isDefaultConstructed(!m_fileEngine),
     #endif
        m_cacheEnabled(copy.m_cacheEnabled), m_fileFlags(0), m_fileSize(0)
   {}
   inline FileInfoPrivate(const String &file)
      : m_fileEntry(Dir::fromNativeSeparators(file)),
        m_fileEngine(FileSystemEngine::resolveEntryAndCreateLegacyEngine(m_fileEntry, m_metaData)),
        m_cachedFlags(0),
     #ifndef PDK_NO_FSFILEENGINE
        m_isDefaultConstructed(file.isEmpty()),
     #else
        m_isDefaultConstructed(!m_fileEngine),
     #endif
        m_cacheEnabled(true), 
        m_fileFlags(0), 
        m_fileSize(0)
   {
   }
   
   inline FileInfoPrivate(const FileSystemEntry &file, const FileSystemMetaData &data)
      : SharedData(),
        m_fileEntry(file),
        m_metaData(data),
        m_fileEngine(FileSystemEngine::resolveEntryAndCreateLegacyEngine(m_fileEntry, m_metaData)),
        m_cachedFlags(0),
        m_isDefaultConstructed(false),
        m_cacheEnabled(true),
        m_fileFlags(0),
        m_fileSize(0)
   {
      //If the file engine is not null, this maybe a "mount point" for a custom file engine
      //in which case we can't trust the metadata
      if (m_fileEngine)
         m_metaData = FileSystemMetaData();
   }
   
   inline FileInfoPrivate(const FileSystemEntry &file, const FileSystemMetaData &data, AbstractFileEngine *engine)
      : m_fileEntry(file),
        m_metaData(data),
        m_fileEngine(engine),
        m_cachedFlags(0),
     #ifndef PDK_NO_FSFILEENGINE
        m_isDefaultConstructed(false),
     #else
        m_isDefaultConstructed(!m_fileEngine),
     #endif
        m_cacheEnabled(true), m_fileFlags(0), m_fileSize(0)
   {
   }
   
   inline void clearFlags() const {
      m_fileFlags = 0;
      m_cachedFlags = 0;
      if (m_fileEngine) {
         (void)m_fileEngine->getFileFlags(AbstractFileEngine::FileFlag::Refresh);
      }
   }
   inline void clear() {
      m_metaData.clear();
      clearFlags();
      for (int i = pdk::as_integer<AbstractFileEngine::FileName>(AbstractFileEngine::FileName::NFileNames) - 1 ; i >= 0 ; --i) {
         m_fileNames[i].clear();
      }
      m_fileOwners[1].clear();
      m_fileOwners[0].clear();
   }
   
   uint getFileFlags(AbstractFileEngine::FileFlags) const;
   DateTime &getFileTime(AbstractFileEngine::FileTime) const;
   String getFileName(AbstractFileEngine::FileName) const;
   String getFileOwner(AbstractFileEngine::FileOwner own) const;
   
   FileSystemEntry m_fileEntry;
   mutable FileSystemMetaData m_metaData;
   
   pdk::utils::ScopedPointer<AbstractFileEngine> const m_fileEngine;
   
   mutable String m_fileNames[pdk::as_integer<AbstractFileEngine::FileName>(AbstractFileEngine::FileName::NFileNames)];
   mutable String m_fileOwners[2];  // AbstractFileEngine::FileName::FileOwner: OwnerUser and OwnerGroup
   mutable DateTime m_fileTimes[4]; // AbstractFileEngine::FileName::FileTime: BirthTime, MetadataChangeTime, ModificationTime, AccessTime
   
   mutable uint m_cachedFlags : 30;
   bool const m_isDefaultConstructed : 1; // FileInfo is a default constructed instance
   bool m_cacheEnabled : 1;
   mutable uint m_fileFlags;
   mutable pdk::pint64 m_fileSize;
   inline bool getCachedFlag(uint c) const
   {
      return m_cacheEnabled ? (m_cachedFlags & c) : 0;
   }
   
   inline void setCachedFlag(uint c) const
   {
      if (m_cacheEnabled) {
         m_cachedFlags |= c;
      }
   }
   
   template <typename Ret, typename FSLambda, typename EngineLambda>
   Ret checkAttribute(Ret defaultValue, FileSystemMetaData::MetaDataFlags fsFlags, const FSLambda &fsLambda,
                      const EngineLambda &engineLambda) const
   {
      if (m_isDefaultConstructed) {
         return defaultValue;
      }
      if (m_fileEngine) {
         return engineLambda();
      }
      if (!m_cacheEnabled || !m_metaData.hasFlags(fsFlags)) {
         FileSystemEngine::fillMetaData(m_fileEntry, m_metaData, fsFlags);
         // ignore errors, fillm_metaData will have cleared the flags
      }
      return fsLambda();
   }
   
   template <typename Ret, typename FSLambda, typename EngineLambda>
   Ret checkAttribute(FileSystemMetaData::MetaDataFlags fsFlags, const FSLambda &fsLambda,
                      const EngineLambda &engineLambda) const
   {
      return checkAttribute(Ret(), fsFlags, fsLambda, engineLambda);
   }
};

} // internal
} // io
} // fs
} // pdk


#endif // PDK_M_BASE_IO_FS_INTERNAL_FILEINFO_PRIVATE_H
