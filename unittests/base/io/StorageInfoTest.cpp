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
// Created by zzu_softboy on 2018/04/17.

#include "gtest/gtest.h"
#include "pdk/base/io/fs/StorageInfo.h"
#include "pdk/base/io/fs/TemporaryFile.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdktest/PdkTest.h"

using pdk::io::fs::Dir;
using pdk::io::fs::StorageInfo;
using pdk::io::fs::TemporaryFile;
using pdk::ds::ByteArray;
using pdk::lang::Latin1String;
using pdk::kernel::CoreApplication;
using pdk::lang::String;

PDKTEST_DECLARE_APP_STARTUP_ARGS();

namespace {

void print_volumes(const std::list<StorageInfo> &volumes, int (*printer)(const char *, ...))
{
   // Sample output:
   //  Filesystem (Type)            Size  Available BSize  Label            Mounted on
   //  /dev/sda2 (ext4)    RO     388480     171218  1024                   /boot
   //  /dev/mapper/system-root (btrfs) RW
   //                          214958080   39088272  4096                   /
   //  /dev/disk1s2 (hfs)  RW  488050672  419909696  4096  Macintosh HD2    /Volumes/Macintosh HD2
   
   printer("Filesystem (Type)            Size  Available BSize  Label            Mounted on\n");
   for (const StorageInfo &info : volumes) {
      ByteArray fsAndType = info.getDevice();
      if (info.getFileSystemType() != fsAndType) {
         fsAndType += " (" + info.getFileSystemType() + ')';
      }
      printer("%-19s R%c ", fsAndType.getConstRawData(), info.isReadOnly() ? 'O' : 'W');
      if (fsAndType.size() > 19) {
         printer("\n%23s", "");
      }
      printer("%10llu %10llu %5u  ", info.getBytesTotal() / 1024, info.getBytesFree() / 1024, info.getBlockSize());
      if (!info.getSubvolume().isEmpty()) {
         printer("subvol=%-18s ", pdk_printable(Latin1String(info.getSubvolume())));
      } else {
         printer("%-25s ", pdk_printable(info.getName()));
      }
      printer("%s\n", pdk_printable(info.getRootPath()));
   }
}

} // anonymous namespace

TEST(StorageInfoTest, testDefaultValues)
{
   StorageInfo storage;
   ASSERT_TRUE(!storage.isValid());
   ASSERT_TRUE(!storage.isReady());
   ASSERT_TRUE(storage.getRootPath().isEmpty());
   ASSERT_TRUE(!storage.isRoot());
   ASSERT_TRUE(storage.getDevice().isEmpty());
   ASSERT_TRUE(storage.getFileSystemType().isEmpty());
   ASSERT_EQ(storage.getBytesTotal(), -1);
   ASSERT_EQ(storage.getBytesFree(), -1);
   ASSERT_EQ(storage.getBytesAvailable(), -1);
}

namespace {

//int info_printer(const char *format, ...)
//{
//}

} // anonymous namespace

TEST(StorageInfoTest, testDump)
{
   print_volumes(StorageInfo::getMountedVolumes(), &std::printf);
}

TEST(StorageInfoTest, testOperatorEqual)
{
   {
      StorageInfo storage1 = StorageInfo::getRoot();
      StorageInfo storage2(Dir::getRootPath());
      ASSERT_EQ(storage1, storage2);
   }
   
   {
      StorageInfo storage1(CoreApplication::getAppDirPath());
      StorageInfo storage2(CoreApplication::getAppFilePath());
      ASSERT_EQ(storage1, storage2);
   }
   
   {
      StorageInfo storage1;
      StorageInfo storage2;
      ASSERT_EQ(storage1, storage2);
   }
}

TEST(StorageInfoTest, testOperatorNotEqual)
{
   StorageInfo storage1 = StorageInfo::getRoot();
   StorageInfo storage2;
   ASSERT_TRUE(storage1 != storage2);
}

