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
// Created by zzu_softboy on 2018/04/20.

#include "gtest/gtest.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/io/fs/TemporaryDir.h"
#include "pdk/base/io/fs/TemporaryFile.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/time/Date.h"
#include "pdktest/PdkTest.h"

#if defined(PDK_OS_WIN)
# include "pdk/global/Windows.h"
#endif

#if defined(PDK_OS_UNIX)
# include <sys/types.h>
# include <sys/stat.h>
# include <errno.h>
# include <fcntl.h>             // open(2)
# include <unistd.h>            // close(2)
#endif
#include <set>

using pdk::lang::String;
using pdk::ds::StringList;
using pdk::ds::ByteArray;
using pdk::kernel::CoreApplication;
using pdk::io::fs::Dir;
using pdk::io::fs::FileInfo;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::io::fs::TemporaryDir;
using pdk::io::fs::TemporaryFile;
using pdk::lang::Character;
using pdk::io::fs::File;
using pdk::io::IoDevice;
using pdk::time::DateTime;
using pdk::time::Date;
using pdk::time::Time;

#define PDKTEST_DIR_SEP "/"
#define PDKTEST_TEMP_FILETEST_SUBDIR "temporaryfiletestdir"
#define PDKTEST_FINDTEST_SRC_DATA(filename) Latin1String(PDKTEST_CURRENT_TEST_SOURCE_DIR PDKTEST_DIR_SEP PDKTEST_TEMP_FILETEST_SUBDIR PDKTEST_DIR_SEP filename)

namespace {

class TemporaryFileTest : public ::testing::Test
{
public:
   void SetUp()
   {
      ASSERT_TRUE(m_temporaryDir.isValid()) << pdk_printable(m_temporaryDir.getErrorString());
      m_previousCurrent = Dir::getCurrentPath();
      ASSERT_TRUE(Dir::setCurrent(m_temporaryDir.getPath()));
      
      // For BUG_4796
      ASSERT_TRUE(Dir(Latin1String("test-XXXXXX")).exists() || Dir().mkdir(Latin1String("test-XXXXXX")));
      CoreApplication::setAppName(Latin1String("test_temporaryfile"));
   }
   
   // Tears down the test fixture.
   void TearDown()
   {
      Dir::setCurrent(m_previousCurrent);
   }
private:
   TemporaryDir m_temporaryDir;
   String m_previousCurrent;
};

} // anonymous namespace

TEST_F(TemporaryFileTest, testConstruction)
{
   TemporaryFile file(0);
   String tmp = Dir::getTempPath();
   ASSERT_EQ(file.getFileTemplate().left(tmp.size()), tmp);
   ASSERT_EQ(file.getFileTemplate().at(tmp.size()), Character('/'));
}

// Testing get/set functions
TEST_F(TemporaryFileTest, testGetSetCheck)
{
   TemporaryFile obj1;
   // bool TemporaryFile::autoRemove()
   // void TemporaryFile::setAutoRemove(bool)
   obj1.setAutoRemove(false);
   ASSERT_EQ(false, obj1.getAutoRemove());
   obj1.setAutoRemove(true);
   ASSERT_EQ(true, obj1.getAutoRemove());
}

namespace {

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

void init_file_template_data(std::list<std::tuple<String, String, String, String>> &data)
{
   data.push_back(std::make_tuple(Latin1String(""), Latin1String("."), Latin1String(""), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXXxxx"), Latin1String("pdk_"), Latin1String("xxx"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXXxXx"), Latin1String("pdk_"), Latin1String("xXx"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXX"), Latin1String("pdk_"), Latin1String(""), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXXXXXXxxx"), Latin1String("pdk_"), Latin1String("xxx"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXXXXXX"), Latin1String("pdk_"), Latin1String(""), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXX_XXXX"), Latin1String("pdk_"), Latin1String("_XXXX"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXXX_XXXXX"), Latin1String("pdk_"), Latin1String("_XXXXX"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXX"), Latin1String("pdk_XXXX"), Latin1String(""), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXX"), Latin1String("pdk_XXXXX."), Latin1String(""), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXX_XXXXXX_XXXX"), Latin1String("pdk_XXXX_"), Latin1String("_XXXX"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String("pdk_XXXXX_XXXXXX_XXXXX"), Latin1String("pdk_XXXXX_"), Latin1String("_XXXXX"), Latin1String("")));
   data.push_back(std::make_tuple(Latin1String(""), Latin1String("foo"), Latin1String(""), Latin1String("foo")));
   
