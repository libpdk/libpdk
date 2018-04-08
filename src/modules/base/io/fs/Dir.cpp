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
// Created by softboy on 2018/02/10.

#include "pdk/global/Global.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/internal/DirPrivate.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemMetaDataPrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/io/fs/Resource.h"
#ifndef PDK_NO_DEBUG_STREAM
#include "pdk/base/io/Debug.h"
#endif
#include "pdk/base/io/fs/DirIterator.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/lang/StringBuilder.h"
#include "pdk/kernel/internal/CoreGlobalDataPrivate.h"
#include "pdk/base/os/thread/ReadWriteLock.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

namespace pdk {
namespace io {
namespace fs {

using pdk::lang::String;
using pdk::lang::Latin1Character;
using pdk::lang::Latin1String;
using pdk::lang::StringRef;
using pdk::ds::StringList;
using pdk::ds::VarLengthArray;
using internal::FileSystemEntry;
using internal::FileSystemEngine;
using internal::FileSystemMetaData;
using internal::AbstractFileEngine;
using pdk::os::thread::ReadLocker;
using pdk::os::thread::WriteLocker;
using pdk::os::thread::ReadWriteLock;
using pdk::kernel::internal::CoreGlobalData;

#ifndef PDK_NO_DEBUG_STREAM
using pdk::io::Debug;
using pdk::io::DebugStateSaver;
#endif

#if defined(PDK_OS_WIN)
constexpr const bool OS_SUPPORT_UNICODE_PATHS = true;
#else
constexpr const bool OS_SUPPORT_UNICODE_PATHS = false;
#endif

namespace {

#if defined(PDK_OS_WIN)
String drive_spec(const String &path)
{
   if (path.size() < 2) {
      return String();
   }
   char c = path.at(0).toLatin1();
   if (c < 'a' && c > 'z' && c < 'A' && c > 'Z') {
      return String();
   }
   if (path.at(1).toLatin1() != ':') {
      return String();
   }
   return path.substring(0, 2);
}
#endif

// Return the length of the root part of an absolute path, for use by cleanPath(), cd().
int root_length(const String &name, bool allowUncPaths)
{
   const int len = name.length();
   // starts with double slash
   if (allowUncPaths && name.startsWith(Latin1String("//"))) {
      // Server name '//server/path' is part of the prefix.
      const int nextSlash = name.indexOf(Latin1Character('/'), 2);
      return nextSlash >= 0 ? nextSlash + 1 : len;
   }
#if defined(PDK_OS_WIN)
   if (len >= 2 && name.at(1) == Latin1Character(':')) {
      // Handle a possible drive letter
      return len > 2 && name.at(2) == Latin1Character('/') ? 3 : 2;
   }
#endif
   if (name.at(0) == Latin1Character('/')) {
      return 1;
   }
   return 0;
}

} // anonymous namespace

namespace internal {

DirPrivate::DirPrivate(const String &path, const StringList &nameFilters, Dir::SortFlags sort, Dir::Filters filters)
   : SharedData(), 
     m_fileListsInitialized(false),
     m_nameFilters(nameFilters),
     m_sort(sort),
     m_filters(filters)
{
   setPath(path.isEmpty() ? String::fromLatin1(".") : path);
   
   bool empty = m_nameFilters.empty();
   if (!empty) {
      empty = true;
      for (StringList::size_type i = 0; i < m_nameFilters.size(); ++i) {
         if (!m_nameFilters.at(i).isEmpty()) {
            empty = false;
            break;
         }
      }
   }
   if (empty) {
      m_nameFilters = StringList(String::fromLatin1("*"));
   }
}

DirPrivate::DirPrivate(const DirPrivate &copy)
   : SharedData(copy),
     m_fileListsInitialized(false),
     m_nameFilters(copy.m_nameFilters),
     m_sort(copy.m_sort),
     m_filters(copy.m_filters),
     m_dirEntry(copy.m_dirEntry),
     m_metaData(copy.m_metaData)
{
}

bool DirPrivate::exists() const
{
   if (m_fileEngine.isNull()) {
      FileSystemEngine::fillMetaData(m_dirEntry, m_metaData,
                                     FileSystemMetaData::MetaDataFlag::ExistsAttribute | 
                                     FileSystemMetaData::MetaDataFlag::DirectoryType); // always stat
      return m_metaData.exists() && m_metaData.isDirectory();
   }
   const AbstractFileEngine::FileFlags info =
         m_fileEngine->getFileFlags(AbstractFileEngine::FileFlag::DirectoryType
                                    | AbstractFileEngine::FileFlag::ExistsFlag
                                    | AbstractFileEngine::FileFlag::Refresh);
   if (!(info & AbstractFileEngine::FileFlag::DirectoryType)) {
      return false;
   }
   return info & AbstractFileEngine::FileFlag::ExistsFlag;
}

// static
inline Character DirPrivate::getFilterSepChar(const String &nameFilter)
{
   Character sep(Latin1Character(';'));
   int i = nameFilter.indexOf(sep, 0);
   if (i == -1 && nameFilter.indexOf(Latin1Character(' '), 0) != -1)
      sep = Character(Latin1Character(' '));
   return sep;
}

// static
inline StringList DirPrivate::splitFilters(const String &nameFilter, Character sep)
{
   if (sep.isNull()) {
      sep = getFilterSepChar(nameFilter);
   }
   const std::vector<StringRef> split = nameFilter.splitRef(sep);
   StringList ret;
   ret.resize(split.size());
   for (const auto &e : split) {
      ret.push_back(e.trimmed().toString());
   }
   return ret;
}

inline void DirPrivate::setPath(const String &path)
{
   String p = Dir::fromNativeSeparators(path);
   if (p.endsWith(Latin1Character('/'))
       && p.length() > 1
    #if defined(PDK_OS_WIN)
       && (!(p.length() == 3 && p.at(1).unicode() == ':' && p.at(0).isLetter()))
    #endif
       ) {
      p.truncate(p.length() - 1);
   }
   m_dirEntry = FileSystemEntry(p, FileSystemEntry::FromInternalPath());
   m_metaData.clear();
   initFileEngine();
   clearFileLists();
   m_absoluteDirEntry = FileSystemEntry();
}

inline void DirPrivate::clearFileLists()
{
   m_fileListsInitialized = false;
   m_files.clear();
   m_fileInfos.clear();
}

inline void DirPrivate::resolveAbsoluteEntry() const
{
   if (!m_absoluteDirEntry.isEmpty() || m_dirEntry.isEmpty()) {
      return;
   }
   String absoluteName;
   if (m_fileEngine.isNull()) {
      if (!m_dirEntry.isRelative() && m_dirEntry.isClean()) {
         m_absoluteDirEntry = m_dirEntry;
         return;
      }
      absoluteName = FileSystemEngine::getAbsoluteName(m_dirEntry).getFilePath();
   } else {
      absoluteName = m_fileEngine->getFileName(AbstractFileEngine::FileName::AbsoluteName);
   }
   m_absoluteDirEntry = FileSystemEntry(Dir::cleanPath(absoluteName), FileSystemEntry::FromInternalPath());
}

/* For sorting */
struct DirSortItem
{
   mutable String m_filenameCache;
   mutable String m_suffixCache;
   FileInfo m_item;
};

class DirSortItemComparator
{
   int m_cmpSiSortFlags;
public:
   DirSortItemComparator(int flags)
      : m_cmpSiSortFlags(flags)
   {}
   bool operator()(const DirSortItem &, const DirSortItem &) const;
};

bool DirSortItemComparator::operator()(const DirSortItem &n1, const DirSortItem &n2) const
{
   const DirSortItem* f1 = &n1;
   const DirSortItem* f2 = &n2;
   if ((m_cmpSiSortFlags & pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::DirsFirst)) && 
       (f1->m_item.isDir() != f2->m_item.isDir())) {
      return f1->m_item.isDir();
   }
   if ((m_cmpSiSortFlags & pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::DirsLast)) && 
       (f1->m_item.isDir() != f2->m_item.isDir())) {
      return !f1->m_item.isDir();
   }
   
