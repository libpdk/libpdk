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
// Created by zzu_softboy on 2018/04/08.

#include "gtest/gtest.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/ds/StringList.h"
#include "../../shared/Filesystem.h"
#include "pdk/utils/Funcs.h"

#if defined(PDK_OS_UNIX)
# include <unistd.h>
# include <sys/stat.h>
#endif

#if defined(PDK_OS_VXWORKS)
#define PDK_NO_SYMLINKS
#endif
#include <iostream>

using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::ds::ByteArray;
using pdk::io::IoDevice;
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::ds::StringList;

#define PDKTEST_DIR_SEP "/"
#define PDKTEST_FINDTESTDATA(subDir) PDKTEST_CURRENT_TEST_DIR PDKTEST_DIR_SEP PDK_STRINGIFY(subDir)

PDK_UNITTEST_EXPORT String normalize_path_segments(const String &name, bool allowUncPaths,
                                                   bool *ok = nullptr);

namespace {

String sg_currentSourceDir(Latin1String(PDKTEST_CURRENT_TEST_SOURCE_DIR));
String sg_currentBinaryDir(Latin1String(PDKTEST_CURRENT_TEST_DIR));

ByteArray msg_does_not_exist(const String &name)
{
   return (Latin1Character('"') + Dir::toNativeSeparators(name)
           + Latin1String("\" does not exist.")).toLocal8Bit();
}

class DirTest : public ::testing::Test
{
   
public:
   enum Cleanup { DoDelete, DontDelete };
   enum UncHandling { HandleUnc, IgnoreUnc };
   static void SetUpTestCase()
   {
      m_dataPath = FileInfo(Latin1String(PDKTEST_CURRENT_TEST_DIR)).getAbsolutePath();
      EXPECT_TRUE(!m_dataPath.isEmpty()) << "test data not found";
      String testDataDir = FileInfo(Latin1String(PDKTEST_FINDTESTDATA("entrylist"))).getAbsolutePath();
      EXPECT_TRUE(Dir::setCurrent(testDataDir)) <<  pdk_printable(Latin1String("Could not chdir to ") + testDataDir);
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
   
   static void TearDownTestCase()
   {
      EXPECT_TRUE(Dir::setCurrent(sg_currentBinaryDir)) <<  pdk_printable(Latin1String("Could not chdir to ") + sg_currentBinaryDir);
      for (String fileName: m_createdFiles) {
         File::remove(fileName);
      }
      for(String dirName: m_createdDirectories) {
         m_currentDir.rmdir(dirName);
      }
   }
   
protected:
   static bool createDirectory(const String &dirName)
   {
      if (m_currentDir.mkdir(dirName)) {
         m_createdDirectories.push_front(dirName);
         return true;
      }
      return false;
   }
   
   static bool createFile(const String &fileName, Cleanup cleanup = DoDelete)
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
   
   static bool createLink(const String &destination, const String &linkName)
   {
      if (File::link(destination, linkName)) {
         m_createdFiles.push_back(linkName);
         return true;
      }
      return false;
   }
   
   static String m_dataPath;
   static StringList m_createdDirectories;
   static StringList m_createdFiles;
   static Dir m_currentDir;
};

String DirTest::m_dataPath;
StringList DirTest::m_createdDirectories;
StringList DirTest::m_createdFiles;
Dir DirTest::m_currentDir(sg_currentBinaryDir);

} // anonymous namespace

//TEST_F(DirTest, testGetSetCheck)
//{
//   Dir obj1;
//   // Filters Dir::getFilter()
//   // void Dir::setFilter(Filters)
//   obj1.setFilter(Dir::Filters(Dir::Filter::Dirs));
//   EXPECT_EQ(Dir::Filters(Dir::Filter::Dirs), obj1.getFilter());
//   obj1.setFilter(Dir::Filters(Dir::Filter::Dirs) | Dir::Filter::Files);
//   EXPECT_EQ(Dir::Filters(Dir::Filter::Dirs) | Dir::Filter::Files, obj1.getFilter());
//   obj1.setFilter(Dir::Filters(Dir::Filter::NoFilter));
//   EXPECT_EQ(Dir::Filters(Dir::Filter::NoFilter), obj1.getFilter());

//   // SortFlags Dir::getSorting()
//   // void Dir::setSorting(SortFlags)   
//   obj1.setSorting(Dir::SortFlags(Dir::SortFlag::Name));
//   EXPECT_EQ(Dir::SortFlags(Dir::SortFlag::Name), obj1.getSorting());
//   obj1.setSorting(Dir::SortFlags(Dir::SortFlag::Name) | Dir::SortFlag::IgnoreCase);
//   EXPECT_EQ(Dir::SortFlags(Dir::SortFlag::Name) | Dir::SortFlag::IgnoreCase, obj1.getSorting());
//   obj1.setSorting(Dir::SortFlags(Dir::SortFlag::NoSort));
//   EXPECT_EQ(Dir::SortFlags(Dir::SortFlag::NoSort), obj1.getSorting());
//}

//TEST_F(DirTest, testConstruction)
//{
//   FileInfo myFileInfo(Latin1String("/machine/share/dir1/file1"));
//   Dir myDir(myFileInfo.getAbsoluteDir()); // this asserte
//   EXPECT_EQ(myFileInfo.getAbsoluteDir().getAbsolutePath(), myDir.getAbsolutePath());
//}

//namespace {

//void init_setpath_data(std::list<std::tuple<String, String>> &data)
//{
//   data.push_back(std::make_tuple(String(Latin1String(".")), String(Latin1String(".."))));
//}

//} // anonymous namespace

//TEST_F(DirTest, testSetPath)
//{
//   std::list<std::tuple<String, String>> data;
//   init_setpath_data(data);
//   for (const auto &item: data) {
//      const String &dir1 = std::get<0>(item);
//      const String &dir2 = std::get<1>(item);

//      Dir shared;
//      Dir tempDir1(dir1);
//      StringList entries1 = tempDir1.entryList();
//      EXPECT_EQ(shared.entryList(), entries1);

//      Dir tempDir2(dir2);
//      StringList entries2 = tempDir2.entryList();
//      shared.setPath(dir2);
//      EXPECT_EQ(shared.entryList(), entries2);
//   }
//}

//namespace {

//void init_mkdir_rmdir_data(std::list<std::tuple<String, bool>> &data)
//{
//   StringList dirs;
//   dirs.push_back(String(Latin1String("testdir/one")));
//   dirs.push_back(String(Latin1String("testdir/two/three/four")));
//   dirs.push_back(String(Latin1String("testdir/../testdir/three")));

//   data.push_back(std::make_tuple(Dir::getCurrentPath() + Latin1String("/") + dirs.at(0), false));
//   data.push_back(std::make_tuple(Dir::getCurrentPath() + Latin1String("/") + dirs.at(1), true));
//   data.push_back(std::make_tuple(Dir::getCurrentPath() + Latin1String("/") + dirs.at(2), false));

//   data.push_back(std::make_tuple(dirs.at(0), false));
//   data.push_back(std::make_tuple(dirs.at(1), true));
//   data.push_back(std::make_tuple(dirs.at(2), false));

//   for (size_t i = 0; i < dirs.size(); ++i) {
//      EXPECT_TRUE(!File::exists(dirs.at(i)));
//   }
//}

//} // anonymous namespace

//TEST_F(DirTest, testMkdirRmdir)
//{
//   std::list<std::tuple<String, bool>> data;
//   init_mkdir_rmdir_data(data);
//   for (const auto &item: data) {
//      const String &path = std::get<0>(item);
//      const bool recurse = std::get<1>(item);
//      Dir dir;
//      dir.rmdir(path);
//      if (recurse) {
//         EXPECT_TRUE(dir.mkpath(path));
//      } else {
//         EXPECT_TRUE(dir.mkdir(path));
//      }
//      //make sure it really exists (ie that mkdir returns the right value)
//      FileInfo fileInfo(path);
//      EXPECT_TRUE(fileInfo.exists() && fileInfo.isDir()) << msg_does_not_exist(path).getConstRawData();
//      if (recurse) {
//         EXPECT_TRUE(dir.rmpath(path));
//      } else {
//         EXPECT_TRUE(dir.rmdir(path));
//      }
//      //make sure it really doesn't exist (ie that rmdir returns the right value)
//      fileInfo.refresh();
//      EXPECT_TRUE(!fileInfo.exists());
//   }
//}

//TEST_F(DirTest, testMkdirOnSymlink)
//{
//#if !defined(PDK_OS_UNIX) || defined(PDK_NO_SYMLINKS)
//   SUCCEED() << "Test only valid on an OS that supports symlinks";
//#else
//   // Create the structure:
//   //    .
//   //    ├── symlink -> two/three
//   //    └── two
//   //        └── three
//   // so when we mkdir("symlink/../four/five"), we end up with:
//   //    .
//   //    ├── symlink -> two/three
//   //    └── two
//   //        ├── four
//   //        │   └── five
//   //        └── three
//   Dir dir;
//   struct Clean
//   {
//      Dir &m_dir;
//      Clean(Dir &dir) : m_dir(dir) {}
//      ~Clean()
//      {
//         doClean();
//      }

//      void doClean()
//      {
//         m_dir.rmpath(Latin1String("two/three"));
//         m_dir.rmpath(Latin1String("two/four/five"));
//         // in case the test fails, don't leave junk behind
//         m_dir.rmpath(Latin1String("four/five"));
//         File::remove(Latin1String("symlink"));
//      }
//   };

//   Clean clean(dir);
//   clean.doClean();

//   // create our structure:
//   dir.mkpath(Latin1String("two/three"));
//   ::symlink("two/three", "symlink");

//   // try it:
//   String path = Latin1String("symlink/../four/five");
//   EXPECT_TRUE(dir.mkpath(path));
//   FileInfo fileInfo(path);
//   EXPECT_TRUE(fileInfo.exists() && fileInfo.isDir()) <<  msg_does_not_exist(path).getConstRawData();

//   path = Latin1String("two/four/five");
//   fileInfo.setFile(path);
//   EXPECT_TRUE(fileInfo.exists() && fileInfo.isDir()) << msg_does_not_exist(path).getConstRawData();
//#endif
//}

//TEST_F(DirTest, testMakeDirReturnCode)
//{
//   String dirName = String::fromLatin1("makedirReturnCode");
//   File file(Dir::getCurrent().getFilePath(dirName));
//   // cleanup a previous run.
//   file.remove();
//   Dir::getCurrent().rmdir(dirName);

//   Dir dir(dirName);
//   EXPECT_TRUE(!dir.exists());
//   EXPECT_TRUE(Dir::getCurrent().mkdir(dirName));
//   EXPECT_TRUE(!Dir::getCurrent().mkdir(dirName)); // calling mkdir on an existing dir will fail.
//   EXPECT_TRUE(Dir::getCurrent().mkpath(dirName)); // calling mkpath on an existing dir will pass

//   // Remove the directory and create a file with the same path
//   Dir::getCurrent().rmdir(dirName);
//   EXPECT_TRUE(!file.exists());
//   file.open(IoDevice::OpenMode::WriteOnly);
//   file.write("test");
//   file.close();
//   EXPECT_TRUE(file.exists()) <<  msg_does_not_exist(file.getFileName()).getConstRawData();
//   EXPECT_TRUE(!Dir::getCurrent().mkdir(dirName)); // calling mkdir on an existing file will fail.
//   EXPECT_TRUE(!Dir::getCurrent().mkpath(dirName)); // calling mkpath on an existing file will fail.
//   file.remove();
//}

namespace {

void init_remove_recursively_data(std::list<String> &data)
{
   const String tmpdir = Dir::getCurrentPath() + Latin1String("/tmpdir/");
   StringList dirs;
   dirs.push_back(tmpdir + Latin1String("empty"));
   dirs.push_back(tmpdir + Latin1String("one"));
   dirs.push_back(tmpdir + Latin1String("two/three"));
   dirs.push_back(Latin1String("relative"));
   Dir dir;
   for (size_t i = 0; i < dirs.size(); ++i) {
      dir.mkpath(dirs.at(i));
   }
   StringList files;
   files << tmpdir + Latin1String("one/file");
   files << tmpdir + Latin1String("two/three/file");
   for (size_t i = 0; i < files.size(); ++i) {
      File file(files.at(i));
      EXPECT_TRUE(file.open(IoDevice::OpenMode::WriteOnly));
      file.write("Hello");
   }
   
   data.push_back(tmpdir + Latin1String("empty"));
   data.push_back(tmpdir + Latin1String("one"));
   data.push_back(tmpdir + Latin1String("two"));
   data.push_back(tmpdir + Latin1String("doesnotexist"));
   data.push_back(Latin1String("relative"));
}

} // anonymous namespace

TEST_F(DirTest, testRemoveRecursively)
{
   std::list<String> data;
   init_remove_recursively_data(data);
   for (const String &path: data) {
      Dir dir(path);
      EXPECT_TRUE(dir.removeRecursively());
      //make sure it really doesn't exist (ie that remove worked)
      EXPECT_TRUE(!dir.exists());
   }
}

TEST_F(DirTest, testRemoveRecursivelyFailure)
{
#ifdef PDK_OS_UNIX
   if (::getuid() == 0) {
      SUCCEED() << "Running this test as root doesn't make sense";
   }
#endif
   const String tmpdir = Dir::getCurrentPath() + Latin1String("/tmpdir/");
   const String path = tmpdir + Latin1String("undeletable");
   Dir().mkpath(path);
   File file(path + Latin1String("/file"));
   EXPECT_TRUE(file.open(IoDevice::OpenMode::WriteOnly));
   file.write("Hello");
   file.close();
   
#ifdef PDK_OS_UNIX
   File dirAsFile(path); // yay, I have to use File to change a dir's permissions...
   EXPECT_TRUE(dirAsFile.setPermissions(File::Permissions(0))); // no permissions
   EXPECT_TRUE(!Dir().rmdir(path));
   Dir dir(path);
   EXPECT_TRUE(!dir.removeRecursively()); // didn't work
   EXPECT_TRUE(dir.exists())<< msg_does_not_exist(dir.getAbsolutePath()).getConstRawData(); // still exists
   
   EXPECT_TRUE(dirAsFile.setPermissions(File::Permissions(File::Permission::ReadOwner) | 
                                        File::Permission::WriteOwner | File::Permission::ExeOwner));
   EXPECT_TRUE(dir.removeRecursively());
   EXPECT_TRUE(!dir.exists());
#else // PDK_OS_UNIX
   EXPECT_TRUE(file.setPermissions(File::ReadOwner));
   EXPECT_TRUE(!Dir().rmdir(path));
   Dir dir(path);
   EXPECT_TRUE(dir.removeRecursively());
   EXPECT_TRUE(!dir.exists());
#endif // !PDK_OS_UNIX
}

TEST_F(DirTest, testRemoveRecursivelySymlink)
{
#ifndef PDK_NO_SYMLINKS
   const String tmpdir = Dir::getCurrentPath() + Latin1String("/tmpdir/");
   Dir().mkpath(tmpdir);
   Dir currentDir;
   currentDir.mkdir(Latin1String("myDir"));
   File(Latin1String("testfile")).open(IoDevice::OpenMode::WriteOnly);
   const String link = tmpdir + Latin1String("linkToDir.lnk");
   const String linkToFile = tmpdir + Latin1String("linkToFile.lnk");
#ifndef PDK_NO_SYMLINKS_TO_DIRS
   EXPECT_TRUE(File::link(Latin1String("../myDir"), link));
   EXPECT_TRUE(File::link(Latin1String("../testfile"), linkToFile));
#endif
   
   Dir dir(tmpdir);
   EXPECT_TRUE(dir.removeRecursively());
   EXPECT_TRUE(Dir(Latin1String("myDir")).exists()); // it didn't follow the symlink, good.
   EXPECT_TRUE(File::exists(Latin1String("testfile")));
   
   currentDir.rmdir(Latin1String("myDir"));
   File::remove(Latin1String("testfile"));
#endif
}

namespace {

void init_exists_data(std::list<std::tuple<String, bool>> &data)
{
   data.push_back(std::make_tuple(Dir::getCurrentPath(), true));
   data.push_back(std::make_tuple(Dir::getCurrentPath() + Latin1String("/"), true));
   data.push_back(std::make_tuple(String(Latin1String("/I/Do_not_expect_this_path_to_exist/")), false));
   data.push_back(std::make_tuple(Dir::getCurrentPath(), true));
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/resources"), true));
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/resources/"), true));
}

void init_is_relative_path_data(std::list<std::tuple<String, bool>> &data)
{
   data.push_back(std::make_tuple(Latin1String("../somedir"), true));
   data.push_back(std::make_tuple(Latin1String("somedir"), true));
   data.push_back(std::make_tuple(Latin1String("/somedir"), false));
}

} // anonymous namespace

TEST_F(DirTest, testExists)
{
   std::list<std::tuple<String, bool>> data;
   init_exists_data(data);
   for (auto item : data) {
      String &path = std::get<0>(item);
      bool expected = std::get<1>(item);
      Dir dir(path);
      if (expected) {
         EXPECT_TRUE(dir.exists()) << msg_does_not_exist(path).getConstRawData();
      } else {
         EXPECT_TRUE(!dir.exists());
      }
   }
}

TEST_F(DirTest, testIsRelativePath)
{
   std::list<std::tuple<String, bool>> data;
   init_is_relative_path_data(data);
   for (auto item : data) {
      String &path = std::get<0>(item);
      bool relative = std::get<1>(item);
      EXPECT_EQ(Dir::isRelativePath(path), relative);
   }
}

TEST_F(DirTest, testDefault)
{
   //default constructor Dir();
   Dir dir; // according to documentation should be currentDirPath
   EXPECT_EQ(dir.getAbsolutePath(), Dir::getCurrentPath());
}

TEST_F(DirTest, testCompare)
{
   Dir dir;
   dir.makeAbsolute();
   EXPECT_TRUE(dir == Dir::getCurrentPath());
   
   EXPECT_EQ(Dir(), Dir(Dir::getCurrentPath()));
   EXPECT_TRUE(Dir(Latin1String("../")) == Dir(Dir::getCurrentPath() + Latin1String("/..")));
}

namespace {

StringList filter_links(const StringList &list)
{
#ifndef PDK_NO_SYMLINKS
   return list;
#else
   StringList result;
   for (String &str: list) {
      if (!str.endsWith(Latin1String(Latin1String(".lnk")))) {
         result.push_back(str);
      }
   }
   return result;
#endif
}

void init_entry_list_data(std::list<std::tuple<String, StringList, int, int, StringList>> &data)
{
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/testdir/spaces"),
                                  StringList(Latin1String(".+\\. bar")), 
                                  (int)(Dir::Filter::NoFilter),
                                  (int)(Dir::SortFlag::NoSort),
                                  StringList(Latin1String("foo. bar"))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/testdir/spaces"),
                                  StringList(Latin1String(".+\\.bar")), 
                                  (int)(Dir::Filter::NoFilter),
                                  (int)(Dir::SortFlag::NoSort),
                                  StringList(Latin1String("foo.bar"))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/testdir/spaces"),
                                  StringList(Latin1String("foo\\..+")), 
                                  (int)(Dir::Filter::NoFilter),
                                  (int)(Dir::SortFlag::NoSort),
                                  String(Latin1String("foo. bar,foo.bar")).split(',')));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/testdir/dir"),
                                  String(Latin1String(".+r\\.cpp .+\\.pdk")).split(Latin1String(" ")), 
                                  (int)(Dir::Filter::NoFilter),
                                  (int)(Dir::SortFlag::NoSort),
                                  String(Latin1String("dir.pdk,rc_dir.cpp,test_dir.cpp")).split(',')));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/testdir"),
                                  StringList(), 
                                  (int)(Dir::Filter::AllDirs),
                                  (int)(Dir::SortFlag::NoSort),
                                  String(Latin1String(".,..,dir,spaces")).split(',')));
}

