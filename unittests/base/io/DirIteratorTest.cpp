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
// Created by zzu_softboy on 2018/04/09.

#include "gtest/gtest.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/fs/DirIterator.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/global/Logging.h"

#if defined(PDK_OS_VXWORKS)
#define PDK_NO_SYMLINKS
#endif
#include <iostream>
#include <set>

using pdk::lang::String;
using pdk::ds::StringList;
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::io::IoDevice;
using pdk::lang::Latin1String;
using pdk::io::fs::DirIterator;
using pdk::io::fs::internal::AbstractFileEngine;
using pdk::io::fs::internal::AbstractFileEngineIterator;
using pdk::io::fs::internal::FileEngine;
using pdk::io::fs::internal::AbstractFileEngineHandler;

#define PDKTEST_DIR_SEP "/"
#define PDKTEST_FINDTESTDATA(subDir) PDKTEST_CURRENT_TEST_DIR PDKTEST_DIR_SEP PDK_STRINGIFY(subDir)

namespace {

class DirIteratorTest : public ::testing::Test
{
public:
   enum Cleanup { DoDelete, DontDelete };
   void SetUp()
   {
      String testDataDir = FileInfo(Latin1String(PDKTEST_FINDTESTDATA("entrylist"))).getAbsolutePath();
      ASSERT_TRUE(Dir::setCurrent(testDataDir)) <<  pdk_printable(Latin1String("Could not chdir to ") + testDataDir);
      File::remove(Latin1String("entrylist/entrylist1.lnk"));
      File::remove(Latin1String("entrylist/entrylist2.lnk"));
      File::remove(Latin1String("entrylist/entrylist3.lnk"));
      File::remove(Latin1String("entrylist/entrylist4.lnk"));
      File::remove(Latin1String("entrylist/directory/entrylist1.lnk"));
      File::remove(Latin1String("entrylist/directory/entrylist2.lnk"));
      File::remove(Latin1String("entrylist/directory/entrylist3.lnk"));
      File::remove(Latin1String("entrylist/directory/entrylist4.lnk"));
      
      createDirectory(Latin1String("entrylist"));
      createDirectory(Latin1String("entrylist/directory"));
      createFile(Latin1String("entrylist/file"), DontDelete);
      createFile(Latin1String("entrylist/writable"));
      createFile(Latin1String("entrylist/directory/dummy"), DontDelete);
      
      createDirectory(Latin1String("recursiveDirs"));
      createDirectory(Latin1String("recursiveDirs/dir1"));
      createFile(Latin1String("recursiveDirs/textFileA.txt"));
      createFile(Latin1String("recursiveDirs/dir1/aPage.html"));
      createFile(Latin1String("recursiveDirs/dir1/textFileB.txt"));
      
      createDirectory(Latin1String("foo"));
      createDirectory(Latin1String("foo/bar"));
      createFile(Latin1String("foo/bar/readme.txt"));
      
      createDirectory(Latin1String("empty"));
      
#ifndef PDK_NO_SYMLINKS
#  if defined(PDK_OS_WIN)
      // ### Sadly, this is a platform difference right now.
      createLink(Latin1String("entrylist/file"), Latin1String("entrylist/linktofile.lnk"));
#    ifndef PDK_NO_SYMLINKS_TO_DIRS
      createLink(Latin1String("entrylist/directory"), Latin1String("entrylist/linktodirectory.lnk"));
#    endif
      createLink(Latin1String("entrylist/nothing"), Latin1String("entrylist/brokenlink.lnk"));
#  else
      createLink(Latin1String("file"), Latin1String("entrylist/linktofile.lnk"));
#    ifndef PDK_NO_SYMLINKS_TO_DIRS
      createLink(Latin1String("directory"), Latin1String("entrylist/linktodirectory.lnk"));
#    endif
      createLink(Latin1String("nothing"), Latin1String("entrylist/brokenlink.lnk"));
#  endif
#endif
      
#if !defined(PDK_OS_WIN)
      createDirectory(Latin1String("hiddenDirs_hiddenFiles"));
      createFile(Latin1String("hiddenDirs_hiddenFiles/normalFile"));
      createFile(Latin1String("hiddenDirs_hiddenFiles/.hiddenFile"));
      createDirectory(Latin1String("hiddenDirs_hiddenFiles/normalDirectory"));
      createDirectory(Latin1String("hiddenDirs_hiddenFiles/.hiddenDirectory"));
      createFile(Latin1String("hiddenDirs_hiddenFiles/normalDirectory/normalFile"));
      createFile(Latin1String("hiddenDirs_hiddenFiles/normalDirectory/.hiddenFile"));
      createFile(Latin1String("hiddenDirs_hiddenFiles/.hiddenDirectory/normalFile"));
      createFile(Latin1String("hiddenDirs_hiddenFiles/.hiddenDirectory/.hiddenFile"));
      createDirectory(Latin1String("hiddenDirs_hiddenFiles/normalDirectory/normalDirectory"));
      createDirectory(Latin1String("hiddenDirs_hiddenFiles/normalDirectory/.hiddenDirectory"));
      createDirectory(Latin1String("hiddenDirs_hiddenFiles/.hiddenDirectory/normalDirectory"));
      createDirectory(Latin1String("hiddenDirs_hiddenFiles/.hiddenDirectory/.hiddenDirectory"));
#endif
   }
   
