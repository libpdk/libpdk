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
// Created by zzu_softboy on 2018/04/16.

#include "gtest/gtest.h"
#include "pdk/global/PlatformDefs.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/StandardPaths.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/TemporaryDir.h"
#include "pdk/base/io/fs/TemporaryFile.h"
#include "pdk/base/io/fs/StorageInfo.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/time/Date.h"
#include "pdktest/PdkTest.h"

#ifdef PDK_OS_UNIX
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef PDK_OS_VXWORKS
#include <pwd.h>
#endif
#endif
#include <memory>

#ifdef PDK_OS_WIN
#include "pdk/global/Windows.h"
#endif
#include "../shared/Filesystem.h"
#include "pdk/base/io/fs/internal/FileInfoPrivate.h"

#if defined(PDK_OS_VXWORKS)
#define PDK_NO_SYMLINKS
#endif

// @TODO add Windows platform testcases

using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::ds::ByteArray;
using pdk::io::IoDevice;
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::io::fs::FileDevice;
using pdk::io::fs::internal::FileInfoPrivate;
using pdk::io::fs::StandardPaths;
using pdk::ds::StringList;
using pdk::io::fs::TemporaryFile;
using pdk::io::fs::TemporaryDir;
using pdk::io::Debug;
using pdk::kernel::CoreApplication;
using pdk::io::fs::StorageInfo;
using pdk::time::DateTime;
using pdk::time::Time;
using pdk::time::Date;

#define PDKTEST_DIR_SEP "/"
#define PDKTEST_FILETEST_SUBDIR "fileinfotestdir"
#define PDKTEST_FINDTEST_SRC_DATA(filename) Latin1String(PDKTEST_CURRENT_TEST_SOURCE_DIR PDKTEST_DIR_SEP PDKTEST_FILETEST_SUBDIR PDKTEST_DIR_SEP filename)

PDKTEST_DECLARE_APP_STARTUP_ARGS();

namespace {

inline bool is_likely_to_be_fat(const String &path)
{
   ByteArray name = StorageInfo(path).getFileSystemType().toLower();
   return name.contains("fat") || name.contains("msdos");
}

inline bool is_likely_to_be_nfs(const String &path)
{
#ifdef PDK_OS_WIN
   PDK_UNUSED(path);
   return false;
#else
   ByteArray type = StorageInfo(path).getFileSystemType();
   const char *name = type.getConstRawData();
   
   return (pdk::strncmp(name, "nfs", 3) == 0
           || pdk::strncmp(name, "autofs", 6) == 0
           || pdk::strncmp(name, "autofsng", 8) == 0
           || pdk::strncmp(name, "cachefs", 7) == 0);
#endif
}

String seed_and_template()
{
   String base;
#if defined(PDK_OS_UNIX)
   // use XDG_RUNTIME_DIR as it's a fully-capable FS
   base = StandardPaths::writableLocation(StandardPaths::StandardLocation::RuntimeLocation);
#endif
   if (base.isEmpty()) {
      base = Dir::getTempPath();
   }
   return base + Latin1String("/FileInfoTest-XXXXXX");
}

ByteArray msg_does_not_exist(const String &name)
{
   return (Latin1Character('"') + Dir::toNativeSeparators(name)
           + Latin1String("\" does not exist.")).toLocal8Bit();
}

ByteArray msg_is_no_directory(const String &name)
{
   return (Latin1Character('"') + Dir::toNativeSeparators(name)
           + Latin1String("\" is not a directory.")).toLocal8Bit();
}

ByteArray msg_is_not_root(const String &name)
{
   return (Latin1Character('"') + Dir::toNativeSeparators(name)
           + Latin1String("\" is no root directory.")).toLocal8Bit();
}

static String sg_sourceFile = String(Latin1String(PDKTEST_CURRENT_TEST_SOURCE_DIR)) + Latin1String("/FileInfoTest.cpp");

class FileInfoTest : public ::testing::Test
{
public:
   FileInfoTest();
   
   // Sets up the test fixture.
   void SetUp()
   {
      m_dataDir.reset(new Dir(Latin1String(PDKTEST_FILETEST_SUBDIR)));
      ASSERT_TRUE(m_dataDir);
      const String dataPath = m_dataDir->getPath();
      ASSERT_TRUE(!dataPath.isEmpty());
      
      m_sourceFile = sg_sourceFile;
      sm_resourcesDir = PDKTEST_FINDTEST_SRC_DATA("resources");
      m_pdkFile = PDKTEST_FINDTEST_SRC_DATA("test.pdk");
      
      ASSERT_TRUE(m_dir.isValid()) << (Latin1String("Failed to create temporary dir: ") + m_dir.getErrorString());
      ASSERT_TRUE(Dir::setCurrent(m_dir.getPath()));
   }
   
   // Tears down the test fixture.
   void TearDown()
   {
      Dir::setCurrent(m_currentDir); // Release temporary directory so that it can be deleted on Windows
   }
   
public:
   const String m_currentDir;
   String m_sourceFile;
   String m_pdkFile;
   static String sm_resourcesDir;
   TemporaryDir m_dir;
   std::shared_ptr<Dir> m_dataDir;
};

FileInfoTest::FileInfoTest()
   : m_currentDir(Dir::getCurrentPath()),
     m_dir(seed_and_template())
{}

String FileInfoTest::sm_resourcesDir;

} // anonymous namespace

TEST_F(FileInfoTest, testGetSetCheck)
{
   FileInfo obj1;
   // bool FileInfo::getCaching()
   // void FileInfo::setCaching(bool)
   obj1.setCaching(false);
   ASSERT_EQ(false, obj1.getCaching());
   obj1.setCaching(true);
   ASSERT_EQ(true, obj1.getCaching());
}

namespace {

FileInfoPrivate* get_private(FileInfo &info)
{
   return (*reinterpret_cast<FileInfoPrivate **>(&info));
}

} // anonymous namespace

TEST_F(FileInfoTest, testCopy)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   TemporaryFile tempFile;
   ASSERT_TRUE(tempFile.open()) << pdk_printable(tempFile.getErrorString());
   FileInfo info(tempFile.getFileName());
   ASSERT_TRUE(info.exists());
   
   //copy constructor
   FileInfo info2(info);
   FileInfoPrivate *privateInfo = get_private(info);
   FileInfoPrivate *privateInfo2 = get_private(info2);
   ASSERT_EQ(privateInfo, privateInfo2);
   
   //operator =
   FileInfo info3 = info;
   FileInfoPrivate *privateInfo3 = get_private(info3);
   ASSERT_EQ(privateInfo, privateInfo3);
   ASSERT_EQ(privateInfo2, privateInfo3);
   
   //refreshing info3 will detach it
   File file(info.getAbsoluteFilePath());
   ASSERT_TRUE(file.open(File::OpenMode::WriteOnly));
   ASSERT_EQ(file.write("JAJAJAA"), pdk::pint64(7));
   file.flush();
   
   pdktest::wait(250);
#if defined(PDK_OS_WIN)
   file.close();
#endif
   info3.refresh();
   privateInfo3 = get_private(info3);
   ASSERT_TRUE(privateInfo != privateInfo3);
   ASSERT_TRUE(privateInfo2 != privateInfo3);
   ASSERT_EQ(privateInfo, privateInfo2);
   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_is_file_data(std::list<std::tuple<String, bool>> &data)
{
   data.push_back(std::make_tuple(Dir::getCurrentPath(), false));
   data.push_back(std::make_tuple(sg_sourceFile, true));
   // @TODO add resources testcases
}