   pdk::pint64 r = 0;
   Dir::SortFlag sortBy = Dir::SortFlag((m_cmpSiSortFlags & pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::SortByMask))
                                        | (m_cmpSiSortFlags & pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::Type)));
   
   switch (sortBy) {
   case Dir::SortFlag::Time: {
      DateTime firstModified = f1->m_item.getLastModified();
      DateTime secondModified = f2->m_item.getLastModified();
      
      // DateTime by default will do all sorts of conversions on these to
      // find timezones, which is incredibly expensive. As we aren't
      // presenting these to the user, we don't care (at all) about the
      // local timezone, so force them to UTC to avoid that conversion.
      firstModified.setTimeSpec(pdk::TimeSpec::UTC);
      secondModified.setTimeSpec(pdk::TimeSpec::UTC);
      r = firstModified.msecsTo(secondModified);
      break;
   }
   case Dir::SortFlag::Size:
      r = f2->m_item.getSize() - f1->m_item.getSize();
      break;
   case Dir::SortFlag::Type:
   {
      bool ic = m_cmpSiSortFlags & pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::IgnoreCase);      
      if (f1->m_suffixCache.isNull()) {
         f1->m_suffixCache = ic ? f1->m_item.getSuffix().toLower()
                                : f1->m_item.getSuffix();
      }
      if (f2->m_suffixCache.isNull()) {
         f2->m_suffixCache = ic ? f2->m_item.getSuffix().toLower()
                                : f2->m_item.getSuffix();
      }
      r = m_cmpSiSortFlags & pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::LocaleAware)
            ? f1->m_suffixCache.localeAwareCompare(f2->m_suffixCache)
            : f1->m_suffixCache.compare(f2->m_suffixCache);
   }
      break;
   default:
      ;
   }
   
   if (r == 0 && sortBy != Dir::SortFlag::Unsorted) {
      // Still not sorted - sort by name
      bool ic = m_cmpSiSortFlags & pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::IgnoreCase);
      if (f1->m_suffixCache.isNull()) {
         f1->m_suffixCache = ic ? f1->m_item.getFileName().toLower()
                                : f1->m_item.getFileName();
      }
      if (f2->m_suffixCache.isNull()) {
         f2->m_suffixCache = ic ? f2->m_item.getFileName().toLower()
                                : f2->m_item.getFileName();
      }
      r = m_cmpSiSortFlags & pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::LocaleAware)
            ? f1->m_suffixCache.localeAwareCompare(f2->m_suffixCache)
            : f1->m_suffixCache.compare(f2->m_suffixCache);
   }
   if (m_cmpSiSortFlags & pdk::as_integer<Dir::SortFlag>(Dir::SortFlag::Reversed)) {
      return r > 0;
   }
   return r < 0;
}