void init_entry_list_with_test_files_data(std::list<std::tuple<String, StringList, int, int, StringList>> &data)
{
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::NoFilter),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,file,linktodirectory.lnk,linktofile.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllEntries),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,file,linktodirectory.lnk,linktofile.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::Files),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("file,linktofile.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::Dirs),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,linktodirectory.lnk")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::Dirs) | int(Dir::Filter::NoDotAndDotDot),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("directory,linktodirectory.lnk")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllDirs),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,linktodirectory.lnk")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllDirs) | int(Dir::Filter::Dirs),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,linktodirectory.lnk")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllDirs) | int(Dir::Filter::Files),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,file,linktodirectory.lnk,linktofile.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllEntries) | int(Dir::Filter::NoSymLinks),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,file,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllEntries) | int(Dir::Filter::NoSymLinks) | int(Dir::Filter::NoDotAndDotDot),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("directory,file,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::Files) | int(Dir::Filter::NoSymLinks),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("file,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::Dirs) | int(Dir::Filter::NoSymLinks),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::Drives) | int(Dir::Filter::Files) | int(Dir::Filter::NoDotAndDotDot),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("file,linktofile.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::System),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("brokenlink.lnk")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::Hidden),
   //                                  int(Dir::SortFlag::Name),
   //                                  StringList()));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::System) | int(Dir::Filter::Hidden),
   //                                  int(Dir::SortFlag::Name),
   //                                  StringList(Latin1String("brokenlink.lnk"))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllDirs) | int(Dir::Filter::NoSymLinks),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllEntries) | int(Dir::Filter::Hidden) | int(Dir::Filter::System),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,brokenlink.lnk,directory,file,linktodirectory.lnk,linktofile.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllEntries) | int(Dir::Filter::Readable),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,file,linktodirectory.lnk,linktofile.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllEntries) | int(Dir::Filter::Writable),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,linktodirectory.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::AllEntries) | int(Dir::Filter::Writable),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,linktodirectory.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::Files) | int(Dir::Filter::Readable),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("file,linktofile.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::Dirs) | int(Dir::Filter::Readable),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,linktodirectory.lnk")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("^d.*")),
   //                                  int(Dir::Filter::NoFilter),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("directory")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("^f.*")),
   //                                  int(Dir::Filter::NoFilter),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("file")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("^link.*")),
   //                                  int(Dir::Filter::NoFilter),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("linktodirectory.lnk,linktofile.lnk")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String(".*to.*")),
   //                                  int(Dir::Filter::NoFilter),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String("directory,linktodirectory.lnk,linktofile.lnk")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::NoFilter),
   //                                  int(Dir::SortFlag::Name),
   //                                  filter_links(String(Latin1String(".,..,directory,file,linktodirectory.lnk,linktofile.lnk,writable")).split(','))));
   
   //   data.push_back(std::make_tuple(sg_currentBinaryDir + Latin1String("/entrylist/"),
   //                                  StringList(Latin1String("*")),
   //                                  int(Dir::Filter::NoFilter),
   //                                  int(Dir::SortFlag::Name) | int(Dir::SortFlag::Reversed),
   //                                  filter_links(String(Latin1String("writable,linktofile.lnk,linktodirectory.lnk,file,directory,..,.")).split(','))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/types/"),
                                  StringList(Latin1String("*")),
                                  int(Dir::Filter::NoFilter),
                                  int(Dir::SortFlag::Type),
                                  filter_links(String(Latin1String(".,..,a,b,c,d,e,f,a.a,b.a,c.a,d.a,e.a,f.a,a.b,b.b,c.b,d.b,e.b,f.b,a.c,b.c,c.c,d.c,e.c,f.c")).split(','))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/types/"),
                                  StringList(Latin1String("*")),
                                  int(Dir::Filter::NoFilter),
                                  int(Dir::SortFlag::Type) | int(Dir::SortFlag::Reversed),
                                  filter_links(String(Latin1String("f.c,e.c,d.c,c.c,b.c,a.c,f.b,e.b,d.b,c.b,b.b,a.b,f.a,e.a,d.a,c.a,b.a,a.a,f,e,d,c,b,a,..,.")).split(','))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/types/"),
                                  StringList(Latin1String("*")),
                                  int(Dir::Filter::NoFilter),
                                  int(Dir::SortFlag::Type) | int(Dir::SortFlag::DirsLast),
                                  filter_links(String(Latin1String("a,b,c,a.a,b.a,c.a,a.b,b.b,c.b,a.c,b.c,c.c,.,..,d,e,f,d.a,e.a,f.a,d.b,e.b,f.b,d.c,e.c,f.c")).split(','))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/types/"),
                                  StringList(Latin1String("*")),
                                  int(Dir::Filter::NoFilter),
                                  int(Dir::SortFlag::Type) | int(Dir::SortFlag::DirsFirst),
                                  filter_links(String(Latin1String(".,..,d,e,f,d.a,e.a,f.a,d.b,e.b,f.b,d.c,e.c,f.c,a,b,c,a.a,b.a,c.a,a.b,b.b,c.b,a.c,b.c,c.c")).split(','))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/types/"),
                                  StringList(Latin1String("*")),
                                  int(Dir::Filter::AllEntries) | int(Dir::Filter::NoDotAndDotDot),
                                  int(Dir::SortFlag::Size) | int(Dir::SortFlag::DirsFirst),
                                  filter_links(String(Latin1String("d,d.a,d.b,d.c,e,e.a,e.b,e.c,f,f.a,f.b,f.c,c.a,c.b,c.c,b.a,b.c,b.b,a.c,a.b,a.a,a,b,c")).split(','))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/types/"),
                                  StringList(Latin1String("*")),
                                  int(Dir::Filter::AllEntries) | int(Dir::Filter::NoDotAndDotDot),
                                  int(Dir::SortFlag::Size) | int(Dir::SortFlag::Reversed) | int(Dir::SortFlag::DirsLast),
                                  filter_links(String(Latin1String("c,b,a,a.a,a.b,a.c,b.b,b.c,b.a,c.c,c.b,c.a,f.c,f.b,f.a,f,e.c,e.b,e.a,e,d.c,d.b,d.a,d")).split(','))));
}

} // anonymous namespace

