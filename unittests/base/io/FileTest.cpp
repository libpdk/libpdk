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
#include "pdk/global/PlatformDefs.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/TemporaryDir.h"
#include "pdktest/PdkTest.h"
#include "pdk/base/time/DateTime.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/time/Date.h"

#include "pdk/base/io/fs/internal/AbstractFileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileEnginePrivate.h"
#include "pdk/base/io/fs/internal/FileSystemEnginePrivate.h"

#ifdef PDK_OS_WIN
# include "pdk/global/Windows.h"
#else
# include <sys/types.h>
# include <unistd.h>
#endif

#ifdef PDK_OS_MAC
# include <sys/mount.h>
#elif defined(PDK_OS_LINUX)
# include <sys/vfs.h>
#elif defined(PDK_OS_FREEBSD)
# include <sys/param.h>
# include <sys/mount.h>
#elif defined(PDK_OS_IRIX)
# include <sys/statfs.h>
#elif defined(PDK_OS_VXWORKS)
# include <fcntl.h>
#if defined(_WRS_KERNEL)
#  undef PDK_OPEN
#  define PDK_OPEN(path, oflag) ::open(path, oflag, 0)
#endif
#endif

#include <stdio.h>
#include <errno.h>

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifndef PDK_OPEN_BINARY
#define PDK_OPEN_BINARY 0
#endif

#define PDKTEST_DIR_SEP "/"
#define PDKTEST_FILETEST_SUBDIR "filetestdir"
#define PDKTEST_FINDTEST_SRC_DATA(filename) Latin1String(PDKTEST_CURRENT_TEST_SOURCE_DIR PDKTEST_DIR_SEP PDKTEST_FILETEST_SUBDIR PDKTEST_DIR_SEP filename)

using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Latin1Character;
using pdk::ds::ByteArray;
using pdk::io::IoDevice;
using pdk::io::fs::Dir;
using pdk::time::DateTime;
using pdk::time::Date;
using pdk::time::Time;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::ds::StringList;
using pdk::io::fs::TemporaryDir;
using pdk::io::Debug;
using pdk::kernel::CoreApplication;
using pdk::io::fs::internal::AbstractFileEngine;
using pdk::io::fs::internal::AbstractFileEngineHandler;
using pdk::io::fs::internal::FileEngine;

namespace {

class StdioFileGuard
{
   PDK_DISABLE_COPY(StdioFileGuard);
public:
   explicit StdioFileGuard(FILE *f = nullptr)
      : m_file(f)
   {}
   
   ~StdioFileGuard()
   {
      close();
   }
   
   operator FILE *() const
   {
      return m_file;
   }
   
   void close();
private:
   FILE * m_file;
};

void StdioFileGuard::close()
{
   if (m_file != nullptr) {
      fclose(m_file);
      m_file = nullptr;
   }
}

static String sg_currentSourceDir(Latin1String(PDKTEST_CURRENT_TEST_SOURCE_DIR));
static String sg_currentBinaryDir(Latin1String(PDKTEST_CURRENT_TEST_DIR));
static const char sg_noReadFile[] = "noreadfile";
static const char sg_readOnlyFile[] = "readonlyfile";
static String sg_testFile = PDKTEST_FINDTEST_SRC_DATA("testfile.txt");
static String sg_dosFile = PDKTEST_FINDTEST_SRC_DATA("dosfile.txt");
static String sg_testSourceFile = Latin1String(PDKTEST_CURRENT_TEST_SOURCE_DIR PDKTEST_DIR_SEP "FileTest.cpp");

ByteArray msg_open_failed(IoDevice::OpenMode om, const File &file)
{
   String result;
   Debug(&result).noquote().nospace() << "Could not open \""
                                      << Dir::toNativeSeparators(file.getFileName()) << "\" using "
                                      << pdk::as_integer<IoDevice::OpenMode>(om) << ": " << file.getErrorString();
   return result.toLocal8Bit();
}

ByteArray msg_open_failed(const File &file)
{
   return (Latin1String("Could not open \"") + Dir::toNativeSeparators(file.getFileName())
           + Latin1String("\": ") + file.getErrorString()).toLocal8Bit();
}

ByteArray msg_file_does_not_exist(const String &name)
{
   return (Latin1Character('"') + Dir::toNativeSeparators(name)
           + Latin1String("\" does not exist.")).toLocal8Bit();
}

class FileTest : public ::testing::Test
{
public:
   enum FileType {
      OpenFile,
      OpenFd,
      OpenStream,
      NumberOfFileTypes
   };
   
   FileTest()
      : m_fd(-1),
        m_stream(0),
        m_oldDir(Dir::getCurrentPath())
   {
   }
   
   ~FileTest() {
      cleanup();
   }
   
   void cleanup()
   {
      if (-1 != m_fd) {
         PDK_CLOSE(m_fd);
      }
      m_fd = -1;
      if (m_stream) {
         ::fclose(m_stream);
      }
      m_stream = 0;
      // Windows UNC tests set a different working directory which might not be restored on failures.
      if (Dir(Dir::getCurrentPath()).getDirName() != Dir(m_temporaryDir.getPath()).getDirName()) {
         Dir::setCurrent(m_temporaryDir.getPath());
      }
      // Clean out everything except the readonly-files.
      const Dir dir(m_temporaryDir.getPath());
      for (const FileInfo &fi: dir.entryInfoList(Dir::Filters(Dir::Filter::AllEntries) | Dir::Filter::NoDotAndDotDot)) {
         const String fileName = fi.getFileName();
         if (fileName != Latin1String(sg_noReadFile) && fileName != Latin1String(sg_readOnlyFile)) {
            const String absoluteFilePath = fi.getAbsoluteFilePath();
            if (fi.isDir() && !fi.isSymLink()) {
               Dir remainingDir(absoluteFilePath);
               ASSERT_TRUE(remainingDir.removeRecursively()) << pdk_printable(absoluteFilePath);
            } else {
               if (!(File::getPermissions(absoluteFilePath) & File::Permission::WriteUser)) {
                  ASSERT_TRUE(File::setPermissions(absoluteFilePath, File::Permission::WriteUser)) << pdk_printable(absoluteFilePath);
               }
               ASSERT_TRUE(File::remove(absoluteFilePath)) << pdk_printable(absoluteFilePath);
            }
         }
      }
      if (Dir(Dir::getCurrentPath()).getDirName() == Dir(m_temporaryDir.getPath()).getDirName()) {
         Dir::setCurrent(m_oldDir);
      }
   }
   
   // Sets up the test fixture.
   void SetUp()
   {
      ASSERT_TRUE(m_temporaryDir.isValid()) << pdk_printable(m_temporaryDir.getErrorString());
      // @TODO add process support testcase
      m_testLogFile = PDKTEST_FINDTEST_SRC_DATA("testlog.txt");
      ASSERT_TRUE(!m_testLogFile.isEmpty());
      m_dosFile = PDKTEST_FINDTEST_SRC_DATA("dosfile.txt");
      ASSERT_TRUE(!m_dosFile.isEmpty());
      m_forCopyingFile = PDKTEST_FINDTEST_SRC_DATA("forCopying.txt");
      ASSERT_TRUE(!m_forCopyingFile .isEmpty());
      m_forRenamingFile = PDKTEST_FINDTEST_SRC_DATA("forRenaming.txt");
      ASSERT_TRUE(!m_forRenamingFile.isEmpty());
      m_twoDotsFile = PDKTEST_FINDTEST_SRC_DATA("two.dots.file");
      ASSERT_TRUE(!m_twoDotsFile.isEmpty());
      
      m_testSourceFile = sg_testSourceFile;
      ASSERT_TRUE(!m_testSourceFile.isEmpty());
      m_testFile = PDKTEST_FINDTEST_SRC_DATA("testfile.txt");
      ASSERT_TRUE(!m_testFile.isEmpty());
      m_resourcesDir = PDKTEST_FINDTEST_SRC_DATA("resources");
      ASSERT_TRUE(!m_resourcesDir.isEmpty());
      
      m_noEndOfLineFile = PDKTEST_FINDTEST_SRC_DATA("noendofline.txt");
      ASSERT_TRUE(!m_noEndOfLineFile.isEmpty());
      
      ASSERT_TRUE(Dir::setCurrent(m_temporaryDir.getPath()));
      
      // create a file and make it read-only
      File file(String::fromLatin1(sg_readOnlyFile));
      ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
      file.write("a", 1);
      file.close();
      ASSERT_TRUE(file.setPermissions(File::Permission::ReadOwner)) << pdk_printable(file.getErrorString());
      // create another file and make it not readable
      file.setFileName(String::fromLatin1(sg_noReadFile));
      ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
      file.write("b", 1);
      file.close();
#ifndef PDK_OS_WIN // Not supported on Windows.
      ASSERT_TRUE(file.setPermissions(0)) << pdk_printable(file.getErrorString());
#else
      ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
#endif
      
   }
   
   // Tears down the test fixture.
   void TearDown()
   {
      File file(String::fromLatin1(sg_readOnlyFile));
      file.setPermissions(File::Permissions(File::Permission::ReadOwner) | File::Permission::WriteOwner);
      ASSERT_TRUE(file.setPermissions(File::Permissions(File::Permission::ReadOwner) | File::Permission::WriteOwner));
      file.setFileName(String::fromLatin1(sg_noReadFile));
      file.setPermissions(File::Permissions(File::Permission::ReadOwner) | File::Permission::WriteOwner);
      ASSERT_TRUE(file.setPermissions(File::Permissions(File::Permission::ReadOwner) | File::Permission::WriteOwner));
      ASSERT_TRUE(Dir::setCurrent(m_oldDir)); //release test directory for removal
   }
   
public:
   bool openFd(File &file, IoDevice::OpenModes mode, File::FileHandleFlags handleFlags)
   {
      int fdMode = PDK_OPEN_LARGEFILE | PDK_OPEN_BINARY;
      // File will be truncated if in Write mode.
      if (mode & IoDevice::OpenMode::WriteOnly) {
         fdMode |= PDK_OPEN_WRONLY | PDK_OPEN_TRUNC;
      }
      
      if (mode & IoDevice::OpenMode::ReadOnly) {
         fdMode |= PDK_OPEN_RDONLY;
      }
      m_fd = PDK_OPEN(pdk_printable(file.getFileName()), fdMode);
      return (-1 != m_fd) && file.open(m_fd, mode, handleFlags);
   }
   
   bool openStream(File &file, IoDevice::OpenModes mode, File::FileHandleFlags handleFlags)
   {
      char const *streamMode = "";
      // File will be truncated if in Write mode.
      if (mode & IoDevice::OpenMode::WriteOnly) {
         streamMode = "wb+";
      }
      else if (mode & IoDevice::OpenMode::ReadOnly) {
         streamMode = "rb";
      }
      m_stream = PDK_FOPEN(pdk_printable(file.getFileName()), streamMode);
      return m_stream && file.open(m_stream, mode, handleFlags);
   }
   
