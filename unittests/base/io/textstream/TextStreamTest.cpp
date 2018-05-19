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
// Created by zzu_softboy on 2018/05/17.

#include "gtest/gtest.h"

#include "pdk/base/io/Buffer.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/time/Time.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/TemporaryDir.h"
#include "pdk/base/io/TextStream.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/lang/Character.h"
#include "pdk/base/text/codecs/TextCodec.h"
#include "pdk/base/os/process/Process.h"
#include "pdk/kernel/CoreApplication.h"

#ifdef PDK_OS_UNIX
#include <locale.h>
#endif

using pdk::ds::ByteArray;
using pdk::ds::StringList;
using pdk::io::fs::TemporaryDir;
using pdk::io::fs::Dir;
using pdk::io::fs::File;
using pdk::io::Buffer;
using pdk::time::Time;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Character;
using pdk::lang::Latin1Character;
using pdk::io::IoDevice;
using pdk::io::TextStream;
using pdk::text::codecs::TextCodec;

#define PDKTEST_DIR_SEP "/"
#define FIND_SOURCE_DATA(name) Latin1String(PDKTEST_CURRENT_SOURCE_DIR PDKTEST_DIR_SEP PDK_STRINGIFY(name))

namespace {

static TemporaryDir sg_tempDir(Dir::getTempPath() + Latin1String("/TextStreamTest.XXXXXX"));
static String sg_currentSourceDir(Latin1String(PDKTEST_CURRENT_SOURCE_DIR));
static String sg_currentBinaryDir(Latin1String(PDKTEST_CURRENT_DIR));
static String sg_testFileName;
static String sg_rfc3261FilePath(FIND_SOURCE_DATA(rfc3261.txt));
static String sg_shiftJisFilePath(FIND_SOURCE_DATA(shift-jis.txt));
static String sg_task113817File(FIND_SOURCE_DATA(task113817.txt));

} // anonymous namespace

class TextStreamTest : public ::testing::Test
{
public:
   
   static void SetUpTestCase()
   {
      ASSERT_TRUE(sg_tempDir.isValid()) << pdk_printable(sg_tempDir.getErrorString());
      ASSERT_TRUE(sg_rfc3261FilePath.size() != 0);
      ASSERT_TRUE(sg_shiftJisFilePath.size() != 0);
      sg_testFileName = sg_tempDir.getPath() + Latin1String("/testfile");
   }
   
   static void TearDownTestCase()
   {
      PDK_RETRIEVE_APP_INSTANCE()->processEvents();
   }
};