   data.push_back(std::make_tuple(Latin1String(""), Latin1String("pdk_"), Latin1String("xxxxxx"), Latin1String("pdk_XXXXXXxxxxxx")));
   data.push_back(std::make_tuple(Latin1String(""), Latin1String("pdk_"), Latin1String(".xxx"), Latin1String("pdk_XXXXXX.xxx")));
   data.push_back(std::make_tuple(Latin1String(""), Latin1String("pdk_"), Latin1String(".xxx"), Latin1String("pdk_XXXXXXXXXXXXXX.xxx")));
   data.push_back(std::make_tuple(Latin1String(""), Latin1String("pdk_"), Latin1String(""), Latin1String("pdk_XXXXXXXXXXXXXX")));
   data.push_back(std::make_tuple(Latin1String(""), Latin1String("."), Latin1String(""), Latin1String("")));
   
   // Test Umlauts (contained in Latin1)
   String prefix = Latin1String("pdk_") + umlaut_test_text();
   data.push_back(std::make_tuple((prefix + Latin1String("XXXXXX")),  prefix, Latin1String(""), Latin1String("")));
   // Test Chinese
   prefix = Latin1String("pdk_") + han_test_text();
   data.push_back(std::make_tuple((prefix + Latin1String("XXXXXX")),  prefix, Latin1String(""), Latin1String("")));
}

} // anonymous namespace

TEST_F(TemporaryFileTest, testFileTemplate)
{
   std::list<std::tuple<String, String, String, String>> data;
   init_file_template_data(data);
   for (auto &item : data) {
      String &constructorTemplate = std::get<0>(item);
      String &prefix = std::get<1>(item);
      String &suffix = std::get<2>(item);
      String &fileTemplate = std::get<3>(item);
      
      TemporaryFile file(constructorTemplate);
      if (!fileTemplate.isEmpty()) {
         file.setFileTemplate(fileTemplate);
      }
      
      ASSERT_EQ(file.open(), true);
      
      String fileName = FileInfo(file).getFileName();
      if (prefix.length()) {
         ASSERT_EQ(fileName.left(prefix.length()), prefix);
      }
      if (suffix.length()) {
         ASSERT_EQ(fileName.right(suffix.length()), suffix);
      }
   }
}

TEST_F(TemporaryFileTest, testFileName)
{
   // Get Dir::tempPath and make an absolute path.
   String tempPath = Dir::getTempPath();
   String absoluteTempPath = Dir(tempPath).getAbsolutePath();
   TemporaryFile file;
   file.setAutoRemove(true);
   file.open();
   String fileName = file.getFileName();
   ASSERT_TRUE(fileName.contains(Latin1String("/test_temporaryfile."))) << pdk_printable(fileName);
   ASSERT_TRUE(File::exists(fileName));
   // Get path to the temp file, without the file name.
   String absoluteFilePath = FileInfo(fileName).getAbsolutePath();
#if defined(PDK_OS_WIN)
   absoluteFilePath = absoluteFilePath.toLower();
   absoluteTempPath = absoluteTempPath.toLower();
#endif
   ASSERT_EQ(absoluteFilePath, absoluteTempPath);
}

TEST_F(TemporaryFileTest, testFileNameIsEmpty)
{
   String filename;
   {
      TemporaryFile file;
      ASSERT_TRUE(file.getFileName().isEmpty());
      
      ASSERT_TRUE(file.open());
      ASSERT_TRUE(!file.getFileName().isEmpty());
      
      filename = file.getFileName();
      ASSERT_TRUE(File::exists(filename));
      
      file.close();
      ASSERT_TRUE(!file.isOpen());
      ASSERT_TRUE(File::exists(filename));
      ASSERT_TRUE(!file.getFileName().isEmpty());
   }
   ASSERT_TRUE(!File::exists(filename));
}