   bool openFile(File &file, IoDevice::OpenModes mode, FileType type = OpenFile, File::FileHandleFlags handleFlags = File::FileHandleFlag::DontCloseHandle)
   {
      if (mode & IoDevice::OpenMode::WriteOnly && !file.exists())
      {
         // Make sure the file exists
         File createFile(file.getFileName());
         if (!createFile.open(IoDevice::OpenMode::ReadWrite)) {
            return false;
         }
         
      }
      // Note: openFd and openStream will truncate the file if write mode.
      switch (type)
      {
      case OpenFile:
         return file.open(mode);
         
      case OpenFd:
         return openFd(file, mode, handleFlags);
         
      case OpenStream:
         return openStream(file, mode, handleFlags);
         
      case NumberOfFileTypes:
         break;
      }
      return false;
   }
   
   void closeFile(File &file)
   {
      file.close();
      
      if (-1 != m_fd) {
         PDK_CLOSE(m_fd);
      }
      if (m_stream) {
         ::fclose(m_stream);
      }
      m_fd = -1;
      m_stream = 0;
   }
   
   int m_fd;
   FILE *m_stream;
   
   TemporaryDir m_temporaryDir;
   const String m_oldDir;
   String m_stdinProcessDir;
   String m_testSourceFile;
   String m_testLogFile;
   String m_dosFile;
   String m_forCopyingFile;
   String m_forRenamingFile;
   String m_twoDotsFile;
   String m_testFile;
   String m_resourcesDir;
   String m_noEndOfLineFile;
};

} // anonymous namespace


//------------------------------------------
// The 'testfile' is currently just a
// testfile. The path of this file, the
// attributes and the contents itself
// will be changed as far as we have a
// proper way to handle files in the
// testing environment.
//------------------------------------------
TEST_F(FileTest, testExists)
{
   File f(m_testFile);
   ASSERT_TRUE(f.exists()) << msg_file_does_not_exist(m_testFile).getConstRawData();
   
   File file(Latin1String("nobodyhassuchafile"));
   file.remove();
   ASSERT_TRUE(!file.exists());
   
   File file2(Latin1String("nobodyhassuchafile"));
   ASSERT_TRUE(file2.open(IoDevice::OpenMode::WriteOnly)) << msg_open_failed(file2).getConstRawData();
   file2.close();
   
   ASSERT_TRUE(file.exists());
   
   ASSERT_TRUE(file.open(IoDevice::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
   file.close();
   ASSERT_TRUE(file.exists());
   
   file.remove();
   ASSERT_TRUE(!file.exists());
   
   // @TODO Windows platform testcase
}

namespace {

void init_open_data(std::list<std::tuple<String, IoDevice::OpenMode, bool, File::FileError>> &data)
{
   data.push_back(std::make_tuple(PDKTEST_FINDTEST_SRC_DATA("testfile.txt"),
                                  IoDevice::OpenMode::ReadOnly, true, File::FileError::NoError));
}

} // anonymous namespace

TEST_F(FileTest, testOpen)
{
   std::list<std::tuple<String, IoDevice::OpenMode, bool, File::FileError>> data;
   init_open_data(data);
   for (auto &item : data) {
      String &filename = std::get<0>(item);
      File::OpenMode mode = std::get<1>(item);
      bool ok = std::get<2>(item);
      File::FileError status = std::get<3>(item);
      File f(filename);
#if defined(PDK_OS_UNIX) && !defined(PDK_OS_VXWORKS)
      if (::getuid() == 0) {
         // root and Chuck Norris don't care for file permissions. Skip.
         std::cout << "Running this test as root doesn't make sense";
         SUCCEED();
         return;
      }
      
#endif
      
#if defined(PDK_OS_WIN32)
      FAIL("noreadfile") << "Windows does not currently support non-readable files.";
      return;
#endif
      const IoDevice::OpenMode om(mode);
      const bool succeeded = f.open(om);
      if (ok) {
         ASSERT_TRUE(succeeded) << msg_open_failed(om, f).getConstRawData();
      } else {
         ASSERT_TRUE(!succeeded);
      }
      ASSERT_EQ(f.getError(), status);
   }
   ASSERT_TRUE(true);
}

TEST_F(FileTest, testOpenUnbuffered)
{
   File file(m_testFile);
   ASSERT_TRUE(file.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | IoDevice::OpenMode::Unbuffered)) << msg_open_failed(file).getConstRawData();
   char c = '\0';
   ASSERT_TRUE(file.seek(1));
   ASSERT_EQ(file.getPosition(), pdk::pint64(1));
   ASSERT_TRUE(file.getChar(&c));
   ASSERT_EQ(file.getPosition(), pdk::pint64(2));
   char d = '\0';
   ASSERT_TRUE(file.seek(3));
   ASSERT_EQ(file.getPosition(), pdk::pint64(3));
   ASSERT_TRUE(file.getChar(&d));
   ASSERT_EQ(file.getPosition(), pdk::pint64(4));
   ASSERT_TRUE(file.seek(1));
   ASSERT_EQ(file.getPosition(), pdk::pint64(1));
   char c2 = '\0';
   ASSERT_TRUE(file.getChar(&c2));
   ASSERT_EQ(file.getPosition(), pdk::pint64(2));
   ASSERT_TRUE(file.seek(3));
   ASSERT_EQ(file.getPosition(), pdk::pint64(3));
   char d2 = '\0';
   ASSERT_TRUE(file.getChar(&d2));
   ASSERT_EQ(file.getPosition(), pdk::pint64(4));
   ASSERT_EQ(c, c2);
   ASSERT_EQ(d, d2);
   ASSERT_EQ(c, '-');
   ASSERT_EQ(d, '-');
}

namespace {

void init_size_data(std::list<std::tuple<String, pdk::pint64>> &data, const String &testFile)
{
   data.push_back(std::make_tuple(testFile, (pdk::pint64)245));
#if defined(PDK_OS_WIN)
   // @TODO add windows testcase
#endif
}

} // anonymous namespace

TEST_F(FileTest, testSize)
{
   std::list<std::tuple<String, pdk::pint64>> data;
   init_size_data(data, m_testFile);
   for (auto &item : data) {
      String &filename = std::get<0>(item);
      pdk::pint64 size = std::get<1>(item);
      
      {
         File f(filename);
         ASSERT_EQ(f.getSize(), size);
         
         ASSERT_TRUE(f.open(IoDevice::OpenMode::ReadOnly)) << msg_open_failed(f).getConstRawData();
         ASSERT_EQ(f.getSize(), size);
      }
      {
         StdioFileGuard stream(PDK_FOPEN(filename.toLocal8Bit().getConstRawData(), "rb"));
         ASSERT_TRUE(stream);
         File f;
         ASSERT_TRUE(f.open(stream, IoDevice::OpenMode::ReadOnly));
         ASSERT_EQ(f.getSize(), size);
         f.close();
      }
      {
         File f;
         int fd = PDK_OPEN(filename.toLocal8Bit().getConstRawData(), PDK_OPEN_RDONLY);
         ASSERT_TRUE(fd != -1);
         ASSERT_TRUE(f.open(fd, IoDevice::OpenMode::ReadOnly));
         ASSERT_EQ(f.getSize(), size);
         f.close();
         PDK_CLOSE(fd);
      }
   }
}

TEST_F(FileTest, testSetSizeSeek)
{
   File file(Latin1String("setsizeseek.txt"));
   ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
   file.write("ABCD");
   ASSERT_EQ(file.getPosition(), pdk::pint64(4));
   file.resize(2);
   ASSERT_EQ(file.getPosition(), pdk::pint64(2));
   file.resize(4);
   ASSERT_EQ(file.getPosition(), pdk::pint64(2));
   file.resize(0);
   ASSERT_EQ(file.getPosition(), pdk::pint64(0));
   file.resize(4);
   ASSERT_EQ(file.getPosition(), pdk::pint64(0));
   
   file.seek(3);
   ASSERT_EQ(file.getPosition(), pdk::pint64(3));
   file.resize(2);
   ASSERT_EQ(file.getPosition(), pdk::pint64(2));
}

TEST_F(FileTest, testAtEnd)
{
   File file(m_testFile);
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
   int size = file.getSize();
   file.seek(size);
   bool end = file.atEnd();
   file.close();
   ASSERT_TRUE(end);
}

TEST_F(FileTest, testReadLine)
{
   File file(m_testFile);
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
   int i = 0;
   char buffer[128];
   int readLength;
   while ((readLength = file.readLine(buffer, 128)) > 0) {
      ++i;
      if (i == 5) {
         ASSERT_EQ(buffer[0], 'T');
         ASSERT_EQ(buffer[3], 's');
         ASSERT_EQ(buffer[11], 'i');
      }
   }
   file.close();
   ASSERT_EQ(i, 6);
}

TEST_F(FileTest, testReadLine2)
{
   File file(m_testFile);
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
   char buffer[128];
   ASSERT_EQ(file.readLine(buffer, 60), pdk::plonglong(59));
   ASSERT_EQ(file.readLine(buffer, 60), pdk::plonglong(59));
   std::memset(buffer, '@', sizeof(buffer));
   ASSERT_EQ(file.readLine(buffer, 60), pdk::plonglong(59));
   
   ASSERT_EQ(buffer[57], '-');
   ASSERT_EQ(buffer[58], '\n');
   ASSERT_EQ(buffer[59], '\0');
   ASSERT_EQ(buffer[60], '@');
}

TEST_F(FileTest, testReadLineNullInLine)
{
   File::remove(Latin1String("nullinline.txt"));
   File file(Latin1String("nullinline.txt"));
   ASSERT_TRUE(file.open(File::OpenMode::ReadWrite)) << msg_open_failed(file).getConstRawData();
   ASSERT_TRUE(file.write("linewith\0null\nanotherline\0withnull\n\0\nnull\0", 42) > 0);
   ASSERT_TRUE(file.flush());
   file.reset();
   
   ASSERT_EQ(file.readLine(), ByteArray("linewith\0null\n", 14));
   ASSERT_EQ(file.readLine(), ByteArray("anotherline\0withnull\n", 21));
   ASSERT_EQ(file.readLine(), ByteArray("\0\n", 2));
   ASSERT_EQ(file.readLine(), ByteArray("null\0", 5));
}

namespace {

void init_read_all_data(std::list<std::tuple<bool, String>> &data)
{
   data.push_back(std::make_tuple(true, sg_testFile));
   data.push_back(std::make_tuple(false, sg_testFile));
   
   data.push_back(std::make_tuple(true, sg_testFile));
   data.push_back(std::make_tuple(false, sg_testFile));
   
   data.push_back(std::make_tuple(true, sg_testSourceFile));
   data.push_back(std::make_tuple(false, sg_testSourceFile));
}

} // anonymous namespace

TEST_F(FileTest, testReadAll)
{
   std::list<std::tuple<bool, String>> data;
   init_read_all_data(data);
   for (auto &item : data) {
      bool textMode = std::get<0>(item);
      String &fileName = std::get<1>(item);
      File file(fileName);
      const IoDevice::OpenModes openModes = textMode ? (File::OpenModes(File::OpenMode::Text) | File::OpenMode::ReadOnly) : File::OpenMode::ReadOnly;
      ASSERT_TRUE(file.open(openModes)) <<  msg_open_failed(File::OpenMode(openModes.getUnderData()), file).getConstRawData();
      
      ByteArray alldata = file.readAll();
      file.reset();
      ASSERT_EQ(file.getPosition(), 0);
      
      ASSERT_TRUE(file.getBytesAvailable() > 7);
      ByteArray byte = file.read(1);
      char x;
      file.getChar(&x);
      byte.append(x);
      byte.append(file.read(5));
      byte.append(file.readAll());
      
      ASSERT_EQ(alldata, byte);
   }
}

TEST_F(FileTest, testReadAllBuffer)
{
   String fileName = Latin1String("readAllBuffer.txt");
   File::remove(fileName);
   File writer(fileName);
   File reader(fileName);
   
   ByteArray data1("This is arguably a very simple text.");
   ByteArray data2("This is surely not as simple a test.");
   
   ASSERT_TRUE(writer.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadWrite) | IoDevice::OpenMode::Unbuffered)) << msg_open_failed(writer).getConstRawData();
   ASSERT_TRUE(reader.open(IoDevice::OpenMode::ReadOnly)) << msg_open_failed(reader).getConstRawData();
   
   ASSERT_EQ(writer.write(data1), pdk::pint64(data1.size()));
   ASSERT_TRUE(writer.seek(0));
   
   ByteArray result;
   result = reader.read(18);
   ASSERT_EQ(result.size(), 18);
   
   ASSERT_EQ(writer.write(data2), pdk::pint64(data2.size())); // new data, old version buffered in reader
   ASSERT_EQ(writer.write(data2), pdk::pint64(data2.size())); // new data, unbuffered in reader
   
   result += reader.readAll();
   ASSERT_EQ(result, data1 + data2);
   File::remove(fileName);
}