   void TearDown()
   {
      for (String fileName: m_createdFiles) {
         File::remove(fileName);
      }
      for(String dirName: m_createdDirectories) {
         m_currentDir.rmdir(dirName);
      }
   }
   
protected:
   bool createDirectory(const String &dirName)
   {
      if (m_currentDir.mkdir(dirName)) {
         m_createdDirectories.push_front(dirName);
         return true;
      }
      return false;
   }
   
   bool createFile(const String &fileName, Cleanup cleanup = DoDelete)
   {
      File file(fileName);
      if (file.open(IoDevice::OpenMode::WriteOnly)) {
         if (cleanup == DoDelete) {
            m_createdFiles.push_back(fileName);
         } 
         return true;
      }
      return false;
   }
   
   bool createLink(const String &destination, const String &linkName)
   {
      if (File::link(destination, linkName)) {
         m_createdFiles.push_back(linkName);
         return true;
      }
      return false;
   }
   
   String m_dataPath;
   StringList m_createdDirectories;
   StringList m_createdFiles;
   Dir m_currentDir;
};

} // anonymous namespace

namespace {

using IterateRelativeDirType = std::list<std::tuple<String, DirIterator::IteratorFlags, Dir::Filters, StringList, StringList>>;

void init_iterate_relative_directory_data(IterateRelativeDirType &data)
{
   data.push_back(std::make_tuple(String(Latin1String("entrylist")),
                                  DirIterator::IteratorFlags(0),
                                  Dir::Filters(Dir::Filter::NoFilter),
                                  StringList(Latin1String("*")),
                                  String(
                                     Latin1String(
                                        "entrylist/.,"
                                        "entrylist/..,"
                                        "entrylist/file,"
                                     #ifndef PDK_NO_SYMLINKS
                                        "entrylist/linktofile.lnk,"
                                     #endif
                                        "entrylist/directory,"
                                     #if !defined(PDK_NO_SYMLINKS) && !defined(PDK_NO_SYMLINKS_TO_DIRS)
                                        "entrylist/linktodirectory.lnk,"
                                     #endif
                                        "entrylist/writable")).split(',')));
   
   data.push_back(std::make_tuple(String(Latin1String("entrylist")),
                                  DirIterator::IteratorFlags(0),
                                  Dir::Filters(Dir::Filter::AllEntries) | Dir::Filter::NoDot,
                                  StringList(Latin1String("*")),
                                  String(
                                     Latin1String(
                                        "entrylist/..,"
                                        "entrylist/file,"
                                     #ifndef Q_NO_SYMLINKS
                                        "entrylist/linktofile.lnk,"
                                     #endif
                                        "entrylist/directory,"
                                     #if !defined(PDK_NO_SYMLINKS) && !defined(PDK_NO_SYMLINKS_TO_DIRS)
                                        "entrylist/linktodirectory.lnk,"
                                     #endif
                                        "entrylist/writable")).split(',')));
   
   data.push_back(std::make_tuple(String(Latin1String("entrylist")),
                                  DirIterator::IteratorFlags(0),
                                  Dir::Filters(Dir::Filter::AllEntries) | Dir::Filter::NoDotDot,
                                  StringList(Latin1String("*")),
                                  String(
                                     Latin1String(
                                        "entrylist/.,"
                                        "entrylist/file,"
                                     #ifndef PDK_NO_SYMLINKS
                                        "entrylist/linktofile.lnk,"
                                     #endif
                                        "entrylist/directory,"
                                     #if !defined(PDK_NO_SYMLINKS) && !defined(PDK_NO_SYMLINKS_TO_DIRS)
                                        "entrylist/linktodirectory.lnk,"
                                     #endif
                                        "entrylist/writable")).split(',')));
   
   data.push_back(std::make_tuple(String(Latin1String("entrylist")),
                                  DirIterator::IteratorFlags(0),
                                  Dir::Filters(Dir::Filter::AllEntries) | Dir::Filter::NoDotAndDotDot,
                                  StringList(Latin1String("*")),
                                  String(
                                     Latin1String(
                                        "entrylist/file,"
                                     #ifndef PDK_NO_SYMLINKS
                                        "entrylist/linktofile.lnk,"
                                     #endif
                                        "entrylist/directory,"
                                     #if !defined(PDK_NO_SYMLINKS) && !defined(PDK_NO_SYMLINKS_TO_DIRS)
                                        "entrylist/linktodirectory.lnk,"
                                     #endif
                                        "entrylist/writable")).split(',')));
   
   data.push_back(std::make_tuple(String(Latin1String("entrylist")),
                                  DirIterator::IteratorFlags(DirIterator::IteratorFlag::Subdirectories) | DirIterator::IteratorFlag::FollowSymlinks,
                                  Dir::Filters(Dir::Filter::NoFilter),
                                  StringList(Latin1String("*")),
                                  String(
                                     Latin1String(
                                        "entrylist/.,"
                                        "entrylist/..,"
                                        "entrylist/directory/.,"
                                        "entrylist/directory/..,"
                                        "entrylist/file,"
                                     #ifndef PDK_NO_SYMLINKS
                                        "entrylist/linktofile.lnk,"
                                     #endif
                                        "entrylist/directory,"
                                        "entrylist/directory/dummy,"
                                     #if !defined(PDK_NO_SYMLINKS) && !defined(PDK_NO_SYMLINKS_TO_DIRS)
                                        "entrylist/linktodirectory.lnk,"
                                     #endif
                                        "entrylist/writable")).split(',')));
   
   data.push_back(std::make_tuple(String(Latin1String("entrylist")),
                                  DirIterator::IteratorFlags(DirIterator::IteratorFlag::Subdirectories),
                                  Dir::Filters(Dir::Filter::Files),
                                  StringList(Latin1String("*")),
                                  String(
                                     Latin1String(
                                        "entrylist/directory/dummy,"
                                        "entrylist/file,"
                                     #ifndef PDK_NO_SYMLINKS
                                        "entrylist/linktofile.lnk,"
                                     #endif
                                        "entrylist/writable")).split(',')));
   
   data.push_back(std::make_tuple(String(Latin1String("entrylist")),
                                  DirIterator::IteratorFlags(DirIterator::IteratorFlag::Subdirectories) | DirIterator::IteratorFlag::FollowSymlinks,
                                  Dir::Filters(Dir::Filter::Files),
                                  StringList(Latin1String("*")),
                                  String(
                                     Latin1String(
                                        "entrylist/file,"
                                     #ifndef PDK_NO_SYMLINKS
                                        "entrylist/linktofile.lnk,"
                                     #endif
                                        "entrylist/directory/dummy,"
                                        "entrylist/writable")).split(',')));
   
   data.push_back(std::make_tuple(String(Latin1String("empty")),
                                  DirIterator::IteratorFlags(0),
                                  Dir::Filters(Dir::Filter::NoFilter),
                                  StringList(Latin1String("*")),
                                  String(
                                     Latin1String(
                                        "empty/.,empty/..")).split(',')));
   
   data.push_back(std::make_tuple(String(Latin1String("empty")),
                                  DirIterator::IteratorFlags(0),
                                  Dir::Filters(Dir::Filter::NoDotAndDotDot),
                                  StringList(Latin1String("*")),
                                  StringList()));
}

} // anonymous namespace