TEST_F(TemporaryFileTest, testAutoRemove)
{
   // Test auto remove
   String fileName;
   {
      TemporaryFile file(Latin1String("tempXXXXXX"));
      file.setAutoRemove(true);
      ASSERT_TRUE(file.open());
      fileName = file.getFileName();
      file.close();
   }
   ASSERT_TRUE(!fileName.isEmpty());
   ASSERT_TRUE(!File::exists(fileName));
   
   // same, but gets the file name after closing
   {
      TemporaryFile file(Latin1String("tempXXXXXX"));
      file.setAutoRemove(true);
      ASSERT_TRUE(file.open());
      file.close();
      fileName = file.getFileName();
   }
   ASSERT_TRUE(!fileName.isEmpty());
   ASSERT_TRUE(!File::exists(fileName));
   
   // Test if disabling auto remove works.
   {
      TemporaryFile file(Latin1String("tempXXXXXX"));
      file.setAutoRemove(false);
      ASSERT_TRUE(file.open());
      fileName = file.getFileName();
      file.close();
   }
   ASSERT_TRUE(!fileName.isEmpty());
   ASSERT_TRUE(File::exists(fileName));
   ASSERT_TRUE(File::remove(fileName));
   
   // same, but gets the file name after closing
   {
      TemporaryFile file(Latin1String("tempXXXXXX"));
      file.setAutoRemove(false);
      ASSERT_TRUE(file.open());
      file.close();
      fileName = file.getFileName();
   }
   ASSERT_TRUE(!fileName.isEmpty());
   ASSERT_TRUE(File::exists(fileName));
   ASSERT_TRUE(File::remove(fileName));
   
   // Do not explicitly call setAutoRemove (tests if it really is the default as documented)
   {
      TemporaryFile file(Latin1String("tempXXXXXX"));
      ASSERT_TRUE(file.open());
      fileName = file.getFileName();
      // BUG-39976, file mappings should be cleared as well.
      ASSERT_TRUE(file.write("test"));
      ASSERT_TRUE(file.flush());
      uchar *mapped = file.map(0, file.getSize());
      ASSERT_TRUE(mapped);
      file.close();
   }
   ASSERT_TRUE(!File::exists(fileName));
}

namespace {

struct ChdirOnReturn
{
   ChdirOnReturn(const String& dir)
      : m_dir(dir) 
   {}
   
   ~ChdirOnReturn()
   {
      Dir::setCurrent(m_dir);
   }
   
   String m_dir;
};

} // anonymous namespace

TEST_F(TemporaryFileTest, testNonWritableCurrentDir)
{
#ifdef PDK_OS_UNIX
   if (::geteuid() == 0) {
      std::cout << "not valid running this test as root" << std::endl;
   }
   ChdirOnReturn cor(Dir::getCurrentPath());
   Dir::setCurrent(Latin1String("/home"));
   
   // TemporaryFile("tempXXXXXX") is probably a bad idea in any app
   // where the current dir could anything...
   TemporaryFile file(Latin1String("tempXXXXXX"));
   file.setAutoRemove(true);
   ASSERT_TRUE(!file.open());
   ASSERT_TRUE(file.getFileName().isEmpty());
#endif
}

TEST_F(TemporaryFileTest, testIO)
{
   ByteArray data("OLE\nOLE\nOLE");
   TemporaryFile file;
   DateTime before = DateTime::getCurrentDateTimeUtc().addMSecs(-250);
   
   // discard msec component (round down) - not all FSs and OSs support them
   before.setSecsSinceEpoch(before.toSecsSinceEpoch());
   
   ASSERT_TRUE(file.open());
   ASSERT_TRUE(file.getSymLinkTarget().isEmpty()); // it's not a link!
   File::Permissions perm = file.getPermissions();
   ASSERT_TRUE(static_cast<bool>(perm & File::Permission::ReadOwner));
   ASSERT_TRUE(file.setPermissions(perm));
   
   ASSERT_EQ(int(file.getSize()), 0);
   ASSERT_TRUE(file.resize(data.size()));
   ASSERT_EQ(int(file.getSize()), data.size());
   ASSERT_EQ((int)file.write(data), data.size());
   ASSERT_EQ(int(file.getSize()), data.size());
   
   DateTime mtime = file.fileTime(File::FileTime::FileModificationTime).toUTC();
   DateTime btime = file.fileTime(File::FileTime::FileBirthTime).toUTC();
   DateTime ctime = file.fileTime(File::FileTime::FileMetadataChangeTime).toUTC();
   DateTime atime = file.fileTime(File::FileTime::FileAccessTime).toUTC();
   
   DateTime after = DateTime::getCurrentDateTimeUtc().toUTC().addMSecs(250);
   // round msecs up
   after.setSecsSinceEpoch(after.toSecsSinceEpoch() + 1);
   
   // mtime must be valid, the rest could fail
   ASSERT_TRUE(mtime <= after && mtime >= before);
   ASSERT_TRUE(!btime.isValid() || (btime <= after && btime >= before));
   ASSERT_TRUE(!ctime.isValid() || (ctime <= after && ctime >= before));
   ASSERT_TRUE(!btime.isValid() || (btime <= after && btime >= before));
   
   ASSERT_TRUE(file.setFileTime(before.addSecs(-10), File::FileTime::FileModificationTime));
   mtime = file.fileTime(File::FileTime::FileModificationTime).toUTC();
   ASSERT_EQ(mtime, before.addSecs(-10));
   
   file.reset();
   File compare(file.getFileName());
   compare.open(IoDevice::OpenMode::ReadOnly);
   ASSERT_EQ(compare.readAll() , data);
   ASSERT_EQ(compare.fileTime(File::FileTime::FileModificationTime), mtime);
}