void init_is_dir_data(std::list<std::tuple<String, bool>> &data)
{
   File::remove(Latin1String("brokenlink.lnk"));
   File::remove(Latin1String("dummyfile"));
   File file3(Latin1String("dummyfile"));
   file3.open(File::OpenMode::WriteOnly);
   if (file3.link(Latin1String("brokenlink.lnk"))) {
      file3.remove();
      FileInfo info3(Latin1String("brokenlink.lnk"));
      ASSERT_TRUE(info3.isSymLink());
   }
   data.push_back(std::make_tuple(Dir::getCurrentPath(), true));
   data.push_back(std::make_tuple(sg_sourceFile, false));
   // @TODO add resources testcases
   data.push_back(std::make_tuple(FileInfoTest::sm_resourcesDir, true));
   data.push_back(std::make_tuple((FileInfoTest::sm_resourcesDir + Latin1Character('/')), true));
   data.push_back(std::make_tuple(Latin1String("brokenlink.lnk"), false));
   // @TODO add windows testcases
}

void init_is_root_data(std::list<std::tuple<String, bool>> &data)
{
   data.push_back(std::make_tuple(Dir::getCurrentPath(), false));
   data.push_back(std::make_tuple(Latin1String("/"), true));
   data.push_back(std::make_tuple(Latin1String("*"), false));
   data.push_back(std::make_tuple(Latin1String("/*"), false));
   // @TODO add resources testcases
   // @TODO add windows testcases
}

void init_is_exists_data(std::list<std::tuple<String, bool>> &data)
{
   data.push_back(std::make_tuple(Dir::getCurrentPath(), true));
   data.push_back(std::make_tuple(sg_sourceFile, true));
   data.push_back(std::make_tuple(Latin1String("/I/do_not_expect_this_path_to_exist/"), false));
   // @TODO add resources testcases
   data.push_back(std::make_tuple(FileInfoTest::sm_resourcesDir + Latin1String("/*"), false));
   data.push_back(std::make_tuple(FileInfoTest::sm_resourcesDir + Latin1String("/*.foo"), false));
   data.push_back(std::make_tuple(FileInfoTest::sm_resourcesDir + Latin1String("/*.ext1"), false));
   data.push_back(std::make_tuple(FileInfoTest::sm_resourcesDir + Latin1String("/file?.ext1"), false));
   data.push_back(std::make_tuple(Latin1String("."), true));
   data.push_back(std::make_tuple(Latin1String(""), false));
   data.push_back(std::make_tuple(FileInfoTest::sm_resourcesDir, true));
   data.push_back(std::make_tuple((FileInfoTest::sm_resourcesDir + Latin1Character('/')), true));
   // @TODO add windows testcases
}

void init_absolute_path_data(std::list<std::tuple<String, String, String>> &data)
{
   String drivePrefix;
   // @TODO add resources testcases
   // @TODO add windows testcases
   data.push_back(std::make_tuple(Latin1String("/machine/share/dir1/"), 
                                  drivePrefix + Latin1String("/machine/share/dir1"),
                                  Latin1String("")));
   
   data.push_back(std::make_tuple(Latin1String("/machine/share/dir1"), 
                                  drivePrefix + Latin1String("/machine/share"),
                                  Latin1String("dir1")));
   
   data.push_back(std::make_tuple(Latin1String("/usr/local/bin"), 
                                  drivePrefix + Latin1String("/usr/local"),
                                  Latin1String("bin")));
   
   data.push_back(std::make_tuple(Latin1String("/usr/local/bin/"), 
                                  drivePrefix + Latin1String("/usr/local/bin"),
                                  Latin1String("")));
   
   data.push_back(std::make_tuple(Latin1String("/test"), 
                                  drivePrefix + Latin1String("/"),
                                  Latin1String("test")));
   
   data.push_back(std::make_tuple(Latin1String("/test"), 
                                  drivePrefix + Latin1String("/"),
                                  Latin1String("test")));
   
   data.push_back(std::make_tuple(drivePrefix + Latin1String("/System/Library/StartupItems/../Frameworks"), 
                                  drivePrefix + Latin1String("/System/Library"),
                                  Latin1String("Frameworks")));
   
   data.push_back(std::make_tuple(drivePrefix + Latin1String("/System/Library/StartupItems/../Frameworks/"), 
                                  drivePrefix + Latin1String("/System/Library/Frameworks"),
                                  Latin1String("")));
}

} // anonymous namespace

TEST_F(FileInfoTest, testIsFile)
{
   std::list<std::tuple<String, bool>> data;
   init_is_file_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      bool expected = std::get<1>(item);
      FileInfo fileInfo(path);
      ASSERT_EQ(fileInfo.isFile(), expected);
   }
}

TEST_F(FileInfoTest, testIsDir)
{
   std::list<std::tuple<String, bool>> data;
   init_is_dir_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      bool expected = std::get<1>(item);
      const bool isDir = FileInfo(path).isDir();
      if (expected) {
         ASSERT_TRUE(isDir) << msg_is_no_directory((path)).getConstRawData();
      } else {
         ASSERT_FALSE(isDir);
      }
   }
}

TEST_F(FileInfoTest, testIsRoot)
{
   std::list<std::tuple<String, bool>> data;
   init_is_dir_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      bool expected = std::get<1>(item);
      const bool isRoot = FileInfo(path).isDir();
      if (expected) {
         ASSERT_TRUE(isRoot) << msg_is_not_root((path)).getConstRawData();
      } else {
         ASSERT_FALSE(isRoot);
      }
   }
}

TEST_F(FileInfoTest, testIsExists)
{
   std::list<std::tuple<String, bool>> data;
   init_is_exists_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      bool expected = std::get<1>(item);
      const bool exists = FileInfo(path).exists();
      if (expected) {
         ASSERT_TRUE(exists) << msg_does_not_exist((path)).getConstRawData();
      } else {
         ASSERT_FALSE(exists);
      }
   }
}

TEST_F(FileInfoTest, testAbsolutePath)
{
   std::list<std::tuple<String, String, String>> data;
   init_absolute_path_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      String &path = std::get<1>(item);
      String &filename = std::get<2>(item);
      FileInfo fileInfo(file);
      ASSERT_EQ(fileInfo.getAbsolutePath(), path);
      ASSERT_EQ(fileInfo.getFileName(), filename);
   }
}

namespace {

void init_abs_file_path_data(std::list<std::tuple<String, String>> &data)
{
   String drivePrefix;
   data.push_back(std::make_tuple(Latin1String("tmp.txt"), Dir::getCurrentPath() + Latin1String("/tmp.txt")));
   data.push_back(std::make_tuple(Latin1String("temp/tmp.txt"), Dir::getCurrentPath() + Latin1String("/temp/tmp.txt")));
   // @TODO add resources testcases
   // @TODO add windows testcases
   data.push_back(std::make_tuple(Latin1String("/home/andy/tmp.txt"), Latin1String("/home/andy/tmp.txt")));
   
   data.push_back(std::make_tuple(drivePrefix + Latin1String("/System/Library/StartupItems/../Frameworks"),
                                  drivePrefix + Latin1String("/System/Library/Frameworks")));
}

} // anonymous namespace

TEST_F(FileInfoTest, testAbsFilePath)
{
   std::list<std::tuple<String, String>> data;
   init_abs_file_path_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      String &expected = std::get<1>(item);
      FileInfo fileInfo(file);
      ASSERT_EQ(fileInfo.getAbsoluteFilePath(), expected);
   }
}

