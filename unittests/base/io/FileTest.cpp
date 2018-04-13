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
#define PDKTEST_FINDTEST_SRC_DATA(subDir) Latin1String(PDKTEST_CURRENT_TEST_SOURCE_DIR PDKTEST_DIR_SEP PDKTEST_FILETEST_SUBDIR PDKTEST_DIR_SEP subDir)

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
      std::cout <<  "---" << m_temporaryDir.getPath().toStdString() <<std::endl;
      Dir::setCurrent(m_temporaryDir.getPath());
      std::cout <<  "---" << Dir::getCurrentPath().toStdString() <<std::endl;
   }
   
   ~FileTest() {
      //cleanup();
   }
   
//   void cleanup()
//   {
//      if (-1 != m_fd) {
//         PDK_CLOSE(m_fd);
//      }
//      m_fd = -1;
//      if (m_stream) {
//         ::fclose(m_stream);
//      }
//      m_stream = 0;
//      // Windows UNC tests set a different working directory which might not be restored on failures.
//      std::cout << Dir::getCurrentPath().toStdString() << "---" << m_temporaryDir.getPath().toStdString() <<std::endl;
//      ASSERT_EQ(Dir::getCurrentPath(), m_temporaryDir.getPath());
//      // Clean out everything except the readonly-files.
//      const Dir dir(m_temporaryDir.getPath());
//      for (const FileInfo &fi: dir.entryInfoList(Dir::Filters(Dir::Filter::AllEntries) | Dir::Filter::NoDotAndDotDot)) {
//         const String fileName = fi.getFileName();
//         std::cout << fileName.toStdString() << std::endl;
//         if (fileName != Latin1String(sg_noReadFile) && fileName != Latin1String(sg_readOnlyFile)) {
//            const String absoluteFilePath = fi.getAbsoluteFilePath();
//            if (fi.isDir() && !fi.isSymLink()) {
//               Dir remainingDir(absoluteFilePath);
//               ASSERT_TRUE(remainingDir.removeRecursively()) << pdk_printable(absoluteFilePath);
//            } else {
//               if (!(File::permissions(absoluteFilePath) & File::Permission::WriteUser)) {
//                  ASSERT_TRUE(File::setPermissions(absoluteFilePath, File::Permission::WriteUser)) << pdk_printable(absoluteFilePath);
//               }
//               ASSERT_TRUE(File::remove(absoluteFilePath)) << pdk_printable(absoluteFilePath);
//            }
//         }
//      }
//   }
   
//   // Sets up the test fixture.
//   void SetUp()
//   {
//      ASSERT_TRUE(m_temporaryDir.isValid()) << pdk_printable(m_temporaryDir.getErrorString());
//      // @TODO add process support testcase
//      m_testLogFile = PDKTEST_FINDTEST_SRC_DATA("testlog.txt");
//      ASSERT_TRUE(!m_testLogFile.isEmpty());
//      m_dosFile = PDKTEST_FINDTEST_SRC_DATA("dosfile.txt");
//      ASSERT_TRUE(!m_dosFile.isEmpty());
//      m_forCopyingFile = PDKTEST_FINDTEST_SRC_DATA("forCopying.txt");
//      ASSERT_TRUE(!m_forCopyingFile .isEmpty());
//      m_forRenamingFile = PDKTEST_FINDTEST_SRC_DATA("forRenaming.txt");
//      ASSERT_TRUE(!m_forRenamingFile.isEmpty());
//      m_twoDotsFile = PDKTEST_FINDTEST_SRC_DATA("two.dots.file");
//      ASSERT_TRUE(!m_twoDotsFile.isEmpty());
      
//      m_testSourceFile = PDKTEST_FINDTEST_SRC_DATA("FileTest.cpp");
//      ASSERT_TRUE(!m_testSourceFile.isEmpty());
//      m_testFile = PDKTEST_FINDTEST_SRC_DATA("testfile.txt");
//      ASSERT_TRUE(!m_testFile.isEmpty());
//      m_resourcesDir = PDKTEST_FINDTEST_SRC_DATA("resources");
//      ASSERT_TRUE(!m_resourcesDir.isEmpty());
      
//      m_noEndOfLineFile = PDKTEST_FINDTEST_SRC_DATA("noendofline.txt");
//      ASSERT_TRUE(!m_noEndOfLineFile.isEmpty());
      