inline void DirPrivate::sortFileList(Dir::SortFlags sort, FileInfoList &list,
                                     StringList *names, FileInfoList *infos)
{
   // names and infos are always empty lists or 0 here
   FileInfoList::size_type n = list.size();
   if (n > 0) {
      if (n == 1 || (sort & Dir::SortFlag::SortByMask) == Dir::SortFlag::Unsorted) {
         if (infos) {
            *infos = list;
         }
         if (names) {
            for (FileInfoList::size_type i = 0; i < n; ++i){
               auto iter = list.begin();
               std::advance(iter, FileInfoList::difference_type(i));
               names->push_back(iter->getFileName());
            }
         }
      } else {
         pdk::utils::ScopedArrayPointer<DirSortItem> si(new DirSortItem[n]);
         for (FileInfoList::size_type i = 0; i < n; ++i) {
            auto iter = list.begin();
            std::advance(iter, FileInfoList::difference_type(i));
            si[i].m_item = *iter;
         }
         
         std::sort(si.getData(), si.getData() + n, DirSortItemComparator(sort));
         // put them back in the list(s)
         if (infos) {
            for (FileInfoList::size_type i = 0; i < n; ++i) {
               infos->push_back(si[i].m_item);
            } 
         }
         if (names) {
            for (FileInfoList::size_type i = 0; i < n; ++i) {
               names->push_back(si[i].m_item.getFileName());
            } 
         }
      }
   }
}
inline void DirPrivate::initFileLists(const Dir &dir) const
{
   if (!m_fileListsInitialized) {
      FileInfoList list;
      DirIterator iter(dir);
      while (iter.hasNext()) {
         iter.next();
         list.push_back(iter.getFileInfo());
      }
      sortFileList(m_sort, list, &m_files, &m_fileInfos);
      m_fileListsInitialized = true;
   }
}

inline void DirPrivate::initFileEngine()
{
   m_fileEngine.reset(FileSystemEngine::resolveEntryAndCreateLegacyEngine(m_dirEntry, m_metaData));
}

} // internal

using internal::DirPrivate;

Dir::Dir(DirPrivate &p) 
   : m_implPtr(&p)
{
}

Dir::Dir(const String &path) 
   : m_implPtr(new DirPrivate(path))
{
}

Dir::Dir(const String &path, const String &nameFilter,
         SortFlags sort, Filters filters)
   : m_implPtr(new DirPrivate(path, Dir::nameFiltersFromString(nameFilter), sort, filters))
{
}

Dir::Dir(const Dir &dir)
   : m_implPtr(dir.m_implPtr)
{
}

Dir::~Dir()
{
}

void Dir::setPath(const String &path)
{
   PDK_D(Dir);
   implPtr->setPath(path);
}

String Dir::getPath() const
{
   PDK_D(const Dir);
   return implPtr->m_dirEntry.getFilePath();
}

String Dir::getAbsolutePath() const
{
   PDK_D(const Dir);
   implPtr->resolveAbsoluteEntry();
   return implPtr->m_absoluteDirEntry.getFilePath();
}

String Dir::getCanonicalPath() const
{
   PDK_D(const Dir);
   if (implPtr->m_fileEngine.isNull()) {
      FileSystemEntry answer = FileSystemEngine::getCanonicalName(implPtr->m_dirEntry, implPtr->m_metaData);
      return answer.getFilePath();
   }
   return implPtr->m_fileEngine->getFileName(AbstractFileEngine::FileName::CanonicalName);
}

String Dir::getDirName() const
{
   PDK_D(const Dir);
   return implPtr->m_dirEntry.getFileName();
}

String Dir::getFilePath(const String &fileName) const
{
   PDK_D(const Dir);
   if (isAbsolutePath(fileName)) {
      return fileName;
   }
   String ret = implPtr->m_dirEntry.getFilePath();
   if (!fileName.isEmpty()) {
      if (!ret.isEmpty() && ret[(int)ret.length()-1] != Latin1Character('/') && fileName[0] != Latin1Character('/')) {
         ret += Latin1Character('/');
      }
      ret += fileName;
   }
   return ret;
}

String Dir::getAbsoluteFilePath(const String &fileName) const
{
   PDK_D(const Dir);
   if (isAbsolutePath(fileName)) {
      return fileName;
   }
   implPtr->resolveAbsoluteEntry();
   const String absoluteDirPath = implPtr->m_absoluteDirEntry.getFilePath();
   if (fileName.isEmpty()) {
      return absoluteDirPath;
   }
   if (!absoluteDirPath.endsWith(Latin1Character('/'))) {
      return absoluteDirPath % Latin1Character('/') % fileName;
   }
   return absoluteDirPath % fileName;
}

