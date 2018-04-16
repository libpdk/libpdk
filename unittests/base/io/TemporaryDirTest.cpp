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
// Created by zzu_softboy on 2018/04/13.

#include "gtest/gtest.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/io/fs/TemporaryDir.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include "pdktest/PdkTest.h"
#include "pdk/base/io/fs/StandardPaths.h"
#ifdef PDK_OS_WIN
# include "pdk/global/Windows.h"
#endif
#ifdef PDK_OS_UNIX // for geteuid()
# include <sys/types.h>
# include <unistd.h>
#endif
#include <iostream>
#include <set>

using pdk::lang::String;
using pdk::ds::StringList;
using pdk::kernel::CoreApplication;
using pdk::io::fs::Dir;
using pdk::io::fs::FileInfo;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::io::fs::TemporaryDir;
using pdk::lang::Character;
using pdk::io::fs::File;
using pdk::io::IoDevice;
using pdk::io::fs::StandardPaths;

namespace {

class TemporaryDirTest : public ::testing::Test
{
public:
   void SetUp()
   {
      m_previousCurrent = Dir::getCurrentPath();
      Dir::setCurrent(Dir::getTempPath());
      ASSERT_TRUE(Dir(Latin1String("test-XXXXXX")).exists() || Dir().mkdir(Latin1String("test-XXXXXX")));
      CoreApplication::setAppName(Latin1String("test_temporarydir"));
   }
   
   // Tears down the test fixture.
   void TearDown()
   {
      ASSERT_TRUE(Dir().rmdir(Latin1String("test-XXXXXX")));
      Dir::setCurrent(m_previousCurrent);
   }
private:
   String m_previousCurrent;
};

String han_test_text()
{
   String text;
   text += Character(0x65B0);
   text += Character(0x5E10);
   text += Character(0x6237);
   return text;
}

String umlaut_test_text()
{
   String text;
   text += Character(0xc4);
   text += Character(0xe4);
   text += Character(0xd6);
   text += Character(0xf6);
   text += Character(0xdc);
   text += Character(0xfc);
   text += Character(0xdf);
   return text;
}

void init_file_template_data(std::list<std::tuple<String, String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String(""), Latin1String("test_temporarydir-"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXXxxx"), Latin1String("pdk_"), Latin1String("xxx")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXXxXx"), Latin1String("pdk_"), Latin1String("xXx")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXX"), Latin1String("pdk_"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXXXXXX"), Latin1String("pdk_"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXX_XXXX"), Latin1String("pdk_"), Latin1String("_XXXX")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXX"), Latin1String("pdk_XXXX"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXX"), Latin1String("pdk_XXXXX"), Latin1String("")));
   
   // Test Umlauts (contained in Latin1)
   String prefix = Latin1String("pdk_") + umlaut_test_text();
   data.push_back(std::make_tuple((prefix + Latin1String("XXXXXX")), prefix, Latin1String("")));
   // test non-Latin1
   prefix = Latin1String("pdk_") + han_test_text();
   data.push_back(std::make_tuple((prefix + Latin1String("XXXXXX") + umlaut_test_text()), prefix, umlaut_test_text()));
}

void init_filepath_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(String(), Latin1String("/tmpfile")));
   data.push_back(std::make_tuple(String(), Latin1String("tmpfile")));
   data.push_back(std::make_tuple(Latin1String("XXXXX"), Latin1String("tmpfile")));
   data.push_back(std::make_tuple(Latin1String("YYYYY"), Latin1String("subdir/file")));
}

} // anonymous namespace

TEST_F(TemporaryDirTest, testConstruction)
{
   TemporaryDir dir;
   String tmp = Dir::getTempPath();
   ASSERT_EQ(dir.getPath().left(tmp.size()), tmp);
   ASSERT_TRUE(dir.getPath().contains(Latin1String("test_temporarydir")));
   ASSERT_TRUE(FileInfo(dir.getPath()).isDir());
   ASSERT_EQ(dir.getErrorString(), String());
}

TEST_F(TemporaryDirTest, testGetSetCheck)
{
   TemporaryDir dir;
   // bool TemporaryDir::autoRemove()
   // void TemporaryDir::setAutoRemove(bool)
   dir.setAutoRemove(false);
   ASSERT_EQ(false, dir.autoRemove());
   dir.setAutoRemove(true);
   ASSERT_EQ(true, dir.autoRemove());
}

