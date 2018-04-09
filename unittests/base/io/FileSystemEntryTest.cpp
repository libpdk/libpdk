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
// Created by zzu_softboy on 2018/04/09.

#include "gtest/gtest.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/io/fs/internal/FileSystemEntryPrivate.h"
#include <list>

using pdk::io::fs::internal::FileSystemEntry;
using pdk::ds::ByteArray;
using pdk::lang::String;
using pdk::lang::Latin1String;

namespace {

using GetSetChechDataType = std::list<std::tuple<ByteArray, String, String, String, String, String, String, bool>>;

void init_getset_check_data(GetSetChechDataType &data)
{
   data.push_back(std::make_tuple(ByteArray("/home/pdk/in/a/dir.tar.gz"), 
                                  String(Latin1String("/home/pdk/in/a/dir.tar.gz")), 
                                  String(Latin1String("dir.tar.gz")), 
                                  String(Latin1String("dir")), 
                                  String(Latin1String("dir.tar")), 
                                  String(Latin1String("gz")),
                                  String(Latin1String("tar.gz")), true));
   
   data.push_back(std::make_tuple(ByteArray("in/a/dir.tar.gz"), 
                                  String(Latin1String("in/a/dir.tar.gz")),
                                  String(Latin1String("dir.tar.gz")),
                                  String(Latin1String("dir")),
                                  String(Latin1String("dir.tar")),
                                  String(Latin1String("gz")),
                                  String(Latin1String("tar.gz")), false));
   
   data.push_back(std::make_tuple(ByteArray("myDir/myfile"), 
                                  String(Latin1String("myDir/myfile")),
                                  String(Latin1String("myfile")),
                                  String(Latin1String("myfile")),
                                  String(Latin1String("myfile")),
                                  String(Latin1String("")),
                                  String(Latin1String("")), false));
   
   data.push_back(std::make_tuple(ByteArray("myDir/myfile.txt"), 
                                  String(Latin1String("myDir/myfile.txt")),
                                  String(Latin1String("myfile.txt")),
                                  String(Latin1String("myfile")),
                                  String(Latin1String("myfile")),
                                  String(Latin1String("txt")),
                                  String(Latin1String("txt")), false));
   
   data.push_back(std::make_tuple(ByteArray("myDir/myfile.bla/"), 
                                  String(Latin1String("myDir/myfile.bla/")),
                                  String(Latin1String("")),
                                  String(Latin1String("")),
                                  String(Latin1String("")),
                                  String(Latin1String("")),
                                  String(Latin1String("")), false));
   
   data.push_back(std::make_tuple(ByteArray("A:dir/without/leading/backslash.bat"), 
                                  String(Latin1String("A:dir/without/leading/backslash.bat")),
                                  String(Latin1String("backslash.bat")),
                                  String(Latin1String("backslash")),
                                  String(Latin1String("backslash")),
                                  String(Latin1String("bat")),
                                  String(Latin1String("bat")), false));
}

} // anonymous namespace

TEST(FileSystemEntryTest, testGetSetCheck)
{
   GetSetChechDataType data;
   init_getset_check_data(data);
   for (const auto &item: data) {
      const ByteArray &nativeFilePath = std::get<0>(item);
      const String &filepath = std::get<1>(item);
      const String &filename = std::get<2>(item);
      const String &basename = std::get<3>(item);
      const String &completeBasename = std::get<4>(item);
      const String &suffix = std::get<5>(item);
      const String &completeSuffix = std::get<6>(item);
      bool absolute = std::get<7>(item);
      FileSystemEntry entry1(filepath);
      ASSERT_EQ(entry1.getFilePath(), filepath);
      ASSERT_EQ(entry1.getNativeFilePath(), nativeFilePath);
      ASSERT_EQ(entry1.getFileName(), filename);
      ASSERT_EQ(entry1.getSuffix(), suffix);
      ASSERT_EQ(entry1.getCompleteSuffix(), completeSuffix);
      ASSERT_EQ(entry1.isAbsolute(), absolute);
      ASSERT_EQ(entry1.isRelative(), !absolute);
      ASSERT_EQ(entry1.getBaseName(), basename);
      ASSERT_EQ(entry1.getCompleteBaseName(), completeBasename);
      
      FileSystemEntry entry2(nativeFilePath, FileSystemEntry::FromNativePath());
      ASSERT_EQ(entry2.getSuffix(), suffix);
      ASSERT_EQ(entry2.getCompleteSuffix(), completeSuffix);
      ASSERT_EQ(entry2.isAbsolute(), absolute);
      ASSERT_EQ(entry2.isRelative(), !absolute);
      ASSERT_EQ(entry2.getFilePath(), filepath);
      ASSERT_EQ(entry2.getNativeFilePath(), nativeFilePath);
      ASSERT_EQ(entry2.getFileName(), filename);
      ASSERT_EQ(entry2.getBaseName(), basename);
      ASSERT_EQ(entry2.getCompleteBaseName(), completeBasename);
   }
}

