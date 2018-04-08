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
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::fs::FileInfo;
using pdk::ds::StringList;

#define PDKTEST_DIR_SEP "/"
#define PDKTEST_FINDTESTDATA(subDir) PDKTEST_CURRENT_TEST_DIR PDKTEST_DIR_SEP PDK_STRINGIFY(subDir)

PDK_UNITTEST_EXPORT String normalize_path_segments(const String &name, bool allowUncPaths,
                                                   bool *ok = nullptr);

namespace {

ByteArray msg_does_not_exist(const String &name)
{
   return (Latin1Character('"') + Dir::toNativeSeparators(name)
           + Latin1String("\" does not exist.")).toLocal8Bit();
}

class DirTest : public ::testing::Test
{
public:
   DirTest()
      : m_dataPath(FileInfo(Latin1String(PDKTEST_FINDTESTDATA("testData"))).getAbsolutePath())
   {
      EXPECT_TRUE(Dir::setCurrent(m_dataPath)) << pdk_printable(Latin1String("Could not chdir to ") + m_dataPath);
   }
   
   void SetUp()
   {
      EXPECT_TRUE(!m_dataPath.isEmpty()) << "test data not found";
   }
   
   void TearDown()
   {
      Dir(Dir::getCurrentPath() + Latin1String("/tmpdir")).removeRecursively();
   }
   
protected:
   String m_dataPath;
};

} // anonymous namespace

TEST_F(DirTest, testGetSetCheck)
{
   Dir obj1;
   // Filters Dir::getFilter()
   // void Dir::setFilter(Filters)
   obj1.setFilter(Dir::Filters(Dir::Filter::Dirs));
   ASSERT_EQ(Dir::Filters(Dir::Filter::Dirs), obj1.getFilter());
   obj1.setFilter(Dir::Filters(Dir::Filter::Dirs) | Dir::Filter::Files);
   ASSERT_EQ(Dir::Filters(Dir::Filter::Dirs) | Dir::Filter::Files, obj1.getFilter());
   obj1.setFilter(Dir::Filters(Dir::Filter::NoFilter));
   ASSERT_EQ(Dir::Filters(Dir::Filter::NoFilter), obj1.getFilter());
   
   // SortFlags Dir::getSorting()
   // void Dir::setSorting(SortFlags)   
   obj1.setSorting(Dir::SortFlags(Dir::SortFlag::Name));
   ASSERT_EQ(Dir::SortFlags(Dir::SortFlag::Name), obj1.getSorting());
   obj1.setSorting(Dir::SortFlags(Dir::SortFlag::Name) | Dir::SortFlag::IgnoreCase);
   ASSERT_EQ(Dir::SortFlags(Dir::SortFlag::Name) | Dir::SortFlag::IgnoreCase, obj1.getSorting());
   obj1.setSorting(Dir::SortFlags(Dir::SortFlag::NoSort));
   ASSERT_EQ(Dir::SortFlags(Dir::SortFlag::NoSort), obj1.getSorting());
}

TEST_F(DirTest, testConstruction)
{
   FileInfo myFileInfo(Latin1String("/machine/share/dir1/file1"));
   Dir myDir(myFileInfo.getAbsoluteDir()); // this asserte
   ASSERT_EQ(myFileInfo.getAbsoluteDir().getAbsolutePath(), myDir.getAbsolutePath());
}

namespace {

void init_setpath_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(String(Latin1String(".")), String(Latin1String(".."))));
}

} // anonymous namespace

TEST_F(DirTest, testSetPath)
{
   std::list<std::tuple<String, String>> data;
   init_setpath_data(data);
   for (const auto &item: data) {
      const String &dir1 = std::get<0>(item);
      const String &dir2 = std::get<1>(item);
      
      Dir shared;
      Dir tempDir1(dir1);
      StringList entries1 = tempDir1.entryList();
      ASSERT_EQ(shared.entryList(), entries1);
      
      Dir tempDir2(dir2);
      StringList entries2 = tempDir2.entryList();
      shared.setPath(dir2);
      ASSERT_EQ(shared.entryList(), entries2);
   }
}

namespace {

void init_mkdir_rmdir_data(std::list<std::tuple<String, bool>> &data)
{
   StringList dirs;
   dirs.push_back(String(Latin1String("testdir/one")));
   dirs.push_back(String(Latin1String("testdir/two/three/four")));
   dirs.push_back(String(Latin1String("testdir/../testdir/three")));
   
   data.push_back(std::make_tuple(Dir::getCurrentPath() + Latin1String("/") + dirs.at(0), false));
   data.push_back(std::make_tuple(Dir::getCurrentPath() + Latin1String("/") + dirs.at(1), true));
   data.push_back(std::make_tuple(Dir::getCurrentPath() + Latin1String("/") + dirs.at(2), false));
   
   data.push_back(std::make_tuple(dirs.at(0), false));
   data.push_back(std::make_tuple(dirs.at(1), true));
   data.push_back(std::make_tuple(dirs.at(2), false));
   
   for (size_t i = 0; i < dirs.size(); ++i) {
      ASSERT_TRUE(!File::exists(dirs.at(i)));
   }
}

} // anonymous namespace

TEST_F(DirTest, testMkdirRmdir)
{
   std::list<std::tuple<String, bool>> data;
   init_mkdir_rmdir_data(data);
   for (const auto &item: data) {
      const String &path = std::get<0>(item);
      const bool recurse = std::get<1>(item);
      Dir dir;
      dir.rmdir(path);
      if (recurse) {
         ASSERT_TRUE(dir.mkpath(path));
      } else {
         ASSERT_TRUE(dir.mkdir(path));
      }
      //make sure it really exists (ie that mkdir returns the right value)
      FileInfo fileInfo(path);
      ASSERT_TRUE(fileInfo.exists() && fileInfo.isDir()) << msg_does_not_exist(path).getConstRawData();
      if (recurse) {
         ASSERT_TRUE(dir.rmpath(path));
      } else {
         ASSERT_TRUE(dir.rmdir(path));
      }
      //make sure it really doesn't exist (ie that rmdir returns the right value)
      fileInfo.refresh();
      ASSERT_TRUE(!fileInfo.exists());
   }
}