TEST_F(TemporaryFileTest, testOpenCloseOpenClose)
{
   String fileName;
   {
      // Create a temp file
      TemporaryFile file(Latin1String("tempXXXXXX"));
      file.setAutoRemove(true);
      ASSERT_TRUE(file.open());
      file.write("OLE");
      fileName = file.getFileName();
      ASSERT_TRUE(File::exists(fileName));
      file.close();
      
      // Check that it still exists after being closed
      ASSERT_TRUE(File::exists(fileName));
      ASSERT_TRUE(!file.isOpen());
      ASSERT_TRUE(file.open());
      ASSERT_EQ(file.readAll(), ByteArray("OLE"));
      // Check that it's still the same file after being opened again.
      ASSERT_EQ(file.getFileName(), fileName);
   }
   ASSERT_TRUE(!File::exists(fileName));
}

TEST_F(TemporaryFileTest, testRemoveAndReOpen)
{
   String fileName;
   {
      TemporaryFile file;
      file.open();
      fileName = file.getFileName();     // materializes any unnamed file
      ASSERT_TRUE(File::exists(fileName));
      
      ASSERT_TRUE(file.remove());
      ASSERT_TRUE(file.getFileName().isEmpty());
      ASSERT_TRUE(!File::exists(fileName));
      ASSERT_TRUE(!file.remove());
      
      ASSERT_TRUE(file.open());
      ASSERT_EQ(FileInfo(file.getFileName()).getPath(), FileInfo(fileName).getPath());
      fileName = file.getFileName();
      ASSERT_TRUE(File::exists(fileName));
   }
   ASSERT_TRUE(!File::exists(fileName));
}

TEST_F(TemporaryFileTest, testRemoveUnnamed)
{
   TemporaryFile file;
   file.open();
   
   // we did not call fileName(), so the file name may not have a name
   ASSERT_TRUE(file.remove());
   ASSERT_TRUE(file.getFileName().isEmpty());
   
   // if it was unnamed, this will succeed again, so we can't check the result
   file.remove();
}

TEST_F(TemporaryFileTest, testSize)
{
   TemporaryFile file;
   ASSERT_TRUE(file.open());
   ASSERT_TRUE(!file.isSequential());
   ByteArray str("foobar");
   file.write(str);
   
   // On CE it takes more time for the filesystem to update
   // the information. Usually you have to close it or seek
   // to get latest information. flush() does not help either.
   ASSERT_EQ(file.getSize(), pdk::pint64(6));
   file.seek(0);
   ASSERT_EQ(file.getSize(), pdk::pint64(6));
   
   ASSERT_TRUE(File::exists(file.getFileName()));
   ASSERT_TRUE(file.exists());
}

TEST_F(TemporaryFileTest, testResize)
{
   TemporaryFile file;
   file.setAutoRemove(true);
   ASSERT_TRUE(file.open());
   ASSERT_TRUE(file.resize(100));
   
   ASSERT_EQ(FileInfo(file.getFileName()).getSize(), pdk::pint64(100));
   
   file.close();
}

TEST_F(TemporaryFileTest, testOpenOnRootDrives)
{
#if defined(PDK_OS_WIN)
   unsigned int lastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
   // If it's possible to create a file in the root directory, it
   // must be possible to create a temp file there too.
   for (FileInfo driveInfo : Dir::getDrives()) {
      File testFile(driveInfo.getFilePath() + Latin1String("XXXXXX.txt"));
      if (testFile.open(IoDevice::OpenMode::ReadWrite)) {
         testFile.remove();
         TemporaryFile file(driveInfo.getFilePath() + Latin1String("XXXXXX.txt"));
         file.setAutoRemove(true);
         ASSERT_TRUE(file.open());
      }
   }
#if defined(PDK_OS_WIN)
   SetErrorMode(lastErrorMode);
#endif
}