TEST_F(FileInfoTest, testCanonicalPath)
{
   TemporaryFile tempFile;
   tempFile.setAutoRemove(true);
   ASSERT_TRUE(tempFile.open()) << pdk_printable(tempFile.getErrorString());
   FileInfo fileInfo(tempFile.getFileName());
   ASSERT_EQ(fileInfo.getCanonicalPath(), FileInfo(Dir::getTempPath()).getCanonicalFilePath());
}

namespace {

class FileDeleter
{
   PDK_DISABLE_COPY(FileDeleter);
public:
   explicit FileDeleter(const String fileName)
      : m_fileName(fileName)
   {}
   
   ~FileDeleter() { File::remove(m_fileName); }
   
private:
   const String m_fileName;
};

} // anonymous namespace

TEST_F(FileInfoTest, testCanonicalFilePath)
{
   const String fileName(Latin1String("tmp.canon"));
   File tempFile(fileName);
   ASSERT_TRUE(tempFile.open(File::OpenMode::WriteOnly));
   FileInfo fileInfo(tempFile.getFileName());
   ASSERT_EQ(fileInfo.getCanonicalFilePath(), Dir::getCurrentPath() + Latin1String("/") + fileName);
   tempFile.remove();
   
   // This used to crash on Mac, verify that it doesn't anymore.
   FileInfo info(Latin1String("/tmp/../../../../../../../../../../../../../../../../../"));
   info.getCanonicalFilePath();
#if defined(PDK_OS_UNIX)
   // This used to crash on Mac
   FileInfo dontCrash(Latin1String("/"));
   ASSERT_EQ(dontCrash.getCanonicalFilePath(), Latin1String("/"));
#endif
   
#ifndef PDK_OS_WIN
   // test symlinks
   File::remove(Latin1String("link.lnk"));
   {
      File file(m_sourceFile);
      if (file.link(Latin1String("link.lnk"))) {
         FileInfo info1(file);
         FileInfo info2(Latin1String("link.lnk"));
         ASSERT_EQ(info1.getCanonicalFilePath(), info2.getCanonicalFilePath());
      }
   }
   
   const String dirSymLinkName = Latin1String(Latin1String("tst_Fileinfo"))
         + DateTime::getCurrentDateTime().toString(Latin1String(Latin1String("yyMMddhhmmss")));
   const String link(Dir::getTempPath() + Latin1Character('/') + dirSymLinkName);
   FileDeleter dirSymLinkDeleter(link);
   
   {
      File file(Dir::getCurrentPath());
      if (file.link(link)) {
         File tempfile(Latin1String("tempfile.txt"));
         tempfile.open(File::OpenMode::ReadWrite);
         tempfile.write("This file is generated by the FileInfo autotest.");
         ASSERT_TRUE(tempfile.flush());
         tempfile.close();
         
         FileInfo info1(Latin1String("tempfile.txt"));
         FileInfo info2(link + Dir::getSeparator() + Latin1String("tempfile.txt"));
         
         ASSERT_TRUE(info1.exists());
         ASSERT_TRUE(info2.exists());
         ASSERT_EQ(info1.getCanonicalFilePath(), info2.getCanonicalFilePath());
         
         FileInfo info3(link + Dir::getSeparator() + Latin1String("link.lnk"));
         FileInfo info4(m_sourceFile);
         ASSERT_TRUE(!info3.getCanonicalFilePath().isEmpty());
         ASSERT_EQ(info4.getCanonicalFilePath(), info3.getCanonicalFilePath());
         
         tempfile.remove();
      }
   }
   {
      String link(Dir::getTempPath() + Latin1Character('/') + dirSymLinkName
                  + Latin1String("/link_to_tst_Fileinfo"));
      File::remove(link);
      
      File file(Dir::getTempPath() + Latin1Character('/') +  dirSymLinkName
                + Latin1String("tst_Fileinfo.cpp"));
      if (file.link(link))
      {
         FileInfo info1(Latin1String("tst_Fileinfo.cpp"));
         FileInfo info2(link);
         ASSERT_EQ(info1.getCanonicalFilePath(), info2.getCanonicalFilePath());
      }
   }
#endif
   // @TODO add windows testcases
#ifdef PDK_OS_DARWIN
   {
      // Check if canonicalFilePath's result is in Composed normalization form.
      String path = String::fromLatin1("caf\xe9");
      Dir dir(Dir::getTempPath());
      dir.mkdir(path);
      String canonical = FileInfo(dir.getFilePath(path)).getCanonicalFilePath();
      String roundtrip = File::decodeName(File::encodeName(canonical));
      ASSERT_EQ(canonical, roundtrip);
      dir.rmdir(path);
   }
#endif
}

namespace {

void init_filename_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("tmp.txt"), Latin1String("tmp.txt")));
   data.push_back(std::make_tuple(Latin1String("temp/tmp.txt"), Latin1String("tmp.txt")));
   // @TODO add windows testcases
   data.push_back(std::make_tuple(Latin1String("/home/andy/tmp.txt"), Latin1String("tmp.txt")));
   // @TODO add resources testcases
   data.push_back(std::make_tuple(String::fromLatin1("/a/"), Latin1String("")));
   data.push_back(std::make_tuple(String::fromLatin1("/a"), Latin1String("a")));
   
   data.push_back(std::make_tuple(String::fromLatin1("/somedir/"), Latin1String("")));
   data.push_back(std::make_tuple(String::fromLatin1("/somedir"), Latin1String("somedir")));
}

void init_boundname_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("/"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("/etc"), Latin1String("")));
#ifdef PDK_OS_MAC
   data.push_back(std::make_tuple(Latin1String("/Applications/Safari.app" ), Latin1String("Safari")));
#endif
}

void init_dir_data(std::list<std::tuple<String, bool, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("tmp.txt"), false, Latin1String(".")));
   data.push_back(std::make_tuple(Latin1String("tmp.txt"), true, Dir::getCurrentPath()));
   data.push_back(std::make_tuple(Latin1String("temp/tmp.txt"), false, Latin1String("temp")));
   data.push_back(std::make_tuple(Latin1String("temp/tmp.txt"), true, Dir::getCurrentPath() + Latin1String("/temp")));
   data.push_back(std::make_tuple(Dir::getCurrentPath() + Latin1String("/tmp.txt"), true, Dir::getCurrentPath()));
   data.push_back(std::make_tuple(Dir::getCurrentPath() + Latin1String("/tmp.txt"), false, Dir::getCurrentPath()));
   // @TODO add windows testcases
   // @TODO add resources testcases
}

void init_suffix_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("file"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("/path/to/file"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("file.tar"), Latin1String("tar")));
   data.push_back(std::make_tuple(Latin1String("file.tar.gz"), Latin1String("gz")));
   data.push_back(std::make_tuple(Latin1String("/path/file/file.tar.gz"), Latin1String("gz")));
   data.push_back(std::make_tuple(Latin1String("/path/file.tar"), Latin1String("tar")));
   
   data.push_back(std::make_tuple(Latin1String(".ext1"), Latin1String("ext1")));
   data.push_back(std::make_tuple(Latin1String(".ext"), Latin1String("ext")));
   
   data.push_back(std::make_tuple(Latin1String(".ex"), Latin1String("ex")));
   data.push_back(std::make_tuple(Latin1String(".e"), Latin1String("e")));
   
   data.push_back(std::make_tuple(Latin1String(".ext1.ext2"), Latin1String("ext2")));
   data.push_back(std::make_tuple(Latin1String(".ext.ext2"), Latin1String("ext2")));
   
   data.push_back(std::make_tuple(Latin1String(".ex.ext2"), Latin1String("ext2")));
   data.push_back(std::make_tuple(Latin1String(".e.ext2"), Latin1String("ext2")));
   
   data.push_back(std::make_tuple(Latin1String("..ext2"), Latin1String("ext2")));
   // @TODO add resources testcases
   // @TODO add windows testcases
}