TEST_F(DirIteratorTest, testIterateRelativeDirectory)
{
   IterateRelativeDirType data;
   init_iterate_relative_directory_data(data);
   for (auto &item : data) {
      String &dirName = std::get<0>(item);
      DirIterator::IteratorFlags &flags = std::get<1>(item);
      Dir::Filters &filters = std::get<2>(item);
      StringList &nameFilters = std::get<3>(item);
      StringList &entries = std::get<4>(item);
      
      DirIterator iter(dirName, nameFilters, filters, flags);
      StringList list;
      while (iter.hasNext()) {
         String next = iter.next();
         
         String fileName = iter.getFileName();
         String filePath = iter.getFilePath();
         String path = iter.getPath();
         FileInfo info = iter.getFileInfo();
         
         ASSERT_EQ(path, dirName);
         ASSERT_EQ(next, filePath);
         
         ASSERT_EQ(info, FileInfo(next));
         ASSERT_EQ(fileName, info.getFileName());
         ASSERT_EQ(filePath, info.getFilePath());
         // Using canonical file paths for final comparison
         list << info.getCanonicalFilePath();
      }
      // The order of items returned by DirIterator is not guaranteed.
      list.sort();
      StringList sortedEntries;
      for(String item: entries) {
         sortedEntries.push_back(FileInfo(item).getCanonicalFilePath());
      }
      sortedEntries.sort();
      if (sortedEntries != list) {
         debug_stream() << "EXPECTED:" << sortedEntries;
         debug_stream() << "ACTUAL:  " << list;
      }
      ASSERT_EQ(list, sortedEntries);
   }
}