// @TODO add process testcases
// @TODO add process testcases
// @TODO add process testcases

TEST_F(FileTest, testText)
{
   // dosfile.txt is a binary CRLF file
   File file(m_dosFile);
   ASSERT_TRUE(file.open(File::OpenModes(File::OpenMode::ReadOnly) | File::OpenMode::Text)) << msg_open_failed(file).getConstRawData();
   ASSERT_EQ(file.readLine(), ByteArray("/dev/system/root     /                    reiserfs   acl,user_xattr        1 1\n"));
   ASSERT_EQ(file.readLine(), ByteArray("/dev/sda1            /boot                ext3       acl,user_xattr        1 2\n"));
   
   file.ungetChar('\n');
   file.ungetChar('2');
   
   ASSERT_STREQ(file.readLine().getConstRawData(), ByteArray("2\n").getConstRawData());
}

TEST_F(FileTest, testMssingEndOfLine)
{
   File file(m_noEndOfLineFile);
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
   
   int nlines = 0;
   while (!file.atEnd()) {
      ++nlines;
      file.readLine();
   }
   ASSERT_EQ(nlines, 3);
}

TEST_F(FileTest, testReadBlock)
{
   File file(sg_testFile);
   file.open(File::OpenMode::ReadOnly);
   int length = 0;
   char buffer[256];
   length = file.read(buffer, 245);
   file.close();
   ASSERT_EQ(length, 245);
   ASSERT_EQ(buffer[59], 'D');
   ASSERT_EQ(buffer[178], 'T');
   ASSERT_EQ(buffer[199], 'l');
}

TEST_F(FileTest, testGetch)
{
   File file(sg_testFile);
   file.open(File::OpenMode::ReadOnly);
   char c;
   int i = 0;
   while (file.getChar(&c)) {
      ASSERT_EQ(file.getPosition(), pdk::pint64(i + 1));
      if (i == 59) {
         ASSERT_EQ(c, 'D');
      } 
      ++i;
   }
   file.close();
   ASSERT_EQ(i, 245);
}

TEST_F(FileTest, testUngetChar)
{
   File file(m_testFile);
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
   
   ByteArray array = file.readLine();
   ASSERT_STREQ(array.getConstRawData(), "----------------------------------------------------------\n");
   file.ungetChar('\n');
   
   array = file.readLine();
   ASSERT_STREQ(array.getConstRawData(), "\n");
   
   file.ungetChar('\n');
   file.ungetChar('-');
   file.ungetChar('-');
   
   array = file.readLine();
   ASSERT_STREQ(array.getConstRawData(), "--\n");
   
   File::remove(Latin1String("genfile.txt"));
   File out(Latin1String("genfile.txt"));
   
   ASSERT_TRUE(out.open(File::OpenMode::ReadWrite)) << msg_open_failed(file).getConstRawData();
   out.write("123");
   out.seek(0);
   
   ASSERT_STREQ(out.readAll().getConstRawData(), "123");
   
   out.ungetChar('3');
   out.write("4");
   out.seek(0);
   
   ASSERT_STREQ(out.readAll().getConstRawData(), "124");
   out.ungetChar('4');
   out.ungetChar('2');
   out.ungetChar('1');
   char buf[3];
   ASSERT_EQ(out.read(buf, sizeof(buf)), pdk::pint64(3));
   ASSERT_EQ(buf[0], '1');
   ASSERT_EQ(buf[1], '2');
   ASSERT_EQ(buf[2], '4');
}

namespace {

void init_invalid_file_data(std::list<String> &data)
{
   // @TODO add Windows testcase data
#if !defined(PDK_OS_WIN)
   data.push_back(String(Latin1String("qwe//")));
#endif
}

} // anonymous namespace

TEST_F(FileTest, testInvalidFile)
{
   std::list<String> data;
   init_invalid_file_data(data);
   for (auto &fileName : data) {
      File file(fileName);
      ASSERT_TRUE(!file.open(IoDevice::OpenMode::ReadWrite)) << pdk_printable(fileName);
   }
}

TEST_F(FileTest, testCreateFile)
{
   if (File::exists(Latin1String("createme.txt"))){
      File::remove(Latin1String("createme.txt"));
   }
   ASSERT_TRUE(!File::exists(Latin1String("createme.txt")));
   File file(Latin1String("createme.txt"));
   ASSERT_TRUE(file.open(IoDevice::OpenMode::WriteOnly));
   file.close();
   ASSERT_TRUE(File::exists(Latin1String("createme.txt")));
}

TEST_F(FileTest, testAppend)
{
   const String name(Latin1String("appendme.txt"));
   if (File::exists(name)) {
      File::remove(name);
   }
   ASSERT_TRUE(!File::exists(name));
   File file(name);
   ASSERT_TRUE(file.open(File::OpenModes(IoDevice::OpenMode::WriteOnly) | IoDevice::OpenMode::Truncate)) << msg_open_failed(file).getConstRawData();
   file.putChar('a');
   file.close();
   
   ASSERT_TRUE(file.open(IoDevice::OpenMode::Append)) << msg_open_failed(file).getConstRawData();
   ASSERT_EQ(file.getPosition(), 1);
   file.putChar('a');
   file.close();
   ASSERT_EQ(int(file.getSize()), 2);
}

namespace {

void init_permissions_data(std::list<std::tuple<String, File::Permission, bool, bool>> &data)
{
   data.push_back(std::make_tuple(CoreApplication::getInstance()->getAppFilePath(), 
                                  File::Permission::ExeUser, true, false));
   data.push_back(std::make_tuple(sg_testSourceFile, 
                                  File::Permission::ReadUser, true, false));
   data.push_back(std::make_tuple(String::fromLatin1("readonlyfile"), 
                                  File::Permission::WriteUser, false, false));
   data.push_back(std::make_tuple(String::fromLatin1("longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName"
                                                     "longFileNamelongFileNamelongFileNamelongFileName.txt"), 
                                  File::Permission::ReadUser, true, true));
   
   // @TODO add Resource testcases
   
}

} // anonymous namespace

PDKTEST_DECLARE_APP_STARTUP_ARGS();

TEST_F(FileTest, testPermissions)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   std::list<std::tuple<String, File::Permission, bool, bool>> data;
   init_permissions_data(data);
   for (auto &item : data) {
      String &fileName = std::get<0>(item);
      File::Permission perms = std::get<1>(item);
      bool expected = std::get<2>(item);
      bool create = std::get<3>(item);
      if (create) {
         File file(fileName);
         ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
         ASSERT_TRUE(file.write("hello\n"));
         file.close();
      }
      
      File file(fileName);
      File::Permissions memberResult = file.getPermissions() & perms;
      File::Permissions staticResult = File::getPermissions(fileName) & perms;
      
      if (create) {
         File::remove(fileName);
      }
      
      // @TODO add Windows platform testcase
#ifdef PDK_OS_UNIX
      // in case accidentally run as root
      if (::getuid() == 0) {
         std::cout << "Running this test as root doesn't make sense";
      }
#endif
      ASSERT_EQ((memberResult == File::Permissions(perms)), expected);
      ASSERT_EQ((staticResult == File::Permissions(perms)), expected);
   }
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(FileTest, testSetPermissions)
{
   if (File::exists(Latin1String("createme.txt"))) {
      File::remove(Latin1String("createme.txt"));
   }
   ASSERT_TRUE(!File::exists(Latin1String("createme.txt")));
   
   File file(Latin1String("createme.txt"));
   ASSERT_TRUE(file.open(File::OpenModes(File::OpenMode::WriteOnly) | File::OpenMode::Truncate)) << msg_open_failed(file).getConstRawData();
   file.putChar('a');
   file.close();
   
   File::Permissions perms(File::Permission::WriteUser);
   perms |= File::Permission::ReadUser;
   ASSERT_TRUE(file.setPermissions(perms));
   ASSERT_TRUE((file.getPermissions() & perms) == perms);
}

TEST_F(FileTest, testCopyAfterFail)
{
   File file1(Latin1String("file-to-be-copied.txt"));
   File file2(Latin1String("existing-file.txt"));
   ASSERT_TRUE(file1.open(IoDevice::OpenMode::ReadWrite)) << msg_open_failed(file1).getConstRawData();
   ASSERT_TRUE(file2.open(IoDevice::OpenMode::ReadWrite)) << msg_open_failed(file2).getConstRawData();
   
   file2.close();
   ASSERT_TRUE(!File::exists(Latin1String("copied-file-1.txt")) && "(test-precondition)");
   ASSERT_TRUE(!File::exists(Latin1String("copied-file-2.txt")) && "(test-precondition)");
   
   ASSERT_TRUE(!file1.copy(Latin1String("existing-file.txt")));
   ASSERT_EQ(file1.getError(), File::FileError::CopyError);
   
   ASSERT_TRUE(file1.copy(Latin1String("copied-file-1.txt")));
   ASSERT_TRUE(!file1.isOpen());
   ASSERT_EQ(file1.getError(), File::FileError::NoError);
   
   ASSERT_TRUE(!file1.copy(Latin1String("existing-file.txt")));
   ASSERT_EQ(file1.getError(), File::FileError::CopyError);
   
   ASSERT_TRUE(file1.copy(Latin1String("copied-file-2.txt")));
   ASSERT_TRUE(!file1.isOpen());
   ASSERT_EQ(file1.getError(), File::FileError::NoError);
   
   ASSERT_TRUE(File::exists(Latin1String("copied-file-1.txt")));
   ASSERT_TRUE(File::exists(Latin1String("copied-file-2.txt")));
}

TEST_F(FileTest, testCopyRemovesTemporaryFile)
{
   const String newName(Latin1String("copyRemovesTemporaryFile"));
   ASSERT_TRUE(File::copy(m_forCopyingFile, newName));
   ASSERT_TRUE(!File::exists(StringLiteral("pdk_temp.XXXXXX")));
}