TEST_F(TextStreamTest, testGetSetCheck)
{
   TextStream textstream1;
   TextCodec *codec1 = TextCodec::codecForName("en");
   textstream1.setCodec(codec1);
   ASSERT_EQ(codec1, textstream1.getCodec());
   textstream1.setCodec((TextCodec *)nullptr);
   ASSERT_EQ(codec1, textstream1.getCodec());
   
   textstream1.setAutoDetectUnicode(false);
   ASSERT_EQ(false, textstream1.getAutoDetectUnicode());
   textstream1.setAutoDetectUnicode(true);
   ASSERT_EQ(true, textstream1.getAutoDetectUnicode());
   
   textstream1.setGenerateByteOrderMark(false);
   ASSERT_EQ(false, textstream1.getGenerateByteOrderMark());
   textstream1.setGenerateByteOrderMark(true);
   ASSERT_EQ(true, textstream1.getGenerateByteOrderMark());
   
   File *file = new File;
   textstream1.setDevice(file);
   ASSERT_EQ(static_cast<IoDevice *>(file), textstream1.getDevice());
   textstream1.setDevice((IoDevice *)nullptr);
   ASSERT_EQ((IoDevice *)nullptr, textstream1.getDevice());
   delete file;
   
   textstream1.setStatus(TextStream::Status::Ok);
   ASSERT_EQ(TextStream::Status::Ok, textstream1.getStatus());
   textstream1.setStatus(TextStream::Status::ReadPastEnd);
   ASSERT_EQ(TextStream::Status::ReadPastEnd, textstream1.getStatus());
   textstream1.resetStatus();
   textstream1.setStatus(TextStream::Status::ReadCorruptData);
   ASSERT_EQ(TextStream::Status::ReadCorruptData, textstream1.getStatus());
   
   textstream1.setFieldAlignment(TextStream::FieldAlignment::AlignLeft);
   ASSERT_EQ(TextStream::FieldAlignment::AlignLeft, textstream1.getFieldAlignment());
   textstream1.setFieldAlignment(TextStream::FieldAlignment::AlignRight);
   ASSERT_EQ(TextStream::FieldAlignment::AlignRight, textstream1.getFieldAlignment());
   textstream1.setFieldAlignment(TextStream::FieldAlignment::AlignCenter);
   ASSERT_EQ(TextStream::FieldAlignment::AlignCenter, textstream1.getFieldAlignment());
   textstream1.setFieldAlignment(TextStream::FieldAlignment::AlignAccountingStyle);
   ASSERT_EQ(TextStream::FieldAlignment::AlignAccountingStyle, textstream1.getFieldAlignment());
   
   Character char1 = 'Q';
   textstream1.setPadChar(char1);
   ASSERT_EQ(char1, textstream1.getPadChar());
   textstream1.setPadChar(Character());
   ASSERT_EQ(Character(), textstream1.getPadChar());
   
   textstream1.setFieldWidth(0);
   ASSERT_EQ(0, textstream1.getFieldWidth());
   textstream1.setFieldWidth(INT_MIN);
   ASSERT_EQ(INT_MIN, textstream1.getFieldWidth());
   textstream1.setFieldWidth(INT_MAX);
   ASSERT_EQ(INT_MAX, textstream1.getFieldWidth());
   
   textstream1.setNumberFlags(TextStream::NumberFlag::ShowBase);
   ASSERT_EQ(textstream1.getNumberFlags(), TextStream::NumberFlag::ShowBase);
   textstream1.setNumberFlags(TextStream::NumberFlag::ForcePoint);
   ASSERT_EQ(textstream1.getNumberFlags(), TextStream::NumberFlag::ForcePoint);
   textstream1.setNumberFlags(TextStream::NumberFlag::ForceSign);
   ASSERT_EQ(textstream1.getNumberFlags(), TextStream::NumberFlag::ForceSign);
   textstream1.setNumberFlags(TextStream::NumberFlag::UppercaseBase);
   ASSERT_EQ(textstream1.getNumberFlags(), TextStream::NumberFlag::UppercaseBase);
   textstream1.setNumberFlags(TextStream::NumberFlag::UppercaseDigits);
   ASSERT_EQ(textstream1.getNumberFlags(), TextStream::NumberFlag::UppercaseDigits);
   
   textstream1.setIntegerBase(0);
   ASSERT_EQ(0, textstream1.getIntegerBase());
   textstream1.setIntegerBase(INT_MIN);
   ASSERT_EQ(INT_MIN, textstream1.getIntegerBase());
   textstream1.setIntegerBase(INT_MAX);
   ASSERT_EQ(INT_MAX, textstream1.getIntegerBase());
   
   textstream1.setRealNumberNotation(TextStream::RealNumberNotation::SmartNotation);
   ASSERT_EQ(TextStream::RealNumberNotation::SmartNotation, textstream1.getRealNumberNotation());
   textstream1.setRealNumberNotation(TextStream::RealNumberNotation::FixedNotation);
   ASSERT_EQ(TextStream::RealNumberNotation::FixedNotation, textstream1.getRealNumberNotation());
   textstream1.setRealNumberNotation(TextStream::RealNumberNotation::ScientificNotation);
   ASSERT_EQ(TextStream::RealNumberNotation::ScientificNotation, textstream1.getRealNumberNotation());
   
   textstream1.setRealNumberPrecision(0);
   ASSERT_EQ(0, textstream1.getRealNumberPrecision());
   textstream1.setRealNumberPrecision(INT_MIN);
   ASSERT_EQ(6, textstream1.getRealNumberPrecision()); // Setting a negative precision reverts it to the default value (6).
   textstream1.setRealNumberPrecision(INT_MAX);
   ASSERT_EQ(INT_MAX, textstream1.getRealNumberPrecision());
}

TEST_F(TextStreamTest, testConstruction)
{
   TextStream stream;
   ASSERT_EQ(stream.getCodec(), TextCodec::getCodecForLocale());
   ASSERT_EQ(stream.getDevice(), static_cast<IoDevice *>(nullptr));
   ASSERT_EQ(stream.getString(), static_cast<String *>(nullptr));
   
   ASSERT_TRUE(stream.atEnd());
   ASSERT_EQ(stream.readAll(), String());
}