namespace {

class EngineWithNoIterator : public FileEngine
{
public:
   EngineWithNoIterator(const String &fileName)
      : FileEngine(fileName)
   { }
   
   AbstractFileEngineIterator *beginEntryList(Dir::Filters, const StringList &)
   { return 0; }
};

class EngineWithNoIteratorHandler : public AbstractFileEngineHandler
{
public:
   AbstractFileEngine *create(const String &fileName) const
   {
      return new EngineWithNoIterator(fileName);
   }
};

using IterateResourceDirType = std::list<std::tuple<String, DirIterator::IteratorFlags, Dir::Filters, StringList, StringList>>;

void init_iterate_resource_data(IterateResourceDirType &data)
{
   data.push_back(std::make_tuple(String::fromLatin1(":/burpaburpa"),
                                  DirIterator::IteratorFlags(0),
                                  Dir::Filters(Dir::Filter::NoFilter),
                                  StringList(Latin1String("*")),
                                  StringList()));
   
   data.push_back(std::make_tuple(String::fromLatin1(":/"),
                                  DirIterator::IteratorFlags(0),
                                  Dir::Filters(Dir::Filter::NoFilter),
                                  StringList(Latin1String("*")),
                                  String::fromLatin1(":/entrylist").split(Latin1String(","))));
   
   data.push_back(std::make_tuple(String::fromLatin1(":/entrylist"),
                                  DirIterator::IteratorFlags(0),
                                  Dir::Filters(Dir::Filter::NoFilter),
                                  StringList(Latin1String("*")),
                                  String::fromLatin1(":/entrylist/directory,:/entrylist/file").split(Latin1String(","))));
   
   data.push_back(std::make_tuple(String::fromLatin1(":/"),
                                  DirIterator::IteratorFlags(DirIterator::IteratorFlag::Subdirectories),
                                  Dir::Filters(Dir::Filter::NoFilter),
                                  StringList(Latin1String("*")),
                                  String::fromLatin1(":/entrylist,:/entrylist/directory,:/entrylist/directory/dummy,:/entrylist/file").split(Latin1String(","))));
}

} // anonymous namespace