TEST_F(FileTest, testCopyShouldntOverwrite)
{
   // Copy should not overwrite existing files.
   File::remove(Latin1String("test_file.backup"));
   File file(m_testSourceFile);
   ASSERT_TRUE(file.copy(Latin1String("test_file.backup")));
   
   bool ok = File::setPermissions(Latin1String("test_file.backup"), File::Permission::WriteOther);
   ASSERT_TRUE(ok);
   ASSERT_TRUE(!file.copy(Latin1String("test_file.backup")));
}

// @TODO file copyFallback testcase,because now resource API is not supported

TEST_F(FileTest, testLink)
{
   File::remove(Latin1String("myLink.lnk"));
   FileInfo info1(m_testSourceFile);
   String referenceTarget = Dir::cleanPath(info1.getAbsoluteFilePath());
   ASSERT_TRUE(File::link(m_testSourceFile, Latin1String("myLink.lnk")));
   
   FileInfo info2(Latin1String("myLink.lnk"));
   ASSERT_TRUE(info2.isSymLink());
   ASSERT_EQ(info2.getSymLinkTarget(), referenceTarget);
   
   File link(Latin1String("myLink.lnk"));
   ASSERT_TRUE(link.open(IoDevice::OpenMode::ReadOnly)) << msg_open_failed(link).getConstRawData();
   ASSERT_EQ(link.getSymLinkTarget(), referenceTarget);
   link.close();
   
   ASSERT_EQ(File::getSymLinkTarget(Latin1String("myLink.lnk")), referenceTarget);
}

TEST_F(FileTest, testLinkToDir)
{
   File::remove(Latin1String("myLinkToDir.lnk"));
   Dir dir;
   dir.mkdir(Latin1String("myDir"));
   FileInfo info1(Latin1String("myDir"));
   ASSERT_TRUE(File::link(Latin1String("myDir"), Latin1String("myLinkToDir.lnk")));
   FileInfo info2(Latin1String("myLinkToDir.lnk"));
#if !(defined PDK_OS_HPUX && defined(__ia64))
   // absurd HP-UX filesystem bug on gravlaks - checking if a symlink
   // resolves or not alters the file system to make the broken symlink
   // later fail...
   ASSERT_TRUE(info2.isSymLink());
#endif
   ASSERT_EQ(info2.getSymLinkTarget(), info1.getAbsoluteFilePath());
   ASSERT_TRUE(File::remove(info2.getAbsoluteFilePath()));
}

TEST_F(FileTest, testAbsolutePathLinkToRelativePath)
{
   File::remove(Latin1String("myDir/test.txt"));
   File::remove(Latin1String("myDir/myLink.lnk"));
   Dir dir;
   dir.mkdir(Latin1String("myDir"));
   File(Latin1String("myDir/test.txt")).open(File::OpenMode::WriteOnly);
   // @TODO add Windows platform testcase
   ASSERT_TRUE(File::link(Latin1String("myDir/test.txt"), Latin1String("myDir/myLink.lnk")));
   //   std::cout << FileInfo(File(FileInfo(Latin1String("myDir/myLink.lnk")).getAbsoluteFilePath()).getSymLinkTarget()).getAbsoluteFilePath() << std::endl;
   //   std::cout << FileInfo(Latin1String("myDir/test.txt")).getAbsoluteFilePath() << std::endl;
   //   ASSERT_EQ(FileInfo(File(FileInfo(Latin1String("myDir/myLink.lnk")).getAbsoluteFilePath()).getSymLinkTarget()).getAbsoluteFilePath(),
   //             FileInfo(Latin1String("myDir/test.txt")).getAbsoluteFilePath());
}

TEST_F(FileTest, testReadBrokenLink)
{
   File::remove(Latin1String("myLink2.lnk"));
   FileInfo info1(Latin1String("file12"));
   ASSERT_TRUE(File::link(Latin1String("file12"), Latin1String("myLink2.lnk")));
   FileInfo info2(Latin1String("myLink2.lnk"));
   ASSERT_TRUE(info2.isSymLink());
   ASSERT_EQ(info2.getSymLinkTarget(), info1.getAbsoluteFilePath());
   ASSERT_TRUE(File::remove(info2.getAbsoluteFilePath()));
   ASSERT_TRUE(File::link(Latin1String("ole/.."), Latin1String("myLink2.lnk")));
   ASSERT_EQ(FileInfo(Latin1String("myLink2.lnk")).getSymLinkTarget(), Dir::getCurrentPath());
}

namespace {

void init_read_textfile_data(std::list<std::tuple<ByteArray, ByteArray>> &data)
{
   data.push_back(std::make_tuple(ByteArray(), ByteArray()));
   data.push_back(std::make_tuple(ByteArray("a"), ByteArray("a")));
   data.push_back(std::make_tuple(ByteArray("a\rb"), ByteArray("ab")));
   data.push_back(std::make_tuple(ByteArray("\n"), ByteArray("\n")));
   data.push_back(std::make_tuple(ByteArray("\r\n"), ByteArray("\n")));
   data.push_back(std::make_tuple(ByteArray("\r"), ByteArray()));
   data.push_back(std::make_tuple(ByteArray("Hello\r\nWorld\r\n"), ByteArray("Hello\nWorld\n")));
   data.push_back(std::make_tuple(ByteArray("Hello\r\nWorld"), ByteArray("Hello\nWorld")));
}

void init_write_textfile_data(std::list<ByteArray> &data)
{
   data.push_back(ByteArray());
   data.push_back(ByteArray("a"));
   data.push_back(ByteArray("a\rb"));
   data.push_back(ByteArray("\n"));
   data.push_back(ByteArray("\r\n"));
   data.push_back(ByteArray("\r"));
   data.push_back(ByteArray("Hello\r\nWorld\r\n"));
   data.push_back(ByteArray("Hello\r\nWorld"));
   data.push_back(ByteArray("Hello\nWorld\n"));
   data.push_back(ByteArray("Hello\nWorld"));
   data.push_back(ByteArray("this\nis\r\na\nmixed\r\nfile\n"));
}

} // anonymous namespace

TEST_F(FileTest, testReadTextFile)
{
   std::list<std::tuple<ByteArray, ByteArray>> data;
   init_read_textfile_data(data);
   for (auto &item : data) {
      ByteArray &in = std::get<0>(item);
      ByteArray &out = std::get<1>(item);
      File winfile(Latin1String("winfile.txt"));
      ASSERT_TRUE(winfile.open(File::OpenModes(File::OpenMode::WriteOnly) | File::OpenMode::Truncate)) << msg_open_failed(winfile).getConstRawData();
      winfile.write(in);
      winfile.close();
      
      ASSERT_TRUE(winfile.open(File::OpenMode::ReadOnly)) << msg_open_failed(winfile).getConstRawData();
      ASSERT_EQ(winfile.readAll(), in);
      winfile.close();
      
      ASSERT_TRUE(winfile.open(File::OpenModes(File::OpenMode::ReadOnly) | File::OpenMode::Text)) << msg_open_failed(winfile).getConstRawData();
      ASSERT_EQ(winfile.readAll(), out);
   }
}

TEST_F(FileTest, testReadTextFile2)
{
   {
      File file(m_testLogFile);
      ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
      file.read(4097);
   }
   
   {
      File file(m_testLogFile);
      ASSERT_TRUE(file.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | IoDevice::OpenMode::Text)) << msg_open_failed(file).getConstRawData();
      file.read(4097);
   }
}

TEST_F(FileTest, testWriteTextFile)
{
   std::list<ByteArray> data;
   init_write_textfile_data(data);
   for (ByteArray &in : data) {
      File file(Latin1String("textfile.txt"));
      ASSERT_TRUE(file.open(File::OpenModes(File::OpenMode::WriteOnly) | File::OpenMode::Truncate | File::OpenMode::Text)) << msg_open_failed(file).getConstRawData();
      ByteArray out = in;
#ifdef PDK_OS_WIN
      out.replace('\n', "\r\n");
#endif
      ASSERT_EQ(file.write(in), pdk::plonglong(in.size()));
      file.close();
      
      file.open(File::OpenMode::ReadOnly);
      ASSERT_EQ(file.readAll(), out);
   }
}

// @TODO add Windows platform testcases
//TEST_F(FileTest, testLargeUncFileSupport)
//{

//}