String Dir::getRelativeFilePath(const String &fileName) const
{
   String dir = cleanPath(getAbsolutePath());
   String file = cleanPath(fileName);
   if (isRelativePath(file) || isRelativePath(dir)) {
      return file;
   }
   
#ifdef PDK_OS_WIN
   String dirDrive = driveSpec(dir);
   String fileDrive = driveSpec(file);
   
   bool fileDriveMissing = false;
   if (fileDrive.isEmpty()) {
      fileDrive = dirDrive;
      fileDriveMissing = true;
   }
   
   if (fileDrive.toLower() != dirDrive.toLower()
       || (file.startsWith(Latin1String("//"))
           && !dir.startsWith(Latin1String("//")))) {
      return file;
   }
   
   
   dir.remove(0, dirDrive.size());
   if (!fileDriveMissing) {
      file.remove(0, fileDrive.size());
   }
   
#endif
   String result;
   using SiteType = std::vector<StringRef>::size_type;
   std::vector<StringRef> dirElts = dir.splitRef(Latin1Character('/'), String::SplitBehavior::SkipEmptyParts);
   std::vector<StringRef> fileElts = file.splitRef(Latin1Character('/'), String::SplitBehavior::SkipEmptyParts);
   
   SiteType i = 0;
   while (i < dirElts.size() && i < fileElts.size() &&
       #if defined(PDK_OS_WIN)
          dirElts.at(i).compare(fileElts.at(i), pdk::CaseSensitivity::Sensitive) == 0)
#else
          dirElts.at(i) == fileElts.at(i))
#endif
      ++i;
   
   for (SiteType j = 0; j < dirElts.size() - i; ++j) {
      result += Latin1String("../");
   }
   for (SiteType j = i; j < fileElts.size(); ++j) {
      result += fileElts.at(j);
      if (j < fileElts.size() - 1) {
         result += Latin1Character('/');
      }
   }
   if (result.isEmpty()) {
      return Latin1String(".");
   }
   return result;
}

String Dir::toNativeSeparators(const String &pathName)
{
#if defined(PDK_OS_WIN)
   int i = pathName.indexOf(Latin1Character('/'));
   if (i != -1) {
      String n(pathName);
      Character * const data = n.getRawData();
      data[i++] = Latin1Character('\\');
      for (; i < n.length(); ++i) {
         if (data[i] == Latin1Character('/')) {
            data[i] = Latin1Character('\\');
         }
      }
      return n;
   }
#endif
   return pathName;
}

String Dir::fromNativeSeparators(const String &pathName)
{
#if defined(PDK_OS_WIN)
   int i = pathName.indexOf(Latin1Character('\\'));
   if (i != -1) {
      String n(pathName);
      Character * const data = n.getRawData();
      data[i++] = Latin1Character('/');
      for (; i < n.length(); ++i) {
         if (data[i] == Latin1Character('\\')) {
            data[i] = Latin1Character('/');
         }
      }
      return n;
   }
#endif
   return pathName;
}

PDK_UNITTEST_EXPORT String normalize_path_segments(const String &name, bool allowUncPaths,
                                                   bool *ok = nullptr)
{
   const int len = name.length();
   if (ok) {
      *ok = false;
   }
   
   if (len == 0) {
      return name;
   }
   int i = len - 1;
   VarLengthArray<char16_t> outVector(len);
   int used = len;
   char16_t *out = outVector.getRawData();
   const char16_t *p = name.utf16();
   const char16_t *prefix = p;
   int up = 0;
   const int prefixLength = root_length(name, allowUncPaths);
   p += prefixLength;
   i -= prefixLength;
   
   // replicate trailing slash (i > 0 checks for emptiness of input string p)
   if (i > 0 && p[i] == '/') {
      out[--used] = '/';
      --i;
   }
   
   while (i >= 0) {
      // remove trailing slashes
      if (p[i] == '/') {
         --i;
         continue;
      }
      
      // remove current directory
      if (p[i] == '.' && (i == 0 || p[i-1] == '/')) {
         --i;
         continue;
      }
      
      // detect up dir
      if (i >= 1 && p[i] == '.' && p[i-1] == '.'
          && (i == 1 || (i >= 2 && p[i-2] == '/'))) {
         ++up;
         i -= 2;
         continue;
      }
      
      // prepend a slash before copying when not empty
      if (!up && used != len && out[used] != '/') {
         out[--used] = '/';
      }
      
      
      // skip or copy
      while (i >= 0) {
         if (p[i] == '/') { // do not copy slashes
            --i;
            break;
         }
         
         // actual copy
         if (!up) {
            out[--used] = p[i];
         }
         --i;
      }
      // decrement up after copying/skipping
      if (up) {
         --up;
      }
   }
   
   // Indicate failure when ".." are left over for an absolute path.
   if (ok) {
      *ok = prefixLength == 0 || up == 0;
   }
   
   
   // add remaining '..'
   while (up) {
      if (used != len && out[used] != '/') {
         // is not empty and there isn't already a '/'
         out[--used] = '/';
      }
      out[--used] = '.';
      out[--used] = '.';
      --up;
   }
   
   bool isEmpty = used == len;
   
   if (prefixLength) {
      if (!isEmpty && out[used] == '/') {
         // Eventhough there is a prefix the out string is a slash. This happens, if the input
         // string only consists of a prefix followed by one or more slashes. Just skip the slash.
         ++used;
      }
      for (int i = prefixLength - 1; i >= 0; --i) {
         out[--used] = prefix[i];
      }
      
   } else {
      if (isEmpty) {
         // After resolving the input path, the resulting string is empty (e.g. "foo/.."). Return
         // a dot in that case.
         out[--used] = '.';
      } else if (out[used] == '/') {
         // After parsing the input string, out only contains a slash. That happens whenever all
         // parts are resolved and there is a trailing slash ("./" or "foo/../" for example).
         // Prepend a dot to have the correct return value.
         out[--used] = '.';
      }
   }
   
   // If path was not modified return the original value
   if (used == 0) {
      return name;
   }
   
   return String::fromUtf16(out + used, len - used);
}