void init_complete_suffix_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("file"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("/path/to/file"), Latin1String("")));
   
   data.push_back(std::make_tuple(Latin1String("file.tar"), Latin1String("tar")));
   data.push_back(std::make_tuple(Latin1String("file.tar.gz"), Latin1String("tar.gz")));
   data.push_back(std::make_tuple(Latin1String("/path/file/file.tar.gz"), Latin1String("tar.gz")));
   data.push_back(std::make_tuple(Latin1String("/path/file.tar"), Latin1String("tar")));
   // @TODO add resources testcases
   // @TODO add windows testcases
}

} // anonymous namespace

TEST_F(FileInfoTest, testFileName)
{
   std::list<std::tuple<String, String>> data;
   init_filename_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      String &expected = std::get<1>(item);
      FileInfo fileInfo(file);
      ASSERT_EQ(fileInfo.getFileName(), expected);
   }
}

TEST_F(FileInfoTest, testBundleName)
{
   std::list<std::tuple<String, String>> data;
   init_boundname_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      String &expected = std::get<1>(item);
      FileInfo fileInfo(file);
      ASSERT_EQ(fileInfo.getBundleName(), expected);
   }
}

TEST_F(FileInfoTest, testDir)
{
   std::list<std::tuple<String, bool, String>> data;
   init_dir_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      bool absPath = std::get<1>(item);
      String &expected = std::get<2>(item);
      FileInfo fileInfo(file);
      if (absPath) {
         ASSERT_EQ(fileInfo.getAbsolutePath(), expected);
         ASSERT_EQ(fileInfo.getAbsoluteDir().getPath(), expected);
      } else {
         ASSERT_EQ(fileInfo.getPath(), expected);
         ASSERT_EQ(fileInfo.getDir().getPath(), expected);
      }
   }
}

TEST_F(FileInfoTest, testSuffix)
{
   std::list<std::tuple<String, String>> data;
   init_suffix_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      String &expected = std::get<1>(item);
      FileInfo fileInfo(file);
      ASSERT_EQ(fileInfo.getSuffix(), expected);
   }
}

TEST_F(FileInfoTest, testCompleteSuffix)
{
   std::list<std::tuple<String, String>> data;
   init_complete_suffix_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      String &expected = std::get<1>(item);
      FileInfo fileInfo(file);
      ASSERT_EQ(fileInfo.getCompleteSuffix(), expected);
   }
}

namespace {

void init_basename_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("file.tar"), Latin1String("file")));
   data.push_back(std::make_tuple(Latin1String("file.tar.gz"), Latin1String("file")));
   data.push_back(std::make_tuple(Latin1String("/path/file/file.tar.gz"), Latin1String("file")));
   data.push_back(std::make_tuple(Latin1String("/path/file.tar"), Latin1String("file")));
   data.push_back(std::make_tuple(Latin1String("/path/file"), Latin1String("file")));
   
   // @TODO add resources testcases
   // @TODO add windows testcases
}

void init_complete_basename_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String("file.tar"), Latin1String("file")));
   data.push_back(std::make_tuple(Latin1String("file.tar.gz"), Latin1String("file.tar")));
   data.push_back(std::make_tuple(Latin1String("/path/file/file.tar.gz"), Latin1String("file.tar")));
   data.push_back(std::make_tuple(Latin1String("/path/file.tar"), Latin1String("file")));
   data.push_back(std::make_tuple(Latin1String("/path/file"), Latin1String("file")));
   
   // @TODO add resources testcases
   // @TODO add windows testcases
}

void init_permissions_data(std::list<std::tuple<String, File::Permission, bool>> &data)
{
   data.push_back(std::make_tuple(CoreApplication::getInstance()->getAppFilePath(), File::Permission::ExeUser, true));
   data.push_back(std::make_tuple(sg_sourceFile, File::Permission::ReadUser, true));
   
   // @TODO add resources testcases
}

void init_size_data(std::list<std::tuple<String, int>> &data)
{
   File::remove(Latin1String("file1"));
   File file(Latin1String("file1"));
   ASSERT_TRUE(file.open(File::OpenMode::WriteOnly));
   ASSERT_EQ(file.write("JAJAJAA"), pdk::pint64(7));
   data.push_back(std::make_tuple(Latin1String("file1"), 7));
   // @TODO add resources testcases
}

void init_compare_data(std::list<std::tuple<String, String, bool>> &data)
{
   String caseChangedSource = sg_sourceFile;
   caseChangedSource.replace(Latin1String("info"), Latin1String("Info"));
   data.push_back(std::make_tuple(sg_sourceFile, sg_sourceFile, true));
   data.push_back(std::make_tuple(sg_sourceFile, String::fromLatin1("/FileInfoTest.cpp"), false));
   data.push_back(std::make_tuple(String::fromLatin1("FileInfoTest.cpp"), Dir::getCurrentPath() + String::fromLatin1("/FileInfoTest.cpp"), true));
   data.push_back(std::make_tuple(caseChangedSource, sg_sourceFile,
                               #if defined(PDK_OS_WIN)
                                  true
                               #elif defined(PDK_OS_MAC)
                                  !pathconf(Dir::getCurrentPath().toLatin1().getConstRawData(), _PC_CASE_SENSITIVE)
                               #else
                                  false
                               #endif  
                                  ));
   
}

void init_consistent_data(std::list<std::tuple<String, String>> &data)
{
   // @TODO add windows testcases
   data.push_back(std::make_tuple(String::fromLatin1("/a/somedir/"), String::fromLatin1("/a/somedir/")));
   data.push_back(std::make_tuple(String::fromLatin1("/a/somedir"), String::fromLatin1("/a/somedir")));
}

void init_filetimes_data(std::list<String> &data)
{
   data.push_back(String::fromLatin1("simplefile.txt"));
   data.push_back(String::fromLatin1("longFileNamelongFileNamelongFileNamelongFileName"
                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                     "longFileNamelongFileNamelongFileNamelongFileName.txt"));
   data.push_back(FileInfo(String::fromLatin1("longFileNamelongFileNamelongFileNamelongFileName"
                                              "longFileNamelongFileNamelongFileNamelongFileName"
                                              "longFileNamelongFileNamelongFileNamelongFileName"
                                              "longFileNamelongFileNamelongFileNamelongFileName"
                                              "longFileNamelongFileNamelongFileNamelongFileName.txt")).getAbsoluteFilePath());
}

} // anonymous namespace

TEST_F(FileInfoTest, testBaseName)
{
   std::list<std::tuple<String, String>> data;
   init_basename_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      String &expected = std::get<1>(item);
      FileInfo fileInfo(file);
      ASSERT_EQ(fileInfo.getBaseName(), expected);
   }
}

TEST_F(FileInfoTest, testCompleteBaseName)
{
   std::list<std::tuple<String, String>> data;
   init_complete_basename_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      String &expected = std::get<1>(item);
      FileInfo fileInfo(file);
      ASSERT_EQ(fileInfo.getCompleteBaseName(), expected);
   }
}