namespace {

void generate_line_data(std::list<std::tuple<ByteArray, StringList>> &data, bool forString)
{
   // emptyer
   data.push_back(std::make_tuple(ByteArray(), StringList()));
   // lf
   data.push_back(std::make_tuple(ByteArray("\n"), (StringList() << Latin1String(""))));
   // crlf
   data.push_back(std::make_tuple(ByteArray("\r\n"), (StringList() << Latin1String(""))));
   // oneline/nothing
   data.push_back(std::make_tuple(ByteArray("ole"), (StringList() << Latin1String("ole"))));
   // oneline/lf
   data.push_back(std::make_tuple(ByteArray("ole\n"), (StringList() << Latin1String("ole"))));
   // oneline/crlf
   data.push_back(std::make_tuple(ByteArray("ole\r\n"), (StringList() << Latin1String("ole"))));
   // twolines/lf/lf
   data.push_back(std::make_tuple(ByteArray("ole\ndole\n"), (StringList() << Latin1String("ole") << Latin1String("dole"))));
   // twolines/crlf/crlf
   data.push_back(std::make_tuple(ByteArray("ole\r\ndole\r\n"), (StringList() << Latin1String("ole") << Latin1String("dole"))));
   // twolines/lf/crlf
   data.push_back(std::make_tuple(ByteArray("ole\ndole\r\n"), (StringList() << Latin1String("ole") << Latin1String("dole"))));
   // twolines/lf/nothing
   data.push_back(std::make_tuple(ByteArray("ole\ndole"), (StringList() << Latin1String("ole") << Latin1String("dole"))));
   // twolines/crlf/nothing
   data.push_back(std::make_tuple(ByteArray("ole\r\ndole"), (StringList() << Latin1String("ole") << Latin1String("dole"))));
   // threelines/lf/lf/lf
   data.push_back(std::make_tuple(ByteArray("ole\ndole\ndoffen\n"), (StringList() << Latin1String("ole") << Latin1String("dole") << Latin1String("doffen"))));
   // threelines/crlf/crlf/crlf
   data.push_back(std::make_tuple(ByteArray("ole\r\ndole\r\ndoffen\r\n"), (StringList() << Latin1String("ole") << Latin1String("dole") << Latin1String("doffen"))));
   // threelines/crlf/crlf/nothing
   data.push_back(std::make_tuple(ByteArray("ole\r\ndole\r\ndoffen"), (StringList() << Latin1String("ole") << Latin1String("dole") << Latin1String("doffen"))));
   if (!forString) {
      // utf-8 utf8/twolines
      data.push_back(std::make_tuple(ByteArray("\xef\xbb\xbf"
                                               "\x66\x67\x65\x0a"
                                               "\x66\x67\x65\x0a", 11), (StringList() << Latin1String("fge") << Latin1String("fge"))));
      
      // utf-16
      // one line
      // utf16-BE/nothing
      data.push_back(std::make_tuple(ByteArray("\xfe\xff"
                                               "\x00\xe5\x00\x67\x00\x65", 8), (StringList() << Latin1String("\345ge"))));
      
      // utf16-LE/nothing
      data.push_back(std::make_tuple(ByteArray("\xff\xfe"
                                               "\xe5\x00\x67\x00\x65\x00", 8), (StringList() << Latin1String("\345ge"))));
      
      // utf16-BE/lf
      data.push_back(std::make_tuple(ByteArray("\xfe\xff"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a", 10), (StringList() << Latin1String("\345ge"))));
      
      // utf16-LE/lf
      data.push_back(std::make_tuple(ByteArray("\xff\xfe"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00", 10), (StringList() << Latin1String("\345ge"))));
      
      // two lines
      // utf16-BE/twolines
      data.push_back(std::make_tuple(ByteArray("\xfe\xff"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a", 18), (StringList() << Latin1String("\345ge") << Latin1String("\345ge"))));
      
      // utf16-LE/twolines
      data.push_back(std::make_tuple(ByteArray("\xff\xfe"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00", 18), (StringList() << Latin1String("\345ge") << Latin1String("\345ge"))));
      
      // three lines
      // utf16-BE/threelines
      data.push_back(std::make_tuple(ByteArray("\xfe\xff"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a", 26),
                                     (StringList() << Latin1String("\345ge") << Latin1String("\345ge") << Latin1String("\345ge"))));
      // utf16-LE/threelines
      data.push_back(std::make_tuple(ByteArray("\xff\xfe"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00", 26),
                                     (StringList() << Latin1String("\345ge") << Latin1String("\345ge") << Latin1String("\345ge"))));
      
      // utf-32
      data.push_back(std::make_tuple(ByteArray("\x00\x00\xfe\xff"
                                               "\x00\x00\x00\xe5\x00\x00\x00\x67\x00\x00\x00\x65\x00\x00\x00\x0a"
                                               "\x00\x00\x00\xe5\x00\x00\x00\x67\x00\x00\x00\x65\x00\x00\x00\x0a", 36),
                                     (StringList() << Latin1String("\345ge") << Latin1String("\345ge"))));
      
      data.push_back(std::make_tuple(ByteArray("\xff\xfe\x00\x00"
                                               "\xe5\x00\x00\x00\x67\x00\x00\x00\x65\x00\x00\x00\x0a\x00\x00\x00"
                                               "\xe5\x00\x00\x00\x67\x00\x00\x00\x65\x00\x00\x00\x0a\x00\x00\x00", 36),
                                     (StringList() << Latin1String("\345ge") << Latin1String("\345ge"))));
   }
   // partials
   // cr
   data.push_back(std::make_tuple(ByteArray("\r"),
                                  (StringList() << Latin1String(""))));
   // oneline/cr
   data.push_back(std::make_tuple(ByteArray("ole\r"),
                                  (StringList() << Latin1String("ole"))));
   
   if (!forString) {
      // utf16-BE/cr
      data.push_back(std::make_tuple(ByteArray("\xfe\xff\x00\xe5\x00\x67\x00\x65\x00\x0d", 10),
                                     (StringList() << Latin1String("\345ge"))));
   }
}

void init_read_line_maxlen_data(std::list<std::tuple<String, StringList>> &data)
{
   // Hey
   data.push_back(std::make_tuple(String(Latin1String("Hey")), 
                                  (StringList() 
                                   << String(Latin1String("Hey")) 
                                   << String(Latin1String("")))));
   
   // Hey\n
   data.push_back(std::make_tuple(String(Latin1String("Hey\n")),
                                  (StringList() 
                                   << String(Latin1String("Hey")) 
                                   << String(Latin1String("")))));
   
   // HelloWorld
   data.push_back(std::make_tuple(String(Latin1String("HelloWorld")), 
                                  (StringList() 
                                   << String(Latin1String("Hello")) 
                                   << String(Latin1String("World")))));
   // Helo\nWorlds
   data.push_back(std::make_tuple(String(Latin1String("Helo\nWorlds")), 
                                  (StringList() 
                                   << String(Latin1String("Helo")) 
                                   << String(Latin1String("World")))));
   
   // AAAAA etc.
   data.push_back(std::make_tuple(String(16385, Latin1Character('A')), 
                                  (StringList() 
                                   << String(Latin1String("AAAAA")) 
                                   << String(Latin1String("AAAAA")))));
   
   // multibyte string
   data.push_back(std::make_tuple(String::fromUtf8("\341\233\222\341\233\226\341\232\251\341\232\271\341\232\242\341\233\232\341\232\240\n"), 
                                  (StringList() 
                                   << String::fromUtf8("\341\233\222\341\233\226\341\232\251\341\232\271\341\232\242")
                                   << String::fromUtf8("\341\233\232\341\232\240"))));
}

} // anonymous namespace

TEST_F(TextStreamTest, testReadLineFromDevice)
{
   std::list<std::tuple<ByteArray, StringList>> items;
   generate_line_data(items, false);
   for (auto &item : items) {
      ByteArray &data = std::get<0>(item);
      StringList &lines = std::get<1>(item);
      File::remove(sg_testFileName);
      File file(sg_testFileName);
      ASSERT_TRUE(file.open(File::OpenMode::ReadWrite));
      ASSERT_EQ(file.write(data), pdk::plonglong(data.size()));
      ASSERT_TRUE(file.flush());
      file.seek(0);
      
      TextStream stream(&file);
      StringList list;
      while (!stream.atEnd()) {
         list << stream.readLine();
      }
      ASSERT_EQ(list, lines);
   }
}

TEST_F(TextStreamTest, testReadLineMaxlen)
{
   std::list<std::tuple<String, StringList>> data;
   init_read_line_maxlen_data(data);
   for (auto &item : data) {
      String &input = std::get<0>(item);
      StringList &lines = std::get<1>(item);
      for (int i = 0; i < 2; i ++) {
         bool useDevice = (i == 1);
         TextStream stream;
         File::remove(Latin1String("testfile"));
         File file(Latin1String("testfile"));
         if (useDevice) {
            file.open(IoDevice::OpenMode::ReadWrite);
            file.write(input.toUtf8());
            file.seek(0);
            stream.setDevice(&file);
            // @TODO here is not good
            stream.setCodec("utf-8");
         } else {
            stream.setString(&input);
         }
         
         StringList list;
         list << stream.readLine(5);
         list << stream.readLine(5);
         ASSERT_EQ(list, lines);
      }
   }
}

TEST_F(TextStreamTest, testReadLinesFromBufferCRCR)
{
   Buffer buffer;
   buffer.open(IoDevice::OpenMode::WriteOnly);
   ByteArray data("0123456789\r\r\n");
   for (int i = 0; i < 10000; ++i) {
      buffer.write(data);
   }
   buffer.close();
   if (buffer.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | IoDevice::OpenMode::Text)) {
      TextStream stream(&buffer);
      while (!stream.atEnd()) {
         ASSERT_EQ(stream.readLine(), String(Latin1String("0123456789")));
      }
   }
}

namespace {

class ErrorDevice : public IoDevice
{
protected:
   pdk::pint64 readData(char *data, pdk::pint64 maxlen) override
   {
      PDK_UNUSED(data);
      PDK_UNUSED(maxlen);
      return -1;
   }
   
   pdk::pint64 writeData(const char *data, pdk::pint64 len) override
   {
      PDK_UNUSED(data);
      PDK_UNUSED(len);
      return -1;
   }
};

} // anonymous namespace

TEST_F(TextStreamTest, testReadLineInto)
{
   ByteArray data = "1\n2\n3";
   TextStream ts(&data);
   String line;
   ts.readLineInto(&line);
   ASSERT_EQ(line, StringLiteral("1"));
   ts.readLineInto(nullptr, 0); // read the second line, but don't store it
   ts.readLineInto(&line);
   ASSERT_EQ(line, StringLiteral("3"));
   ASSERT_TRUE(!ts.readLineInto(&line));
   ASSERT_TRUE(line.isEmpty());
   File file(sg_rfc3261FilePath);
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly));
   
   ts.setDevice(&file);
   line.reserve(1);
   int maxLineCapacity = line.capacity();
   while (ts.readLineInto(&line)) {
      ASSERT_TRUE(line.capacity() >= maxLineCapacity);
      maxLineCapacity = line.capacity();
   }
   line = Latin1String("Test string");
   ErrorDevice errorDevice;
   ASSERT_TRUE(errorDevice.open(IoDevice::OpenMode::ReadOnly));
   ts.setDevice(&errorDevice);
   ASSERT_TRUE(!ts.readLineInto(&line));
   ASSERT_TRUE(line.isEmpty());
}

TEST_F(TextStreamTest, testReadLineFromString)
{
   std::list<std::tuple<ByteArray, StringList>> tdata;
   generate_line_data(tdata, true);
   for (auto &item : tdata) {
      ByteArray &data = std::get<0>(item);
      StringList &lines = std::get<1>(item);
      String dataString = Latin1String(data);
      TextStream stream(&dataString, IoDevice::OpenMode::ReadOnly);
      StringList list;
      while (!stream.atEnd()) {
         list << stream.readLine();
      }
      ASSERT_EQ(list, lines);
   }
}

TEST_F(TextStreamTest, testReadLineFromStringThenChangeString)
{
   String first = Latin1String("First string");
   String second = Latin1String("Second string");
   TextStream stream(&first, IoDevice::OpenMode::ReadOnly);
   String result = stream.readLine();
   ASSERT_EQ(first, result);
   
   stream.setString(&second, IoDevice::OpenMode::ReadOnly);
   result = stream.readLine();
   ASSERT_EQ(second, result);
}

TEST_F(TextStreamTest, testSetDevice)
{
   ByteArray data1("Hello World");
   ByteArray data2("How are you");
   Buffer bufferOld(&data1);
   bufferOld.open(IoDevice::OpenMode::ReadOnly);
   Buffer bufferNew(&data2);
   bufferNew.open(IoDevice::OpenMode::ReadOnly);
   
   String text;
   TextStream stream(&bufferOld);
   stream >> text;
   ASSERT_EQ(text, String(Latin1String("Hello")));
   stream.setDevice(&bufferNew);
   stream >> text;
   ASSERT_EQ(text, String(Latin1String("How")));
}

TEST_F(TextStreamTest, testReadLineFromTextDevice)
{
   std::list<std::tuple<ByteArray, StringList>> tdata;
   generate_line_data(tdata, false);
   for (auto &item : tdata) {
      ByteArray &data = std::get<0>(item);
      StringList &lines = std::get<1>(item);
      for (int i = 0; i < 8; ++i) {
         Buffer buffer(&data);
         if (i < 4) {
            ASSERT_TRUE(buffer.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | IoDevice::OpenMode::Text));
         } else {
            ASSERT_TRUE(buffer.open(IoDevice::OpenMode::ReadOnly));
         }
         TextStream stream(&buffer);
         StringList list;
         while (!stream.atEnd()) {
            stream.getPosition(); // <- triggers side effects
            String line;
            if (i & 1) {
               Character c;
               while (!stream.atEnd()) {
                  stream >> c;
                  if (stream.getStatus() == TextStream::Status::Ok) {
                     if (c != Latin1Character('\n') && c != Latin1Character('\r')) {
                        line += c;
                     }  
                     if (c == Latin1Character('\n')) {
                        break;
                     }
                  }
               }
            } else {
               line = stream.readLine();
            }
            
            if ((i & 3) == 3) {
               stream.seek(stream.getPosition());
            }
            list << line;
         }
         ASSERT_EQ(list, lines);
      }
   }
}

TEST_F(TextStreamTest, testReadLineUntilNull)
{
   File file(sg_rfc3261FilePath);
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly));
   TextStream stream(&file);
   for (int i = 0; i < 15066; ++i) {
      String line = stream.readLine();
      ASSERT_TRUE(!line.isNull());
   }
   ASSERT_TRUE(!stream.readLine().isEmpty());
   ASSERT_TRUE(stream.readLine().isEmpty());
}

namespace {

void generate_all_data(std::list<std::tuple<ByteArray, String>> &data, bool forString)
{
   // latin-1
   data.push_back(std::make_tuple(ByteArray(), String()));
   data.push_back(std::make_tuple(ByteArray("a"), String(Latin1String("a"))));
   data.push_back(std::make_tuple(ByteArray("a\r"), String(Latin1String("a\r"))));
   data.push_back(std::make_tuple(ByteArray("a\r\n"), String(Latin1String("a\r\n"))));
   data.push_back(std::make_tuple(ByteArray("a\n"), String(Latin1String("a\n"))));
   
   // utf-16
   if (!forString) {
      // one line
      // utf16-BE/nothing
      data.push_back(std::make_tuple(ByteArray("\xfe\xff"
                                               "\x00\xe5\x00\x67\x00\x65", 8), 
                                     String::fromLatin1("\345ge")));
      // utf16-LE/nothing
      data.push_back(std::make_tuple(ByteArray("\xff\xfe"
                                               "\xe5\x00\x67\x00\x65\x00", 8), 
                                     String::fromLatin1("\345ge")));
      // utf16-BE/lf
      data.push_back(std::make_tuple(ByteArray("\xfe\xff"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a", 10), 
                                     String::fromLatin1("\345ge\n")));
      // utf16-LE/lf
      data.push_back(std::make_tuple(ByteArray("\xff\xfe"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00", 10), 
                                     String::fromLatin1("\345ge\n")));
      
      // utf16-BE/crlf
      data.push_back(std::make_tuple(ByteArray("\xfe\xff"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0d\x00\x0a", 12), 
                                     String::fromLatin1("\345ge\r\n")));
      // utf16-LE/crlf
      data.push_back(std::make_tuple(ByteArray("\xff\xfe"
                                               "\xe5\x00\x67\x00\x65\x00\x0d\x00\x0a\x00", 12), 
                                     String::fromLatin1("\345ge\r\n")));
      
      // two lines
      // utf16-BE/twolines
      data.push_back(std::make_tuple(ByteArray("\xfe\xff"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a", 18), 
                                     String::fromLatin1("\345ge\n\345ge\n")));
      
      // utf16-LE/twolines
      data.push_back(std::make_tuple(ByteArray("\xff\xfe"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00", 18), 
                                     String::fromLatin1("\345ge\n\345ge\n")));
      
      // three lines
      // utf16-BE/threelines
      data.push_back(std::make_tuple(ByteArray("\xfe\xff"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a"
                                               "\x00\xe5\x00\x67\x00\x65\x00\x0a", 26), 
                                     String::fromLatin1("\345ge\n\345ge\n\345ge\n")));
      
      // utf16-LE/threelines
      data.push_back(std::make_tuple(ByteArray("\xff\xfe"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00"
                                               "\xe5\x00\x67\x00\x65\x00\x0a\x00", 26), 
                                     String::fromLatin1("\345ge\n\345ge\n\345ge\n")));
   }
}

void init_skip_white_space_data(std::list<std::tuple<ByteArray, Character>> &data)
{
   // latin1
   data.push_back(std::make_tuple(ByteArray(), Character('\0')));
   data.push_back(std::make_tuple(ByteArray(" one"), Character('o')));
   data.push_back(std::make_tuple(ByteArray("\none"), Character('o')));
   data.push_back(std::make_tuple(ByteArray("\n one"), Character('o')));
   data.push_back(std::make_tuple(ByteArray(" \r\n one"), Character('o')));
   
   // utf-16
   // utf16-BE (empty)
   data.push_back(std::make_tuple(ByteArray("\xfe\xff", 2), Character('\0')));
   // utf16-BE ( one)
   data.push_back(std::make_tuple(ByteArray("\xfe\xff\x00 \x00o\x00n\x00e", 10), Character('o')));
   // utf16-BE (\none)
   data.push_back(std::make_tuple(ByteArray("\xfe\xff\x00\n\x00o\x00n\x00e", 10), Character('o')));
   // utf16-BE (\n one)
   data.push_back(std::make_tuple(ByteArray("\xfe\xff\x00\n\x00 \x00o\x00n\x00e", 12), Character('o')));
   // utf16-BE ( \r\n one)
   data.push_back(std::make_tuple(ByteArray("\xfe\xff\x00 \x00\r\x00\n\x00 \x00o\x00n\x00e", 16), Character('o')));
   
   // utf16-LE (empty)
   data.push_back(std::make_tuple(ByteArray("\xff\xfe", 2), Character('\0')));
   // utf16-LE ( one)
   data.push_back(std::make_tuple(ByteArray("\xff\xfe \x00o\x00n\x00e\x00", 10), Character('o')));
   // utf16-LE (\none)
   data.push_back(std::make_tuple(ByteArray("\xff\xfe\n\x00o\x00n\x00e\x00", 10), Character('o')));
   // utf16-LE (\n one)
   data.push_back(std::make_tuple(ByteArray("\xff\xfe\n\x00 \x00o\x00n\x00e\x00", 12), Character('o')));
   // utf16-LE ( \r\n one)
   data.push_back(std::make_tuple(ByteArray("\xff\xfe \x00\r\x00\n\x00 \x00o\x00n\x00e\x00", 16), Character('o')));
}

} // anonymous namespace

TEST_F(TextStreamTest, testReadAllFromDevice)
{
   std::list<std::tuple<ByteArray, String>> tdata;
   generate_all_data(tdata, false);
   for (auto &item : tdata) {
      ByteArray &input = std::get<0>(item);
      String &output = std::get<1>(item);
      Buffer buffer(&input);
      buffer.open(Buffer::OpenMode::ReadOnly);
      
      TextStream stream(&buffer);
      ASSERT_EQ(stream.readAll(), output);
   }
}

TEST_F(TextStreamTest, testReadAllFromString)
{
   std::list<std::tuple<ByteArray, String>> tdata;
   generate_all_data(tdata, true);
   for (auto &item : tdata) {
      ByteArray input = std::get<0>(item);
      String output = std::get<1>(item);
      String str{Latin1String(input)};
      TextStream stream(&str);
      ASSERT_EQ(stream.readAll(), output);
   }
}

TEST_F(TextStreamTest, testSkipWhiteSpace)
{
   std::list<std::tuple<ByteArray, Character>> data;
   init_skip_white_space_data(data);
   for (auto &item : data) {
      ByteArray input = std::get<0>(item);
      Character output = std::get<1>(item);
      Buffer buffer(&input);
      buffer.open(Buffer::OpenMode::ReadOnly);
      TextStream stream(&buffer);
      stream.skipWhiteSpace();
      
      Character tmp;
      stream >> tmp;
      
      ASSERT_EQ(tmp, output);
      
      String str{Latin1String(input)};
      TextStream stream2(&input);
      stream2.skipWhiteSpace();
      
      stream2 >> tmp;
      
      ASSERT_EQ(tmp, output);
   }
}

namespace {

void init_line_count_data(std::list<std::tuple<ByteArray, int>> &data)
{
   data.push_back(std::make_tuple(ByteArray(), 0));
   data.push_back(std::make_tuple(ByteArray("a\n"), 1));
   data.push_back(std::make_tuple(ByteArray("a\nb\n"), 2));
   data.push_back(std::make_tuple(ByteArray("\n"), 1));
   data.push_back(std::make_tuple(ByteArray("\n\n"), 2));
   data.push_back(std::make_tuple(ByteArray(16382, '\n'), 16382));
   data.push_back(std::make_tuple(ByteArray(16383, '\n'), 16383));
   data.push_back(std::make_tuple(ByteArray(16384, '\n'), 16384));
   data.push_back(std::make_tuple(ByteArray(16385, '\n'), 16385));
   
   File file(sg_rfc3261FilePath);
   file.open(File::OpenMode::ReadOnly);
   data.push_back(std::make_tuple(file.readAll(), 15067));
}

struct CompareIndicesForArray
{
   int *array;
   CompareIndicesForArray(int *array) : array(array) {}
   bool operator() (const int i1, const int i2)
   {
      return array[i1] < array[i2];
   }
};

} // anonymous namespace

TEST_F(TextStreamTest, testLineCount)
{
   std::list<std::tuple<ByteArray, int>> tdata;
   init_line_count_data(tdata);
   for (auto &item : tdata) {
      ByteArray &data = std::get<0>(item);
      int lineCount = std::get<1>(item);
      File out(Latin1String("out.txt"));
      out.open(File::OpenMode::WriteOnly);
      TextStream lineReader(data);
      int lines = 0;
      while (!lineReader.atEnd()) {
         String line = lineReader.readLine();
         out.write(line.toLatin1() + "\n");
         ++lines;
      }
      out.close();
      ASSERT_EQ(lines, lineCount);
   }
}

TEST_F(TextStreamTest, testPerformance)
{
   Time stopWatch;
   const int N = 3;
   const char * readMethods[N] = {
      "File::readLine()",
      "TextStream::readLine()",
      "TextStream::readLine(String *)"
   };
   int elapsed[N] = {0, 0, 0};
   stopWatch.restart();
   int nlines1 = 0;
   File file(sg_rfc3261FilePath);
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly));
   
   while (!file.atEnd()) {
      ++nlines1;
      file.readLine();
   }
   
   elapsed[0] = stopWatch.elapsed();
   stopWatch.restart();
   
   int nlines2 = 0;
   File file2(sg_rfc3261FilePath);
   ASSERT_TRUE(file2.open(File::OpenMode::ReadOnly));
   
   TextStream stream(&file2);
   while (!stream.atEnd()) {
      ++nlines2;
      stream.readLine();
   }
   
   elapsed[1] = stopWatch.elapsed();
   stopWatch.restart();
   
   int nlines3 = 0;
   File file3(sg_rfc3261FilePath);
   ASSERT_TRUE(file3.open(File::OpenMode::ReadOnly));
   
   TextStream stream2(&file3);
   String line;
   while (stream2.readLineInto(&line)) {
      ++nlines3;
   }
   
   
   elapsed[2] = stopWatch.elapsed();
   
   ASSERT_EQ(nlines1, nlines2);
   ASSERT_EQ(nlines2, nlines3);
   
   for (int i = 0; i < N; i++) {
      std::printf("%s used %.3f seconds to read the file\n", readMethods[i],
                  elapsed[i] / 1000.0);
   }
   int idx[N] = {0, 1, 2};
   std::sort(idx, idx + N, CompareIndicesForArray(elapsed));
   
   for (int i = 0; i < N-1; i++) {
      int i1 = idx[i];
      int i2 = idx[i+1];
      std::printf("Reading by %s is %.2fx faster than by %s\n",
                  readMethods[i1],
                  double(elapsed[i2]) / double(elapsed[i1]),
                  readMethods[i2]);
   }
}

namespace {

void init_hex_test_data(std::list<std::tuple<pdk::plonglong, ByteArray>> &data)
{
   data.push_back(std::make_tuple(PDK_INT64_C(0), ByteArray("0x0")));
   data.push_back(std::make_tuple(PDK_INT64_C(1), ByteArray("0x1")));
   data.push_back(std::make_tuple(PDK_INT64_C(2), ByteArray("0x2")));
   data.push_back(std::make_tuple(PDK_INT64_C(3), ByteArray("0x3")));
   data.push_back(std::make_tuple(PDK_INT64_C(4), ByteArray("0x4")));
   data.push_back(std::make_tuple(PDK_INT64_C(5), ByteArray("0x5")));
   data.push_back(std::make_tuple(PDK_INT64_C(6), ByteArray("0x6")));
   data.push_back(std::make_tuple(PDK_INT64_C(7), ByteArray("0x7")));
   data.push_back(std::make_tuple(PDK_INT64_C(8), ByteArray("0x8")));
   data.push_back(std::make_tuple(PDK_INT64_C(9), ByteArray("0x9")));
   data.push_back(std::make_tuple(PDK_INT64_C(0xa), ByteArray("0xa")));
   data.push_back(std::make_tuple(PDK_INT64_C(0xb), ByteArray("0xb")));
   data.push_back(std::make_tuple(PDK_INT64_C(0xc), ByteArray("0xc")));
   data.push_back(std::make_tuple(PDK_INT64_C(0xd), ByteArray("0xd")));
   data.push_back(std::make_tuple(PDK_INT64_C(0xe), ByteArray("0xe")));
   data.push_back(std::make_tuple(PDK_INT64_C(0xf), ByteArray("0xf")));
   data.push_back(std::make_tuple(PDK_INT64_C(-1), ByteArray("-0x1")));
   data.push_back(std::make_tuple(PDK_INT64_C(0xffffffff), ByteArray("0xffffffff")));
   data.push_back(std::make_tuple(PDK_INT64_C(0xfffffffffffffffe), ByteArray("-0x2")));
   data.push_back(std::make_tuple(PDK_INT64_C(0xffffffffffffffff), ByteArray("-0x1")));
   data.push_back(std::make_tuple(PDK_INT64_C(0x7fffffffffffffff), ByteArray("0x7fffffffffffffff")));
}

void init_bin_test(std::list<std::tuple<int, ByteArray>> &data)
{
   data.push_back(std::make_tuple(0, ByteArray("0b0")));
   data.push_back(std::make_tuple(1, ByteArray("0b1")));
   data.push_back(std::make_tuple(2, ByteArray("0b10")));
   data.push_back(std::make_tuple(5, ByteArray("0b101")));
   data.push_back(std::make_tuple(-1, ByteArray("-0b1")));
   data.push_back(std::make_tuple(0xff, ByteArray("0b11111111")));
   data.push_back(std::make_tuple(0xffff, ByteArray("0b1111111111111111")));
   data.push_back(std::make_tuple(0xfefe, ByteArray("0b1111111011111110")));
}

void init_oct_test(std::list<std::tuple<int, ByteArray>> &data)
{
   data.push_back(std::make_tuple(0, ByteArray("00")));
   data.push_back(std::make_tuple(1, ByteArray("01")));
   data.push_back(std::make_tuple(2, ByteArray("02")));
   data.push_back(std::make_tuple(5, ByteArray("05")));
   data.push_back(std::make_tuple(-1, ByteArray("-01")));
   data.push_back(std::make_tuple(0xff, ByteArray("0377")));
   data.push_back(std::make_tuple(0xffff, ByteArray("0177777")));
   data.push_back(std::make_tuple(0xfefe, ByteArray("0177376")));
}

} // anonymous namespace

TEST_F(TextStreamTest, testHexTest)
{
   std::list<std::tuple<pdk::plonglong, ByteArray>> tdata;
   init_hex_test_data(tdata);
   for (auto &item : tdata) {
      pdk::plonglong number = std::get<0>(item);
      ByteArray &data = std::get<1>(item);
      ByteArray array;
      TextStream stream(&array);
      stream << pdk::io::showbase << pdk::io::hex << number;
      stream.flush();
      ASSERT_EQ(array, data);
   }
}

TEST_F(TextStreamTest, testBinTest)
{
   std::list<std::tuple<int, ByteArray>> tdata;
   init_bin_test(tdata);
   for (auto &item : tdata) {
      int number = std::get<0>(item);
      ByteArray &data = std::get<1>(item);
      ByteArray array;
      TextStream stream(&array);
      stream << pdk::io::showbase << pdk::io::bin << number;
      stream.flush();
      ASSERT_EQ(array, data);
   }
}

TEST_F(TextStreamTest, testOctTest)
{
   std::list<std::tuple<int, ByteArray>> tdata;
   init_oct_test(tdata);
   for (auto &item : tdata) {
      int number = std::get<0>(item);
      ByteArray &data = std::get<1>(item);
      ByteArray array;
      TextStream stream(&array);
      stream << pdk::io::showbase << pdk::io::oct << number;
      stream.flush();
      ASSERT_EQ(array, data);
   }
}

TEST_F(TextStreamTest, testZeroTermination)
{
   TextStream stream;
   char c = '@';
   stream >> c;
   ASSERT_EQ(c, '\0');
   
   c = '@';
   
   stream >> &c;
   ASSERT_EQ(c, '\0');
}

TEST_F(TextStreamTest, testWsManipulator)
{
   {
      String string = Latin1String("a b c d");
      TextStream stream(&string);
      
      char a, b, c, d;
      stream >> a >> b >> c >> d;
      ASSERT_EQ(a, 'a');
      ASSERT_EQ(b, ' ');
      ASSERT_EQ(c, 'b');
      ASSERT_EQ(d, ' ');
   }
   {
      String string = Latin1String("a b c d");
      TextStream stream(&string);
      
      char a, b, c, d;
      stream >> a >> pdk::io::ws >> b >> pdk::io::ws >> c >> pdk::io::ws >> d;
      ASSERT_EQ(a, 'a');
      ASSERT_EQ(b, 'b');
      ASSERT_EQ(c, 'c');
      ASSERT_EQ(d, 'd');
   }
}

TEST_F(TextStreamTest, testStillOpenWhenAtEnd)
{
   File file(FIND_SOURCE_DATA(TextStreamTest.cpp));
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly));
   TextStream stream(&file);
   while (!stream.readLine().isEmpty()) {}
   ASSERT_TRUE(file.isOpen());
   // @TODO add socket testcases
}
