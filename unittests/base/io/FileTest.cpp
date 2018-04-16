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
#include "pdk/base/io/fs/TemporaryDir.h"
#include "pdktest/PdkTest.h"

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
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::ds::StringList;
using pdk::io::fs::TemporaryDir;
using pdk::io::Debug;
using pdk::kernel::CoreApplication;

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

//TEST_F(FileTest, testAbsolutePathLinkToRelativePath)
//{
//   File::remove(Latin1String("myDir/test.txt"));
//   File::remove(Latin1String("myDir/myLink.lnk"));
//   Dir dir;
//   dir.mkdir(Latin1String("myDir"));
//   File(Latin1String("myDir/test.txt")).open(File::OpenMode::WriteOnly);
//   // @TODO add Windows platform testcase
//   ASSERT_TRUE(File::link(Latin1String("myDir/test.txt"), Latin1String("myDir/myLink.lnk")));
//   ASSERT_EQ(FileInfo(File(FileInfo(Latin1String("myDir/myLink.lnk")).getAbsoluteFilePath()).getSymLinkTarget()).getAbsoluteFilePath(),
//             FileInfo(Latin1String("myDir/test.txt")).getAbsoluteFilePath());
//}

TEST_F(FileTest, testReadBrokenLink)
{
   File::remove(Latin1String("myLink2.lnk"));
   FileInfo info1(Latin1String("file12"));
   std::cout << info1.isFile() << std::endl;
//   ASSERT_TRUE(File::link(Latin1String("file12"), Latin1String("myLink2.lnk")));
//   FileInfo info2(Latin1String("myLink2.lnk"));
//   ASSERT_TRUE(info2.isSymLink());
//   ASSERT_EQ(info2.getSymLinkTarget(), info1.getAbsoluteFilePath());
//   ASSERT_TRUE(File::remove(info2.getAbsoluteFilePath()));
//   ASSERT_TRUE(File::link(Latin1String("ole/.."), Latin1String("myLink2.lnk")));
//   std::cout << FileInfo(Latin1String("myLink2.lnk")).getSymLinkTarget() << std::endl;
//   std::cout << Dir::getCurrentPath() << std::endl;
   //ASSERT_EQ(FileInfo(Latin1String("myLink2.lnk")).getSymLinkTarget(), Dir::getCurrentPath());
}