//TEST_F(DirTest, testEntryList)
//{
//   std::list<std::tuple<String, StringList, int, int, StringList>> data;
//   init_entry_list_data(data);
//   for (auto &item : data) {
//      String &dirName = std::get<0>(item);
//      StringList &nameFilters = std::get<1>(item);
//      int filterSpec = std::get<2>(item);
//      int sortSpec = std::get<3>(item);
//      StringList &expected = std::get<4>(item);

//      Dir dir(dirName);
//      EXPECT_TRUE(dir.exists()) << msg_does_not_exist(dirName).getConstRawData();
//      StringList actual = dir.entryList(nameFilters, (Dir::Filters)filterSpec,
//                                        (Dir::SortFlags)sortSpec);
//      EXPECT_EQ(actual, expected);
//   }
//}

//TEST_F(DirTest, testEntryListWithTestFiles)
//{
//   std::list<std::tuple<String, StringList, int, int, StringList>> data;
//   init_entry_list_with_test_files_data(data);

//   for (auto &item : data) {
//      String &dirName = std::get<0>(item);
//      StringList &nameFilters = std::get<1>(item);
//      int filterSpec = std::get<2>(item);
//      int sortSpec = std::get<3>(item);
//      StringList &expected = std::get<4>(item);
//      StringList testFiles;
//      Dir::setCurrent(dirName);
//      String entrylistPath = (sg_currentBinaryDir + Latin1String("/entrylist/"));