TEST_F(FileInfoTest, testPermissions)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   std::list<std::tuple<String, File::Permission, bool>> data;
   init_permissions_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      File::Permission perms = std::get<1>(item);
      bool expected = std::get<2>(item);
      FileInfo fileInfo(file);
      ASSERT_EQ(fileInfo.getPermission(File::Permissions(perms)), expected);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(FileInfoTest, testSize)
{
   std::list<std::tuple<String, int>> data;
   init_size_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      int size = std::get<1>(item);
      FileInfo fileInfo(file);
      (void) fileInfo.getPermissions();
      ASSERT_EQ(fileInfo.getSize(), size);
   }
}

TEST_F(FileInfoTest, testSystemFiles)
{
#if !defined(PDK_OS_WIN)
   std::cout << "This is a Windows only test" << std::endl;
#endif
}

TEST_F(FileInfoTest, testCompare)
{
   std::list<std::tuple<String, String, bool>> data;
   init_compare_data(data);
   for (auto &item : data) {
      String &file1 = std::get<0>(item);
      String &file2 = std::get<1>(item);
      bool same = std::get<2>(item);
      FileInfo fileInfo1(file1);
      FileInfo fileInfo2(file2);
      ASSERT_EQ(fileInfo1 == fileInfo2, same);
   }
}

TEST_F(FileInfoTest, testConsistent)
{
   std::list<std::tuple<String, String>> data;
   init_consistent_data(data);
   for (auto &item : data) {
      String &file = std::get<0>(item);
      String &expected = std::get<1>(item);
      FileInfo fileInfo(file);
      ASSERT_EQ(fileInfo.getFilePath(), expected);
      ASSERT_EQ(fileInfo.getDir().getPath() + Latin1Character('/') + fileInfo.getFileName(), expected);
   }
}

TEST_F(FileInfoTest, testFileTimes)
{
   auto datePairString = [](const DateTime &actual, const DateTime &before) {
      return (actual.toString(pdk::DateFormat::ISODateWithMs) + Latin1String(" (should be >) ") + before.toString(pdk::DateFormat::ISODateWithMs))
            .toLatin1();
   };
   std::list<String> data;
   init_filetimes_data(data);
   for (auto &fileName : data) {
      int sleepTime = 100;
      // on Linux and Windows, the filesystem timestamps may be slightly out of
      // sync with the system clock (maybe they're using CLOCK_REALTIME_COARSE),
      // so add a margin of error to our comparisons
      int fsClockSkew = 10;
#ifdef PDK_OS_WIN
      fsClockSkew = 500;
#endif
      // NFS clocks may be WAY out of sync
      if (is_likely_to_be_nfs(fileName)) {
         std::cout << "This test doesn't work on NFS" << std::endl;
         return;
      }
      bool noAccessTime = false;
      {
         // try to guess if file times on this filesystem round to the second
         FileInfo cwd(Latin1String("."));
         if (cwd.getLastModified().toMSecsSinceEpoch() % 1000 == 0
             && cwd.getLastRead().toMSecsSinceEpoch() % 1000 == 0) {
            fsClockSkew = sleepTime = 1000;
            noAccessTime = is_likely_to_be_fat(fileName);
            if (noAccessTime) {
               // FAT filesystems (but maybe not exFAT) store timestamps with 2-second
               // granularity and access time with 1-day granularity
               fsClockSkew = sleepTime = 2000;
            }
         }
      }
      
      if (File::exists(fileName)) {
         ASSERT_TRUE(File::remove(fileName));
      }
      
      DateTime beforeBirth;
      DateTime beforeWrite;
      DateTime beforeMetadataChange;
      DateTime beforeRead;
      DateTime birthTime;
      DateTime writeTime;
      DateTime metadataChangeTime;
      DateTime readTime;
      
      // --- Create file and write to it
      beforeBirth = DateTime::getCurrentDateTime().addMSecs(-fsClockSkew);
      {
         File file(fileName);
         ASSERT_TRUE(file.open(File::OpenModes(File::OpenMode::WriteOnly) | File::OpenMode::Text));
         FileInfo fileInfo(fileName);
         birthTime = fileInfo.getBirthTime();
         ASSERT_TRUE(!birthTime.isValid() || birthTime > beforeBirth) << datePairString(birthTime, beforeBirth).getConstRawData();
         
         pdktest::sleep(sleepTime);
         beforeWrite = DateTime::getCurrentDateTime().addMSecs(-fsClockSkew);
         // @TODO change for TextStream
         //         TextStream ts(&file);
         //         ts << fileName << endl;
         file.write(fileName.toLatin1());
      }
      
      {
         FileInfo fileInfo(fileName);
         writeTime = fileInfo.getLastModified();
         ASSERT_TRUE(writeTime > beforeWrite) << datePairString(writeTime, beforeWrite).getConstRawData();
         ASSERT_EQ(fileInfo.getBirthTime(), birthTime); // mustn't have changed
      }
      pdktest::sleep(sleepTime);
      beforeMetadataChange = DateTime::getCurrentDateTime().addMSecs(-fsClockSkew);
      {
         File file(fileName);
         file.setPermissions(file.getPermissions());
      }
      {
         FileInfo fileInfo(fileName);
         metadataChangeTime = fileInfo.getMetadataChangeTime();
         ASSERT_TRUE(metadataChangeTime > beforeMetadataChange) << datePairString(metadataChangeTime, beforeMetadataChange).getConstRawData();
         ASSERT_TRUE(metadataChangeTime >= writeTime); // not all filesystems can store both times
         ASSERT_EQ(fileInfo.getBirthTime(), birthTime); // mustn't have changed
      }
      
      
      // --- Read the file
      pdktest::sleep(sleepTime);
      beforeRead = DateTime::getCurrentDateTime().addMSecs(-fsClockSkew);
      {
         File file(fileName);
         ASSERT_TRUE(file.open(File::OpenModes(File::OpenMode::ReadOnly) | File::OpenMode::Text));
         //         TextStream ts(&file);
         //         String line = ts.readLine();
         String line(Latin1String(file.readLine()));
         ASSERT_EQ(line, fileName);
      }
      
      FileInfo fileInfo(fileName);
      readTime = fileInfo.getLastRead();
      ASSERT_EQ(fileInfo.getLastModified(), writeTime); // mustn't have changed
      ASSERT_EQ(fileInfo.getBirthTime(), birthTime); // mustn't have changed
      ASSERT_TRUE(readTime.isValid());
      // @TODO add windows testcases
      if (noAccessTime) {
         return;
      }
      ASSERT_TRUE(readTime > beforeRead) << datePairString(readTime, beforeRead).getConstRawData();
      ASSERT_TRUE(writeTime < beforeRead);
   }
}

TEST_F(FileInfoTest, testFileTimesOldFile)
{
   // This is 2^{31} seconds before 1970-01-01 15:14:8,
   // i.e. shortly after the start of time_t, in any time-zone:
   const DateTime early(Date(1901, 12, 14), Time(12, 0));
   File file(Latin1String("ancientfile.txt"));
   file.open(IoDevice::OpenMode::WriteOnly);
   file.write("\n", 1);
   file.close();
   
   /*
        File's setFileTime calls FileEngine::setFileTime() which fails unless
        the file is open at the time.  Of course, when writing, close() changes
        modification time, so need to re-open for read in order to setFileTime().
       */
   file.open(IoDevice::OpenMode::ReadOnly);
   bool ok = file.setFileTime(early, FileDevice::FileTime::FileModificationTime);
   file.close();
   if (ok) {
      FileInfo info(file.getFileName());
      ASSERT_EQ(info.getLastModified(), early);
   } else {
      std::cout << "Unable to set file metadata to ancient values" << std::endl;
      return;
   }
}