TEST_F(TemporaryDirTest, testFileTemplate)
{
   std::list<std::tuple<String, String, String>> data;
   init_file_template_data(data);
   for (auto &item : data) {
      String constructorTemplate = std::get<0>(item);
      String prefix = std::get<1>(item);
      String suffix = std::get<2>(item);
      TemporaryDir tempDir(constructorTemplate);
      ASSERT_TRUE(tempDir.isValid());
      String dirName = Dir(tempDir.getPath()).getDirName();
      if (prefix.length()) {
         ASSERT_EQ(dirName.left(prefix.length()), prefix);
         ASSERT_EQ(dirName.right(suffix.length()), suffix);
      }
   }
}

TEST_F(TemporaryDirTest, testFileName)
{
   // Get Dir::tempPath and make an absolute path.
   String tempPath = Dir::getTempPath();
   String absoluteTempPath = Dir(tempPath).getAbsolutePath();
   TemporaryDir dir;
   dir.setAutoRemove(true);
   String fileName = dir.getPath();
   ASSERT_TRUE(fileName.contains(Latin1String("/test_temporarydir-"))) << pdk_printable(fileName);
   ASSERT_TRUE(Dir(fileName).exists());
   // Get path to the temp dir, without the file name.
   String absoluteFilePath = FileInfo(fileName).getAbsolutePath();
#if defined(PDK_OS_WIN)
   absoluteFilePath = absoluteFilePath.toLower();
   absoluteTempPath = absoluteTempPath.toLower();
#endif
   ASSERT_EQ(absoluteFilePath, absoluteTempPath);
}

TEST_F(TemporaryDirTest, testFilePath)
{
   std::list<std::tuple<String, String>> data;
   init_filepath_data(data);
   for (auto &item : data) {
      String &templatePath = std::get<0>(item);
      String &fileName = std::get<1>(item);
      
      TemporaryDir dir(templatePath);
      const String filePath = dir.getFilePath(fileName);
      const String expectedFilePath = Dir::isAbsolutePath(fileName) ?
               String() : dir.getPath() + Latin1Character('/') + fileName;
      ASSERT_EQ(filePath, expectedFilePath);
   }
}

TEST_F(TemporaryDirTest, testAutoRemove)
{
   // Test auto remove
   String dirName;
   {
      TemporaryDir dir(Latin1String("tempXXXXXX"));
      dir.setAutoRemove(true);
      ASSERT_TRUE(dir.isValid());
      dirName = dir.getPath();
   }
#ifdef PDK_OS_WIN
   // Windows seems unreliable here: sometimes it says the directory still exists,
   // immediately after we deleted it.
   PDK_TRY_VERIFY(!Dir(dirName).exists());
#else
   ASSERT_TRUE(!Dir(dirName).exists());
#endif
   
   // Test if disabling auto remove works.
   {
      TemporaryDir dir(Latin1String("tempXXXXXX"));
      dir.setAutoRemove(false);
      ASSERT_TRUE(dir.isValid());
      dirName = dir.getPath();
   }
   ASSERT_TRUE(Dir(dirName).exists());
   ASSERT_TRUE(Dir().rmdir(dirName));
   ASSERT_TRUE(!Dir(dirName).exists());
   
   // Do not explicitly call setAutoRemove (tests if it really is the default as documented)
   {
      TemporaryDir dir(Latin1String("tempXXXXXX"));
      ASSERT_TRUE(dir.isValid());
      dirName = dir.getPath();
   }
#ifdef PDK_OS_WIN
   PDK_TRY_VERIFY(!Dir(dirName).exists());
#else
   ASSERT_TRUE(!Dir(dirName).exists());
#endif
   
   // Test autoremove with files and subdirs in the temp dir
   {
      TemporaryDir tempDir(Latin1String("tempXXXXXX"));
      ASSERT_TRUE(tempDir.isValid());
      dirName = tempDir.getPath();
      Dir dir(dirName);
      ASSERT_TRUE(dir.mkdir(String::fromLatin1("dir1")));
      ASSERT_TRUE(dir.mkdir(String::fromLatin1("dir2")));
      ASSERT_TRUE(dir.mkdir(String::fromLatin1("dir2/nested")));
      File file(dirName + Latin1String("/dir1/file"));
      ASSERT_TRUE(file.open(IoDevice::OpenMode::WriteOnly));
      ASSERT_EQ(file.write("Hello"), 5LL);
      file.close();
      ASSERT_TRUE(file.setPermissions(File::Permission::ReadUser));
   }
#ifdef PDK_OS_WIN
   PDK_TRY_VERIFY(!Dir(dirName).exists());
#else
   ASSERT_TRUE(!Dir(dirName).exists());
#endif
}

