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

#ifndef PDK_M_BASE_IO_FS_DIR_H
#define PDK_M_BASE_IO_FS_DIR_H

#include "pdk/base/lang/String.h"
#include "pdk/base/lang/Character.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/utils/SharedData.h"

namespace pdk {
namespace io {
namespace fs {

class DirIterator;

// forward declare class with namespace
namespace internal {
class DirPrivate;
} // internal

using pdk::ds::StringList;
using internal::DirPrivate;
using pdk::lang::String;
using pdk::lang::Latin1Character;
using pdk::lang::Character;

class PDK_CORE_EXPORT Dir
{
public:
   enum class Filter
   { 
      Dirs        = 0x001,
      Files       = 0x002,
      Drives      = 0x004,
      NoSymLinks  = 0x008,
      AllEntries  = Dirs | Files | Drives,
      TypeMask    = 0x00f,
      
      Readable    = 0x010,
      Writable    = 0x020,
      Executable  = 0x040,
      PermissionMask    = 0x070,
      
      Modified    = 0x080,
      Hidden      = 0x100,
      System      = 0x200,
      
      AccessMask  = 0x3F0,
      
      AllDirs       = 0x400,
      CaseSensitive = 0x800,
      NoDot         = 0x2000,
      NoDotDot      = 0x4000,
      NoDotAndDotDot = NoDot | NoDotDot,
      
      NoFilter = -1
   };
   PDK_DECLARE_FLAGS(Filters, Filter);
   
   enum class SortFlag 
   { 
      Name        = 0x00,
      Time        = 0x01,
      Size        = 0x02,
      Unsorted    = 0x03,
      SortByMask  = 0x03,
      
      DirsFirst   = 0x04,
      Reversed    = 0x08,
      IgnoreCase  = 0x10,
      DirsLast    = 0x20,
      LocaleAware = 0x40,
      Type        = 0x80,
      NoSort = -1
   };
   PDK_DECLARE_FLAGS(SortFlags, SortFlag);
   
   Dir(const Dir &);
   Dir(const String &path = String());
   Dir(const String &path, const String &nameFilter,
       SortFlags sort = SortFlags(pdk::as_integer<SortFlag>(SortFlag::Name) | pdk::as_integer<SortFlag>(SortFlag::IgnoreCase)), 
       Filters filter = Filter::AllEntries);
   ~Dir();
   
   Dir &operator=(const Dir &);
   Dir &operator=(const String &path);
   Dir &operator=(Dir &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   void swap(Dir &other) noexcept
   {
      std::swap(m_implPtr, other.m_implPtr);
   }
   
   void setPath(const String &path);
   String getPath() const;
   String getAbsolutePath() const;
   String getCanonicalPath() const;
   
   static void addResourceSearchPath(const String &path);
   
   static void setSearchPaths(const String &prefix, const StringList &searchPaths);
   static void addSearchPath(const String &prefix, const String &path);
   static StringList getSearchPaths(const String &prefix);
   
   String getDirName() const;
   String getFilePath(const String &fileName) const;
   String getAbsoluteFilePath(const String &fileName) const;
   String getRelativeFilePath(const String &fileName) const;
   
   static String toNativeSeparators(const String &pathName);
   static String fromNativeSeparators(const String &pathName);
   
   bool cd(const String &dirName);
   bool cdUp();
   
   StringList getNameFilters() const;
   void setNameFilters(const StringList &nameFilters);
   
   Filters getFilter() const;
   void setFilter(Filters filter);
   SortFlags getSorting() const;
   void setSorting(SortFlags sort);
   
   uint count() const;
   bool isEmpty(Filters filters = Filters(pdk::as_integer<Filter>(Filter::AllEntries) | 
                                          pdk::as_integer<Filter>(Filter::NoDotAndDotDot))) const;
   
   String operator[](int) const;
   
   static StringList nameFiltersFromString(const String &nameFilter);
   
   StringList entryList(Filters filters = Filter::NoFilter, SortFlags sort = SortFlag::NoSort) const;
   StringList entryList(const StringList &nameFilters, Filters filters = Filter::NoFilter,
                        SortFlags sort = SortFlag::NoSort) const;
   
   FileInfoList entryInfoList(Filters filters = Filter::NoFilter, SortFlags sort = SortFlag::NoSort) const;
   FileInfoList entryInfoList(const StringList &nameFilters, Filters filters = Filter::NoFilter,
                              SortFlags sort = SortFlag::NoSort) const;
   
   bool mkdir(const String &dirName) const;
   bool rmdir(const String &dirName) const;
   bool mkpath(const String &dirPath) const;
   bool rmpath(const String &dirPath) const;
   
   bool removeRecursively();
   
   bool isReadable() const;
   bool exists() const;
   bool isRoot() const;
   
   static bool isRelativePath(const String &path);
   inline static bool isAbsolutePath(const String &path)
   {
      return !isRelativePath(path);
   }
   
   bool isRelative() const;
   inline bool isAbsolute() const
   {
      return !isRelative();
   }
   
   bool makeAbsolute();
   
   bool operator==(const Dir &dir) const;
   inline bool operator!=(const Dir &dir) const
   {
      return !operator==(dir);
   }
   
   bool remove(const String &fileName);
   bool rename(const String &oldName, const String &newName);
   bool exists(const String &name) const;
   
   static FileInfoList getDrives();
   
   constexpr static inline Character getListSeparator() noexcept
   {
#if defined(PDK_OS_WIN)
      return Latin1Character(';');
#else
      return Latin1Character(':');
#endif
   }
   
   static Character getSeparator(); // ### Qt6: Make it inline
   
   static bool setCurrent(const String &path);
   static inline Dir getCurrent()
   {
      return Dir(getCurrentPath());
   }
   
   static String getCurrentPath();
   
   static inline Dir getHome()
   {
      return Dir(getHomePath());
   }
   
   static String getHomePath();
   static inline Dir getRoot()
   {
      return Dir(getRootPath());
   }
   
   static String getRootPath();
   static inline Dir getTemp()
   {
      return Dir(getTempPath());
   }
   
   static String getTempPath();
   
#ifndef PDK_NO_REGULAREXPRESSION
   static bool match(const StringList &filters, const String &fileName);
   static bool match(const String &filter, const String &fileName);
#endif
   
   static String cleanPath(const String &path);
   void refresh() const;
   
protected:
   explicit Dir(DirPrivate &d);
   
   pdk::utils::SharedDataPointer<DirPrivate> m_implPtr;
   
private:
   friend class DirIterator;
   // PDK_DECLARE_PRIVATE equivalent for shared data pointers
   DirPrivate* getImplPtr();
   inline const DirPrivate* getImplPtr() const
   {
      return m_implPtr.constData();
   }
   
};

} // fs
} // io
} // pdk

#endif // PDK_M_BASE_IO_FS_DIR_H