namespace {

void init_is_syslink_data(std::list<std::tuple<String, bool, String>> &data)
{
#ifndef PDK_NO_SYMLINKS
   File::remove(Latin1String("link.lnk"));
   File::remove(Latin1String("brokenlink.lnk"));
   File::remove(Latin1String("dummyfile"));
   File::remove(Latin1String("relative/link.lnk"));
   
   File file1(sg_sourceFile);
   ASSERT_TRUE(file1.link(Latin1String("link.lnk")));
   
   File file2(Latin1String("dummyfile"));
   file2.open(File::OpenMode::WriteOnly);
   ASSERT_TRUE(file2.link(Latin1String("brokenlink.lnk")));
   file2.remove();
   
   data.push_back(std::make_tuple(sg_sourceFile, false, Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("link.lnk"), true, FileInfo(sg_sourceFile).getAbsoluteFilePath()));
   data.push_back(std::make_tuple(Latin1String("brokenlink.lnk"), true, FileInfo(Latin1String("dummyfile")).getAbsoluteFilePath()));
   
   // @TODO add windows testcases
#endif
}

void init_is_hidden_data(std::list<std::tuple<String, bool>> &data)
{
   for (const FileInfo& info : Dir::getDrives()) {
      data.push_back(std::make_tuple(info.getPath(), false));
   }
   // @TODO add windows testcases
#if defined(PDK_OS_UNIX)
   ASSERT_TRUE(Dir(Latin1String("./.hidden-directory")).exists() || Dir().mkdir(Latin1String("./.hidden-directory")));
   data.push_back(std::make_tuple(Dir::getCurrentPath() + String(Latin1String("/.hidden-directory")), true));
   data.push_back(std::make_tuple(Dir::getCurrentPath() + String(Latin1String("/.hidden-directory/.")), true));
   data.push_back(std::make_tuple(Dir::getCurrentPath() + String(Latin1String("/.hidden-directory/..")), true));
#endif
   
#if defined(PDK_OS_MAC)
   // /bin has the hidden attribute on OS X
   data.push_back(std::make_tuple(String::fromLatin1("/bin/"), true));
#elif !defined(PDK_OS_WIN)
   data.push_back(std::make_tuple(String::fromLatin1("/bin/"), false));
#endif
   
#ifdef PDK_OS_MAC
   data.push_back(std::make_tuple(String::fromLatin1("/etc"), true));
   data.push_back(std::make_tuple(String::fromLatin1("/usr/lib"), false));
   data.push_back(std::make_tuple(String::fromLatin1("/Applications"), false));
#endif
}

void init_is_bundle_data(std::list<std::tuple<String, bool>> &data)
{
   data.push_back(std::make_tuple(String::fromLatin1("/"), false));
#ifdef PDK_OS_MAC
   data.push_back(std::make_tuple(String::fromLatin1("/Applications"), false));
   data.push_back(std::make_tuple(String::fromLatin1("/Applications/Safari.app"), true));
#endif
}

} // anonymous namespace

TEST_F(FileInfoTest, testIsSymLink)
{
#ifdef PDK_NO_SYMLINKS
   std::cout << "No symlink support" << std::endl;
   return;
#else
   std::list<std::tuple<String, bool, String>> data;
   init_is_syslink_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      bool isSymLink = std::get<1>(item);
      String &linkTarget = std::get<2>(item);
      FileInfo fileInfo(path);
      ASSERT_EQ(fileInfo.isSymLink(), isSymLink);
      ASSERT_EQ(fileInfo.getSymLinkTarget(), linkTarget);
   }
   
#endif
}

TEST_F(FileInfoTest, testIsHidden)
{
   std::list<std::tuple<String, bool>> data;
   init_is_hidden_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      bool isHidden = std::get<1>(item);
      FileInfo fileInfo(path);
      ASSERT_EQ(fileInfo.isHidden(), isHidden);
   }
}

#if defined(PDK_OS_MAC)
TEST_F(FileInfoTest, testIsHiddenFromFinder)
{
   Latin1String filename("test_foobar.txt");
   
   File testFile(filename);
   testFile.open(IoDevice::OpenModes(IoDevice::OpenMode::WriteOnly) | IoDevice::OpenMode::Append);
   testFile.write(ByteArray("world"));
   testFile.close();
   
   struct stat buf;
   stat(filename.getRawData(), &buf);
   chflags(filename.getRawData(), buf.st_flags | UF_HIDDEN);
   
   FileInfo fileInfo(filename);
   ASSERT_EQ(fileInfo.isHidden(), true);
   
   testFile.remove();
}
#endif

TEST_F(FileInfoTest, testIsBundle)
{
   std::list<std::tuple<String, bool>> data;
   init_is_bundle_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      bool isBundle = std::get<1>(item);
      FileInfo fileInfo(path);
      ASSERT_EQ(fileInfo.isBundle(), isBundle);
   }
}

namespace {

void init_native_path_data(std::list<std::tuple<String, bool>> &data)
{
   data.push_back(std::make_tuple(String(), false));
   data.push_back(std::make_tuple(String(Latin1String("")), false));
   data.push_back(std::make_tuple(String::fromLatin1("/"), true));
   data.push_back(std::make_tuple(String::fromLatin1("/abrakadabra.boo"), true));
   // @TODO add resources testcases
}

} // anonymous namespace

TEST_F(FileInfoTest, testIsNativePath)
{
   std::list<std::tuple<String, bool>> data;
   init_native_path_data(data);
   for (auto &item : data) {
      String &path = std::get<0>(item);
      bool isNativePath = std::get<1>(item);
      FileInfo fileInfo(path);
      if (path.isNull()) {
         fileInfo = FileInfo();
      }
      ASSERT_EQ(fileInfo.isNativePath(), isNativePath);
   }
}

TEST_F(FileInfoTest, testRefresh)
{
#if defined(PDK_OS_WIN)
   int sleepTime = 3000;
#else
   int sleepTime = 2000;
#endif
   File::remove(Latin1String("file1"));
   File file(Latin1String("file1"));
   
   ASSERT_TRUE(file.open(File::OpenMode::WriteOnly));
   ASSERT_EQ(file.write("JAJAJAA"), pdk::pint64(7));
   file.flush();
   
   FileInfo info(file);
   DateTime lastModified = info.getLastModified();
   ASSERT_EQ(info.getSize(), pdk::pint64(7));
   
   pdktest::sleep(sleepTime);
   
   ASSERT_EQ(file.write("JOJOJO"), pdk::pint64(6));
   file.flush();
   ASSERT_EQ(info.getLastModified(), lastModified);
   
   ASSERT_EQ(info.getSize(), pdk::pint64(7));
   
#if defined(PDK_OS_WIN)
   file.close();
#endif
   info.refresh();
   ASSERT_EQ(info.getSize(), pdk::pint64(13));
   ASSERT_TRUE(info.getLastModified() > lastModified);
   
   FileInfo info2 = info;
   ASSERT_EQ(info2.getSize(), info.getSize());
   
   info2.refresh();
   ASSERT_EQ(info2.getSize(), info.getSize());
}

// @TODO add Windows testcases
//TEST_F(FileInfoTest, testNtfsJunctionPointsAndSymlinks)
//{

//}

//TEST_F(FileInfoTest, testBrokenShortcut)
//{

//}