//      ASSERT_TRUE(Dir::setCurrent(m_temporaryDir.getPath()));
      
//      // create a file and make it read-only
//      File file(String::fromLatin1(sg_readOnlyFile));
//      ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
//      file.write("a", 1);
//      file.close();
//      ASSERT_TRUE(file.setPermissions(File::Permission::ReadOwner)) << pdk_printable(file.getErrorString());
//      // create another file and make it not readable
//      file.setFileName(String::fromLatin1(sg_noReadFile));
//      ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
//      file.write("b", 1);
//      file.close();
//#ifndef PDK_OS_WIN // Not supported on Windows.
//      ASSERT_TRUE(file.setPermissions(0)) << pdk_printable(file.getErrorString());
//#else
//      ASSERT_TRUE(file.open(File::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
//#endif
      
//   }
   
//   // Tears down the test fixture.
//   void TearDown()
//   {
//      File file(String::fromLatin1(sg_readOnlyFile));
//      ASSERT_TRUE(file.setPermissions(File::Permissions(File::Permission::ReadOwner) | File::Permission::WriteOwner));
//      file.setFileName(String::fromLatin1(sg_noReadFile));
//      ASSERT_TRUE(file.setPermissions(File::Permissions(File::Permission::ReadOwner) | File::Permission::WriteOwner));
//   }
   
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
   std::cout << "1111111111111111" << std::endl;
////   File f(m_testFile);
////   ASSERT_TRUE(f.exists()) << msg_file_does_not_exist(m_testFile).getConstRawData();
   
////   File file(Latin1String("nobodyhassuchafile"));
////   file.remove();
////   ASSERT_TRUE(!file.exists());
   
////   File file2(Latin1String("nobodyhassuchafile"));
////   ASSERT_TRUE(file2.open(IoDevice::OpenMode::WriteOnly)) << msg_open_failed(file2).getConstRawData();
////   file2.close();
   
////   ASSERT_TRUE(file.exists());
   
////   ASSERT_TRUE(file.open(IoDevice::OpenMode::WriteOnly)) << msg_open_failed(file).getConstRawData();
////   file.close();
////   ASSERT_TRUE(file.exists());
   
////   file.remove();
////   ASSERT_TRUE(!file.exists());
   
//   // @TODO Windows platform testcase
//   ASSERT_TRUE(Dir::setCurrent(m_temporaryDir.getPath()));
}

//namespace {

//void init_open_data(std::list<std::tuple<String, IoDevice::OpenMode, bool, File::FileError>> &data)
//{
//   data.push_back(std::make_tuple(PDKTEST_FINDTEST_SRC_DATA("testfile.txt"),
//                                  IoDevice::OpenMode::ReadOnly, true, File::FileError::NoError));
//}

//} // anonymous namespace

//TEST_F(FileTest, testXXXXXX)
//{
//   std::cout << "222222222" << std::endl;
////   std::list<std::tuple<String, IoDevice::OpenMode, bool, File::FileError>> data;
////   init_open_data(data);
////   for (auto &item : data) {
//////      String &filename = std::get<0>(item);
//////      File::OpenMode mode = std::get<1>(item);
//////      bool ok = std::get<2>(item);
//////      File::FileError status = std::get<3>(item);
//////      File f(filename);
//////#if defined(PDK_OS_UNIX) && !defined(PDK_OS_VXWORKS)
//////      if (::getuid() == 0) {
//////         // root and Chuck Norris don't care for file permissions. Skip.
//////         std::cout << "Running this test as root doesn't make sense";
//////         SUCCEED();
//////         return;
//////      }
      
//////#endif
      
//////#if defined(PDK_OS_WIN32)
//////      FAIL("noreadfile") << "Windows does not currently support non-readable files.";
//////      return;
//////#endif
//////      const IoDevice::OpenMode om(mode);
//////      const bool succeeded = f.open(om);
//////      if (ok) {
//////         ASSERT_TRUE(succeeded) << msg_open_failed(om, f).getConstRawData();
//////      } else {
//////         ASSERT_TRUE(!succeeded);
//////      }
//////      ASSERT_EQ(f.getError(), status);
////   }
////   ASSERT_TRUE(true);
//   ASSERT_TRUE(Dir::setCurrent(m_temporaryDir.getPath()));
//}
