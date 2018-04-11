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

#include "pdk/base/io/fs/DirIterator.h"
#include "pdk/base/io/fs/internal/DirPrivate.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemIteratorPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemMetaDataPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileInfoPrivate.h"
#include "pdk/base/text/RegularExpression.h"
#include "pdk/stdext/utility/Algorithms.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/base/lang/String.h"

#include <any>
#include <vector>
#include <stack>
#include <set>

namespace pdk {
namespace io {
namespace fs {

using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using internal::FileSystemEntry;
using pdk::text::RegularExpression;

namespace internal {

template <class Iterator>
class DirIteratorPrivateIteratorStack : public std::stack<Iterator *>
{
public:
   ~DirIteratorPrivateIteratorStack()
   {
      while (!this->empty()) {
         delete this->top();
         this->pop();
      }
   }
};

class DirIteratorPrivate
{
public:
   DirIteratorPrivate(const FileSystemEntry &entry, const StringList &nameFilters,
                      Dir::Filters filters, DirIterator::IteratorFlags flags, bool resolveEngine = true);
   
   void advance();
   
   bool entryMatches(const String &fileName, const FileInfo &fileInfo);
   void pushDirectory(const FileInfo &fileInfo);
   void checkAndPushDirectory(const FileInfo &);
   bool matchesFilters(const String &fileName, const FileInfo &fi) const;
   
   pdk::utils::ScopedPointer<AbstractFileEngine> m_engine;
   
   FileSystemEntry m_dirEntry;
   const StringList m_nameFilters;
   const Dir::Filters m_filters;
   const DirIterator::IteratorFlags m_iteratorFlags;
   
   #ifndef PDK_NO_FILESYSTEMITERATOR
      std::vector<RegularExpression> m_nameRegExps;
   #endif
   
   DirIteratorPrivateIteratorStack<AbstractFileEngineIterator> m_fileEngineIterators;
#ifndef PDK_NO_FILESYSTEMITERATOR
   DirIteratorPrivateIteratorStack<FileSystemIterator> m_nativeIterators;
#endif
   
   FileInfo m_currentFileInfo;
   FileInfo m_nextFileInfo;
   