//      {
//         const String writableFileName = entrylistPath + Latin1String("writable");
//         File writableFile(writableFileName);
//         testFiles.push_back(writableFileName);

//         EXPECT_TRUE(writableFile.open(IoDevice::OpenMode::ReadWrite)) << pdk_printable(writableFile.getErrorString());
//      }
//      {
//         File readOnlyFile(entrylistPath + Latin1String("file"));
//         EXPECT_TRUE(readOnlyFile.setPermissions(File::Permissions(File::Permission::ReadOwner) | File::Permission::ReadUser)) << pdk_printable(readOnlyFile.getErrorString());
//      }


//#ifndef PDK_NO_SYMLINKS
//#if defined(PDK_OS_WIN)
//      // ### Sadly, this is a platform difference right now.
//      // Note we are using capital L in entryList on one side here, to test case-insensitivity
//      const std::vector<std::pair<String, String>> symLinks =
//      {
//         {sg_currentSourceDir + Latin1String("/entryList/file"), entrylistPath + Latin1String("linktofile.lnk")},
//         {sg_currentSourceDir + Latin1String("/entryList/directory"), entrylistPath + Latin1String("linktodirectory.lnk")},
//         {sg_currentSourceDir + Latin1String("/entryList/nothing"), entrylistPath + Latin1String("brokenlink.lnk")}
//      };
//#else
//      const std::vector<std::pair<String, String>> symLinks =
//      {
//         {Latin1String("file"), entrylistPath + Latin1String("linktofile.lnk")},
//         {Latin1String("directory"), entrylistPath + Latin1String("linktodirectory.lnk")},
//         {Latin1String("nothing"), entrylistPath + Latin1String("brokenlink.lnk")}
//      };
//#endif

//      for (const auto &symLink : symLinks) {
//         EXPECT_TRUE(File::link(symLink.first, symLink.second)) << pdk_printable(symLink.first + Latin1String("->") + symLink.second);
//         testFiles.push_back(symLink.second);
//      }
//#endif //PDK_NO_SYMLINKS

//      Dir dir(dirName);
//      EXPECT_TRUE(dir.exists()) << msg_does_not_exist(dirName).getConstRawData();
//      StringList actual = dir.entryList(nameFilters, (Dir::Filters)filterSpec,
//                                        (Dir::SortFlags)sortSpec);
//      bool doContentCheck = true;
////#if defined(PDK_OS_UNIX)
////      if (pdk::strcmp(currentDataTag(), "Dir::AllEntries | Dir::Writable") == 0) {
////         // for root, everything is writeable
////         if (::getuid() == 0)
////            doContentCheck = false;
////      }
////#endif
//      for (int i = testFiles.size() - 1; i >= 0; --i) {
//         EXPECT_TRUE(File::remove(testFiles.at(i))) << pdk_printable(testFiles.at(i));
//      }
//      if (doContentCheck) {
//         EXPECT_EQ(actual, expected);
//      }
//   }

//}

// @TODO testEntryListTimedSort

namespace {

void init_entry_list_simple_data(std::list<std::tuple<String, int>> &data)
{
   data.push_back(std::make_tuple(Latin1String("do_not_expect_this_path_to_exist/"), 0));
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/resources"), 2));
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/resources/"), 2));
   // @TODO Windows platform test
}

ByteArray msg_entry_list_failed(int actual, int expectedMin, const String &name)
{
   return ByteArray::number(actual) + " < " + ByteArray::number(expectedMin) + " in \""
         + File::encodeName(Dir::toNativeSeparators(name)) + '"';
}

} // anonymous namespace

TEST_F(DirTest, testEntryListSimple)
{
   std::list<std::tuple<String, int>> data;
   init_entry_list_simple_data(data);
   for (auto &item : data) {
      String &dirName = std::get<0>(item);
      size_t countMin = std::get<1>(item);
      Dir dir(dirName);
      StringList actual = dir.entryList();
      ASSERT_TRUE(actual.size() >= countMin) << msg_entry_list_failed(actual.size(), countMin, dirName).getConstRawData();
   }   
}

TEST_F(DirTest, testEntryListWithSymLinks)
{
#ifndef PDK_NO_SYMLINKS
#  ifndef PDK_NO_SYMLINKS_TO_DIRS
   File::remove(Latin1String("myLinkToDir.lnk"));
#  endif
   File::remove(Latin1String("myLinkToFile.lnk"));
   File::remove(Latin1String("testfile.cpp"));
   Dir dir;
   dir.mkdir(Latin1String("myDir"));
   File(Latin1String("testfile.cpp")).open(IoDevice::OpenMode::WriteOnly);
#  ifndef PDK_NO_SYMLINKS_TO_DIRS
   ASSERT_TRUE(File::link(Latin1String("myDir"), Latin1String("myLinkToDir.lnk")));
#  endif
   ASSERT_TRUE(File::link(Latin1String("testfile.cpp"), Latin1String("myLinkToFile.lnk")));
   
   {
      StringList entryList = Dir().entryList();
      ASSERT_TRUE(entryList.contains(Latin1String("myDir")));
#  ifndef PDK_NO_SYMLINKS_TO_DIRS
      ASSERT_TRUE(entryList.contains(Latin1String("myLinkToDir.lnk")));
#endif
      ASSERT_TRUE(entryList.contains(Latin1String("myLinkToFile.lnk")));
   }
   {
      StringList entryList = Dir().entryList(Dir::Filters(Dir::Filter::Dirs));
      ASSERT_TRUE(entryList.contains(Latin1String("myDir")));
#  ifndef Q_NO_SYMLINKS_TO_DIRS
      ASSERT_TRUE(entryList.contains(Latin1String("myLinkToDir.lnk")));
#endif
      ASSERT_TRUE(!entryList.contains(Latin1String("myLinkToFile.lnk")));
   }
   {
      StringList entryList = Dir().entryList(Dir::Filters(Dir::Filter::Dirs) | Dir::Filter::NoSymLinks);
      ASSERT_TRUE(entryList.contains(Latin1String("myDir")));
      ASSERT_TRUE(!entryList.contains(Latin1String("myLinkToDir.lnk")));
      ASSERT_TRUE(!entryList.contains(Latin1String("myLinkToFile.lnk")));
   }
   
   File::remove(Latin1String("myLinkToDir.lnk"));
   File::remove(Latin1String("myLinkToFile.lnk"));
   File::remove(Latin1String("testfile.cpp"));
   dir.rmdir(Latin1String("myDir"));
#endif
}

namespace {

void init_canonical_path_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("."), sg_currentSourceDir));
   data.push_back(std::make_tuple(Latin1String("./testData/../testData"), sg_currentSourceDir + Latin1String("/testData")));
   // @TODO test Windows Platform
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/testData/../testData"), sg_currentSourceDir + Latin1String("/testData")));
   data.push_back(std::make_tuple(Latin1String("testd"), String()));
   data.push_back(std::make_tuple(Dir::getRootPath(), Dir::getRootPath()));
   data.push_back(std::make_tuple(Dir::getRootPath().append(Latin1String("./")), Dir::getRootPath()));
   data.push_back(std::make_tuple(Dir::getRootPath().append(Latin1String("../..")), Dir::getRootPath()));
}

void init_current_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(String(), sg_currentSourceDir));
   data.push_back(std::make_tuple(String(Latin1String("testData")), sg_currentSourceDir + Latin1String("/testData")));