TEST_F(TemporaryDirTest, testNonWritableCurrentDir)
{
#ifdef PDK_OS_UNIX
   
   const char nonWritableDir[] = "/home";
   if (::geteuid() == 0) {
      std::cerr << "not valid running this test as root";
      return;
   }
   struct ChdirOnReturn
   {
      ChdirOnReturn(const String& dir) : m_dir(dir) {}
      ~ChdirOnReturn() {
         Dir::setCurrent(m_dir);
      }
      String m_dir;
   };
   
   const FileInfo nonWritableDirFi = FileInfo(Latin1String(nonWritableDir));
   ASSERT_TRUE(nonWritableDirFi.isDir());
   
   ASSERT_TRUE(!nonWritableDirFi.isWritable());
   
   ChdirOnReturn cor(Dir::getCurrentPath());
   ASSERT_TRUE(Dir::setCurrent(nonWritableDirFi.getAbsoluteFilePath()));
   // TemporaryDir("tempXXXXXX") is probably a bad idea in any app
   // where the current dir could anything...
   TemporaryDir dir(Latin1String("tempXXXXXX"));
   dir.setAutoRemove(true);
   ASSERT_TRUE(!dir.isValid());
   ASSERT_TRUE(!dir.getErrorString().isEmpty());
   ASSERT_TRUE(dir.getPath().isEmpty());
#endif
}

TEST_F(TemporaryDirTest, testOpenOnRootDrives)
{
#if defined(PDK_OS_WIN)
   unsigned int lastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
   // If it's possible to create a file in the root directory, it
   // must be possible to create a temp dir there too.
   for (const FileInfo &driveInfo : Dir::getDrives()) {
      File testFile(driveInfo.getFilePath() + Latin1String("XXXXXX"));
      if (testFile.open(IoDevice::OpenMode::ReadWrite)) {
         testFile.remove();
         TemporaryDir dir(driveInfo.getFilePath() + Latin1String("XXXXXX"));
         dir.setAutoRemove(true);
         ASSERT_TRUE(dir.isValid());
      }
   }
#if defined(PDK_OS_WIN)
   SetErrorMode(lastErrorMode);
#endif
}

TEST_F(TemporaryDirTest, testStressTest)
{
   const int iterations = 1000;
   TemporaryDir rootDir;
   ASSERT_TRUE(rootDir.isValid());
   
   std::set<String> names;
   const String pattern = rootDir.getPath() + StringLiteral("/XXXXXX");
   for (int i = 0; i < iterations; ++i) {
      TemporaryDir dir(pattern);
      dir.setAutoRemove(false);
      ASSERT_TRUE(dir.isValid()) << pdk_printable(String::fromLatin1("Failed to create #%1 under %2: %3.")
                                                  .arg(i)
                                                  .arg(Dir::toNativeSeparators(pattern))
                                                  .arg(dir.getErrorString()));
      ASSERT_TRUE(names.find(dir.getPath()) == names.end());
      names.insert(dir.getPath());
   }
}

TEST_F(TemporaryDirTest, testRename)
{
   // This test checks what happens if the temporary dir is renamed.
   // Then the autodelete feature can't possibly find it.
   
   Dir dir;
   ASSERT_TRUE(!dir.exists(Latin1String("temporary-dir.renamed")));
   
   String tempname;
   {
      TemporaryDir tempDir(dir.getFilePath(Latin1String("temporary-dir.XXXXXX")));
      
      ASSERT_TRUE(tempDir.isValid());
      tempname = tempDir.getPath();
      
      ASSERT_TRUE(Dir().rename(tempname, Latin1String("temporary-dir.renamed")));
      ASSERT_TRUE(!Dir(tempname).exists());
      dir.setPath(Latin1String("temporary-dir.renamed"));
      ASSERT_EQ(dir.getPath(), String(Latin1String("temporary-dir.renamed")));
      ASSERT_TRUE(dir.exists());
   }
   
   // Auto-delete couldn't find it
   ASSERT_TRUE(dir.exists());
   // Clean up by hand
   ASSERT_TRUE(dir.removeRecursively());
   ASSERT_TRUE(!dir.exists());
}