TEST_F(FileInfoTest, testIsWritable)
{
   File tempfile(Latin1String("tempfile.txt"));
   tempfile.open(IoDevice::OpenMode::WriteOnly);
   tempfile.write("This file is generated by the FileInfo autotest.");
   tempfile.close();
   
   ASSERT_TRUE(FileInfo(Latin1String("tempfile.txt")).isWritable());
   tempfile.remove();
#if defined(PDK_OS_WIN)
   FileInfo fileInfo(Latin1String("c:\\pagefile.sys"));
   ASSERT_TRUE(fileInfo.exists()) << msg_does_not_exist(fileInfo.getAbsoluteFilePath()).getConstRawData();
   ASSERT_TRUE(!fileInfo.isWritable());
#endif
   
#if defined (PDK_OS_WIN)
   ScopedValueRollback<int> ntfsMode(pdk_ntfs_permission_lookup);
   pdk_ntfs_permission_lookup = 1;
   FileInfo fi2(File::decodeName(pdk::getenv("SystemRoot") + "/system.ini"));
   ASSERT_TRUE(fi2.exists());
   ASSERT_EQ(fi2.isWritable(), IsUserAdmin());
#endif
   
#if defined (PDK_OS_UNIX) && !defined(PDK_OS_VXWORKS) // VxWorks does not have users/groups
   if (::getuid() == 0) {
      ASSERT_TRUE(FileInfo(Latin1String("/etc/passwd")).isWritable());
   } else {
      ASSERT_TRUE(!FileInfo(Latin1String("/etc/passwd")).isWritable());
   }
#endif
}

TEST_F(FileInfoTest, testIsExecutable)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   String appPath = CoreApplication::getAppDirPath();
   appPath += Latin1String("/IoTest");
# if defined(PDK_OS_WIN)
   appPath += ".exe";
# endif
   FileInfo fileInfo(appPath);
   ASSERT_EQ(fileInfo.isExecutable(), true);
   ASSERT_EQ(FileInfo(m_pdkFile).isExecutable(), false);
   
#ifdef PDK_OS_UNIX
   File::remove(Latin1String("link.lnk"));
   
   // Symlink to executable
   File appFile(appPath);
   ASSERT_TRUE(appFile.link(Latin1String("link.lnk")));
   ASSERT_EQ(FileInfo(Latin1String("link.lnk")).isExecutable(), true);
   File::remove(Latin1String("link.lnk"));
   
   // Symlink to .pdk file
   File pdkFile(Latin1String("/IoTest"));
   ASSERT_TRUE(pdkFile.link(Latin1String("link.lnk")));
   ASSERT_EQ(FileInfo(Latin1String("link.lnk")).isExecutable(), false);
   File::remove(Latin1String("link.lnk"));
#endif
   PDKTEST_END_APP_CONTEXT();
}