#ifndef PDK_OS_WIN
   data.push_back(std::make_tuple(sg_currentSourceDir + String(Latin1String("/testData")), sg_currentSourceDir + Latin1String("/testData")));
#else
   data.push_back(std::make_tuple(sg_currentSourceDir + String(Latin1String("\\testData")), sg_currentSourceDir + Latin1String("/testData")));
#endif
   data.push_back(std::make_tuple(String(Latin1String("testd")), String()));
   data.push_back(std::make_tuple(String(Latin1String("..")), sg_currentSourceDir.left(sg_currentSourceDir.lastIndexOf('/'))));
}

void init_cd_data(std::list<std::tuple<String, String, bool, String>> &data)
{
   int index = sg_currentSourceDir.lastIndexOf(Latin1Character('/'));
   data.push_back(std::make_tuple(sg_currentSourceDir, Latin1String(".."), true, 
                                  sg_currentSourceDir.left(index == 0 ? 1 : index)));
   
   data.push_back(std::make_tuple(Latin1String("anonexistingDir"), Latin1String(".."), true, 
                                  sg_currentSourceDir));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/anonexistingDir"), Latin1String(".."), true, 
                                  sg_currentSourceDir));
   
   data.push_back(std::make_tuple(sg_currentSourceDir, Latin1String("."), true, 
                                  sg_currentSourceDir));
#if defined(PDK_OS_WIN)  // on windows Dir::getRoot() is usually c:/ but cd "/" will not force it to be root
   data.push_back(std::make_tuple(sg_currentSourceDir, Latin1String("/"), true, 
                                  Latin1String("/")));
#else
   data.push_back(std::make_tuple(sg_currentSourceDir, Latin1String("/"), true, 
                                  Dir::getRoot().getAbsolutePath()));
#endif
   data.push_back(std::make_tuple(Latin1String("."), Latin1String("../anonexistingdir"), false, 
                                  sg_currentSourceDir));
   data.push_back(std::make_tuple(Latin1String("."), Latin1String("../") + FileInfo(sg_currentSourceDir).getFileName(), true, 
                                  sg_currentSourceDir));
   data.push_back(std::make_tuple(Latin1String("."), Latin1String("dir.pdk"), false, 
                                  sg_currentSourceDir));
}

} // anonymous namespace

TEST_F(DirTest, testCanonicalPath)
{
   Dir dataDir(sg_currentSourceDir);
   if (dataDir.getAbsolutePath() != dataDir.getCanonicalPath()) {
      FAIL() << "This test does not work if this directory path consists of symlinks.";
   }
   String oldpwd = Dir::getCurrentPath();
   Dir::setCurrent(dataDir.getAbsolutePath());
   
   std::list<std::tuple<String, String>> data;
   for (auto &item : data) {
      String &path = std::get<0>(item);
      String &canonicalPath = std::get<1>(item);
      Dir dir(path);
#if defined(PDK_OS_WIN)
      ASSERT_EQ(dir.getCanonicalPath().toLower(), canonicalPath.toLower());
#else
      ASSERT_EQ(dir.getCanonicalPath(), canonicalPath);
#endif
      Dir::setCurrent(oldpwd);
   }
}

TEST_F(DirTest, testCurrent)
{
   std::list<std::tuple<String, String>> data;
   init_current_data(data);
   String oldDir = Dir::getCurrentPath();
   Dir::setCurrent(sg_currentSourceDir);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      String &currentDir = std::get<1>(item);
      if (!path.isEmpty()) {
         bool b = Dir::setCurrent(path);
         // If path is non existent, then setCurrent should be false (currentDir is empty in testData)
         ASSERT_EQ(b, !currentDir.isEmpty());
      }
      if (!currentDir.isEmpty()) {
         Dir newCurrent = Dir::getCurrent();
         Dir::setCurrent(oldDir);
#if defined(PDK_OS_WIN)
         ASSERT_EQ(newCurrent.getAbsolutePath().toLower(), currentDir.toLower());
#else
         ASSERT_EQ(newCurrent.getAbsolutePath(), currentDir);
#endif
      }
      Dir::setCurrent(oldDir);
   }
}

TEST_F(DirTest, testCd)
{
   std::list<std::tuple<String, String, bool, String>> data;
   init_cd_data(data);
   for (auto &item : data) {
      String &startDir = std::get<0>(item);
      String &cdDir = std::get<1>(item);
      bool successExpected = std::get<2>(item);
      String &newDir = std::get<3>(item);
      Dir d = startDir;
      bool notUsed = d.exists(); // make sure we cache this before so we can see if 'cd' fails to flush this
      PDK_UNUSED(notUsed);
      ASSERT_EQ(d.cd(cdDir), successExpected);
      ASSERT_EQ(d.getAbsolutePath(), newDir);
   }
}

namespace {

void init_set_name_filters_data(std::list<std::tuple<String, StringList, StringList>> &data)
{
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/testdir/spaces"),
                                  StringList(Latin1String(".+\\. bar")),
                                  StringList(Latin1String("foo. bar"))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/testdir/spaces"),
                                  StringList(Latin1String(".+\\.bar")),
                                  StringList(Latin1String("foo.bar"))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/testdir/spaces"),
                                  StringList(Latin1String("foo.*")),
                                  String(Latin1String("foo. bar,foo.bar")).split(Latin1Character(','))));
   
   data.push_back(std::make_tuple(sg_currentSourceDir + Latin1String("/testdir/dir"),
                                  String(Latin1String(".*r\\.cpp .*\\.pdk")).split(Latin1Character(' ')),
                                  String(Latin1String("dir.pdk,rc_dir.cpp,test_dir.cpp")).split(Latin1Character(','))));
   // @TODO test resource
}

void init_clean_path_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("/Users/programs/pdk1.0//.."),
                                  Latin1String("/Users/programs")));
   
   data.push_back(std::make_tuple(Latin1String("/Users/programs////qcoreteam/pdk1.0//.."),
                                  Latin1String("/Users/programs/qcoreteam")));
   
   data.push_back(std::make_tuple(Latin1String("/"),
                                  Latin1String("/")));
   
   data.push_back(std::make_tuple(Latin1String("/path/.."),
                                  Latin1String("/")));
   
   data.push_back(std::make_tuple(Latin1String("/.."),
                                  Latin1String("/..")));
   
   data.push_back(std::make_tuple(Dir::cleanPath(Latin1String("../.")),
                                  Latin1String("..")));
   
   data.push_back(std::make_tuple(Dir::cleanPath(Latin1String("../..")),
                                  Latin1String("../..")));
   
#if defined(PDK_OS_WIN)
   data.push_back(std::make_tuple(Latin1String("d:\\a\\bc\\def\\.."),
                                  Latin1String("d:/a/bc")));
   data.push_back(std::make_tuple(Latin1String("d:\\a\\bc\\def\\../../.."),
                                  Latin1String("d:/a/bc")));
#else
   data.push_back(std::make_tuple(Latin1String("d:\\a\\bc\\def\\.."),
                                  Latin1String("d:\\a\\bc\\def\\..")));
   data.push_back(std::make_tuple(Latin1String("d:\\a\\bc\\def\\../../.."),
                                  Latin1String("..")));
   
#endif
   
   data.push_back(std::make_tuple(Latin1String(".//file1.txt"),
                                  Latin1String("file1.txt")));
   data.push_back(std::make_tuple(Latin1String("/foo/bar/..//file1.txt"),
                                  Latin1String("/foo/file1.txt")));
   data.push_back(std::make_tuple(Latin1String("//"),
                                  Latin1String("/")));
   
#if defined PDK_OS_WIN
   data.push_back(std::make_tuple(Latin1String("c:\\"),
                                  Latin1String("c:/")));
#else
   data.push_back(std::make_tuple(Latin1String("/:/"), Latin1String("/:")));
#endif
#if defined(PDK_OS_WIN)
   data.push_back(std::make_tuple(Latin1String("//foo//bar"),
                                  Latin1String("//foo/bar")));
#endif
   data.push_back(std::make_tuple(Latin1String("ab/a/"), Latin1String("ab/a")));
   
#ifdef PDK_OS_WIN
   data.push_back(std::make_tuple(Latin1String("c://"),
                                  Latin1String("c:/")));
#else
   data.push_back(std::make_tuple(Latin1String("c://"), Latin1String("c:")));
#endif
   data.push_back(std::make_tuple(Latin1String("c://foo"), Latin1String("c:/foo")));
   
   data.push_back(std::make_tuple(Latin1String("foo/.."), Latin1String(".")));
   data.push_back(std::make_tuple(Latin1String("foo/../"), Latin1String(".")));
   data.push_back(std::make_tuple(Latin1String("/foo/./bar"), Latin1String("/foo/bar")));
   data.push_back(std::make_tuple(Latin1String("./foo/.."), Latin1String(".")));
   data.push_back(std::make_tuple(Latin1String("./foo/../"), Latin1String(".")));
   
}