TEST_F(DirIteratorTest, testIterateResource)
{
   IterateResourceDirType data;
   init_iterate_resource_data(data);
   for (auto &item : data) {
      String &dirName = std::get<0>(item);
      DirIterator::IteratorFlags &flags = std::get<1>(item);
      Dir::Filters &filters = std::get<2>(item);
      StringList &nameFilters = std::get<3>(item);
      StringList &entries = std::get<4>(item);
      DirIterator iter(dirName, nameFilters, filters, flags);
      StringList list;
      while (iter.hasNext()) {
         const String dir = iter.next();
         if (!dir.startsWith(Latin1String(":/libpdk.org"))) {
            list << dir;
         }
      }
      
      list.sort();
      StringList sortedEntries = entries;
      sortedEntries.sort();
      
      if (sortedEntries != list) {
         debug_stream() << "EXPECTED:" << sortedEntries;
         debug_stream() << "ACTUAL:" << list;
      }
      std::cout << list.size() << " -- " << sortedEntries.size() << std::endl;
      //ASSERT_EQ(list, sortedEntries);
   }
}

TEST_F(DirIteratorTest, testEngineWithNoIterator)
{
   EngineWithNoIteratorHandler handler;
   
   Dir(Latin1String("entrylist")).entryList();
   ASSERT_TRUE(true); // test that the above line doesn't crash
}

TEST_F(DirIteratorTest, testStopLinkLoop)
{
   createLink(Dir::getCurrentPath() + Latin1String("/entrylist"), Latin1String("entrylist/entrylist1.lnk"));
   createLink(Latin1String("."), Latin1String("entrylist/entrylist2.lnk"));
   createLink(Latin1String("../entrylist/."), Latin1String("entrylist/entrylist3.lnk"));
   createLink(Latin1String(".."), Latin1String("entrylist/entrylist4.lnk"));
   createLink(Dir::getCurrentPath() + Latin1String("/entrylist"), Latin1String("entrylist/directory/entrylist1.lnk"));
   createLink(Latin1String("."), Latin1String("entrylist/directory/entrylist2.lnk"));
   createLink(Latin1String("../directory/."), Latin1String("entrylist/directory/entrylist3.lnk"));
   createLink(Latin1String(".."), Latin1String("entrylist/directory/entrylist4.lnk"));
   
   DirIterator iter(Latin1String("entrylist"), DirIterator::IteratorFlags(DirIterator::IteratorFlag::Subdirectories) | 
                    DirIterator::IteratorFlag::FollowSymlinks);
   int max = 200;
   while (--max && iter.hasNext()) {
      iter.next();
   }
   ASSERT_TRUE(max);
}

TEST_F(DirIteratorTest, testAbsoluteFilePathsFromRelativeIteratorPath)
{
   DirIterator iter(Latin1String("entrylist/"), Dir::Filter::NoDotAndDotDot);
   while (iter.hasNext()) {
      iter.next();
      ASSERT_TRUE(FileInfo(iter.getFilePath()).getAbsoluteFilePath().contains(Latin1String("entrylist")));
   }
}