namespace
{

String clean_path(const String &path, bool *ok = nullptr)
{
   if (path.isEmpty()) {
      return path;
   }
   String name = path;
   Character dirSeparator = Dir::getSeparator();
   if (dirSeparator != Latin1Character('/')) {
      name.replace(dirSeparator, Latin1Character('/'));
   }
   String ret = normalize_path_segments(name, OS_SUPPORT_UNICODE_PATHS, ok);
   
   // Strip away last slash except for root directories
   if (ret.length() > 1 && ret.endsWith(Latin1Character('/'))) {
#if defined (PDK_OS_WIN)
      if (!(ret.length() == 3 && ret.at(1) == Latin1Character(':')))
#endif
         ret.chop(1);
   }
   
   return ret;
}
} // anonymous namespace


bool Dir::cd(const String &dirName)
{
   // Don't detach just yet.
   PDK_D(const Dir);
   if (dirName.isEmpty() || dirName == Latin1String(".")) {
      return true;
   }
   String newPath;
   if (isAbsolutePath(dirName)) {
      newPath = clean_path(dirName);
   } else {
      newPath = implPtr->m_dirEntry.getFilePath();
      if (!newPath.endsWith(Latin1Character('/')))
         newPath += Latin1Character('/');
      newPath += dirName;
      if (dirName.indexOf(Latin1Character('/')) >= 0
          || dirName == Latin1String("..")
          || implPtr->m_dirEntry.getFilePath() == Latin1String(".")) {
         bool ok;
         newPath = clean_path(newPath, &ok);
         if (!ok)
            return false;
         /*
              If newPath starts with .., we convert it to absolute to
              avoid infinite looping on
              
                  Dir dir(".");
                  while (dir.cdUp())
                      ;
            */
         if (newPath.startsWith(Latin1String(".."))) {
            newPath = FileInfo(newPath).getAbsoluteFilePath();
         }
      }
   }
   
   pdk::utils::ScopedPointer<DirPrivate> dir(new DirPrivate(*implPtr));
   dir->setPath(newPath);
   if (!dir->exists()) {
      return false;
   }
   m_implPtr = dir.take();
   return true;
}

bool Dir::cdUp()
{
   return cd(String::fromLatin1(".."));
}

StringList Dir::getNameFilters() const
{
   PDK_D(const Dir);
   return implPtr->m_nameFilters;
}

void Dir::setNameFilters(const StringList &nameFilters)
{
   PDK_D(Dir);
   implPtr->initFileEngine();
   implPtr->clearFileLists();
   
   implPtr->m_nameFilters = nameFilters;
}

void Dir::setSearchPaths(const String &prefix, const StringList &searchPaths)
{
   if (prefix.length() < 2) {
      warning_stream("Dir::setSearchPaths: Prefix must be longer than 1 character");
      return;
   }
   
   for (int i = 0; i < prefix.count(); ++i) {
      if (!prefix.at(i).isLetterOrNumber()) {
         warning_stream("Dir::setSearchPaths: Prefix can only contain letters or numbers");
         return;
      }
   }
   
   WriteLocker lock(&CoreGlobalData::getInstance()->m_dirSearchPathsLock);
   std::map<String, StringList> &paths = CoreGlobalData::getInstance()->m_dirSearchPaths;
   if (searchPaths.empty()) {
      paths.erase(prefix);
   } else {
      paths[prefix] = searchPaths;
   }
}

void Dir::addSearchPath(const String &prefix, const String &path)
{
   if (path.isEmpty())
      return;
   
   WriteLocker lock(&CoreGlobalData::getInstance()->m_dirSearchPathsLock);
   CoreGlobalData::getInstance()->m_dirSearchPaths[prefix].push_back(path);
}

StringList Dir::searchPaths(const String &prefix)
{
   ReadLocker lock(&CoreGlobalData::getInstance()->m_dirSearchPathsLock);
   return CoreGlobalData::getInstance()->m_dirSearchPaths.at(prefix);
}

Dir::Filters Dir::getFilter() const
{
   PDK_D(const Dir);
   return implPtr->m_filters;
}

void Dir::setFilter(Filters filters)
{
   PDK_D(Dir);
   implPtr->initFileEngine();
   implPtr->clearFileLists();
   implPtr->m_filters = filters;
}

Dir::SortFlags Dir::getSorting() const
{
   PDK_D(const Dir);
   return implPtr->m_sort;
}

void Dir::setSorting(SortFlags sort)
{
   PDK_D(Dir);
   implPtr->initFileEngine();
   implPtr->clearFileLists();
   implPtr->m_sort = sort;
}

uint Dir::count() const
{
   PDK_D(const Dir);
   implPtr->initFileLists(*this);
   return implPtr->m_files.size();
}

String Dir::operator[](int pos) const
{
   PDK_D(const Dir);
   implPtr->initFileLists(*this);
   return implPtr->m_files[pos];
}

StringList Dir::entryList(Filters filters, SortFlags sort) const
{
   PDK_D(const Dir);
   return entryList(implPtr->m_nameFilters, filters, sort);
}