void init_normalize_path_segments_data(std::list<std::tuple<String, DirTest::UncHandling, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("/Users/programs/qcoreteam/pdk1.0//.."),
                                  DirTest::HandleUnc,
                                  Latin1String("/Users/programs/qcoreteam")));
   
   data.push_back(std::make_tuple(Latin1String("/Users/programs////qcoreteam/pdk1.0//.."),
                                  DirTest::HandleUnc,
                                  Latin1String("/Users/programs/qcoreteam")));
   
   data.push_back(std::make_tuple(Latin1String("/"),
                                  DirTest::HandleUnc,
                                  Latin1String("/")));
   
   data.push_back(std::make_tuple(Latin1String("//"),
                                  DirTest::HandleUnc,
                                  Latin1String("//")));
   
   data.push_back(std::make_tuple(Latin1String("//"),
                                  DirTest::IgnoreUnc,
                                  Latin1String("/")));
   
   data.push_back(std::make_tuple(Latin1String("/."),
                                  DirTest::HandleUnc,
                                  Latin1String("/")));
   
   data.push_back(std::make_tuple(Latin1String("/./"),
                                  DirTest::HandleUnc,
                                  Latin1String("/")));
   
   data.push_back(std::make_tuple(Latin1String("/.."),
                                  DirTest::HandleUnc,
                                  Latin1String("/..")));
   
   data.push_back(std::make_tuple(Latin1String("/../"),
                                  DirTest::HandleUnc,
                                  Latin1String("/../")));
   
   data.push_back(std::make_tuple(Latin1String("."),
                                  DirTest::HandleUnc,
                                  Latin1String(".")));
   
   data.push_back(std::make_tuple(Latin1String("./"),
                                  DirTest::HandleUnc,
                                  Latin1String("./")));
   
   data.push_back(std::make_tuple(Latin1String("./."),
                                  DirTest::HandleUnc,
                                  Latin1String(".")));
   
   data.push_back(std::make_tuple(Latin1String("././"),
                                  DirTest::HandleUnc,
                                  Latin1String("./")));
   
   data.push_back(std::make_tuple(Latin1String(".."),
                                  DirTest::HandleUnc,
                                  Latin1String("..")));
   
   data.push_back(std::make_tuple(Latin1String("../"),
                                  DirTest::HandleUnc,
                                  Latin1String("../")));
   
   data.push_back(std::make_tuple(Latin1String("../."),
                                  DirTest::HandleUnc,
                                  Latin1String("..")));
   
   data.push_back(std::make_tuple(Latin1String(".././"),
                                  DirTest::HandleUnc,
                                  Latin1String("../")));
   
   data.push_back(std::make_tuple(Latin1String("../.."),
                                  DirTest::HandleUnc,
                                  Latin1String("../..")));
   
   data.push_back(std::make_tuple(Latin1String("../../"),
                                  DirTest::HandleUnc,
                                  Latin1String("../../")));
   
   data.push_back(std::make_tuple(Latin1String(".//file1.txt"),
                                  DirTest::HandleUnc,
                                  Latin1String("file1.txt")));
   
   data.push_back(std::make_tuple(Latin1String("/foo/bar/..//file1.txt"),
                                  DirTest::HandleUnc,
                                  Latin1String("/foo/file1.txt")));
   
   data.push_back(std::make_tuple(Latin1String("foo/.."),
                                  DirTest::HandleUnc,
                                  Latin1String(".")));
   
   data.push_back(std::make_tuple(Latin1String("./foo/.."),
                                  DirTest::HandleUnc,
                                  Latin1String(".")));
   
   data.push_back(std::make_tuple(Latin1String(".foo/.."),
                                  DirTest::HandleUnc,
                                  Latin1String(".")));
   
   data.push_back(std::make_tuple(Latin1String("foo/bar/../.."),
                                  DirTest::HandleUnc,
                                  Latin1String(".")));
   
   data.push_back(std::make_tuple(Latin1String("./foo/bar/../.."),
                                  DirTest::HandleUnc,
                                  Latin1String(".")));
   
   data.push_back(std::make_tuple(Latin1String("../foo/bar"),
                                  DirTest::HandleUnc,
                                  Latin1String("../foo/bar")));
   
   data.push_back(std::make_tuple(Latin1String("./../foo/bar"),
                                  DirTest::HandleUnc,
                                  Latin1String("../foo/bar")));
   
   data.push_back(std::make_tuple(Latin1String("../../foo/../bar"),
                                  DirTest::HandleUnc,
                                  Latin1String("../../bar")));
   
   data.push_back(std::make_tuple(Latin1String("./foo/bar/.././.."),
                                  DirTest::HandleUnc,
                                  Latin1String(".")));
   
   data.push_back(std::make_tuple(Latin1String("/./foo"),
                                  DirTest::HandleUnc,
                                  Latin1String("/foo")));
   
   data.push_back(std::make_tuple(Latin1String("/../foo/"),
                                  DirTest::HandleUnc,
                                  Latin1String("/../foo/")));
   
   data.push_back(std::make_tuple(Latin1String("c:/"),
                                  DirTest::HandleUnc,
                                  Latin1String("c:/")));
   
   data.push_back(std::make_tuple(Latin1String("c://"),
                                  DirTest::HandleUnc,
                                  Latin1String("c:/")));
   
   data.push_back(std::make_tuple(Latin1String("c://foo"),
                                  DirTest::HandleUnc,
                                  Latin1String("c:/foo")));
   
   data.push_back(std::make_tuple(Latin1String("c:"),
                                  DirTest::HandleUnc,
                                  Latin1String("c:")));
   
   data.push_back(std::make_tuple(Latin1String("c:foo/bar"),
                                  DirTest::HandleUnc,
                                  Latin1String("c:foo/bar")));
   
#if defined PDK_OS_WIN
   QTest::newRow("data37") << "c:/." << HandleUnc << "c:/";
   QTest::newRow("data38") << "c:/.." << HandleUnc << "c:/..";
   QTest::newRow("data39") << "c:/../" << HandleUnc << "c:/../";
   data.push_back(std::make_tuple(Latin1String("c:/."),
                                  DirTest::HandleUnc,
                                  Latin1String("c:/")));
   data.push_back(std::make_tuple(Latin1String("c:/.."),
                                  DirTest::HandleUnc,
                                  Latin1String("c:/..")));
   data.push_back(std::make_tuple(Latin1String("c:/../"),
                                  DirTest::HandleUnc,
                                  Latin1String("c:/../")));
#else
   
   data.push_back(std::make_tuple(Latin1String("c:/."),
                                  DirTest::HandleUnc,
                                  Latin1String("c:")));
   data.push_back(std::make_tuple(Latin1String("c:/.."),
                                  DirTest::HandleUnc,
                                  Latin1String(".")));
   data.push_back(std::make_tuple(Latin1String("c:/../"),
                                  DirTest::HandleUnc,
                                  Latin1String("./")));
#endif
   
   data.push_back(std::make_tuple(Latin1String("c:/./"),
                                  DirTest::HandleUnc,
                                  Latin1String("c:/")));
   
   data.push_back(std::make_tuple(Latin1String("foo/../foo/.."),
                                  DirTest::HandleUnc,
                                  Latin1String(".")));
   
   data.push_back(std::make_tuple(Latin1String("foo/../foo/../.."),
                                  DirTest::HandleUnc,
                                  Latin1String("..")));
   
   data.push_back(std::make_tuple(Latin1String("..foo.bar/foo"),
                                  DirTest::HandleUnc,
                                  Latin1String("..foo.bar/foo")));
   
   data.push_back(std::make_tuple(Latin1String(".foo./bar/.."),
                                  DirTest::HandleUnc,
                                  Latin1String(".foo.")));
   
   data.push_back(std::make_tuple(Latin1String("foo/..bar.."),
                                  DirTest::HandleUnc,
                                  Latin1String("foo/..bar..")));
   
   data.push_back(std::make_tuple(Latin1String("foo/.bar./.."),
                                  DirTest::HandleUnc,
                                  Latin1String("foo")));
   
   data.push_back(std::make_tuple(Latin1String("//foo//bar"),
                                  DirTest::HandleUnc,
                                  Latin1String("//foo/bar")));
   
   data.push_back(std::make_tuple(Latin1String("..."),
                                  DirTest::HandleUnc,
                                  Latin1String("...")));
   
   data.push_back(std::make_tuple(Latin1String("foo/.../bar"),
                                  DirTest::HandleUnc,
                                  Latin1String("foo/.../bar")));
   
   data.push_back(std::make_tuple(Latin1String("ab/a/"),
                                  DirTest::HandleUnc,
                                  Latin1String("ab/a/")));
   
   // Drive letters and unc path in one string. The drive letter isn't handled as a drive letter
   // but as a host name in this case (even though Windows host names can't contain a ':')
   
   data.push_back(std::make_tuple(Latin1String("//c:/foo"),
                                  DirTest::HandleUnc,
                                  Latin1String("//c:/foo")));
   
   data.push_back(std::make_tuple(Latin1String("//c:/foo"),
                                  DirTest::IgnoreUnc,
                                  Latin1String("/c:/foo")));
   
   // @TODO resource test
   
}

} // anonymous namespace