TEST_F(FileTest, testFlush)
{
   String fileName(Latin1String("stdfile.txt"));
   
   File::remove(fileName);
   
   {
      File file(fileName);
      ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
      ASSERT_EQ(file.write("abc", 3), pdk::pint64(3));
   }
   
   {
      File file(fileName);
      ASSERT_TRUE(file.open(File::OpenModes(File::OpenMode::WriteOnly) | File::OpenMode::Append)) << msg_open_failed(file).getConstRawData();
      ASSERT_EQ(file.getPosition(), pdk::plonglong(3));
      ASSERT_EQ(file.write("def", 3), pdk::plonglong(3));
      ASSERT_EQ(file.getPosition(), pdk::plonglong(6));
   }
   
   {
      File file(Latin1String("stdfile.txt"));
      ASSERT_TRUE(file.open(File::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
      ASSERT_EQ(file.readAll(), ByteArray("abcdef"));
   }
}

TEST_F(FileTest, testBufferedRead)
{
   File::remove(Latin1String("stdfile.txt"));
   
   File file(Latin1String("stdfile.txt"));
   ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
   file.write("abcdef");
   file.close();
   
   StdioFileGuard stdFile(fopen("stdfile.txt", "r"));
   ASSERT_TRUE(stdFile);
   char c;
   ASSERT_EQ(int(fread(&c, 1, 1, stdFile)), 1);
   ASSERT_EQ(c, 'a');
   ASSERT_EQ(int(ftell(stdFile)), 1);
   
   {
      File file;
      ASSERT_TRUE(file.open(stdFile, File::OpenMode::ReadOnly)) <<  msg_open_failed(file).getConstRawData();
      ASSERT_EQ(file.getPosition(), pdk::plonglong(1));
      ASSERT_EQ(file.read(&c, 1), pdk::plonglong(1));
      ASSERT_EQ(c, 'b');
      ASSERT_EQ(file.getPosition(), pdk::plonglong(2));
   }
}

#ifdef PDK_OS_UNIX
TEST_F(FileTest, testIsSequential)
{
   File zero(Latin1String("/dev/null"));
   ASSERT_TRUE(zero.open(File::OpenMode::ReadOnly)) << msg_open_failed(zero).getConstRawData();
   ASSERT_TRUE(zero.isSequential());
}
#endif

TEST_F(FileTest, testEncodeName)
{
   ASSERT_EQ(File::encodeName(String()), ByteArray());
}

TEST_F(FileTest, testTruncate)
{
   for (int i = 0; i < 2; ++i) {
      File file(Latin1String("truncate.txt"));
      ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
      file.write(ByteArray(200, '@'));
      file.close();
      
      ASSERT_TRUE(file.open(File::OpenModes((i ? File::OpenMode::WriteOnly : File::OpenMode::ReadWrite)) | File::OpenMode::Truncate)) << msg_open_failed(file).getConstRawData();
      file.write(ByteArray(100, '$'));
      file.close();
      
      ASSERT_TRUE(file.open(File::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
      ASSERT_EQ(file.readAll(), ByteArray(100, '$'));
   }
}

TEST_F(FileTest, testSeekToPos)
{
   {
      File file(Latin1String("seekToPos.txt"));
      ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
      file.write("a\r\nb\r\nc\r\n");
      file.flush();
   }
   
   File file(Latin1String("seekToPos.txt"));
   ASSERT_TRUE(file.open(File::OpenModes(File::OpenMode::ReadOnly) | File::OpenMode::Text)) << msg_open_failed(file).getConstRawData();
   file.seek(1);
   char c;
   ASSERT_TRUE(file.getChar(&c));
   ASSERT_EQ(c, '\n');
   
   ASSERT_EQ(file.getPosition(), pdk::pint64(3));
   file.seek(file.getPosition());
   ASSERT_EQ(file.getPosition(), pdk::pint64(3));
   
   file.seek(1);
   file.seek(file.getPosition());
   ASSERT_EQ(file.getPosition(), pdk::pint64(1));
}

TEST_F(FileTest, testSeekAfterEndOfFile)
{
   Latin1String filename("seekAfterEof.dat");
   File::remove(filename);
   {
      File file(filename);
      ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
      file.write("abcd");
      ASSERT_EQ(file.getSize(), pdk::pint64(4));
      file.seek(8);
      file.write("ijkl");
      ASSERT_EQ(file.getSize(), pdk::pint64(12));
      file.seek(4);
      file.write("efgh");
      ASSERT_EQ(file.getSize(), pdk::pint64(12));
      file.seek(16);
      file.write("----");
      ASSERT_EQ(file.getSize(), pdk::pint64(20));
      file.flush();
   }
   
   File file(filename);
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
   ByteArray contents = file.readAll();
   ASSERT_EQ(contents.left(12), ByteArray("abcdefghijkl", 12));
   //bytes 12-15 are uninitialised so we don't care what they read as.
   ASSERT_EQ(contents.mid(16), ByteArray("----", 4));
   file.close();
}

// @TODO add DataStream testcases
//TEST_F(FileTest, testFILEReadWrite)
//{
//   // Tests modifying a file. First creates it then reads in 4 bytes and then overwrites these
//   // 4 bytes with new values. At the end check to see the file contains the new values.
//   File::remove("FILEReadWrite.txt");
//   // create test file
//   {
//      File f("FILEReadWrite.txt");
//      ASSERT_TRIE(f.open(File::WriteOnly)) << msg_open_failed(file).getConstRawData();
//      DataStream ds(&f);
//      pdk::pint8 c = 0;
//      ds << c;
//      c = 1;
//      ds << c;
//      c = 2;
//      ds << c;
//      c = 3;
//      ds << c;
//      c = 4;
//      ds << c;
//      c = 5;
//      ds << c;
//      c = 6;
//      ds << c;
//      c = 7;
//      ds << c;
//      c = 8;
//      ds << c;
//      c = 9;
//      ds << c;
//      c = 10;
//      ds << c;
//      c = 11;
//      ds << c;
//      f.close();
//   }

//   StdioFileGuard fp(fopen("FILEReadWrite.txt", "r+b"));
//   ASSERT_TRUE(fp);
//   File file;
//   ASSERT_TRUE(file.open(fp, File::ReadWrite)) << msg_open_failed(file).getConstRawData();
//   DataStream sfile(&file) ;

//   pdk::pint8 var1,var2,var3,var4;
//   while (!sfile.atEnd())
//   {
//      pdk::pint64 base = file.getPosition();

//      ASSERT_EQ(file.getPosition(), base + 0);
//      sfile >> var1;
//      ASSERT_EQ(file.getPosition(), base + 1);
//      file.flush(); // flushing should not change the base
//      ASSERT_EQ(file.getPosition(), base + 1);
//      sfile >> var2;
//      ASSERT_EQ(file.getPosition(), base + 2);
//      sfile >> var3;
//      ASSERT_EQ(file.getPosition(), base + 3);
//      sfile >> var4;
//      ASSERT_EQ(file.getPosition(), base + 4);
//      file.seek(file.getPosition() - 4) ;   // Move it back 4, for we are going to write new values based on old ones
//      ASSERT_EQ(file.getPosition(), base + 0);
//      sfile << pdk::pint8(var1 + 5);
//      ASSERT_EQ(file.getPosition(), base + 1);
//      sfile << pdk::pint8(var2 + 5);
//      ASSERT_EQ(file.getPosition(), base + 2);
//      sfile << pdk::pint8(var3 + 5);
//      ASSERT_EQ(file.getPosition(), base + 3);
//      sfile << pdk::pint8(var4 + 5);
//      ASSERT_EQ(file.getPosition(), base + 4);

//   }
//   file.close();
//   fp.close();

//   // check modified file
//   {
//      File f("FILEReadWrite.txt");
//      ASSERT_TRUE(f.open(File::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
//      DataStream ds(&f);
//      pdk::pint8 c = 0;
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)5);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)6);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)7);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)8);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)9);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)10);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)11);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)12);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)13);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)14);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)15);
//      ds >> c;
//      ASSERT_EQ(c, (pdk::pint8)16);
//      f.close();
//   }
//}

namespace {

void init_i18n_file_name_data(std::list<String> &data)
{
   data.push_back(String::fromUtf8("xxxxxxx.txt"));
}

} // anonymous namespace

// @TODO add TextStream testcases
//TEST_F(FileTest, testi18nFileName)
//{
//   std::list<String> data;
//   init_i18n_file_name_data(data);
//   for (String &filename : data) {
//      if (File::exists(fileName)) {
//         ASSERT_TRUE(File::remove(fileName));
//      }
//      {
//         File file(fileName);
//         ASSERT_TRUE(file.open(File::WriteOnly | File::Text)) << msg_open_failed(file).getConstRawData();
//         TextStream ts(&file);
//         ts.setCodec("UTF-8");
//         ts << fileName << endl;
//      }
//      {
//         File file(fileName);
//         ASSERT_TRUE(file.open(File::ReadOnly | File::Text)) << msg_open_failed(file).getConstRawData();
//         TextStream ts(&file);
//         ts.setCodec("UTF-8");
//         String line = ts.readLine();
//         ASSERT_EQ(line, fileName);
//      }
//   }
//}

namespace {

void init_long_filename_data(std::list<String> &data)
{
   data.push_back(String::fromLatin1("longFileName.txt"));
}

} // anonymous namespace

//TEST_F(FileTest, testLongFileName)
//{
//   std::list<String> data;
//   init_long_filename_data(data);
//   for (String &fileName : data) {
//      if (File::exists(fileName)) {
//         ASSERT_TRUE(File::remove(fileName));
//      }
//      {
//         File file(fileName);
//         ASSERT_TRUE(file.open(File::OpenModes(File::OpenMode::WriteOnly) | File::OpenMode::Text)) << msg_open_failed(file).getConstRawData();
//         TextStream ts(&file);
//         ts << fileName << endl;
//      }
//      {
//         File file(fileName);
//         ASSERT_TRUE(file.open(File::OpenModes(File::OpenMode::ReadOnly) | File::OpenMode::Text)) << msg_open_failed(file).getConstRawData();
//         TextStream ts(&file);
//         String line = ts.readLine();
//         ASSERT_EQ(line, fileName);
//      }
//      String newName = fileName + Latin1Character('1');
//      {
//         ASSERT_TRUE(File::copy(fileName, newName));
//         File file(newName);
//         ASSERT_TRUE(file.open(File::OpenModes(File::OpenMode::ReadOnly) | File::OpenMode::Text)) << msg_open_failed(file).getConstRawData();
//         TextStream ts(&file);
//         String line = ts.readLine();
//         ASSERT_EQ(line, fileName);

//      }
//      ASSERT_TRUE(File::remove(newName));
//      {
//         ASSERT_TRUE(File::rename(fileName, newName));
//         File file(newName);
//         ASSERT_TRUE(file.open((File::OpenModes(File::OpenMode::ReadOnly) | File::OpenMode::Text))) << msg_open_failed(file).getConstRawData();
//         TextStream ts(&file);
//         String line = ts.readLine();
//         ASSERT_EQ(line, fileName);
//      }
//      ASSERT_TRUE(File::exists(newName)) << msg_file_does_not_exist(newName).getConstRawData();
//   }
//}

//#ifdef PDK_BUILD_INTERNAL
class MyEngine : public AbstractFileEngine
{
public:
   MyEngine(int n) { number = n; }
   virtual ~MyEngine() {}
   
   void setFileName(const String &) override 
   {}
   
   bool open(IoDevice::OpenModes openMode) override
   {
      return false;
   }
   
   bool close() override
   {
      return false;
   }
   
   bool flush() override
   {
      return false;
   }
   
   pdk::pint64 getSize() const override
   {
      return 123 + number;
   }
   
   pdk::pint64 at() const 
   {
      return -1;
   }
   
   bool seek(pdk::pint64) override
   {
      return false;
   }
   
   bool isSequential() const override
   {
      return false;
   }
   
   pdk::pint64 read(char *, pdk::pint64) override
   {
      return -1;
   }
   
   pdk::pint64 write(const char *, pdk::pint64) override { return -1; }
   
   bool remove() override
   {
      return false;
   }
   
   bool copy(const String &) override
   {
      return false;
   }
   
   bool rename(const String &) override
   {
      return false;
   }
   
   bool link(const String &) override
   {
      return false;
   }
   
   bool mkdir(const String &, bool) const override
   {
      return false;
   }
   
   bool rmdir(const String &, bool) const override
   {
      return false;
   }
   
   bool setSize(pdk::pint64) override
   {
      return false;
   }
   
   StringList entryList(Dir::Filters, const StringList &) const
   {
      return StringList();
   }
   
   bool caseSensitive() const override
   { 
      return false;
   }
   
   bool isRelativePath() const override
   {
      return false;
   }
   
   FileFlags fileFlags(FileFlags) const
   {
      return 0;
   }
   
   bool chmod(uint)
   {
      return false;}
   
   
   String fileName(FileName) const
   {
      return name;
   }
   
   uint ownerId(FileOwner) const
   {
      return 0;
   }
   
   String owner(FileOwner) const
   {
      return String();
   }
   
   DateTime fileTime(FileTime) const
   {
      return DateTime();
   }
   
   bool setFileTime(const DateTime &newDate, FileTime time) override
   {
      return false;
   }
   
private:
   int number;
   String name;
};

class MyHandler : public AbstractFileEngineHandler
{
public:
   inline AbstractFileEngine *create(const String &) const
   {
      return new MyEngine(1);
   }
};

class MyHandler2 : public AbstractFileEngineHandler
{
public:
   inline AbstractFileEngine *create(const String &) const
   {
      return new MyEngine(2);
   }
};
//#endif

TEST_F(FileTest, testFileEngineHandler)
{
   // A file that does not exist has a size of 0.
   File::remove(Latin1String("ole.bull"));
   File file(Latin1String("ole.bull"));
   ASSERT_EQ(file.getSize(), pdk::pint64(0));
   
   //#ifdef PDK_BUILD_INTERNAL
   // Instantiating our handler will enable the new engine.
   MyHandler handler;
   file.setFileName(Latin1String("ole.bull"));
   ASSERT_EQ(file.getSize(), pdk::pint64(124));
   
   // A new, identical handler should take preference over the last one.
   MyHandler2 handler2;
   file.setFileName(Latin1String("ole.bull"));
   ASSERT_EQ(file.getSize(), pdk::pint64(125));
   //#endif
}