TEST_F(TemporaryFileTest, testStressTest)
{
   const int iterations = 1000;
   std::set<String> names;
   for (int i = 0; i < iterations; ++i) {
      TemporaryFile file;
      file.setAutoRemove(false);
      ASSERT_TRUE(file.open()) << pdk_printable(file.getErrorString());
      ASSERT_TRUE(names.find(file.getFileName()) == names.end());
      names.insert(file.getFileName());
   }
   for (std::set<String>::const_iterator iter = names.cbegin(); iter != names.cend(); ++iter) {
      File::remove(*iter);
   }
}

TEST_F(TemporaryFileTest, testRename)
{
   // This test checks that the temporary file is deleted, even after a
   // rename.
   Dir dir;
   ASSERT_TRUE(!dir.exists(Latin1String("temporary-file.txt")));
   String tempname;
   {
      TemporaryFile file(dir.getFilePath(Latin1String()));
      ASSERT_TRUE(file.open());
      tempname = file.getFileName();
      ASSERT_TRUE(dir.exists(tempname));
      ASSERT_TRUE(file.rename(Latin1String("temporary-file.txt")));
      ASSERT_TRUE(!dir.exists(tempname));
      ASSERT_TRUE(dir.exists(Latin1String("temporary-file.txt")));
      ASSERT_EQ(file.getFileName(), String(Latin1String("temporary-file.txt")));
   }
   
   ASSERT_TRUE(!dir.exists(tempname));
   ASSERT_TRUE(!dir.exists(Latin1String("temporary-file.txt")));
}

TEST_F(TemporaryFileTest, testRenameFdLeak)
{
#ifdef PDK_OS_UNIX
   const ByteArray sourceFile = File::encodeName(Latin1String(__FILE__));
   ASSERT_TRUE(!sourceFile.isEmpty());
   // Test this on Unix only
   
   // Open a bunch of files to force the fd count to go up
   static const int count = 10;
   int bunch_of_files[count];
   for (int i = 0; i < count; ++i) {
      bunch_of_files[i] = ::open(sourceFile.getConstRawData(), O_RDONLY);
      ASSERT_TRUE(bunch_of_files[i] != -1);
   }
   
   int fd;
   {
      TemporaryFile file;
      file.setAutoRemove(false);
      ASSERT_TRUE(file.open());
      
      // close the bunch of files
      for (int i = 0; i < count; ++i)
         ::close(bunch_of_files[i]);
      
      // save the file descriptor for later
      fd = file.getHandle();
      
      // rename the file to something
      String newPath = Dir::getTempPath() + Latin1String("/test_temporary_file-renameFdLeak-") + String::number(getpid());
      file.rename(newPath);
      File::remove(newPath);
   }
   
   // check if TemporaryFile closed the file
   ASSERT_TRUE(::close(fd) == -1 && errno == EBADF);
#endif
}

TEST_F(TemporaryFileTest, testReOpenThroughFile)
{
   ByteArray data("abcdefghij");
   
   TemporaryFile file;
   ASSERT_TRUE(((File &)file).open(IoDevice::OpenMode::WriteOnly));
   ASSERT_EQ(file.write(data), (pdk::pint64)data.size());
   
   file.close();
   ASSERT_TRUE(file.open());
   ASSERT_EQ(file.readAll(), data);
}

TEST_F(TemporaryFileTest, testKeepOpenMode)
{
   ByteArray data("abcdefghij");
   
   {
      TemporaryFile file;
      ASSERT_TRUE(((File &)file).open(IoDevice::OpenMode::WriteOnly));
      ASSERT_TRUE(static_cast<bool>(file.getOpenMode() & IoDevice::OpenMode::WriteOnly));
      
      ASSERT_EQ(file.write(data), (pdk::pint64)data.size());
      file.close();
      
      ASSERT_TRUE(((File &)file).open(IoDevice::OpenMode::ReadOnly));
      ASSERT_TRUE(static_cast<bool>(file.getOpenMode() & IoDevice::OpenMode::ReadOnly));
      ASSERT_EQ(file.readAll(), data);
   }
   
   {
      TemporaryFile file;
      ASSERT_TRUE(file.open());
      ASSERT_EQ(file.getOpenMode(), IoDevice::OpenMode::ReadWrite);
      ASSERT_EQ(file.write(data), (pdk::pint64)data.size());
      ASSERT_TRUE(file.rename(Latin1String("temporary-file.txt")));
      
      ASSERT_TRUE(((File &)file).open(IoDevice::OpenMode::ReadOnly));
      ASSERT_TRUE(static_cast<bool>(file.getOpenMode() & IoDevice::OpenMode::ReadOnly));
      ASSERT_EQ(file.readAll(), data);
      
      ASSERT_TRUE(((File &)file).open(IoDevice::OpenMode::WriteOnly));
      ASSERT_EQ(file.getOpenMode(), IoDevice::OpenMode::WriteOnly);
   }
}