   // Loop protection
   std::set<String> m_visitedLinks;
};

DirIteratorPrivate::DirIteratorPrivate(const FileSystemEntry &entry, const StringList &nameFilters,
                                       Dir::Filters filters, DirIterator::IteratorFlags flags, bool resolveEngine)
   : m_dirEntry(entry),
     m_nameFilters(nameFilters.contains(Latin1String("*")) ? StringList() : nameFilters),
     m_filters(filters == Dir::Filter::NoFilter ? Dir::Filter::AllEntries : filters),
     m_iteratorFlags(flags)
{
#ifndef PDK_NO_FILESYSTEMITERATOR
   m_nameRegExps.reserve(nameFilters.size());
   for (size_t i = 0; i < nameFilters.size(); ++i) {
      RegularExpression::PatternOptions options(RegularExpression::PatternOption::DotMatchesEverythingOption);
      if (!(filters & Dir::Filter::CaseSensitive)) {
         options |= RegularExpression::PatternOption::CaseInsensitiveOption;
      }
      m_nameRegExps.push_back(RegularExpression(nameFilters.at(i)));
   }
#endif
   FileSystemMetaData metaData;
   if (resolveEngine) {
      m_engine.reset(FileSystemEngine::resolveEntryAndCreateLegacyEngine(m_dirEntry, metaData));
   }
   FileInfo fileInfo(new FileInfoPrivate(m_dirEntry, metaData));
   // Populate fields for hasNext() and next()
   pushDirectory(fileInfo);
   advance();
}

void DirIteratorPrivate::pushDirectory(const FileInfo &fileInfo)
{
   String path = fileInfo.getFilePath();
#ifdef PDK_OS_WIN
   if (fileInfo.isSymLink()) {
      path = fileInfo.getCanonicalFilePath();
   }
#endif
   if (m_iteratorFlags & DirIterator::IteratorFlag::FollowSymlinks) {
      m_visitedLinks.insert(fileInfo.getCanonicalFilePath());
   }
   if (m_engine) {
      m_engine->setFileName(path);
      AbstractFileEngineIterator *iter = m_engine->beginEntryList(m_filters, m_nameFilters);
      if (iter) {
         iter->setPath(path);
         m_fileEngineIterators.push(iter);
      } else {
         // No iterator; no entry list.
      }
   } else {
#ifndef PDK_NO_FILESYSTEMITERATOR
      FileSystemIterator *iter = new FileSystemIterator(fileInfo.m_implPtr->m_fileEntry,
                                                        m_filters, m_nameFilters, m_iteratorFlags);
      m_nativeIterators.push(iter);
#else
      warning_stream("pdk was built with -no-feature-filesystemiterator: no files/plugins will be found!");
#endif
   }
}

inline bool DirIteratorPrivate::entryMatches(const String &fileName, const FileInfo &fileInfo)
{
   checkAndPushDirectory(fileInfo);
   if (matchesFilters(fileName, fileInfo)) {
      m_currentFileInfo = m_nextFileInfo;
      m_nextFileInfo = fileInfo;
      //We found a matching entry.
      return true;
   }
   return false;
}

void DirIteratorPrivate::advance()
{
   if (m_engine) {
      while (!m_fileEngineIterators.empty()) {
         // Find the next valid iterator that matches the filters.
         AbstractFileEngineIterator *iter;
         while (iter = m_fileEngineIterators.top(), iter->hasNext()) {
            iter->next();
            if (entryMatches(iter->getCurrentFileName(), iter->getCurrentFileInfo())) {
               return;
            }  
         }
         m_fileEngineIterators.pop();
         delete iter;
      }
   } else {
#ifndef PDK_NO_FILESYSTEMITERATOR
      FileSystemEntry nextEntry;
      FileSystemMetaData nextMetaData;
      while (!m_nativeIterators.empty()) {
         // Find the next valid iterator that matches the filters.
         FileSystemIterator *iter;
         while (iter = m_nativeIterators.top(), iter->advance(nextEntry, nextMetaData)) {
            FileInfo info(new FileInfoPrivate(nextEntry, nextMetaData));
            if (entryMatches(nextEntry.getFileName(), info)) {
               return;
            }
            nextMetaData = FileSystemMetaData();
         }
         m_nativeIterators.pop();
         delete iter;
      }
#endif
   }
   m_currentFileInfo = m_nextFileInfo;
   m_nextFileInfo = FileInfo();
}

void DirIteratorPrivate::checkAndPushDirectory(const FileInfo &fileInfo)
{
   // If we're doing flat iteration, we're done.
   if (!(m_iteratorFlags & DirIterator::IteratorFlag::Subdirectories)) {
      return;
   }
   // Never follow non-directory entries
   if (!fileInfo.isDir()) {
      return;
   }
   // Follow symlinks only when asked
   if (!(m_iteratorFlags & DirIterator::IteratorFlag::FollowSymlinks) && fileInfo.isSymLink()) {
      return;
   }
   
   // Never follow . and ..
   String fileName = fileInfo.getFileName();
   if (Latin1String(".") == fileName || Latin1String("..") == fileName) {
      return;
   }
   // No hidden directories unless requested
   if (!(m_filters & Dir::Filter::AllDirs) && !(m_filters & Dir::Filter::Hidden) && fileInfo.isHidden()) {
      return;
   }
   // Stop link loops
   if (!m_visitedLinks.empty() &&
       m_visitedLinks.find(fileInfo.getCanonicalFilePath()) != m_visitedLinks.end()) {
      return;
   }
   pushDirectory(fileInfo);
}

bool DirIteratorPrivate::matchesFilters(const String &fileName, const FileInfo &file) const
{
   PDK_ASSERT(!fileName.isEmpty());
   // filter . and ..?
   const int fileNameSize = fileName.size();
   const bool dotOrDotDot = fileName[0] == Latin1Character('.')
         && ((fileNameSize == 1)
             ||(fileNameSize == 2 && fileName[1] == Latin1Character('.')));
   if ((m_filters & Dir::Filter::NoDot) && dotOrDotDot && fileNameSize == 1) {
      return false;
   }
   
   if ((m_filters & Dir::Filter::NoDotDot) && dotOrDotDot && fileNameSize == 2) {
      return false;
   }
   
   // name filter
   #ifndef PDK_NO_REGULAREXPRESSION
      // Pass all entries through name filters, except dirs if the AllDirs
      if (!m_nameFilters.empty() && !((m_filters & Dir::Filter::AllDirs) && file.isDir())) {
         bool matched = false;
         for (std::vector<RegularExpression>::const_iterator iter = m_nameRegExps.cbegin(),
              end = m_nameRegExps.cend();
              iter != end; ++iter) {
   
            RegularExpression copy = *iter;
            std::cout << "regex > " << fileName.toStdString() << std::endl;
            // @TODO is really ok here?
            if (copy.match(fileName).hasMatch()) {
               matched = true;
               break;
            }
         }
         if (!matched) {
            return false;
         }
      }
   #endif
   // skip symlinks
   const bool skipSymlinks = (m_filters & Dir::Filter::NoSymLinks);
   const bool includeSystem = (m_filters & Dir::Filter::System);
   if(skipSymlinks && file.isSymLink()) {
      // The only reason to save this file is if it is a broken link and we are requesting system files.
      if(!includeSystem || file.exists()) {
         return false;
      } 
   }
   
   // filter hidden
   const bool includeHidden = (m_filters & Dir::Filter::Hidden);
   if (!includeHidden && !dotOrDotDot && file.isHidden()) {
      return false;
   }
   
   // filter system files
   if (!includeSystem && (!(file.isFile() || file.isDir() || file.isSymLink())
                          || (!file.exists() && file.isSymLink()))) {
      return false;
   }
   // skip directories
   const bool skipDirs = !(m_filters & (pdk::as_integer<Dir::Filter>(Dir::Filter::Dirs) | 
                                        pdk::as_integer<Dir::Filter>(Dir::Filter::AllDirs)));
   if (skipDirs && file.isDir()) {
      return false;
   }
   
   // skip files
   const bool skipFiles = !(m_filters & Dir::Filter::Files);
   if (skipFiles && file.isFile()) {
      // Basically we need a reason not to exclude this file otherwise we just eliminate it.
      return false;
   }
   // filter permissions
   const bool filterPermissions = ((m_filters & Dir::Filter::PermissionMask)
                                   && (m_filters & Dir::Filter::PermissionMask) != Dir::Filter::PermissionMask);
   const bool doWritable = !filterPermissions || (m_filters & Dir::Filter::Writable);
   const bool doExecutable = !filterPermissions || (m_filters & Dir::Filter::Executable);
   const bool doReadable = !filterPermissions || (m_filters & Dir::Filter::Readable);
   if (filterPermissions
       && ((doReadable && !file.isReadable())
           || (doWritable && !file.isWritable())
           || (doExecutable && !file.isExecutable()))) {
      return false;
   }
   return true;
}

} // internal


DirIterator::DirIterator(const Dir &dir, IteratorFlags flags)
{
   const DirPrivate *other = dir.m_implPtr.constData();
   m_implPtr.reset(new DirIteratorPrivate(other->m_dirEntry, other->m_nameFilters, other->m_filters, flags, !other->m_fileEngine.isNull()));
}

DirIterator::DirIterator(const String &path, Dir::Filters filters, IteratorFlags flags)
   : m_implPtr(new DirIteratorPrivate(FileSystemEntry(path), StringList(), filters, flags))
{
}

DirIterator::DirIterator(const String &path, IteratorFlags flags)
   : m_implPtr(new DirIteratorPrivate(FileSystemEntry(path), StringList(), Dir::Filter::NoFilter, flags))
{
}

DirIterator::DirIterator(const String &path, const StringList &nameFilters,
                         Dir::Filters filters, IteratorFlags flags)
   : m_implPtr(new DirIteratorPrivate(FileSystemEntry(path), nameFilters, filters, flags))
{
}

DirIterator::~DirIterator()
{
}

String DirIterator::next()
{
   m_implPtr->advance();
   return getFilePath();
}

bool DirIterator::hasNext() const
{
   if (m_implPtr->m_engine) {
      return !m_implPtr->m_fileEngineIterators.empty();
   }
   else {
#ifndef PDK_NO_FILESYSTEMITERATOR
      return !m_implPtr->m_nativeIterators.empty();
#else
      return false;
#endif
   }
}

String DirIterator::getFileName() const
{
   return m_implPtr->m_currentFileInfo.getFileName();
}

String DirIterator::getFilePath() const
{
   return m_implPtr->m_currentFileInfo.getFilePath();
}

FileInfo DirIterator::getFileInfo() const
{
   return m_implPtr->m_currentFileInfo;
}

String DirIterator::getPath() const
{
   return m_implPtr->m_dirEntry.getFilePath();
}

} // fs
} // io
} // pdk