FileInfoList Dir::entryInfoList(Filters filters, SortFlags sort) const
{
   PDK_D(const Dir);
   return entryInfoList(implPtr->m_nameFilters, filters, sort);
}

StringList Dir::entryList(const StringList &nameFilters, Filters filters,
                          SortFlags sort) const
{
   PDK_D(const Dir);
   if (filters == Filter::NoFilter) {
      filters = implPtr->m_filters;
   }
   if (sort == SortFlag::NoSort) {
      sort = implPtr->m_sort;
   }
   if (filters == implPtr->m_filters && sort == implPtr->m_sort && nameFilters == implPtr->m_nameFilters) {
      implPtr->initFileLists(*this);
      return implPtr->m_files;
   }
   
   FileInfoList list;
   DirIterator iter(implPtr->m_dirEntry.getFilePath(), nameFilters, filters);
   while (iter.hasNext()) {
      iter.next();
      list.push_back(iter.getFileInfo());
   }
   StringList ret;
   implPtr->sortFileList(sort, list, &ret, 0);
   return ret;
}

FileInfoList Dir::entryInfoList(const StringList &nameFilters, Filters filters,
                                SortFlags sort) const
{
   PDK_D(const Dir);
   if (filters == Filter::NoFilter) {
      filters = implPtr->m_filters;
   }
   if (sort == SortFlag::NoSort) {
      sort = implPtr->m_sort;
   }
   if (filters == implPtr->m_filters && sort == implPtr->m_sort && 
       nameFilters == implPtr->m_nameFilters) {
      implPtr->initFileLists(*this);
      return implPtr->m_fileInfos;
   }
   
   FileInfoList list;
   DirIterator iter(implPtr->m_dirEntry.getFilePath(), nameFilters, filters);
   while (iter.hasNext()) {
      iter.next();
      list.push_back(iter.getFileInfo());
   }
   FileInfoList ret;
   implPtr->sortFileList(sort, list, 0, &ret);
   return ret;
}

bool Dir::mkdir(const String &dirName) const
{
   PDK_D(const Dir);
   if (dirName.isEmpty()) {
      warning_stream("Dir::mkdir: Empty or null file name");
      return false;
   }
   String fn = getFilePath(dirName);
   if (implPtr->m_fileEngine.isNull()) {
      return FileSystemEngine::createDirectory(FileSystemEntry(fn), false);
   }
   return implPtr->m_fileEngine->mkdir(fn, false);
}

bool Dir::rmdir(const String &dirName) const
{
   PDK_D(const Dir);
   if (dirName.isEmpty()) {
      warning_stream("Dir::rmdir: Empty or null file name");
      return false;
   }
   String fn = getFilePath(dirName);
   if (implPtr->m_fileEngine.isNull()) {
      return FileSystemEngine::removeDirectory(FileSystemEntry(fn), false);
   }
   return implPtr->m_fileEngine->rmdir(fn, false);
}

bool Dir::mkpath(const String &dirPath) const
{
   PDK_D(const Dir);
   if (dirPath.isEmpty()) {
      warning_stream("Dir::mkpath: Empty or null file name");
      return false;
   }
   String fn = getFilePath(dirPath);
   if (implPtr->m_fileEngine.isNull()) {
      return FileSystemEngine::createDirectory(FileSystemEntry(fn), true);
   }
   return implPtr->m_fileEngine->mkdir(fn, true);
}

bool Dir::rmpath(const String &dirPath) const
{
   PDK_D(const Dir);
   if (dirPath.isEmpty()) {
      warning_stream("Dir::rmpath: Empty or null file name");
      return false;
   }
   
   String fn = getFilePath(dirPath);
   if (implPtr->m_fileEngine.isNull()) {
      return FileSystemEngine::removeDirectory(FileSystemEntry(fn), true);
   }
   return implPtr->m_fileEngine->rmdir(fn, true);
}

bool Dir::removeRecursively()
{
   if (!m_implPtr->exists()) {
      return true;
   }
   bool success = true;
   const String dirPath = getPath();
   // not empty -- we must empty it first
   DirIterator diter(dirPath, DirIterator::IteratorFlag(pdk::as_integer<Dir::Filter>(Dir::Filter::AllEntries) | 
                                                        pdk::as_integer<Dir::Filter>(Dir::Filter::Hidden) | 
                                                        pdk::as_integer<Dir::Filter>(Dir::Filter::System) | 
                                                        pdk::as_integer<Dir::Filter>(Dir::Filter::NoDotAndDotDot)));
   while (diter.hasNext()) {
      diter.next();
      const FileInfo& finfo = diter.getFileInfo();
      const String &filePath = diter.getFilePath();
      bool ok;
      if (finfo.isDir() && !finfo.isSymLink()) {
         ok = Dir(filePath).removeRecursively(); // recursive
      } else {
         ok = File::remove(filePath);
         if (!ok) { // Read-only files prevent directory deletion on Windows, retry with Write permission.
            const File::Permissions permissions = File::permissions(filePath);
            if (!(permissions & File::Permission::WriteUser))
               ok = File::setPermissions(filePath, permissions | File::Permission::WriteUser)
                     && File::remove(filePath);
         }
      }
      if (!ok)
         success = false;
   }
   
   if (success) {
      success = rmdir(getAbsolutePath());
   }
   return success;
}