namespace {

void init_unicode_support_data(std::list<std::tuple<String, String, bool>> &data)
{
   String unicode = String::fromUtf8("\xc3\xa5\xc3\xa6\xc3\xb8");
   data.push_back(std::make_tuple(String(), String(), true));
   data.push_back(std::make_tuple(String(Latin1String(".")), String(), true));
   data.push_back(std::make_tuple(String(Latin1String("..")), String(), true));
   data.push_back(std::make_tuple(String(Latin1String("something")), String(), true));
   data.push_back(std::make_tuple(String(Latin1String("does-not-exist/pdk_temp")), String(), false));
   data.push_back(std::make_tuple(String(), unicode, true));
   data.push_back(std::make_tuple(unicode, String(), true));
}

} // anonymous namespce

TEST_F(TemporaryDirTest, testUnicodeSupport)
{
   ASSERT_TRUE(Dir(Latin1String("test-XXXXXX")).exists());
   
   struct CleanOnReturn
   {
      ~CleanOnReturn()
      {
         cleanup();
      }
      
      void cleanup()
      {
         for (const String &tempName : tempNames) {
            ASSERT_TRUE(Dir(tempName).removeRecursively());
         }
      }
      
      void reset()
      {
         tempNames.clear();
      }
      
      StringList tempNames;
   };
   
   CleanOnReturn cleaner;
   
   std::list<std::tuple<String, String, bool>> data;
   init_unicode_support_data(data);
   for (auto &item : data) {
      String &prefix = std::get<0>(item);
      String &suffix = std::get<1>(item);
      bool openResult = std::get<2>(item);
      String fileTemplate1 = prefix + String(Latin1String("XX")) + suffix;
      String fileTemplate2 = prefix + String(Latin1String("XXXX")) + suffix;
      String fileTemplate3 = prefix + String(Latin1String("XXXXXX")) + suffix;
      String fileTemplate4 = prefix + String(Latin1String("XXXXXXXX")) + suffix;
      
      TemporaryDir dir1(fileTemplate1);
      TemporaryDir dir2(fileTemplate2);
      TemporaryDir dir3(fileTemplate3);
      TemporaryDir dir4(fileTemplate4);
      TemporaryDir dir5(Latin1String("test-XXXXXX/") + fileTemplate1);
      TemporaryDir dir6(Latin1String("test-XXXXXX/") + fileTemplate3);
      
      ASSERT_EQ(dir1.isValid(), openResult);
      ASSERT_EQ(dir2.isValid(), openResult);
      ASSERT_EQ(dir3.isValid(), openResult);
      ASSERT_EQ(dir4.isValid(), openResult);
      ASSERT_EQ(dir5.isValid(), openResult);
      ASSERT_EQ(dir6.isValid(), openResult);
      
      // make sure the dir exists under the *correct* name
      if (openResult) {
         cleaner.tempNames << dir1.getPath()
                           << dir2.getPath()
                           << dir3.getPath()
                           << dir4.getPath()
                           << dir5.getPath()
                           << dir6.getPath();
         
         Dir currentDir;
         String fileName1 = currentDir.getRelativeFilePath(dir1.getPath());
         String fileName2 = currentDir.getRelativeFilePath(dir2.getPath());
         String fileName3 = currentDir.getRelativeFilePath(dir3.getPath());
         String fileName4 = currentDir.getRelativeFilePath(dir4.getPath());
         String fileName5 = currentDir.getRelativeFilePath(dir5.getPath());
         String fileName6 = currentDir.getRelativeFilePath(dir6.getPath());
         
         ASSERT_TRUE(fileName1.startsWith(prefix));
         ASSERT_TRUE(fileName2.startsWith(prefix));
         ASSERT_TRUE(fileName5.startsWith(Latin1String("test-XXXXXX/") + prefix));
         ASSERT_TRUE(fileName6.startsWith(Latin1String("test-XXXXXX/") + prefix));
         
         if (!prefix.isEmpty()) {
            ASSERT_TRUE(fileName3.startsWith(prefix));
            ASSERT_TRUE(fileName4.startsWith(prefix));
         }
      }
   }
   
#ifdef PDK_OS_WIN
   Test::wait(20);
#endif
   for (const String &tempName : cleaner.tempNames) {
      ASSERT_TRUE(!Dir(tempName).exists()) <<  pdk_printable(tempName);
   }
   cleaner.reset();
}

TEST_F(TemporaryDirTest, testFailedSetPermissions)
{
   String path = StandardPaths::writableLocation(StandardPaths::StandardLocation::DownloadLocation) + StringLiteral("/");
   int count = Dir(path).entryList().size();
   
   {
      TemporaryDir dir(path);
   }
   
   ASSERT_EQ(Dir(path).entryList().size(), (size_t)count);
}

