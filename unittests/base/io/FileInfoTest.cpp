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
using pdk::io::fs::internal::FileInfoPrivate;
using pdk::io::fs::StandardPaths;
using pdk::ds::StringList;
using pdk::io::fs::TemporaryFile;
using pdk::io::fs::TemporaryDir;
using pdk::io::Debug;
using pdk::kernel::CoreApplication;
using pdk::io::fs::StorageInfo;

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