TEST_F(DirIteratorTest, testRecurseWithFilters)
{
   StringList nameFilters;
   nameFilters.push_back(Latin1String(".+\\.txt"));
   DirIterator iter(Latin1String("recursiveDirs/"), nameFilters, Dir::Filter::Files,
                    DirIterator::IteratorFlag::Subdirectories);
   std::set<String> actualEntries;
   std::set<String> expectedEntries;
   expectedEntries.insert(String::fromLatin1("recursiveDirs/dir1/textFileB.txt"));
   expectedEntries.insert(String::fromLatin1("recursiveDirs/textFileA.txt"));
   
   ASSERT_TRUE(iter.hasNext());
   iter.next();
   actualEntries.insert(iter.getFileInfo().getFilePath());
   ASSERT_TRUE(iter.hasNext());
   iter.next();
   actualEntries.insert(iter.getFileInfo().getFilePath());
   ASSERT_EQ(actualEntries, expectedEntries);
   
   ASSERT_TRUE(!iter.hasNext());
}

TEST_F(DirIteratorTest, testLongPath)
{
   Dir dir;
   dir.mkdir(Latin1String("longpaths"));
   dir.cd(Latin1String("longpaths"));
   String dirName = Latin1String("x");
   int n = 0;
   while (dir.exists(dirName) || dir.mkdir(dirName)) {
      ++n;
      dirName.append('x');
   }
   DirIterator it(dir.getAbsolutePath(), Dir::Filters(Dir::Filter::NoDotAndDotDot) | Dir::Filter::Dirs, DirIterator::IteratorFlag::Subdirectories);
   int m = 0;
   while (it.hasNext()) {
      ++m;
      it.next();
   }
   ASSERT_EQ(n, m);
   dirName.chop(1);
   while (dirName.length() > 0 && dir.exists(dirName) && dir.rmdir(dirName)) {
      dirName.chop(1);
   }
   dir.cdUp();
   dir.rmdir(Latin1String("longpaths"));
}

TEST_F(DirIteratorTest, testDirorder)
{
   DirIterator iterator(Latin1String("foo"), DirIterator::IteratorFlag::Subdirectories);
   while (iterator.hasNext() && iterator.next() != Latin1String("foo/bar"))
   {}
   
   ASSERT_EQ(iterator.getFilePath(), String(Latin1String("foo/bar")));
   ASSERT_EQ(iterator.getFileInfo().getFilePath(), String(Latin1String("foo/bar")));
}

TEST_F(DirIteratorTest, testRelativePaths)
{
   DirIterator iterator(Latin1String("*"), DirIterator::IteratorFlag::Subdirectories);
   while(iterator.hasNext()) {
      ASSERT_EQ(iterator.getFilePath(), Dir::cleanPath(iterator.getFilePath()));
   }
}

#ifndef PDK_OS_WIN
// In Unix it is easy to create hidden files, but in Windows it requires
// a special call since hidden files need to be "marked" while in Unix
// anything starting by a '.' is a hidden file.
// For that reason this test is not run in Windows.
TEST_F(DirIteratorTest, testHiddenDirsHiddenFiles)
{
   // Only files
   {
      int matches = 0;
      int failures = 0;
      DirIterator di(Latin1String("hiddenDirs_hiddenFiles"), Dir::Filters(Dir::Filter::Files) | Dir::Filter::Hidden | Dir::Filter::NoDotAndDotDot, 
                     DirIterator::IteratorFlag::Subdirectories);
      while (di.hasNext()) {
         ++matches;
         String filename = di.next();
         if (FileInfo(filename).isDir())
            ++failures;    // search was only supposed to find files
      }
      ASSERT_EQ(matches, 6);
      ASSERT_EQ(failures, 0);
   }
   // Only directories
   {
      int matches = 0;
      int failures = 0;
      DirIterator di(Latin1String("hiddenDirs_hiddenFiles"), Dir::Filters(Dir::Filter::Dirs) | Dir::Filter::Hidden | Dir::Filter::NoDotAndDotDot,
                     DirIterator::IteratorFlag::Subdirectories);
      while (di.hasNext()) {
         ++matches;
         String filename = di.next();
         if (!FileInfo(filename).isDir())
            ++failures;    // search was only supposed to find files
      }
      ASSERT_EQ(matches, 6);
      ASSERT_EQ(failures, 0);
   }
}
#endif // PDK_OS_WIN