TEST_F(DirTest, testSetNameFilters)
{
   std::list<std::tuple<String, StringList, StringList>> data;
   init_set_name_filters_data(data);
   for (auto &item : data) {
      String &dirName = std::get<0>(item);
      StringList &nameFilters = std::get<1>(item);
      StringList &expected = std::get<2>(item);
      Dir dir(dirName);
      ASSERT_TRUE(dir.exists()) << msg_does_not_exist(dirName).getConstRawData();
      
      dir.setNameFilters(nameFilters);
      StringList actual = dir.entryList();
      int max = std::min(actual.size(), expected.size());
      for (int i=0; i < max; ++i) {
         ASSERT_EQ(actual[i], expected[i]);
      }
      ASSERT_EQ(actual.size(), expected.size());
   }
}

TEST_F(DirTest, testCleanPath)
{
   std::list<std::tuple<String, String>> data;
   init_clean_path_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      String &expected = std::get<1>(item);
      String cleaned = Dir::cleanPath(path);
      ASSERT_EQ(cleaned, expected);
   }
}

// forward declare function with namespace
namespace pdk {
namespace io {
namespace fs {
String normalize_path_segments(const String &name, bool allowUncPaths,
                               bool *ok = nullptr);
} // fs
} // io
} // pdk

TEST_F(DirTest, testNormalizePathSegments)
{
   std::list<std::tuple<String, DirTest::UncHandling, String>> data;
   init_normalize_path_segments_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      UncHandling uncHandling = std::get<1>(item);
      String &expected = std::get<2>(item);
      String cleaned = pdk::io::fs::normalize_path_segments(path, uncHandling == HandleUnc);
      ASSERT_EQ(cleaned, expected);
      if (path == expected) {
         ASSERT_TRUE(path.isSharedWith(cleaned)) << "Strings are same but data is not shared";
      }      
   }
}

namespace {

void init_absolute_file_path_data(std::list<std::tuple<String, String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("/etc"), Latin1String("/passwd"), Latin1String("/passwd")));
   data.push_back(std::make_tuple(Latin1String("/etc"), Latin1String("passwd"), Latin1String("/etc/passwd")));
   data.push_back(std::make_tuple(Latin1String("/"), Latin1String("passwd"), Latin1String("/passwd")));
   data.push_back(std::make_tuple(Latin1String("relative"), Latin1String("path"), Dir::getCurrentPath() + Latin1String("/relative/path")));
   data.push_back(std::make_tuple(Latin1String(""), Latin1String(""), Dir::getCurrentPath()));
#if defined(PDK_OS_WIN)
   data.push_back(std::make_tuple(Latin1String("//machine"), Latin1String("share"), Latin1String("//machine/share")));
#endif
   // @TODO resource test
}

void init_absolute_path_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("/machine/share/dir1"), Latin1String("/machine/share/dir1")));
#if defined(PDK_OS_WIN)
   data.push_back(std::make_tuple(Latin1String("\\machine\\share\\dir1"), Latin1String("/machine/share/dir1")));
   data.push_back(std::make_tuple(Latin1String("//machine/share/dir1"), Latin1String("//machine/share/dir1")));
   data.push_back(std::make_tuple(Latin1String("\\\\machine\\share\\dir1"), Latin1String("//machine/share/dir1")));
   data.push_back(std::make_tuple(Latin1String("c:/machine/share/dir1"), Latin1String("c:/machine/share/dir1")));
   data.push_back(std::make_tuple(Latin1String("c:\\machine\\share\\dir1"), Latin1String("c:/machine/share/dir1")));
#endif
   
   data.push_back(std::make_tuple(Dir::getRootPath() + Latin1String("home/pdk/."), 
                                  Dir::getRootPath() + Latin1String("home/pdk")));
   
   data.push_back(std::make_tuple(Dir::getRootPath() + Latin1String("system/data/../config"),
                                  Dir::getRootPath() + Latin1String("system/config")));
   
   data.push_back(std::make_tuple(Dir::getRootPath() + Latin1String("/home//pdk"),
                                  Dir::getRootPath() + Latin1String("home/pdk")));
   
   data.push_back(std::make_tuple(Latin1String("foo/../bar"),
                                  Dir::getCurrentPath() + Latin1String("/bar")));
}

void init_relative_file_path_data(std::list<std::tuple<String, String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("/foo/bar"), Latin1String("ding.txt"), Latin1String("ding.txt")));
   data.push_back(std::make_tuple(Latin1String("/foo/bar"), Latin1String("ding/dong.txt"), Latin1String("ding/dong.txt")));
   data.push_back(std::make_tuple(Latin1String("/foo/bar"), Latin1String("../ding/dong.txt"), Latin1String("../ding/dong.txt")));
   
   data.push_back(std::make_tuple(Latin1String("/foo/bar"), Latin1String("/foo/bar/ding.txt"), Latin1String("ding.txt")));
   data.push_back(std::make_tuple(Latin1String("/foo/bar/"), Latin1String("/foo/bar/ding/dong.txt"), Latin1String("ding/dong.txt")));
   data.push_back(std::make_tuple(Latin1String("/foo/bar/"), Latin1String("/ding/dong.txt"), Latin1String("../../ding/dong.txt")));
   
   data.push_back(std::make_tuple(Latin1String("/"), Latin1String("/ding/dong.txt"), Latin1String("ding/dong.txt")));
   data.push_back(std::make_tuple(Latin1String("/"), Latin1String("/ding/"), Latin1String("ding")));
   data.push_back(std::make_tuple(Latin1String("/"), Latin1String("/ding//"), Latin1String("ding")));
   data.push_back(std::make_tuple(Latin1String("/"), Latin1String("/ding/../dong"), Latin1String("dong")));
   data.push_back(std::make_tuple(Latin1String("/"), Latin1String("/ding/../../../../dong"), Latin1String("../../../dong")));
   
   data.push_back(std::make_tuple(Latin1String(""), Latin1String(""), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("/tmp"), Latin1String("/tmp/"), Latin1String(".")));
   data.push_back(std::make_tuple(Latin1String("//tmp"), Latin1String("/tmp/"), Latin1String(".")));
   
   // @TODO Windows testcases
   // @TODO Resource testcases
}

} // anonymous namespace

TEST_F(DirTest, testAbsoluteFilePath)
{
   std::list<std::tuple<String, String, String>> data;
   init_absolute_file_path_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      String &fileName = std::get<1>(item);
      String &expectedFilePath = std::get<2>(item);
      Dir dir(path);
      String absFilePath = dir.getAbsoluteFilePath(fileName);
      ASSERT_EQ(absFilePath, expectedFilePath);
   }
}

TEST_F(DirTest, testAbsolutePath)
{
   std::list<std::tuple<String, String>> data;
   init_absolute_path_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      String &expectedPath = std::get<1>(item);
      Dir dir(path);
      ASSERT_EQ(dir.getAbsolutePath(), expectedPath);
   }
}

TEST_F(DirTest, testRelativeFilePath)
{
   std::list<std::tuple<String, String, String>> data;
   init_relative_file_path_data(data);
   for (auto &item : data) {
      String &dir = std::get<0>(item);
      String &path = std::get<1>(item);
      String &expected = std::get<2>(item);
      ASSERT_EQ(Dir(dir).getRelativeFilePath(path), expected);
   }
}

namespace {

void init_filepath_data(std::list<std::tuple<String, String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("/etc"), Latin1String("/passwd"), Latin1String("/passwd")));
   data.push_back(std::make_tuple(Latin1String("/etc"), Latin1String("passwd"), Latin1String("/etc/passwd")));
   data.push_back(std::make_tuple(Latin1String("/"), Latin1String("passwd"), Latin1String("/passwd")));
   data.push_back(std::make_tuple(Latin1String("relative"), Latin1String("path"), Latin1String("relative/path")));
   data.push_back(std::make_tuple(Latin1String(""), Latin1String(""), Latin1String(".")));
   // @TODO resource testcases
}

}

TEST_F(DirTest, testFilePath)
{
   std::list<std::tuple<String, String, String>> data;
   init_filepath_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      String &fileName = std::get<1>(item);
      String &expectedFilePath = std::get<2>(item);
      
      Dir dir(path);
      String absFilePath = dir.getFilePath(fileName);
      ASSERT_EQ(absFilePath, expectedFilePath);
   }
}