namespace {

void init_decomposed_unicode_names_data(std::list<std::tuple<String, String, bool>> &data)
{
   String currPath = Dir::getCurrentPath();
   data.push_back(std::make_tuple(currPath + Latin1String("/4.pdf"), Latin1String("4.pdf"), true));
   data.push_back(std::make_tuple(currPath + String::fromUtf8("/4 ä.pdf"), String::fromUtf8("4 ä.pdf"), true));
   data.push_back(std::make_tuple(currPath + String::fromUtf8("/4 äääcopy.pdf"), String::fromUtf8("4 äääcopy.pdf"), true));
   data.push_back(std::make_tuple(currPath + String::fromUtf8("/4 øøøcopy.pdf"), String::fromUtf8("4 øøøcopy.pdf"), true));
}

// This is a helper class that ensures that files created during the test
// will be removed afterwards, even if the test fails or throws an exception.
class NativeFileCreator
{
public:
   NativeFileCreator(const String &filePath)
      : m_filePath(filePath), m_error(0)
   {
#ifdef PDK_OS_UNIX
      int fd = open(m_filePath.normalized(String::NormalizationForm::Form_D).toUtf8().getConstRawData(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
      if (fd >= 0) {
         close(fd);
      } else {
         m_error = errno;
      }
#endif
   }
   ~NativeFileCreator()
   {
#ifdef PDK_OS_UNIX
      if (m_error == 0) {
         unlink(m_filePath.normalized(String::NormalizationForm::Form_D).toUtf8().getConstRawData());
      }
#endif
   }
   int getError() const
   {
      return m_error;
   }
   
private:
   String m_filePath;
   int m_error;
};

} // anonymous namespace

TEST_F(FileInfoTest, testDecomposedUnicodeNames)
{
#ifndef PDK_OS_MAC
   std::cout << "This is a OS X only test (unless you know more about filesystems, then maybe you should try it ;)" << std::endl;
#else
   std::list<std::tuple<String, String, bool>> data;
   init_decomposed_unicode_names_data(data);
   for (auto &item : data) {
      String &filePath = std::get<0>(item);
      String &fileName = std::get<1>(item);
      bool exists = std::get<2>(item);
      
      NativeFileCreator nativeFileCreator(filePath);
      int error = nativeFileCreator.getError();
      ASSERT_TRUE(error == 0) << pdk_printable(String(Latin1String("Couldn't create native file %1: %2")).arg(filePath).arg(Latin1String(strerror(error))));
      FileInfo fileInfo(filePath);
      ASSERT_EQ(fileInfo.getFileName(), fileName);
      ASSERT_EQ(fileInfo.exists(), exists);
   }
#endif
}

TEST_F(FileInfoTest, testEqualOperator)
{
   /* Compare two default constructed values. Yes, to me it seems it should be the opposite too, but
     * this is how the code was written. */
   ASSERT_TRUE(!(FileInfo() == FileInfo()));
}

TEST_F(FileInfoTest, testEqualOperatorWithDifferentSlashes)
{
   const FileInfo fi1(Latin1String("/usr"));
   const FileInfo fi2(Latin1String("/usr/"));
   ASSERT_EQ(fi1, fi2);
}

TEST_F(FileInfoTest, testNotEqualOperator)
{
   /* Compare two default constructed values. Yes, to me it seems it should be the opposite too, but
     * this is how the code was written. */
   ASSERT_TRUE(FileInfo() != FileInfo());
}

TEST_F(FileInfoTest, testDetachingOperations)
{
   FileInfo info1;
   ASSERT_TRUE(info1.getCaching());
   info1.setCaching(false);
   
   {
      FileInfo info2 = info1;
      ASSERT_TRUE(!info1.getCaching());
      ASSERT_TRUE(!info2.getCaching());
      
      info2.setCaching(true);
      ASSERT_TRUE(info2.getCaching());
      
      info1.setFile(Latin1String("foo"));
      ASSERT_TRUE(!info1.getCaching());
   }
   
   {
      File file(Latin1String("foo"));
      info1.setFile(file);
      ASSERT_TRUE(!info1.getCaching());
   }
   
   info1.setFile(Dir(), Latin1String("foo"));
   ASSERT_TRUE(!info1.getCaching());
   
   {
      FileInfo info3;
      ASSERT_TRUE(info3.getCaching());
      
      info3 = info1;
      ASSERT_TRUE(!info3.getCaching());
   }
   
   info1.refresh();
   ASSERT_TRUE(!info1.getCaching());
   
   ASSERT_TRUE(info1.makeAbsolute());
   ASSERT_TRUE(!info1.getCaching());
}

TEST_F(FileInfoTest, testOwner)
{
   String userName;
#if defined(PDK_OS_UNIX) && !defined(PDK_OS_VXWORKS)
   {
      passwd *user = getpwuid(geteuid());
      ASSERT_TRUE(user);
      char *usernameBuf = user->pw_name;
      userName = String::fromLocal8Bit(usernameBuf);
   }
#endif
#if defined(PDK_OS_WIN)
   wchar_t  usernameBuf[1024];
   DWORD  bufSize = 1024;
   if (GetUserNameW(usernameBuf, &bufSize)) {
      userName = String::fromWCharArray(usernameBuf);
      if (SysInfo::WindowsVersion >= SysInfo::WV_VISTA && IsUserAdmin()) {
         // Special case : If the user is a member of Administrators group, all files
         // created by the current user are owned by the Administrators group.
         LPLOCALGROUP_USERS_INFO_0 pBuf = NULL;
         DWORD dwLevel = 0;
         DWORD dwFlags = LG_INCLUDE_INDIRECT ;
         DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
         DWORD dwEntriesRead = 0;
         DWORD dwTotalEntries = 0;
         NET_API_STATUS nStatus;
         nStatus = NetUserGetLocalGroups(0, usernameBuf, dwLevel, dwFlags, (LPBYTE *) &pBuf,
                                         dwPrefMaxLen, &dwEntriesRead, &dwTotalEntries);
         // Check if the current user is a member of Administrators group
         if (nStatus == NERR_Success && pBuf){
            for (int i = 0; i < (int)dwEntriesRead; i++) {
               String groupName = String::fromWCharArray(pBuf[i].lgrui0_name);
               if (!groupName.compare(QLatin1String("Administrators")))
                  userName = groupName;
            }
         }
         if (pBuf != NULL) {
            NetApiBufferFree(pBuf);
         }
         
      }
   }
   pdk_ntfs_permission_lookup = 1;
#endif
   if (userName.isEmpty()) {
      std::cout << "Can't retrieve the user name" << std::endl;
      return;
   }
   String fileName(Latin1String("ownertest.txt"));
   ASSERT_TRUE(!File::exists(fileName) || File::remove(fileName));
   {
      File testFile(fileName);
      ASSERT_TRUE(testFile.open(IoDevice::OpenModes(IoDevice::OpenMode::WriteOnly) | IoDevice::OpenMode::Text));
      ByteArray testData("testfile");
      ASSERT_TRUE(testFile.write(testData) != -1);
   }
   FileInfo fileInfo(fileName);
   ASSERT_TRUE(fileInfo.exists()) << msg_does_not_exist(fileInfo.getAbsoluteFilePath()).getConstRawData();
   ASSERT_EQ(fileInfo.getOwner(), userName);
   
   File::remove(fileName);
#if defined(PDK_OS_WIN)
   pdk_ntfs_permission_lookup = 0;
#endif
}

TEST_F(FileInfoTest, testGroup)
{
   String expected;
#if defined(PDK_OS_UNIX) && !defined(PDK_OS_VXWORKS)
   struct group *gr;
   gid_t gid = getegid();
   
   errno = 0;
   gr = getgrgid(gid);
   
   ASSERT_TRUE(gr) << pdk_printable(
                         String(Latin1String("getgrgid returned 0: %1, cannot determine my own group"))
                         .arg(String::fromLocal8Bit(strerror(errno))));
   expected = String::fromLocal8Bit(gr->gr_name);
#endif
   String fileName(Latin1String("ownertest.txt"));
   if (File::exists(fileName)) {
      File::remove(fileName);
   }
   File testFile(fileName);
   ASSERT_TRUE(testFile.open(IoDevice::OpenModes(IoDevice::OpenMode::WriteOnly) | IoDevice::OpenMode::Text));
   ByteArray testData("testfile");
   ASSERT_TRUE(testFile.write(testData) != -1);
   testFile.close();
   FileInfo fileInfo(fileName);
   ASSERT_TRUE(fileInfo.exists()) << msg_does_not_exist(fileInfo.getAbsoluteFilePath()).getConstRawData();
   ASSERT_EQ(fileInfo.getGroup(), expected);
}

namespace {

void state_check(const FileInfo &info, const String &dirname, const String &filename)
{
   ASSERT_EQ(info.getSize(), pdk::pint64(0));
   ASSERT_TRUE(!info.exists());
   
   String path;
   String abspath;
   if (!dirname.isEmpty()) {
      path = Latin1String(".");
      abspath = dirname + '/' + filename;
   }
   
   ASSERT_EQ(info.getFilePath(), filename);
   ASSERT_EQ(info.getAbsoluteFilePath(), abspath);
   ASSERT_EQ(info.getCanonicalFilePath(), String());
   ASSERT_EQ(info.getFileName(), filename);
   ASSERT_EQ(info.getBaseName(), filename);
   ASSERT_EQ(info.getCompleteBaseName(), filename);
   ASSERT_EQ(info.getSuffix(), String());
   ASSERT_EQ(info.getBundleName(), String());
   ASSERT_EQ(info.getCompleteSuffix(), String());
   
   ASSERT_TRUE(info.isRelative());
   ASSERT_EQ(info.getPath(), path);
   ASSERT_EQ(info.getAbsolutePath(), dirname);
   ASSERT_EQ(info.getDir().getPath(), Latin1String("."));
   
   // these don't look right
   ASSERT_EQ(info.getCanonicalPath(), path);
   ASSERT_EQ(info.getAbsoluteDir().getPath(), dirname.isEmpty() ? Latin1String(".") : dirname);
   
   ASSERT_TRUE(!info.isReadable());
   ASSERT_TRUE(!info.isWritable());
   ASSERT_TRUE(!info.isExecutable());
   ASSERT_TRUE(!info.isHidden());
   ASSERT_TRUE(!info.isFile());
   ASSERT_TRUE(!info.isDir());
   ASSERT_TRUE(!info.isSymLink());
   ASSERT_TRUE(!info.isBundle());
   ASSERT_TRUE(!info.isRoot());
   ASSERT_EQ(info.isNativePath(), !filename.isEmpty());
   
   ASSERT_EQ(info.getSymLinkTarget(), String());
   ASSERT_EQ(info.getOwnerId(), uint(-2));
   ASSERT_EQ(info.getGroupId(), uint(-2));
   ASSERT_EQ(info.getOwner(), String());
   ASSERT_EQ(info.getGroup(), String());
   
   ASSERT_EQ(info.getPermissions(), File::Permissions());
   
   ASSERT_TRUE(!info.getBirthTime().isValid());
   ASSERT_TRUE(!info.getMetadataChangeTime().isValid());
   ASSERT_TRUE(!info.getLastRead().isValid());
   ASSERT_TRUE(!info.getLastModified().isValid());
}

void init_invalid_state_data(std::list<int> &data)
{
   data.push_back(0);
   data.push_back(1);
   data.push_back(2);
   data.push_back(3);
}

} // anonymous namespace

TEST_F(FileInfoTest, testinvalidState)
{
   // Shouldn't crash or produce warnings
   std::list<int> data;
   init_invalid_state_data(data);
   for (int mode : data) {
      const FileInfo &info = (mode & 1 ? FileInfo(Latin1String("")) : FileInfo());
      if (mode & 2) {
         FileInfo copy(info);
         state_check(copy, String(), String());
      } else {
         state_check(info, String(), String());
      }
   }
}

TEST_F(FileInfoTest, testNonExistingFile)
{
   String dirname = Dir::getCurrentPath();
   String cdirname = FileInfo(dirname).getCanonicalFilePath();
   if (dirname != cdirname) {
      Dir::setCurrent(cdirname); // chdir() to our canonical path
   }
   String filename = Latin1String("non-existing-file-foobar");
   FileInfo info(filename);
   state_check(info, dirname, filename);
}