//#ifdef PDK_BUILD_INTERNAL
class MyRecursiveHandler : public AbstractFileEngineHandler
{
public:
   inline AbstractFileEngine *create(const String &fileName) const
   {
      if (fileName.startsWith(Latin1String(":!"))) {
         Dir dir;
         const String realFile = String(Latin1String(PDKTEST_CURRENT_TEST_SOURCE_DIR)) + Latin1String("/") + (fileName.substring(2));
         if (dir.exists(realFile)) {
            return new FileEngine(realFile);
         }
      }
      return 0;
   }
};
//#endif

//#ifdef PDK_BUILD_INTERNAL
TEST_F(FileTest, testUseFileInAFileHandler)
{
   // This test should not dead-lock
   MyRecursiveHandler handler;
   File file(Latin1String(":!FileTest.cpp"));
   ASSERT_TRUE(file.exists());
}
//#endif

TEST_F(FileTest, testGetCharFF)
{
   File file(Latin1String("file.txt"));
   file.open(File::OpenMode::ReadWrite);
   file.write("\xff\xff\xff");
   file.flush();
   file.seek(0);
   
   char c;
   ASSERT_TRUE(file.getChar(&c));
   ASSERT_TRUE(file.getChar(&c));
   ASSERT_TRUE(file.getChar(&c));
}

TEST_F(FileTest, removeAndExists)
{
   File::remove(Latin1String("somefilename.txt"));
   File file(Latin1String("somefilename.txt"));
   ASSERT_TRUE(!file.exists());
   bool opened = file.open(IoDevice::OpenMode::WriteOnly);
   ASSERT_TRUE(opened);
   file.write("testing that remove/exists work...");
   file.close();
   ASSERT_TRUE(file.exists());
   file.remove();
   ASSERT_TRUE(!file.exists());
}

TEST_F(FileTest, testRemoveOpenFile)
{
   {
      // remove an opened, write-only file
      File::remove(Latin1String("remove_unclosed.txt"));
      File file(Latin1String("remove_unclosed.txt"));
      
      ASSERT_TRUE(!file.exists());
      bool opened = file.open(IoDevice::OpenMode::WriteOnly);
      ASSERT_TRUE(opened);
      file.write("testing that remove closes the file first...");
      
      bool removed = file.remove(); // remove should both close and remove the file
      ASSERT_TRUE(removed);
      ASSERT_TRUE(!file.isOpen());
      ASSERT_TRUE(!file.exists());
      ASSERT_EQ(file.getError(), File::FileError::NoError);
   }
   
   {
      // remove an opened, read-only file
      File::remove(Latin1String("remove_unclosed.txt"));
      
      // first, write a file that we can remove
      {
         File file(Latin1String("remove_unclosed.txt"));
         ASSERT_TRUE(!file.exists());
         bool opened = file.open(IoDevice::OpenMode::WriteOnly);
         ASSERT_TRUE(opened);
         file.write("testing that remove closes the file first...");
         file.close();
      }
      
      File file(Latin1String("remove_unclosed.txt"));
      bool opened = file.open(IoDevice::OpenMode::ReadOnly);
      ASSERT_TRUE(opened);
      file.readAll();
      // this used to only fail on FreeBSD (and OS X)
      ASSERT_TRUE(file.flush());
      bool removed = file.remove(); // remove should both close and remove the file
      ASSERT_TRUE(removed);
      ASSERT_TRUE(!file.isOpen());
      ASSERT_TRUE(!file.exists());
      ASSERT_EQ(file.getError(), File::FileError::NoError);
   }
}

TEST_F(FileTest, testFullDisk)
{
   File file(Latin1String("/dev/full"));
   if (!file.exists()) {
      std::cout << "/dev/full doesn't exist on this system" << std::endl;
      return;
   }
   ASSERT_TRUE(file.open(IoDevice::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
   file.write("foobar", 6);
   
   ASSERT_TRUE(!file.flush());
   ASSERT_EQ(file.getError(), File::FileError::ResourceError);
   ASSERT_TRUE(!file.flush());
   ASSERT_EQ(file.getError(), File::FileError::ResourceError);
   
   char c = 0;
   file.write(&c, 0);
   ASSERT_TRUE(!file.flush());
   ASSERT_EQ(file.getError(), File::FileError::ResourceError);
   ASSERT_EQ(file.write(&c, 1), pdk::pint64(1));
   ASSERT_TRUE(!file.flush());
   ASSERT_EQ(file.getError(), File::FileError::ResourceError);
   
   file.close();
   ASSERT_TRUE(!file.isOpen());
   ASSERT_EQ(file.getError(), File::FileError::ResourceError);
   
   file.open(IoDevice::OpenMode::WriteOnly);
   ASSERT_EQ(file.getError(), File::FileError::NoError);
   ASSERT_TRUE(file.flush()); // Shouldn't inherit write buffer
   file.close();
   ASSERT_EQ(file.getError(), File::FileError::NoError);
   
   // try again without flush:
   ASSERT_TRUE(file.open(IoDevice::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
   file.write("foobar", 6);
   file.close();
   ASSERT_TRUE(file.getError() != File::FileError::NoError);
}

namespace {

void init_write_large_data_block_data(std::list<std::tuple<String, FileTest::FileType>> &data)
{
   data.push_back(std::make_tuple(Latin1String("./largeblockfile.txt"), FileTest::FileType::OpenFile));
   data.push_back(std::make_tuple(Latin1String("./largeblockfile.txt"), FileTest::FileType::OpenFd));
   data.push_back(std::make_tuple(Latin1String("./largeblockfile.txt"), FileTest::FileType::OpenStream));
   // @TODO add windows platform testcases
}

ByteArray get_large_data_block()
{
   static ByteArray array;
   
   if (array.isNull())
   {
#if defined(PDK_OS_VXWORKS)
      int resizeSize = 1024 * 1024; // VxWorks does not have much space
#else
      int resizeSize = 64 * 1024 * 1024;
#endif
      array.resize(resizeSize);
      for (int i = 0; i < array.size(); ++i) {
         array[i] = uchar(i);
      }
   }
   return array;
}

} // anonynmous namespace

//TEST_F(FileTest, testWriteLargeDataBlock)
//{
//   std::list<std::tuple<String, FileType>> data;
//   init_write_large_data_block_data(data);
//   for (auto &item : data) {
//      String &fileName = std::get<0>(item);
//      FileType type = std::get<1>(item);
//      ByteArray const originalData = get_large_data_block();
//      {
//         File file(fileName);

//         ASSERT_TRUE(openFile(file, IoDevice::OpenMode::WriteOnly, type)) << msg_open_failed(file).getConstRawData();
//         pdk::pint64 fileWriteOriginalData = file.write(originalData);
//         pdk::pint64 originalDataSize      = (pdk::pint64)originalData.size();
//#if defined(PDK_OS_WIN)
//         if (fileWriteOriginalData != originalDataSize) {
//            warning_stream() << pdk_printable(String("Error writing a large data block to [%1]: %2")
//                                              .arg(fileName)
//                                              .arg(file.getErrorString()));
//            FAIL() << "unc file";
//         }
//#endif
//         ASSERT_EQ(fileWriteOriginalData, originalDataSize);
//         ASSERT_TRUE(file.flush());   
//         closeFile(file);
//      }
//      ByteArray readData;
//      {
//         File file(fileName);
//         ASSERT_TRUE(openFile(file, IoDevice::OpenMode::ReadOnly, type)) 
//               << pdk_printable(String(Latin1String("Couldn't open file for reading: [%1]")).arg(fileName));
//         readData = file.readAll();

//#if defined(PDK_OS_WIN)
//         if (readData != originalData) {
//            warning_stream() << pdk_printable(String("Error reading a large data block from [%1]: %2")
//                                              .arg(fileName)
//                                              .arg(file.getErrorString()));
//            FAIL() << "unc file";
//         }
//#endif
//         closeFile(file);
//      }
//      ASSERT_EQ(readData, originalData);
//      ASSERT_TRUE(File::remove(fileName));
//   }
//}

TEST_F(FileTest, testReadFromWriteOnlyFile)
{
   File file(Latin1String("writeonlyfile"));
   ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
   char c;
   ASSERT_EQ(file.read(&c, 1), pdk::pint64(-1));
}

TEST_F(FileTest, testWriteToReadOnlyFile)
{
   File file(Latin1String("readonlyfile"));
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
   char c = 0;
   ASSERT_EQ(file.write(&c, 1), pdk::pint64(-1));
}

#if defined(PDK_OS_LINUX) || defined(PDK_OS_AIX) || defined(PDK_OS_FREEBSD) || defined(PDK_OS_NETBSD)
// This platform have 0-sized virtual files
TEST_F(FileTest, testVirtualFile)
{
   // test if File works with virtual files
   String fname;
#if defined(PDK_OS_LINUX)
   fname = Latin1String("/proc/self/maps");
#elif defined(PDK_OS_AIX)
   fname = String(Latin1String("/proc/%1/map").arg(getpid());
      #else // defined(PDK_OS_FREEBSD) || defined(PDK_OS_NETBSD)
   fname = Latin1String("/proc/curproc/map");
#endif
   
   // consistency check
   FileInfo fi(fname);
   ASSERT_TRUE(fi.exists()) << msg_file_does_not_exist(fname).getConstRawData();
   ASSERT_TRUE(fi.isFile());
   ASSERT_EQ(fi.size(), PDK_INT64_C(0));
   
   // open the file
   File f(fname);
   ASSERT_TRUE(f.open(IoDevice::OpenMode::ReadOnly)) << msg_open_failed(f).getConstRawData();
   if (EmulationDetector::isRunningArmOnX86()) {
      FAIL() << "QEMU does not read /proc/self/maps size correctly";
   }
   
   ASSERT_EQ(f.size(), PDK_INT64_C(0));
   if (EmulationDetector::isRunningArmOnX86()) {
      FAIL() << "QEMU does not read /proc/self/maps size correctly";
   }
   
   ASSERT_TRUE(f.atEnd());
   
   // read data
   ByteArray data = f.read(16);
   ASSERT_EQ(data.size(), 16);
   ASSERT_EQ(f.getPosition(), PDK_INT64_C(16));
   
   // line-reading
   data = f.readLine();
   ASSERT_TRUE(!data.isEmpty());
   
   // read all:
   data = f.readAll();
   ASSERT_TRUE(f.getPosition() != 0);
   ASSERT_TRUE(!data.isEmpty());
   
   // seeking
   ASSERT_TRUE(f.seek(1));
   ASSERT_EQ(f.getPosition(), PDK_INT64_C(1));
}
#endif

TEST_F(FileTest, testTextFile)
{
   //   const char *openMode = OperatingSystemVersion::current().type() != OperatingSystemVersion::Windows
   //         ? "w" : "wt";
   const char *openMode = "w";
   StdioFileGuard fs(fopen("writeabletextfile", openMode));
   ASSERT_TRUE(fs);
   File f;
   ByteArray part1("This\nis\na\nfile\nwith\nnewlines\n");
   ByteArray part2("Add\nsome\nmore\nnewlines\n");
   
   ASSERT_TRUE(f.open(fs, IoDevice::OpenMode::WriteOnly));
   f.write(part1);
   f.write(part2);
   f.close();
   fs.close();
   
   File file(Latin1String("writeabletextfile"));
   ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
   
   ByteArray data = file.readAll();
   
   ByteArray expected = part1 + part2;
#ifdef PDK_OS_WIN
   expected.replace("\n", "\015\012");
#endif
   ASSERT_EQ(data, expected);
   file.close();
}

namespace {

static const char sg_renameSourceFile[] = "renamefile";

void init_rename_data(std::list<std::tuple<String, String, bool>> &data)
{
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String("b")), false));
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String(".")), false));
   data.push_back(std::make_tuple(String::fromLatin1(sg_renameSourceFile) , String::fromLatin1(sg_renameSourceFile) , false));
   data.push_back(std::make_tuple(String(Latin1String("a")), String(Latin1String(".")), false));
#if defined(PDK_OS_UNIX)
   data.push_back(std::make_tuple(String::fromLatin1(sg_renameSourceFile), String(Latin1String("/etc/renamefile")), false));
#endif
   data.push_back(std::make_tuple(String::fromLatin1(sg_renameSourceFile), String(Latin1String("renamedfile")), true));
   data.push_back(std::make_tuple(String::fromLatin1(sg_renameSourceFile), String(Latin1String("..")), false));
   data.push_back(std::make_tuple(String::fromLatin1(sg_renameSourceFile), StringLiteral("dfsadfsadf"), true));
}

} // anonymous namespace