TEST_F(TemporaryFileTest, testResetTemplateAfterError)
{
   // calling setFileTemplate on a failed open
   String tempPath = Dir::getTempPath();
   const String fileTemplate(Latin1String("destination/pdk_temp_file_test.XXXXXX"));
   const String fileTemplate2(tempPath + Latin1String("/pdk_temp_file_test.XXXXXX"));
   ASSERT_TRUE(Dir(tempPath).exists() || Dir().mkpath(tempPath)) << "Test precondition";
   ASSERT_TRUE(!File::exists(Latin1String("destination"))) << "Test precondition";
   ASSERT_TRUE(!File::exists(fileTemplate2) || File::remove(fileTemplate2)) << "Test precondition";
   File file(fileTemplate2);
   ByteArray fileContent("This file is intentionally NOT left empty.");
   ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadWrite | IoDevice::OpenMode::Truncate));
   ASSERT_EQ(file.write(fileContent), (pdk::pint64)fileContent.size());
   ASSERT_TRUE(file.flush());
   
   String fileName;
   {
      TemporaryFile temp;
      ASSERT_TRUE(temp.getFileName().isEmpty());
      ASSERT_TRUE(!temp.getFileTemplate().isEmpty());
      temp.setFileTemplate(fileTemplate);
      ASSERT_TRUE(temp.getFileName().isEmpty());
      ASSERT_EQ(temp.getFileTemplate(), fileTemplate);
      ASSERT_TRUE(!temp.open());
      ASSERT_TRUE(temp.getFileName().isEmpty());
      ASSERT_EQ(temp.getFileTemplate(), fileTemplate);
      temp.setFileTemplate(fileTemplate2);
      ASSERT_TRUE(temp.open());
      fileName = temp.getFileName();
      ASSERT_TRUE(File::exists(fileName));
      ASSERT_TRUE(!fileName.isEmpty());
      ASSERT_TRUE(fileName != fileTemplate2) << (Latin1String("Generated name shouldn't be same as template: ") + fileTemplate2).toLocal8Bit().getConstRawData();
   }
   
   ASSERT_TRUE(!File::exists(fileName));
   
   file.seek(0);
   ASSERT_EQ(String(Latin1String(file.readAll())), String(Latin1String(fileContent)));
   ASSERT_TRUE(file.remove());
}

TEST_F(TemporaryFileTest, testSetTemplateAfterOpen)
{
   TemporaryFile temp;
   ASSERT_TRUE(temp.getFileName().isEmpty());
   ASSERT_TRUE(!temp.getFileTemplate().isEmpty());
   ASSERT_TRUE(temp.open());
   const String fileName = temp.getFileName();
   const String newTemplate(Latin1String("funny-path/funny-name-XXXXXX.tmp"));
   ASSERT_TRUE(!fileName.isEmpty());
   ASSERT_TRUE(File::exists(fileName));
   ASSERT_TRUE(!temp.getFileTemplate().isEmpty());
   ASSERT_TRUE(temp.getFileTemplate() != newTemplate);
   
   temp.close(); // TemporaryFile::setFileTemplate will assert on isOpen() up to 4.5.2
   temp.setFileTemplate(newTemplate);
   ASSERT_EQ(temp.getFileTemplate(), newTemplate);
   
   ASSERT_TRUE(temp.open());
   ASSERT_EQ(temp.getFileName(), fileName);
   ASSERT_EQ(temp.getFileTemplate(), newTemplate);
}