bool Dir::isReadable() const
{
   PDK_D(const Dir);
   if (implPtr->m_fileEngine.isNull()) {
      if (!implPtr->m_metaData.hasFlags(FileSystemMetaData::MetaDataFlag::UserReadPermission)) {
         FileSystemEngine::fillMetaData(implPtr->m_dirEntry, implPtr->m_metaData, 
                                        FileSystemMetaData::MetaDataFlag::UserReadPermission);
      }
      return (implPtr->m_metaData.permissions() & File::Permission::ReadUser) != 0;
   }
   const AbstractFileEngine::FileFlags info =
         implPtr->m_fileEngine->getFileFlags(AbstractFileEngine::FileFlag::DirectoryType
                                             | AbstractFileEngine::FileFlag::PermsMask);
   if (!(info & AbstractFileEngine::FileFlag::DirectoryType)) {
      return false;
   }
   return info & AbstractFileEngine::FileFlag::ReadUserPerm;
}

bool Dir::exists() const
{
   return m_implPtr->exists();
}

bool Dir::isRoot() const
{
   PDK_D(const Dir);
   if (implPtr->m_fileEngine.isNull()) {
      return implPtr->m_dirEntry.isRoot();
   }
   return implPtr->m_fileEngine->getFileFlags(AbstractFileEngine::FileFlag::FlagsMask) & AbstractFileEngine::FileFlag::RootFlag;
}

bool Dir::isRelative() const
{
   PDK_D(const Dir);
   if (implPtr->m_fileEngine.isNull()) {
      return implPtr->m_dirEntry.isRelative();
   }
   return implPtr->m_fileEngine->isRelativePath();
}

bool Dir::makeAbsolute()
{
   PDK_D(const Dir);
   pdk::utils::ScopedPointer<DirPrivate> dir;
   if (!implPtr->m_fileEngine.isNull()) {
      String absolutePath = implPtr->m_fileEngine->getFileName(AbstractFileEngine::FileName::AbsoluteName);
      if (Dir::isRelativePath(absolutePath)) {
         return false;
      }
      dir.reset(new DirPrivate(*implPtr));
      dir->setPath(absolutePath);
   } else { // native FS
      implPtr->resolveAbsoluteEntry();
      dir.reset(new DirPrivate(*implPtr));
      dir->setPath(implPtr->m_absoluteDirEntry.getFilePath());
   }
   m_implPtr = dir.take(); // actually detach
   return true;
}

bool Dir::operator==(const Dir &dir) const
{
   PDK_D(const Dir);
   const DirPrivate *other = dir.m_implPtr.constData();
   if (implPtr == other) {
      return true;
   }
   
   pdk::CaseSensitivity sensitive;
   if (implPtr->m_fileEngine.isNull() || other->m_fileEngine.isNull()) {
      if (implPtr->m_fileEngine.getData() != other->m_fileEngine.getData()) {
         // one is native, the other is a custom file-engine
         return false;
      }
      sensitive = FileSystemEngine::isCaseSensitive() 
            ? pdk::CaseSensitivity::Sensitive 
            : pdk::CaseSensitivity::Insensitive;
   } else {
      if (implPtr->m_fileEngine->caseSensitive() != other->m_fileEngine->caseSensitive())
         return false;
      sensitive = implPtr->m_fileEngine->caseSensitive() 
            ? pdk::CaseSensitivity::Sensitive 
            : pdk::CaseSensitivity::Insensitive;
   }
   
   if (implPtr->m_filters == other->m_filters
       && implPtr->m_sort == other->m_sort
       && implPtr->m_nameFilters == other->m_nameFilters) {
      
      // Assume directories are the same if path is the same
      if (implPtr->m_dirEntry.getFilePath() == other->m_dirEntry.getFilePath()) {
         return true;
      }
      if (exists()) {
         if (!dir.exists()) {
            return false; //can't be equal if only one exists
         }
         // Both exist, fallback to expensive canonical path computation
         return getCanonicalPath().compare(dir.getCanonicalPath(), sensitive) == 0;
      } else {
         if (dir.exists())
            return false; //can't be equal if only one exists
         // Neither exists, compare absolute paths rather than canonical (which would be empty strings)
         implPtr->resolveAbsoluteEntry();
         other->resolveAbsoluteEntry();
         return implPtr->m_absoluteDirEntry.getFilePath().compare(other->m_absoluteDirEntry.getFilePath(), sensitive) == 0;
      }
   }
   return false;
}

Dir &Dir::operator=(const Dir &dir)
{
   m_implPtr = dir.m_implPtr;
   return *this;
}

Dir &Dir::operator=(const String &path)
{
   m_implPtr->setPath(path);
   return *this;
}

bool Dir::remove(const String &fileName)
{
   if (fileName.isEmpty()) {
      warning_stream("Dir::remove: Empty or null file name");
      return false;
   }
   return File::remove(getFilePath(fileName));
}

bool Dir::rename(const String &oldName, const String &newName)
{
   if (oldName.isEmpty() || newName.isEmpty()) {
      warning_stream("Dir::rename: Empty or null file name(s)");
      return false;
   }
   File file(getFilePath(oldName));
   if (!file.exists()) {
      return false;
   }
   return file.rename(getFilePath(newName));
}

bool Dir::exists(const String &name) const
{
   if (name.isEmpty()) {
      warning_stream("Dir::exists: Empty or null file name");
      return false;
   }
   return File::exists(getFilePath(name));
}