TEST_F(FileTest, testRename)
{
   std::list<std::tuple<String, String, bool>> data;
   init_rename_data(data);
   for (auto &item : data) {
      String &source = std::get<0>(item);
      String &destination = std::get<1>(item);
      bool result = std::get<2>(item);
      const ByteArray content = ByteArrayLiteral("testdatacontent") + Time::getCurrentTime().toString().toLatin1();
      
#if defined(PDK_OS_UNIX)
      if (::getuid() == 0) {
         std::cout << "Running this test as root doesn't make sense" << std::endl;
         return;
      }
#endif
      
      const String sourceFileName = String::fromLatin1(sg_renameSourceFile);
      File sourceFile(sourceFileName);
      ASSERT_TRUE(sourceFile.open(File::OpenModes(File::OpenMode::WriteOnly) | File::OpenMode::Text)) << pdk_printable(sourceFile.getErrorString());
      ASSERT_TRUE(sourceFile.write(content)) << pdk_printable(sourceFile.getErrorString());
      sourceFile.close();
      File file(source);
      const bool success = file.rename(destination);
      if (result) {
         ASSERT_TRUE(success) << pdk_printable(file.getErrorString());
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         // This will report the source file still existing for a rename changing the case
         // on Windows, Mac.
         if (sourceFileName.compare(destination, pdk::CaseSensitivity::Insensitive)) {
            ASSERT_TRUE(!sourceFile.exists());
         }
         File destinationFile(destination);
         ASSERT_TRUE(destinationFile.open(File::OpenModes(File::OpenMode::ReadOnly) | File::OpenMode::Text)) << pdk_printable(destinationFile.getErrorString());
         ASSERT_EQ(destinationFile.readAll(), content);
         destinationFile.close();
      } else {
         ASSERT_TRUE(!success);
         ASSERT_EQ(file.getError(), File::FileError::RenameError);
      }
   }
}

TEST_F(FileTest, testRenameWithAtEndSpecialFile)
{
   class PeculiarAtEnd : public File
   {
   public:
      virtual bool atEnd() const
      {
         return true;
      }
   };
   
   const String newName(Latin1String("newName.txt"));
   /* Cleanup, so we're a bit more robust. */
   File::remove(newName);
   
   const String originalName = StringLiteral("forRenaming.txt");
   // Copy from source tree
   if (!File::exists(originalName)) {
      ASSERT_TRUE(File::copy(m_forRenamingFile, originalName));
   }  
   PeculiarAtEnd file;
   file.setFileName(originalName);
   ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadOnly)) << pdk_printable(file.getErrorString());
   ASSERT_TRUE(file.rename(newName));
   file.close();
}

TEST_F(FileTest, testRenameFallback)
{
   // Using a resource file both to trigger File::rename's fallback handling
   // and as a *read-only* source whose move should fail.
   //   File file(":/rename-fallback.prc");
   //   ASSERT_TRUE(file.exists() && "(test-precondition)");
   //   File::remove("file-rename-destination.txt");
   
   //   ASSERT_TRUE(!file.rename(Latin1String("file-rename-destination.txt"));
   //   ASSERT_TRUE(!File::exists(Latin1String("file-rename-destination.txt")));
   //   ASSERT_TRUE(!file.isOpen());
}

TEST_F(FileTest, testRenameMultiple)
{
   // create the file if it doesn't exist
   File file(Latin1String("file-to-be-renamed.txt"));
   File file2(Latin1String("existing-file.txt"));
   ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadWrite)) << msg_open_failed(file).getConstRawData();
   ASSERT_TRUE(file2.open(IoDevice::OpenMode::ReadWrite)) << msg_open_failed(file2).getConstRawData();
   
   // any stale files from previous test failures?
   File::remove(Latin1String("file-renamed-once.txt"));
   File::remove(Latin1String("file-renamed-twice.txt"));
   
   // begin testing
   ASSERT_TRUE(File::exists(Latin1String("existing-file.txt")));
   ASSERT_TRUE(!file.rename(Latin1String("existing-file.txt")));
   ASSERT_EQ(file.getError(), File::FileError::RenameError);
   ASSERT_EQ(file.getFileName(), String(Latin1String("file-to-be-renamed.txt")));
   
   ASSERT_TRUE(file.rename(Latin1String("file-renamed-once.txt")));
   ASSERT_TRUE(!file.isOpen());
   ASSERT_EQ(file.getFileName(), String(Latin1String("file-renamed-once.txt")));
   
   ASSERT_TRUE(File::exists(Latin1String("existing-file.txt")));
   ASSERT_TRUE(!file.rename(Latin1String("existing-file.txt")));
   ASSERT_EQ(file.getError(), File::FileError::RenameError);
   ASSERT_EQ(file.getFileName(), String(Latin1String("file-renamed-once.txt")));
   
   ASSERT_TRUE(file.rename(Latin1String("file-renamed-twice.txt")));
   ASSERT_TRUE(!file.isOpen());
   ASSERT_EQ(file.getFileName(), String(Latin1String("file-renamed-twice.txt")));
   
   ASSERT_TRUE(File::exists(Latin1String("existing-file.txt")));
   ASSERT_TRUE(!File::exists(Latin1String("file-to-be-renamed.txt")));
   ASSERT_TRUE(!File::exists(Latin1String("file-renamed-once.txt")));
   ASSERT_TRUE(File::exists(Latin1String("file-renamed-twice.txt")));
   
   file.remove();
   file2.remove();
   ASSERT_TRUE(!File::exists(Latin1String("file-renamed-twice.txt")));
   ASSERT_TRUE(!File::exists(Latin1String("existing-file.txt")));
}

TEST_F(FileTest, testAppendAndRead)
{
   File writeFile(Latin1String("appendfile.txt"));
   ASSERT_TRUE(writeFile.open(IoDevice::OpenModes(IoDevice::OpenMode::WriteOnly) | IoDevice::OpenMode::Truncate)) << msg_open_failed(writeFile).getConstRawData();
   
   File readFile(Latin1String("appendfile.txt"));
   ASSERT_TRUE(readFile.open(IoDevice::OpenMode::ReadOnly)) << msg_open_failed(readFile).getConstRawData();
   
   // Write to the end of the file, then read that character back, and so on.
   for (int i = 0; i < 100; ++i) {
      char c = '\0';
      writeFile.putChar(char(i % 256));
      writeFile.flush();
      ASSERT_TRUE(readFile.getChar(&c));
      ASSERT_EQ(c, char(i % 256));
      ASSERT_EQ(readFile.getPosition(), writeFile.getPosition());
   }
   
   // Write blocks and read them back
   for (int j = 0; j < 18; ++j) {
      const int size = 1 << j;
      writeFile.write(ByteArray(size, '@'));
      writeFile.flush();
      ASSERT_EQ(readFile.read(size).size(), size);
   }
   
   readFile.close();
}

TEST_F(FileTest, testmiscWithUncPathAsCurrentDir)
{
   // @TODO add Windows platform testcases
}

TEST_F(FileTest, testStandarderror)
{
   File f;
   bool ok = f.open(stderr, File::OpenMode::WriteOnly);
   ASSERT_TRUE(ok);
   f.close();
}

TEST_F(FileTest, testHandle)
{
   int fd;
   File file(m_testSourceFile);
   ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
   fd = int(file.getHandle());
   ASSERT_TRUE(fd > 2);
   ASSERT_EQ(int(file.getHandle()), fd);
   char c = '\0';
   const auto readResult = PDK_READ(int(file.getHandle()), &c, 1);
   ASSERT_EQ(readResult, static_cast<decltype(readResult)>(1));
   ASSERT_EQ(c, '/');
   
   // test if the File and the handle remain in sync
   ASSERT_TRUE(file.getChar(&c));
   ASSERT_EQ(c, '/');
   
   // same, but read from File first now
   file.close();
   ASSERT_TRUE(file.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | IoDevice::OpenMode::Unbuffered)) << msg_open_failed(file).getConstRawData();
   fd = int(file.getHandle());
   ASSERT_TRUE(fd > 2);
   ASSERT_TRUE(file.getChar(&c));
   ASSERT_EQ(c, '/');
#ifdef PDK_OS_UNIX
   ASSERT_EQ(PDK_READ(fd, &c, 1), ssize_t(1));
#else
   ASSERT_EQ(PDK_READ(fd, &c, 1), 1);
#endif
   
   ASSERT_EQ(c, '/');
   
   //test round trip of adopted stdio file handle
   File file2;
   StdioFileGuard fp(fopen(pdk_printable(m_testSourceFile), "r"));
   ASSERT_TRUE(fp);
   file2.open(fp, IoDevice::OpenMode::ReadOnly);
   ASSERT_EQ(int(file2.getHandle()), int(fileno(fp)));
   ASSERT_EQ(int(file2.getHandle()), int(fileno(fp)));
   fp.close();
   
   //test round trip of adopted posix file handle
#ifdef PDK_OS_UNIX
   File file3;
   fd = PDK_OPEN(pdk_printable(m_testSourceFile), PDK_OPEN_RDONLY);
   file3.open(fd, IoDevice::OpenMode::ReadOnly);
   ASSERT_EQ(int(file3.getHandle()), fd);
   PDK_CLOSE(fd);
#endif
}

TEST_F(FileTest, testNativeHandleLeaks)
{
   int fd1, fd2;
   
#ifdef PDK_OS_WIN
   HANDLE handle1, handle2;
#endif
   
   {
      File file(Latin1String("pdk_file.tmp"));
      ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadWrite)) << msg_open_failed(file).getConstRawData();
      fd1 = file.getHandle();
      ASSERT_TRUE(-1 != fd1);
   }
   