TEST_F(TemporaryFileTest, testAutoRemoveAfterFailedRename)
{
   struct CleanOnReturn
   {
      ~CleanOnReturn()
      {
         if (!tempName.isEmpty())
            File::remove(tempName);
      }
      
      void reset()
      {
         tempName.clear();
      }
      
      String tempName;
   };
   
   CleanOnReturn cleaner;
   
   {
      TemporaryFile file;
      ASSERT_TRUE(file.open());
      cleaner.tempName = file.getFileName();
      
      ASSERT_TRUE(File::exists(cleaner.tempName));
      ASSERT_TRUE(!FileInfo(Latin1String("i-do-not-exist")).isDir());
      ASSERT_TRUE(!file.rename(Latin1String("i-do-not-exist/file.txt")));
      ASSERT_TRUE(File::exists(cleaner.tempName));
   }
   
   ASSERT_TRUE(!File::exists(cleaner.tempName));
   cleaner.reset();
}

void init_create_nativefile_data(std::list<std::tuple<String, pdk::pint64, bool, ByteArray>> &data)
{
   const String nativeFilePath = PDKTEST_FINDTEST_SRC_DATA("resources/test.txt");
   // File might not exist locally in case of sandboxing or remote testing
   if (!nativeFilePath.startsWith(Latin1String(":/"))) {
      data.push_back(std::make_tuple(nativeFilePath, (pdk::pint64)-1, false, ByteArray()));
      data.push_back(std::make_tuple(nativeFilePath, (pdk::pint64)5, false, ByteArray()));
   }
   // @TODO add resources testcases
}

TEST_F(TemporaryFileTest, testCreateNativeFile)
{
   std::list<std::tuple<String, pdk::pint64, bool, ByteArray>> data;
   init_create_nativefile_data(data);
   for (auto &item : data) {
      String &filePath = std::get<0>(item);
      pdk::pint64 currentPos = std::get<1>(item);
      bool valid = std::get<2>(item);
      ByteArray &content = std::get<3>(item);
      File file(filePath);
      if (currentPos != -1) {
         file.open(IoDevice::OpenMode::ReadOnly);
         file.seek(currentPos);
      }
      TemporaryFile *tempFile = TemporaryFile::createNativeFile(file);
      ASSERT_EQ(valid, (bool)tempFile);
      if (currentPos != -1)
         ASSERT_EQ(currentPos, file.getPosition());
      if (valid) {
         ASSERT_EQ(content, tempFile->readAll());
         delete tempFile;
      }
   }
}

void init_bug_4796_data(std::list<std::tuple<String, String, bool>> &data)
{
   String unicode = String::fromUtf8("\xc3\xa5\xc3\xa6\xc3\xb8");
   data.push_back(std::make_tuple(String(), String(), true));
   data.push_back(std::make_tuple(String(Latin1String(".")), String(), true));
   data.push_back(std::make_tuple(String(Latin1String("..")), String(), true));
   data.push_back(std::make_tuple(String(Latin1String("bla")), String(), true));
   data.push_back(std::make_tuple(String(), String(Latin1String("bla")), true));
   data.push_back(std::make_tuple(String(Latin1String("does-not-exist/pdk_temp")), String(), false));
   
   data.push_back(std::make_tuple(String(), unicode, true));
   data.push_back(std::make_tuple(unicode, String(), true));
   data.push_back(std::make_tuple(unicode, unicode, true));
}