namespace {

void init_suffix_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(String(), String()));
   data.push_back(std::make_tuple(String(Latin1String("file")), String(Latin1String(""))));
   
   data.push_back(std::make_tuple(String(Latin1String("/path/to/file")), String(Latin1String(""))));
   data.push_back(std::make_tuple(String(Latin1String("file.tar")), String(Latin1String("tar"))));
   data.push_back(std::make_tuple(String(Latin1String("file.tar.gz")), String(Latin1String("gz"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file/file.tar.gz")), String(Latin1String("gz"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.tar")), String(Latin1String("tar"))));
   data.push_back(std::make_tuple(String(Latin1String(".ext1")), String(Latin1String("ext1"))));
   data.push_back(std::make_tuple(String(Latin1String(".ext")), String(Latin1String("ext"))));
   data.push_back(std::make_tuple(String(Latin1String(".ex")), String(Latin1String("ex"))));
   data.push_back(std::make_tuple(String(Latin1String(".e")), String(Latin1String("e"))));
   data.push_back(std::make_tuple(String(Latin1String(".ext1.ext2")), String(Latin1String("ext2"))));
   data.push_back(std::make_tuple(String(Latin1String(".ext.ext2")), String(Latin1String("ext2"))));
   data.push_back(std::make_tuple(String(Latin1String(".ex.ext2")), String(Latin1String("ext2"))));
   data.push_back(std::make_tuple(String(Latin1String(".e.ext2")), String(Latin1String("ext2"))));
   data.push_back(std::make_tuple(String(Latin1String("..ext2")), String(Latin1String("ext2"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.with.dots/file..ext2")), String(Latin1String("ext2"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.with.dots/.file..ext2")), String(Latin1String("ext2"))));
}

void init_complete_suffix(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(String(), String()));
   data.push_back(std::make_tuple(String(Latin1String("file")), String(Latin1String(""))));
   data.push_back(std::make_tuple(String(Latin1String("/path/to/file")), String(Latin1String(""))));
   data.push_back(std::make_tuple(String(Latin1String("file.tar")), String(Latin1String("tar"))));
   data.push_back(std::make_tuple(String(Latin1String("file.tar.gz")), String(Latin1String("tar.gz"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file/file.tar.gz")), String(Latin1String("tar.gz"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.tar")), String(Latin1String("tar"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.with.dots/file..ext2")), String(Latin1String(".ext2"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.with.dots/.file..ext2")), String(Latin1String("file..ext2"))));
}