bool Dir::isEmpty(Filters filters) const
{
   PDK_D(const Dir);
   DirIterator iter(implPtr->m_dirEntry.getFilePath(), implPtr->m_nameFilters, filters);
   return !iter.hasNext();
}

FileInfoList Dir::getDrives()
{
#ifdef PDK_NO_FSFILEENGINE
   return FileInfoList();
#else
   return internal::FileEngine::getDrives();
#endif
}

Character Dir::getSeparator()
{
#if defined(PDK_OS_WIN)
   return Latin1Character('\\');
#else
   return Latin1Character('/');
#endif
}

bool Dir::setCurrent(const String &path)
{
   return FileSystemEngine::setCurrentPath(FileSystemEntry(path));
}

String Dir::getCurrentPath()
{
   return FileSystemEngine::getCurrentPath().getFilePath();
}

String Dir::getHomePath()
{
   return FileSystemEngine::getHomePath();
}

String Dir::getTempPath()
{
   return FileSystemEngine::getTempPath();
}

String Dir::getRootPath()
{
   return FileSystemEngine::getRootPath();
}

String Dir::cleanPath(const String &path)
{
   return clean_path(path);
}

bool Dir::isRelativePath(const String &path)
{
   return FileInfo(path).isRelative();
}

void Dir::refresh() const
{
   DirPrivate *implPtr = const_cast<Dir *>(this)->m_implPtr.data();
   implPtr->m_metaData.clear();
   implPtr->initFileEngine();
   implPtr->clearFileLists();
}

DirPrivate *Dir::getImplPtr()
{
   return m_implPtr.data();
}

StringList Dir::nameFiltersFromString(const String &nameFilter)
{
   return DirPrivate::splitFilters(nameFilter);
}

#ifndef PDK_NO_DEBUG_STREAM
Debug operator<<(Debug debug, Dir::Filters filters)
{
   DebugStateSaver save(debug);
   debug.resetFormat();
   StringList flags;
   if (filters == Dir::Filter::NoFilter) {
      flags << Latin1String("NoFilter");
   } else {
      if (filters & Dir::Filter::Dirs) flags << Latin1String("Dirs");
      if (filters & Dir::Filter::AllDirs) flags << Latin1String("AllDirs");
      if (filters & Dir::Filter::Files) flags << Latin1String("Files");
      if (filters & Dir::Filter::Drives) flags << Latin1String("Drives");
      if (filters & Dir::Filter::NoSymLinks) flags << Latin1String("NoSymLinks");
      if (filters & Dir::Filter::NoDot) flags << Latin1String("NoDot");
      if (filters & Dir::Filter::NoDotDot) flags << Latin1String("NoDotDot");
      if ((filters & Dir::Filter::AllEntries) == Dir::Filter::AllEntries) flags << Latin1String("AllEntries");
      if (filters & Dir::Filter::Readable) flags << Latin1String("Readable");
      if (filters & Dir::Filter::Writable) flags << Latin1String("Writable");
      if (filters & Dir::Filter::Executable) flags << Latin1String("Executable");
      if (filters & Dir::Filter::Modified) flags << Latin1String("Modified");
      if (filters & Dir::Filter::Hidden) flags << Latin1String("Hidden");
      if (filters & Dir::Filter::System) flags << Latin1String("System");
      if (filters & Dir::Filter::CaseSensitive) flags << Latin1String("CaseSensitive");
   }
   debug.noquote() << "Dir::Filters(" << flags.join(Latin1Character('|')) << ')';
   return debug;
}

static Debug operator<<(Debug debug, Dir::SortFlags sorting)
{
   DebugStateSaver save(debug);
   debug.resetFormat();
   if (sorting == Dir::SortFlag::NoSort) {
      debug << "Dir::SortFlags(NoSort)";
   } else {
      String type;
      if ((sorting & 3) == Dir::SortFlag::Name) type = Latin1String("Name");
      if ((sorting & 3) == Dir::SortFlag::Time) type = Latin1String("Time");
      if ((sorting & 3) == Dir::SortFlag::Size) type = Latin1String("Size");
      if ((sorting & 3) == Dir::SortFlag::Unsorted) type = Latin1String("Unsorted");
      
      StringList flags;
      if (sorting & Dir::SortFlag::DirsFirst) flags << Latin1String("DirsFirst");
      if (sorting & Dir::SortFlag::DirsLast) flags << Latin1String("DirsLast");
      if (sorting & Dir::SortFlag::IgnoreCase) flags << Latin1String("IgnoreCase");
      if (sorting & Dir::SortFlag::LocaleAware) flags << Latin1String("LocaleAware");
      if (sorting & Dir::SortFlag::Type) flags << Latin1String("Type");
      debug.noquote() << "Dir::SortFlags(" << type << '|' << flags.join(Latin1Character('|')) << ')';
   }
   return debug;
}

Debug operator<<(Debug debug, const Dir &dir)
{
   DebugStateSaver save(debug);
   debug.resetFormat();
   debug << "Dir(" << dir.getPath() << ", nameFilters = {"
         << dir.getNameFilters().join(Latin1Character(','))
         << "}, "
         << dir.getSorting()
         << ','
         << dir.getFilter()
         << ')';
   return debug;
}
#endif // PDK_NO_DEBUG_STREAM

} // fs
} // io
} // pdk