TEST_F(TemporaryFileTest, test_bug_4796_data)
{
   ASSERT_TRUE(Dir(Latin1String("test-XXXXXX")).exists());
   struct CleanOnReturn
   {
      ~CleanOnReturn()
      {
         for(String &tempName : m_tempNames) {
            File::remove(tempName);
         }
      }
      
      void reset()
      {
         m_tempNames.clear();
      }
      
      StringList m_tempNames;
   };
   
   CleanOnReturn cleaner;
   
   std::list<std::tuple<String, String, bool>> data;
   init_bug_4796_data(data);
   for (auto &item : data) {
      String &prefix = std::get<0>(item);
      String &suffix = std::get<1>(item);
      bool openResult = std::get<2>(item);
      String fileTemplate1 = prefix + String(Latin1String("XX")) + suffix;
      String fileTemplate2 = prefix + String(Latin1String("XXXX")) + suffix;
      String fileTemplate3 = prefix + String(Latin1String("XXXXXX")) + suffix;
      String fileTemplate4 = prefix + String(Latin1String("XXXXXXXX")) + suffix;
      
      TemporaryFile file1(fileTemplate1);
      TemporaryFile file2(fileTemplate2);
      TemporaryFile file3(fileTemplate3);
      TemporaryFile file4(fileTemplate4);
      TemporaryFile file5(Latin1String("test-XXXXXX/") + fileTemplate1);
      TemporaryFile file6(Latin1String("test-XXXXXX/") + fileTemplate3);
      
      ASSERT_EQ(file1.open(), openResult);
      ASSERT_EQ(file2.open(), openResult);
      ASSERT_EQ(file3.open(), openResult);
      ASSERT_EQ(file4.open(), openResult);
      ASSERT_EQ(file5.open(), openResult);
      ASSERT_EQ(file6.open(), openResult);
      
      // force the files to exist, if they are supposed to
      ASSERT_EQ(!file1.getFileName().isEmpty(), openResult);
      ASSERT_EQ(!file2.getFileName().isEmpty(), openResult);
      ASSERT_EQ(!file3.getFileName().isEmpty(), openResult);
      ASSERT_EQ(!file4.getFileName().isEmpty(), openResult);
      ASSERT_EQ(!file5.getFileName().isEmpty(), openResult);
      ASSERT_EQ(!file6.getFileName().isEmpty(), openResult);
      
      ASSERT_EQ(file1.exists(), openResult);
      ASSERT_EQ(file2.exists(), openResult);
      ASSERT_EQ(file3.exists(), openResult);
      ASSERT_EQ(file4.exists(), openResult);
      ASSERT_EQ(file5.exists(), openResult);
      ASSERT_EQ(file6.exists(), openResult);
      
      // make sure the file exists under the *correct* name
      if (openResult) {
         cleaner.m_tempNames << file1.getFileName()
                             << file2.getFileName()
                             << file3.getFileName()
                             << file4.getFileName()
                             << file5.getFileName()
                             << file6.getFileName();
         
         Dir currentDir;
         String fileName1 = currentDir.getRelativeFilePath(file1.getFileName());
         String fileName2 = currentDir.getRelativeFilePath(file2.getFileName());
         String fileName3 = currentDir.getRelativeFilePath(file3.getFileName());
         String fileName4 = currentDir.getRelativeFilePath(file4.getFileName());
         String fileName5 = currentDir.getRelativeFilePath(file5.getFileName());
         String fileName6 = currentDir.getRelativeFilePath(file6.getFileName());
         
         ASSERT_TRUE(fileName1.startsWith(fileTemplate1 + Latin1Character('.')));
         ASSERT_TRUE(fileName2.startsWith(fileTemplate2 + Latin1Character('.')));
         ASSERT_TRUE(fileName5.startsWith(Latin1String("test-XXXXXX/") + fileTemplate1 + Latin1Character('.')));
         ASSERT_TRUE(fileName6.startsWith(Latin1String("test-XXXXXX/") + prefix));
         
         if (!prefix.isEmpty()) {
            ASSERT_TRUE(fileName3.startsWith(prefix));
            ASSERT_TRUE(fileName4.startsWith(prefix));
         }
         
         if (!suffix.isEmpty()) {
            ASSERT_TRUE(fileName3.endsWith(suffix));
            ASSERT_TRUE(fileName4.endsWith(suffix));
            ASSERT_TRUE(fileName6.endsWith(suffix));
         }
      }
   }
   
   for(String const &tempName : cleaner.m_tempNames) {
      ASSERT_TRUE(!File::exists(tempName));
   }
   cleaner.reset();
}

TEST_F(TemporaryFileTest, testGuaranteeUnique)
{
   Dir dir(Dir::getTempPath());
   String takenFileName;
   
   // First pass. See which filename TemporaryFile will try first.
   {
      TemporaryFile tmpFile(Latin1String("testFile1.XXXXXX"));
      tmpFile.open();
      takenFileName = tmpFile.getFileName();
      ASSERT_TRUE(File::exists(takenFileName));
   }
   
   ASSERT_TRUE(!File::exists(takenFileName));
   
   // Create a directory with same name.
   ASSERT_TRUE(dir.mkdir(takenFileName));
   
   // Second pass, now we have blocked its first attempt with a directory.
   {
      TemporaryFile tmpFile(Latin1String("testFile1.XXXXXX"));
      ASSERT_TRUE(tmpFile.open());
      String uniqueFileName = tmpFile.getFileName();
      ASSERT_TRUE(FileInfo(uniqueFileName).isFile());
      ASSERT_TRUE(uniqueFileName != takenFileName);
   }
   
   ASSERT_TRUE(dir.rmdir(takenFileName));
}