TEST_F(DirTest, testRemove)
{
   File f(Latin1String("remove-test"));
   f.open(IoDevice::OpenMode::WriteOnly);
   f.close();
   Dir dir;
   ASSERT_TRUE(dir.remove(Latin1String("remove-test")));
   // Test that the file just removed is gone
   ASSERT_TRUE(!dir.remove(Latin1String("remove-test")));
   ASSERT_TRUE(!dir.remove(Latin1String("")));
}

TEST_F(DirTest, testRename)
{
   File f(Latin1String("rename-test"));
   f.open(IoDevice::OpenMode::WriteOnly);
   f.close();
   Dir dir;
   ASSERT_TRUE(dir.rename(Latin1String("rename-test"), Latin1String("rename-test-renamed")));
   ASSERT_TRUE(dir.rename(Latin1String("rename-test-renamed"), Latin1String("rename-test")));
#if defined(PDK_OS_MAC)
   ASSERT_TRUE(!dir.rename(Latin1String("rename-test"), Latin1String("/etc/rename-test-renamed")));
#elif !defined(PDK_OS_WIN)
   // on windows this is possible - maybe make the test a bit better
#ifdef PDK_OS_UNIX
   // not valid if run as root so skip if needed
   if (::getuid() != 0)
      ASSERT_TRUE(!dir.rename(Latin1String("rename-test")), Latin1String("/rename-test-renamed")));
#else
   ASSERT_TRUE(!dir.rename(Latin1String("rename-test")), Latin1String("/rename-test-renamed")));
#endif
#endif
   ASSERT_TRUE(!dir.rename(Latin1String("rename-test"), Latin1String("")));
   ASSERT_TRUE(!dir.rename(Latin1String(""), Latin1String("rename-test-renamed")));
   ASSERT_TRUE(!dir.rename(Latin1String("some-file-that-does-not-exist"), Latin1String("rename-test-renamed")));
   
   ASSERT_TRUE(dir.remove(Latin1String("rename-test")));
}

namespace {

void init_exists2_data(std::list<std::tuple<String, bool>> &data)
{
   data.push_back(std::make_tuple(Latin1String("."), true));
   data.push_back(std::make_tuple(Latin1String("/"), true));
   data.push_back(std::make_tuple(Latin1String(""), false));
   data.push_back(std::make_tuple(Latin1String("testData"), true));
   data.push_back(std::make_tuple(Latin1String("/testData"), false));
   data.push_back(std::make_tuple(Latin1String("testdir/dir/test_dir.cpp"), true));
   data.push_back(std::make_tuple(Latin1String("/resources.cpp"), false));
   
   // @TODO resources testcases
}

void init_dirname_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("c:/winnt/system32"), Latin1String("system32")));
   data.push_back(std::make_tuple(Latin1String("/winnt/system32"), Latin1String("system32")));
   data.push_back(std::make_tuple(Latin1String("c:/winnt/system32/kernel32.dll"), Latin1String("kernel32.dll")));
   
#if defined(PDK_OS_WIN)
   data.push_back(std::make_tuple(Latin1String("c:\\winnt\\system32"), Latin1String("system32")));
   data.push_back(std::make_tuple(Latin1String("\\winnt\\system32"), Latin1String("system32")));
   data.push_back(std::make_tuple(Latin1String("c:\\winnt\\system32\\kernel32.dll"), Latin1String("system32")));
#endif
   // @TODO Windows platform
   // @TODO resources testcases
}

}

TEST_F(DirTest, testExists2)
{
   std::list<std::tuple<String, bool>> data;
   init_exists2_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      bool exists = std::get<1>(item);
      String oldpwd = Dir::getCurrentPath();
      Dir::setCurrent((sg_currentSourceDir + Latin1String("/.")));
      if (path.isEmpty()) {
         
      }
      Dir dir;
      if (exists) {
         ASSERT_TRUE(dir.exists(path)) << msg_does_not_exist(path).getConstRawData();
      } else {
         ASSERT_TRUE(!dir.exists(path));
      }
      Dir::setCurrent(oldpwd);
   }
}

TEST_F(DirTest, testDirName)
{
   std::list<std::tuple<String, String>> data;
   init_dirname_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      String &dirName = std::get<1>(item);
      Dir dir(path);
      ASSERT_EQ(dir.getDirName(), dirName);
   }
}

TEST_F(DirTest, testOperatoreq)
{
   Dir dir1(Latin1String("."));
   dir1 = dir1;
   dir1.setPath(Latin1String(".."));
}

TEST_F(DirTest, testDotAndDotDot)
{
   Dir dir(String((sg_currentSourceDir + Latin1String("/testdir/"))));
   StringList entryList = dir.entryList(Dir::Filter::Dirs);
   ASSERT_EQ(entryList, StringList() << String(Latin1String(".")) << String(Latin1String("..")) 
             << String(Latin1String("dir")) << String(Latin1String("spaces")));
   entryList = dir.entryList(Dir::Filters(Dir::Filter::Dirs) | Dir::Filter::NoDotAndDotDot);
   ASSERT_EQ(entryList, StringList() << String(Latin1String("dir")) << String(Latin1String("spaces")));
}

TEST_F(DirTest, testHomePath)
{
   Dir homeDir = Dir::getHome();
   String strHome = Dir::getHomePath();
   
   // docs say that homePath() is an absolute path
   ASSERT_EQ(strHome, homeDir.getAbsolutePath());
   ASSERT_TRUE(Dir::isAbsolutePath(strHome));
   
#ifdef PDK_OS_UNIX
   if (strHome.length() > 1)      // root dir = "/"
      ASSERT_TRUE(!strHome.endsWith('/'));
   
   ByteArray envHome = pdk::pdk_getenv("HOME");
   unsetenv("HOME");
   ASSERT_EQ(Dir::getHomePath(), Dir::getRootPath());
   pdk::pdk_putenv("HOME", envHome);
   
#elif defined(PDK_OS_WIN)
   if (strHome.length() > 3      // root dir = "c:/"; "//" is not really valid...
       )
      ASSERT_TRUE(!strHome.endsWith('/'));
#endif
   
   StringList entries = homeDir.entryList();
   for (size_t i = 0; i < entries.size(); ++i) {
      FileInfo fi(Dir::getHomePath() + Latin1String("/") + entries[i]);
      ASSERT_EQ(fi.exists(), true);
   }
}

TEST_F(DirTest, testTempPath)
{
   Dir dir = Dir::getTemp();
   String path = Dir::getTempPath();
   // docs say that tempPath() is an absolute path
   ASSERT_EQ(path, dir.getAbsolutePath());
   ASSERT_TRUE(Dir::isAbsolutePath(path));
   
#ifdef PDK_OS_UNIX
   if (path.length() > 1) {
      ASSERT_TRUE(!path.endsWith('/'));
   }
   
#elif defined(PDK_OS_WIN)
   if (path.length() > 3)      // root dir = "c:/"; "//" is not really valid...
   {
      ASSERT_TRUE(!path.endsWith('/'));
   }
   ASSERT_TRUE(!path.contains(Latin1Character('~'))) << pdk_printable(String::fromLatin1("Temp path (%1) must not be a short name.").arg(path));
#endif
}

TEST_F(DirTest, getRootPath)
{
   Dir dir = Dir::getRoot();
   String path = Dir::getRootPath();
   
   // docs say that tempPath() is an absolute path
   ASSERT_EQ(path, dir.getAbsolutePath());
   ASSERT_TRUE(Dir::isAbsolutePath(path));
   
#if defined(PDK_OS_UNIX)
   ASSERT_EQ(path, String(Latin1String("/")));
#endif
}

TEST_F(DirTest, testNativeSeparators)
{
#if defined(PDK_OS_WIN)
   ASSERT_EQ(Dir::toNativeSeparators(Latin1String("/")), String("\\"));
   ASSERT_EQ(Dir::toNativeSeparators(Latin1String("\\")), String("\\"));
   ASSERT_EQ(Dir::fromNativeSeparators(Latin1String("/")), String("/"));
   ASSERT_EQ(Dir::fromNativeSeparators(Latin1String("\\")), String("/"));
#else
   ASSERT_EQ(Dir::toNativeSeparators(Latin1String("/")), String(Latin1String("/")));
   ASSERT_EQ(Dir::toNativeSeparators(Latin1String("\\")), String(Latin1String("\\")));
   ASSERT_EQ(Dir::fromNativeSeparators(Latin1String("/")), String(Latin1String("/")));
   ASSERT_EQ(Dir::fromNativeSeparators(Latin1String("\\")), String(Latin1String("\\")));
#endif
}