#ifdef PDK_OS_WIN
   handle1 = ::CreateFileA("pdk_file.tmp", GENERIC_READ, 0, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   ASSERT_TRUE(INVALID_HANDLE_VALUE != handle1);
   ASSERT_TRUE(::CloseHandle(handle1));
#endif
   
   {
      File file(Latin1String("pdk_file.tmp"));
      ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadOnly)) << msg_open_failed(file).getConstRawData();
      fd2 = file.getHandle();
      ASSERT_TRUE(-1 != fd2);
   }
   
#ifdef PDK_OS_WIN
   handle2 = ::CreateFileA("pdk_file.tmp", GENERIC_READ, 0, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   ASSERT_TRUE(INVALID_HANDLE_VALUE != handle2);
   ASSERT_TRUE(::CloseHandle(handle2));
#endif
   ASSERT_EQ( fd2, fd1 );
#ifdef PDK_OS_WIN
   ASSERT_EQ(handle2, handle1);
#endif
}

namespace
{
void init_read_eof_data(std::list<std::tuple<String, IoDevice::OpenMode>> &data)
{
   data.push_back(std::make_tuple(sg_testFile, IoDevice::OpenMode::NotOpen));
   data.push_back(std::make_tuple(sg_testFile, IoDevice::OpenMode::Unbuffered));
   
#if defined(PDK_OS_UNIX)
   data.push_back(std::make_tuple(Latin1String("/dev/null"), IoDevice::OpenMode::NotOpen));
   data.push_back(std::make_tuple(Latin1String("/dev/null"), IoDevice::OpenMode::Unbuffered));
#endif
}
} // anonymous namespace

TEST_F(FileTest, testReadEof)
{
   std::list<std::tuple<String, IoDevice::OpenMode>> data;
   init_read_eof_data(data);
   for (auto &item : data) {
      String &filename = std::get<0>(item);
      IoDevice::OpenMode imode = std::get<1>(item);
      IoDevice::OpenMode mode = IoDevice::OpenMode(imode);
      
      {
         File file(filename);
         ASSERT_TRUE(file.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | mode)) << msg_open_failed(file).getConstRawData();
         bool isSequential = file.isSequential();
         if (!isSequential) {
            ASSERT_TRUE(file.seek(245));
            ASSERT_TRUE(file.atEnd());
         }
         
         char buf[10];
         int ret = file.read(buf, sizeof buf);
         ASSERT_EQ(ret, 0);
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         ASSERT_TRUE(file.atEnd());
         
         // Do it again to ensure that we get the same result
         ret = file.read(buf, sizeof buf);
         ASSERT_EQ(ret, 0);
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         ASSERT_TRUE(file.atEnd());
      }
      
      {
         File file(filename);
         ASSERT_TRUE(file.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | mode)) << msg_open_failed(file).getConstRawData();
         bool isSequential = file.isSequential();
         if (!isSequential) {
            ASSERT_TRUE(file.seek(245));
            ASSERT_TRUE(file.atEnd());
         }
         
         ByteArray ret = file.read(10);
         ASSERT_TRUE(ret.isEmpty());
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         ASSERT_TRUE(file.atEnd());
         
         // Do it again to ensure that we get the same result
         ret = file.read(10);
         ASSERT_TRUE(ret.isEmpty());
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         ASSERT_TRUE(file.atEnd());
      }
      
      {
         File file(filename);
         ASSERT_TRUE(file.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | mode)) << msg_open_failed(file).getConstRawData();
         bool isSequential = file.isSequential();
         if (!isSequential) {
            ASSERT_TRUE(file.seek(245));
            ASSERT_TRUE(file.atEnd());
         }
         
         char buf[10];
         int ret = file.readLine(buf, sizeof buf);
         ASSERT_EQ(ret, -1);
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         ASSERT_TRUE(file.atEnd());
         
         // Do it again to ensure that we get the same result
         ret = file.readLine(buf, sizeof buf);
         ASSERT_EQ(ret, -1);
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         ASSERT_TRUE(file.atEnd());
      }
      
      {
         File file(filename);
         ASSERT_TRUE(file.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | mode)) << msg_open_failed(file).getConstRawData();
         bool isSequential = file.isSequential();
         if (!isSequential) {
            ASSERT_TRUE(file.seek(245));
            ASSERT_TRUE(file.atEnd());
         }
         
         ByteArray ret = file.readLine();
         ASSERT_TRUE(ret.isNull());
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         ASSERT_TRUE(file.atEnd());
         
         // Do it again to ensure that we get the same result
         ret = file.readLine();
         ASSERT_TRUE(ret.isNull());
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         ASSERT_TRUE(file.atEnd());
      }
      
      {
         File file(filename);
         ASSERT_TRUE(file.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | mode)) << msg_open_failed(file).getConstRawData();
         bool isSequential = file.isSequential();
         if (!isSequential) {
            ASSERT_TRUE(file.seek(245));
            ASSERT_TRUE(file.atEnd());
         }
         
         char c;
         ASSERT_TRUE(!file.getChar(&c));
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         ASSERT_TRUE(file.atEnd());
         
         // Do it again to ensure that we get the same result
         ASSERT_TRUE(!file.getChar(&c));
         ASSERT_EQ(file.getError(), File::FileError::NoError);
         ASSERT_TRUE(file.atEnd());
      }
   }
   
}

TEST_F(FileTest, testPosAfterFailedStat)
{
   // Regression test for a bug introduced in 4.3.0; after a failed stat,
   // pos() could no longer be calculated correctly.
   File::remove(Latin1String("tmp.txt"));
   File file(Latin1String("tmp.txt"));
   ASSERT_TRUE(!file.exists());
   ASSERT_TRUE(file.open(IoDevice::OpenMode::Append)) << msg_open_failed(file).getConstRawData();
   ASSERT_TRUE(file.exists());
   file.write("pdk510", 5);
   ASSERT_TRUE(!file.isSequential());
   ASSERT_EQ(file.getPosition(), pdk::pint64(5));
   file.remove();
}

#define FILESIZE 65536 * 3

namespace {

void init_map_data(std::list<std::tuple<int, int, int, File::FileError>> &data)
{
   data.push_back(std::make_tuple(FILESIZE, 0, FILESIZE, File::FileError::NoError));
   
   //   QTest::newRow("zero")         << FILESIZE << 0     << FILESIZE         << File::NoError;
   //   QTest::newRow("small, but 0") << FILESIZE << 30    << FILESIZE - 30    << File::NoError;
   //   QTest::newRow("a page")       << FILESIZE << 4096  << FILESIZE - 4096  << File::NoError;
   //   QTest::newRow("+page")        << FILESIZE << 5000  << FILESIZE - 5000  << File::NoError;
   //   QTest::newRow("++page")       << FILESIZE << 65576 << FILESIZE - 65576 << File::NoError;
   //   QTest::newRow("bad size")     << FILESIZE << 0     << -1               << File::ResourceError;
   //   QTest::newRow("bad offset")   << FILESIZE << -1    << 1                << File::UnspecifiedError;
   //   QTest::newRow("zerozero")     << FILESIZE << 0     << 0                << File::UnspecifiedError;
}

} // anonymous namespace

TEST_F(FileTest, testMap)
{
   std::list<std::tuple<int, int, int, File::FileError>> data;
   init_map_data(data);
   for (auto &item : data) {
      int fileSize = std::get<0>(item);
      int offset = std::get<1>(item);
      int size = std::get<2>(item);
      File::FileError error = std::get<3>(item);
      String fileName = Dir::getCurrentPath() + Latin1String("/file_map_testfile");
      
      if (File::exists(fileName)) {
         ASSERT_TRUE(File::setPermissions(fileName,
                                          File::Permissions(File::Permission::WriteOwner) | File::Permission::ReadOwner | 
                                          File::Permission::WriteUser | File::Permission::ReadUser));
         File::remove(fileName);
      }
      File file(fileName);
      
      // invalid, not open
      uchar *memory = file.map(0, size);
      ASSERT_TRUE(!memory);
      ASSERT_EQ(file.getError(), File::FileError::PermissionsError);
      ASSERT_TRUE(!file.unmap(memory));
      ASSERT_EQ(file.getError(), File::FileError::PermissionsError);
      
      // make a file
      ASSERT_TRUE(file.open(File::OpenMode::ReadWrite)) << msg_open_failed(file).getConstRawData();
      ASSERT_TRUE(file.resize(fileSize));
      ASSERT_TRUE(file.flush());
      file.close();
      ASSERT_TRUE(file.open(File::OpenMode::ReadWrite)) << msg_open_failed(file).getConstRawData();
      memory = file.map(offset, size);
      if (error != File::FileError::NoError) {
         ASSERT_TRUE(file.getError() != File::FileError::NoError);
         return;
      }
      
      ASSERT_EQ(file.getError(), error);
      ASSERT_TRUE(memory);
      memory[0] = 'Q';
      ASSERT_TRUE(file.unmap(memory));
      ASSERT_EQ(file.getError(), File::FileError::NoError);
      
      // Verify changes were saved
      memory = file.map(offset, size);
      ASSERT_EQ(file.getError(), File::FileError::NoError);
      ASSERT_TRUE(memory);
      ASSERT_EQ(memory[0], uchar('Q'));
      ASSERT_TRUE(file.unmap(memory));
      ASSERT_EQ(file.getError(), File::FileError::NoError);
      
      // hpux won't let you map multiple times.
#if !defined(PDK_OS_HPUX) && !defined(PDK_USE_DEPRECATED_MAP_API)
      // exotic test to make sure that multiple maps work
      
      // note: windows ce does not reference count mutliple maps
      // it's essentially just the same reference but it
      // cause a resource lock on the file which prevents it
      // from being removed    uchar *memory1 = file.map(0, file.size());
      uchar *memory1 = file.map(0, file.getSize());
      ASSERT_EQ(file.getError(), File::FileError::NoError);
      uchar *memory2 = file.map(0, file.getSize());
      ASSERT_EQ(file.getError(), File::FileError::NoError);
      ASSERT_TRUE(memory1);
      ASSERT_TRUE(memory2);
      ASSERT_TRUE(file.unmap(memory1));
      ASSERT_EQ(file.getError(), File::FileError::NoError);
      ASSERT_TRUE(file.unmap(memory2));
      ASSERT_EQ(file.getError(), File::FileError::NoError);
      memory1 = file.map(0, file.getSize());
      ASSERT_EQ(file.getError(), File::FileError::NoError);
      ASSERT_TRUE(memory1);
      ASSERT_TRUE(file.unmap(memory1));
      ASSERT_EQ(file.getError(), File::FileError::NoError);
#endif
      
      file.close();
      
#if !defined(PDK_OS_VXWORKS)
#if defined(PDK_OS_UNIX)
      if (::getuid() != 0)
         // root always has permissions
#endif
      {
         // Change permissions on a file, just to confirm it would fail
         File::Permissions originalPermissions = file.getPermissions();
         ASSERT_TRUE(file.setPermissions(File::Permission::ReadOther));
         ASSERT_TRUE(!file.open(File::OpenMode::ReadWrite));
         memory = file.map(offset, size);
         ASSERT_EQ(file.getError(), File::FileError::PermissionsError);
         ASSERT_TRUE(!memory);
         ASSERT_TRUE(file.setPermissions(originalPermissions));
      }
#endif
      ASSERT_TRUE(file.remove());
   }
}
