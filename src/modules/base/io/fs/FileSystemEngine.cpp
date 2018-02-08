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
// Created by softboy on 2018/02/08.

#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/lang/StringBuilder.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/lang/Character.h"
#include "pdk/base/ds/StringList.h"
#include <set>

namespace pdk {
namespace io {
namespace fs {
namespace internal {

using pdk::lang::Latin1Character;
using pdk::lang::String;
using pdk::lang::Character;
using pdk::ds::StringList;

namespace {

inline bool pdk_check_entry(FileSystemEntry &entry, FileSystemMetaData &data, bool resolvingEntry)
{
   if (resolvingEntry) {
      if (!FileSystemEngine::fillMetaData(entry, data, FileSystemMetaData::MetaDataFlag::ExistsAttribute)
          || !data.exists()) {
         data.clear();
         return false;
      }
   }
   
   return true;
}

inline bool pdk_check_entry(AbstractFileEngine *&engine, bool resolvingEntry)
{
   if (resolvingEntry) {
      if (!(engine->getFileFlags(AbstractFileEngine::FileFlag::FlagsMask) & AbstractFileEngine::FileFlag::ExistsFlag)) {
         delete engine;
         engine = nullptr;
         return false;
      }
   }
   return true;
}

bool pdk_resolve_entry_and_create_legacy_engine_recursive(FileSystemEntry &entry, FileSystemMetaData &data,
                                                          AbstractFileEngine *&engine, bool resolvingEntry = false)
{
   String const &filePath = entry.getFilePath();
   if ((engine = pdk_custom_file_engine_handler_create(filePath))) {
      return pdk_check_entry(engine, resolvingEntry);
   }
   for (int prefixSeparator = 0; prefixSeparator < filePath.size(); ++prefixSeparator) {
      Character const ch = filePath[prefixSeparator];
      if (ch == Latin1Character('/')) {
         break;
      }
      if (ch == Latin1Character(':')) {
         if (prefixSeparator == 0) {
            engine = new QResourceFileEngine(filePath);
            return pdk_check_entry(engine, resolvingEntry);
         }
         
         if (prefixSeparator == 1) {
            break;
         }
         const StringList &paths = Dir::searchPaths(filePath.left(prefixSeparator));
         for (StringList::size_type i = 0; i < paths.size(); i++) {
            entry = FileSystemEntry(Dir::cleanPath(paths.at(i) % Latin1Character('/') % filePath.substringRef(prefixSeparator + 1)));
            // Recurse!
            if (pdk_resolve_entry_and_create_legacy_engine_recursive(entry, data, engine, true)) {
               return true;
            }
         }
         // entry may have been clobbered at this point.
         return false;
      }
      //  There's no need to fully validate the prefix here. Consulting the
      //  unicode tables could be expensive and validation is already
      //  performed in Dir::setSearchPaths.
      //
      //  if (!ch.isLetterOrNumber())
      //      break;
   }
   return pdk_check_entry(entry, data, resolvingEntry);
}

} // anonymous namespace

String FileSystemEngine::slowCanonicalized(const String &path)
{
   if (path.isEmpty()) {
      return path;
   }
   FileInfo fi;
   const Character slash(Latin1Character('/'));
   String tmpPath = path;
   int separatorPos = 0;
   std::set<String> nonSymlinks;
   std::set<String> known;
   known.insert(path);
   do {
#ifdef PDK_OS_WIN
      if (separatorPos == 0) {
         if (tmpPath.size() >= 2 && tmpPath.at(0) == slash && tmpPath.at(1) == slash) {
            // UNC, skip past the first two elements
            separatorPos = tmpPath.indexOf(slash, 2);
         } else if (tmpPath.size() >= 3 && tmpPath.at(1) == Latin1Character(':') && tmpPath.at(2) == slash) {
            // volume root, skip since it can not be a symlink
            separatorPos = 2;
         }
      }
      if (separatorPos != -1)
#endif
         separatorPos = tmpPath.indexOf(slash, separatorPos + 1);
      String prefix = separatorPos == -1 ? tmpPath : tmpPath.left(separatorPos);
      if (nonSymlinks.find(prefix) == nonSymlinks.end()) {
         fi.setFile(prefix);
         if (fi.isSymLink()) {
            String target = fi.symLinkTarget();
            if (separatorPos != -1) {
               if (fi.isDir() && !target.endsWith(slash)) {
                  target.append(slash);
               }
               target.append(tmpPath.substringRef(separatorPos));
            }
            tmpPath = Dir::cleanPath(target);
            separatorPos = 0;
            if (known.find(tmpPath) != known.end()) {
               return String();
            }
            known.insert(tmpPath);
         } else {
            nonSymlinks.insert(prefix);
         }
      }
   } while (separatorPos != -1);
   
   return Dir::cleanPath(tmpPath);
}

AbstractFileEngine *FileSystemEngine::resolveEntryAndCreateLegacyEngine(
      FileSystemEntry &entry, FileSystemMetaData &data) {
   FileSystemEntry copy = entry;
   AbstractFileEngine *engine = 0;
   
   if (pdk_resolve_entry_and_create_legacy_engine_recursive(copy, data, engine))
      // Reset entry to resolved copy.
      entry = copy;
   else
      data.clear();
   
   return engine;
}

//static
String FileSystemEngine::resolveUserName(const FileSystemEntry &entry, FileSystemMetaData &metaData)
{
#if defined(PDK_OS_WIN)
   PDK_UNUSED(metaData);
   return FileSystemEngine::owner(entry, AbstractFileEngine::FileOwner::OwnerUser);
#else //(PDK_OS_UNIX)
   if (!metaData.hasFlags(FileSystemMetaData::MetaDataFlag::UserId)) {
      FileSystemEngine::fillMetaData(entry, metaData, FileSystemMetaData::MetaDataFlag::UserId);
   }
   if (!metaData.exists()) {
      return String();
   }
   return resolveUserName(metaData.getUserId());
#endif
}

//static
String FileSystemEngine::resolveGroupName(const FileSystemEntry &entry, FileSystemMetaData &metaData)
{
#if defined(PDK_OS_WIN)
   PDK_UNUSED(metaData);
   return FileSystemEngine::owner(entry, AbstractFileEngine::FileOwner::OwnerGroup);
#else //(PDK_OS_UNIX)
   if (!metaData.hasFlags(FileSystemMetaData::MetaDataFlag::GroupId))
      FileSystemEngine::fillMetaData(entry, metaData, FileSystemMetaData::MetaDataFlag::GroupId);
   if (!metaData.exists()) {
      return String();
   }
   return resolveGroupName(metaData.getGroupId());
#endif
}

} // internal
} // fs
} // io
} // pdk