void init_basename_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(String(), String()));
   data.push_back(std::make_tuple(String(Latin1String("file.tar")), String(Latin1String("file"))));
   data.push_back(std::make_tuple(String(Latin1String("file.tar.gz")), String(Latin1String("file"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file/file.tar.gz")), String(Latin1String("file"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.tar")), String(Latin1String("file"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file")), String(Latin1String("file"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.with.dots/file..ext2")), String(Latin1String("file"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.with.dots/.file..ext2")), String(Latin1String(""))));
}

void init_complete_basename_data(std::list<std::tuple<String, String>> &data)
{
   data.push_back(std::make_tuple(String(), String()));
   data.push_back(std::make_tuple(String(Latin1String("file.tar")), String(Latin1String("file"))));
   data.push_back(std::make_tuple(String(Latin1String("file.tar.gz")), String(Latin1String("file.tar"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file/file.tar.gz")), String(Latin1String("file.tar"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.tar")), String(Latin1String("file"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file")), String(Latin1String("file"))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.with.dots/file..ext2")), String(Latin1String("file."))));
   data.push_back(std::make_tuple(String(Latin1String("/path/file.with.dots/.file..ext2")), String(Latin1String(".file."))));
}

void init_is_clean_data(std::list<std::tuple<String, bool>> &data)
{
   data.push_back(std::make_tuple(String(), true));
   
   data.push_back(std::make_tuple(String(Latin1String("foo")), true));
   data.push_back(std::make_tuple(String(Latin1String("/foo/bar/bz")), true));
   data.push_back(std::make_tuple(String(Latin1String("/foo/.file")), true));
   data.push_back(std::make_tuple(String(Latin1String("/foo/..file")), true));
   data.push_back(std::make_tuple(String(Latin1String("/foo/.../bar")), true));
   data.push_back(std::make_tuple(String(Latin1String("./")), false));
   data.push_back(std::make_tuple(String(Latin1String("../")), false));
   data.push_back(std::make_tuple(String(Latin1String(".")), false));
   data.push_back(std::make_tuple(String(Latin1String("..")), false));
   data.push_back(std::make_tuple(String(Latin1String("/.")), false));
   data.push_back(std::make_tuple(String(Latin1String("/..")), false));
   data.push_back(std::make_tuple(String(Latin1String("foo/../bar")), false));
   data.push_back(std::make_tuple(String(Latin1String("foo/./bar")), false));
   data.push_back(std::make_tuple(String(Latin1String("foo//bar")), false));
}

}

TEST(FileSystemEntryTest, testSuffix)
{
   std::list<std::tuple<String, String>> data;
   init_suffix_data(data);
   for (const auto &item : data) {
      const String &file = std::get<0>(item);
      const String &expected = std::get<1>(item);
      FileSystemEntry fe(file);
      ASSERT_EQ(fe.getSuffix(), expected);
      FileSystemEntry fi2(file);
      // first resolve the last slash
      (void) fi2.getPath();
      ASSERT_EQ(fi2.getSuffix(), expected);
   }
}

TEST(FileSystemEntryTest, testCompleteSuffix)
{
   std::list<std::tuple<String, String>> data;
   init_complete_suffix(data);
   for (const auto &item : data) {
      const String &file = std::get<0>(item);
      const String &expected = std::get<1>(item);
      FileSystemEntry fe(file);
      ASSERT_EQ(fe.getCompleteSuffix(), expected);
      FileSystemEntry fi2(file);
      // first resolve the last slash
      (void) fi2.getPath();
      ASSERT_EQ(fi2.getCompleteSuffix(), expected);
   }
}

TEST(FileSystemEntryTest, testGetBaseName)
{
   std::list<std::tuple<String, String>> data;
   init_basename_data(data);
   for (const auto &item : data) {
      const String &file = std::get<0>(item);
      const String &expected = std::get<1>(item);
      FileSystemEntry fe(file);
      ASSERT_EQ(fe.getBaseName(), expected);
      FileSystemEntry fi2(file);
      // first resolve the last slash
      (void) fi2.getPath();
      ASSERT_EQ(fi2.getBaseName(), expected);
   }
}

TEST(FileSystemEntryTest, testGetCompleteBaseName)
{
   std::list<std::tuple<String, String>> data;
   init_complete_basename_data(data);
   for (const auto &item : data) {
      const String &file = std::get<0>(item);
      const String &expected = std::get<1>(item);
      FileSystemEntry fe(file);
      ASSERT_EQ(fe.getCompleteBaseName(), expected);
      FileSystemEntry fi2(file);
      // first resolve the last slash
      (void) fi2.getPath();
      ASSERT_EQ(fi2.getCompleteBaseName(), expected);
   }
}

TEST(FileSystemEntryTest, testIsClean)
{
   std::list<std::tuple<String, bool>> data;
   init_is_clean_data(data);
   for (const auto &item : data) {
      const String &path = std::get<0>(item);
      bool isClean = std::get<1>(item);
      FileSystemEntry fi(path);
      ASSERT_EQ(fi.isClean(), isClean);
   }
}

TEST(FileSystemEntryTest, testDefaultCtor)
{
   FileSystemEntry entry;
   ASSERT_TRUE(entry.getFilePath().isNull());
   ASSERT_TRUE(entry.getNativeFilePath().isNull());
   ASSERT_TRUE(entry.getFileName().isNull());
   ASSERT_EQ(entry.getPath(), String(Latin1String(".")));
   ASSERT_TRUE(entry.getBaseName().isNull());
   ASSERT_TRUE(entry.getCompleteBaseName().isNull());
   ASSERT_TRUE(entry.getSuffix().isNull());
   ASSERT_TRUE(entry.getCompleteSuffix().isNull());
   ASSERT_TRUE(!entry.isAbsolute());
   ASSERT_TRUE(entry.isRelative());
   ASSERT_TRUE(entry.isClean());
   ASSERT_TRUE(!entry.isRoot());
   ASSERT_TRUE(entry.isEmpty());
}