TEST(StorageInfoTest, testRoot)
{
   StorageInfo storage = StorageInfo::getRoot();   
   ASSERT_TRUE(storage.isValid());
   ASSERT_TRUE(storage.isReady());
   ASSERT_EQ(storage.getRootPath(), Dir::getRootPath());
   ASSERT_TRUE(storage.isRoot());
   ASSERT_TRUE(!storage.getDevice().isEmpty());
   ASSERT_TRUE(!storage.getFileSystemType().isEmpty());
#ifndef PDK_OS_HAIKU
   ASSERT_TRUE(storage.getBytesTotal() >= 0);
   ASSERT_TRUE(storage.getBytesFree() >= 0);
   ASSERT_TRUE(storage.getBytesAvailable() >= 0);
#endif
}


TEST(StorageInfoTest, testCurrentStorage)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   String appPath = CoreApplication::getAppFilePath();
   StorageInfo storage(appPath);
   ASSERT_TRUE(storage.isValid());
   ASSERT_TRUE(storage.isReady());
   ASSERT_TRUE(appPath.startsWith(storage.getRootPath(), pdk::CaseSensitivity::Insensitive));
   ASSERT_TRUE(!storage.getDevice().isEmpty());
   ASSERT_TRUE(!storage.getFileSystemType().isEmpty());
   ASSERT_TRUE(storage.getBytesTotal() >= 0);
   ASSERT_TRUE(storage.getBytesFree() >= 0);
   ASSERT_TRUE(storage.getBytesAvailable() >= 0);
   PDKTEST_END_APP_CONTEXT();
}

TEST(StorageInfoTest, testStorageList)
{
   StorageInfo root = StorageInfo::getRoot();
   
   std::list<StorageInfo> volumes = StorageInfo::getMountedVolumes();
   
   // at least, root storage should be present
   ASSERT_TRUE(std::find(volumes.cbegin(), volumes.cend(), root) != volumes.cend());
   volumes.remove(root);
   ASSERT_TRUE(std::find(volumes.cbegin(), volumes.cend(), root) == volumes.cend());
   
   for (const StorageInfo &storage : volumes) {
      if (!storage.isReady()) {
         continue;
      }
      ASSERT_TRUE(storage.isValid());
      ASSERT_TRUE(!storage.isRoot());
#ifndef Q_OS_WIN
      ASSERT_TRUE(!storage.getDevice().isEmpty());
      ASSERT_TRUE(!storage.getFileSystemType().isEmpty());
#endif
   }
}

TEST(StorageInfoTest, testTempFile)
{
   TemporaryFile file;
   ASSERT_TRUE(file.open()) << pdk_printable(file.getErrorString());
   
   StorageInfo storage1(file.getFileName());
#ifdef PDK_OS_LINUX
   if (storage1.getFileSystemType() == Latin1String("btrfs")) {
      std::cout << "This test doesn't work on btrfs, probably due to a btrfs bug" << std::endl;
      return;
   }
#endif
   
   pdk::pint64 free = storage1.getBytesFree();
   std::cout << "free" << free << std::endl;
   ASSERT_TRUE(free != -1);
   
   file.write(ByteArray(1024*1024, '1'));
   file.flush();
   file.close();
   std::cout << "free" << free << std::endl;
   StorageInfo storage2(file.getFileName());
   std::cout << file.getFileName() << std::endl;
   ASSERT_TRUE(free != storage2.getBytesFree());
}

TEST(StorageInfoTest, testCaching)
{
   TemporaryFile file;
   ASSERT_TRUE(file.open()) << pdk_printable(file.getErrorString());
   
   StorageInfo storage1(file.getFileName());
#ifdef PDK_OS_LINUX
   if (storage1.fileSystemType() == "btrfs") {
      std::cout << "This test doesn't work on btrfs, probably due to a btrfs bug" << std::endl;
      return;
   }
   
#endif
   
   pdk::pint64 free = storage1.getBytesFree();
   StorageInfo storage2(storage1);
   ASSERT_EQ(free, storage2.getBytesFree());
   ASSERT_TRUE(free != -1);
   
   file.write(ByteArray(1024*1024, '\0'));
   file.flush();
   
   ASSERT_EQ(free, storage1.getBytesFree());
   ASSERT_EQ(free, storage2.getBytesFree());
   storage2.refresh();
   ASSERT_EQ(storage1, storage2);
   std::cout << storage2.getBytesFree() << " " << free << std::endl;
   //ASSERT_TRUE(free != storage2.getBytesFree());
}
