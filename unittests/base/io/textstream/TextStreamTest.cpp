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
#include "pdktest/PdkTest.h"

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
using pdk::os::process::Process;

#define PDKTEST_DIR_SEP "/"
#define FIND_SOURCE_DATA(name) Latin1String(PDKTEST_CURRENT_SOURCE_DIR PDKTEST_DIR_SEP PDK_STRINGIFY(name))
#define APP_FILENAME(name) Latin1String(PDKTEST_TEXTSTREAM_APPS_DIR PDKTEST_DIR_SEP PDK_STRINGIFY(name)) 

int sg_argc;
char **sg_argv;

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

namespace {

void init_read_newlines_data(std::list<std::tuple<ByteArray, String>> &data)
{
   data.push_back(std::make_tuple(ByteArray(), String()));
   data.push_back(std::make_tuple(ByteArray("\r\n"), String(Latin1String("\n"))));
   data.push_back(std::make_tuple(ByteArray("\r\r\n"), String(Latin1String("\n"))));
   data.push_back(std::make_tuple(ByteArray("\r\n\r\n"), String(Latin1String("\n\n"))));
   data.push_back(std::make_tuple(ByteArray("\n"), String(Latin1String("\n"))));
   data.push_back(std::make_tuple(ByteArray("\n\n"), String(Latin1String("\n\n"))));
}

} // anonymous namespace

TEST_F(TextStreamTest, testReadNewLines)
{
   std::list<std::tuple<ByteArray, String>> data;
   init_read_newlines_data(data);
   for (auto &item : data) {
      ByteArray &input = std::get<0>(item);
      String &output = std::get<1>(item);
      
      Buffer buffer(&input);
      buffer.open(Buffer::OpenModes(Buffer::OpenMode::ReadOnly) | Buffer::OpenMode::Text);
      TextStream stream(&buffer);
      ASSERT_EQ(stream.readAll(), output);
   }
}

TEST_F(TextStreamTest, testSeek)
{
   File file(sg_rfc3261FilePath);
   ASSERT_TRUE(file.open(File::OpenMode::ReadOnly));
   
   TextStream stream(&file);
   String tmp;
   stream >> tmp;
   ASSERT_EQ(tmp, String::fromLatin1("Network"));
   // TextStream::seek(0) should both clear its internal read/write buffers
   // and seek the device.
   for (int i = 0; i < 4; ++i) {
      stream.seek(6 + i);
      stream >> tmp;
      ASSERT_EQ(tmp, String(Latin1String("Network")).substring(i));
   }
   
   for (int i = 0; i < 4; ++i) {
      stream.seek(10 - i);
      stream >> tmp;
      ASSERT_EQ(tmp, String(Latin1String("Network")).substring(4 - i));
   }
   
   stream.seek(2331);
   stream >> tmp;
   
   ASSERT_EQ(tmp, String(Latin1String("authenticate")));
   stream.seek(4273);
   stream >> tmp;
   ASSERT_EQ(tmp, String(Latin1String("35")));
   // Also test this with a string
   String words = Latin1String("thisisa");
   TextStream stream2(&words, IoDevice::OpenMode::ReadOnly);
   stream2 >> tmp;
   ASSERT_EQ(tmp, String::fromLatin1("thisisa"));
   for (int i = 0; i < 4; ++i) {
      stream2.seek(i);
      stream2 >> tmp;
      ASSERT_EQ(tmp, String(Latin1String("thisisa")).substring(i));
   }
   for (int i = 0; i < 4; ++i) {
      stream2.seek(4 - i);
      stream2 >> tmp;
      ASSERT_EQ(tmp, String(Latin1String("thisisa")).substring(4 - i));
   }
}

TEST_F(TextStreamTest, testGetPosition)
{
   {
      // Strings
      String str(Latin1String("this is a test"));
      TextStream stream(&str, IoDevice::OpenMode::ReadWrite);
      ASSERT_EQ(stream.getPosition(), pdk::pint64(0));
      for (int i = 0; i <= str.size(); ++i) {
         ASSERT_TRUE(stream.seek(i));
         ASSERT_EQ(stream.getPosition(), pdk::pint64(i));
      }
      for (int j = str.size(); j >= 0; --j) {
         ASSERT_TRUE(stream.seek(j));
         ASSERT_EQ(stream.getPosition(), pdk::pint64(j));
      }
      ASSERT_TRUE(stream.seek(0));
      
      Character ch;
      stream >> ch;
      ASSERT_EQ(ch, Character('t'));
      
      ASSERT_EQ(stream.getPosition(), pdk::pint64(1));
      ASSERT_TRUE(stream.seek(1));
      ASSERT_EQ(stream.getPosition(), pdk::pint64(1));
      ASSERT_TRUE(stream.seek(0));
      
      String strtmp;
      stream >> strtmp;
      ASSERT_EQ(strtmp, String(Latin1String("this")));
      
      ASSERT_EQ(stream.getPosition(), pdk::pint64(4));
      stream.seek(0);
      stream.seek(4);
      
      stream >> ch;
      ASSERT_EQ(ch, Character(' '));
      ASSERT_EQ(stream.getPosition(), pdk::pint64(5));
      
      stream.seek(10);
      stream >> strtmp;
      ASSERT_EQ(strtmp, String(Latin1String("test")));
      ASSERT_EQ(stream.getPosition(), pdk::pint64(14));
   }
   {
      // Latin1 device
      File file(sg_rfc3261FilePath);
      ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadOnly));
      
      TextStream stream(&file);
      ASSERT_EQ(stream.getPosition(), pdk::pint64(0));
      
      for (int i = 0; i <= file.getSize(); i += 7) {
         ASSERT_TRUE(stream.seek(i));
         ASSERT_EQ(stream.getPosition(), pdk::pint64(i));
      }
      for (int j = file.getSize(); j >= 0; j -= 7) {
         ASSERT_TRUE(stream.seek(j));
         ASSERT_EQ(stream.getPosition(), pdk::pint64(j));
      }
      
      stream.seek(0);
      
      String strtmp;
      stream >> strtmp;
      ASSERT_EQ(strtmp, String(Latin1String("Network")));
      ASSERT_EQ(stream.getPosition(), pdk::pint64(13));
      
      stream.seek(2598);
      ASSERT_EQ(stream.getPosition(), pdk::pint64(2598));
      stream >> strtmp;
      ASSERT_EQ(stream.getPosition(), pdk::pint64(2601));
      ASSERT_EQ(strtmp, String(Latin1String("top")));
   }
   
   {
      // Shift-JIS device
      for (int i = 0; i < 2; ++i) {
         File file(sg_shiftJisFilePath);
         if (i == 0) {
            ASSERT_TRUE(file.open(IoDevice::OpenMode::ReadOnly));
         } else {
            ASSERT_TRUE(file.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | IoDevice::OpenMode::Text));
         }
         TextStream stream(&file);
         stream.setCodec("Shift-JIS");
         ASSERT_TRUE(stream.getCodec());
         
         ASSERT_EQ(stream.getPosition(), pdk::pint64(0));
         for (int i = 0; i <= file.getSize(); i += 7) {
            ASSERT_TRUE(stream.seek(i));
            ASSERT_EQ(stream.getPosition(), pdk::pint64(i));
         }
         for (int j = file.getSize(); j >= 0; j -= 7) {
            ASSERT_TRUE(stream.seek(j));
            ASSERT_EQ(stream.getPosition(), pdk::pint64(j));
         }
         
         stream.seek(2089);
         String strtmp;
         stream >> strtmp;
         ASSERT_EQ(strtmp, String(Latin1String("AUnicode")));
         ASSERT_EQ(stream.getPosition(), pdk::pint64(2097));
         
         stream.seek(43325);
         stream >> strtmp;
         ASSERT_EQ(strtmp, String(Latin1String("Shift-JIS")));
         stream >> strtmp;
         ASSERT_EQ(strtmp, String::fromUtf8("\343\201\247\346\233\270\343\201\213\343\202\214\343\201\237"));
         ASSERT_EQ(stream.getPosition(), pdk::pint64(43345));
         stream >> strtmp;
         ASSERT_EQ(strtmp, String(Latin1String("POD")));
         ASSERT_EQ(stream.getPosition(), pdk::pint64(43349));
      }
   }
}

TEST_F(TextStreamTest, testGetPosition2)
{
   ByteArray data("abcdef\r\nghijkl\r\n");
   Buffer buffer(&data);
   ASSERT_TRUE(buffer.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | IoDevice::OpenMode::Text));
   
   TextStream stream(&buffer);
   
   Character ch;
   
   ASSERT_EQ(stream.getPosition(), pdk::pint64(0));
   stream >> ch;
   ASSERT_EQ(ch, Character('a'));
   ASSERT_EQ(stream.getPosition(), pdk::pint64(1));
   
   String str;
   stream >> str;
   ASSERT_EQ(str, String(Latin1String("bcdef")));
   ASSERT_EQ(stream.getPosition(), pdk::pint64(6));
   
   stream >> str;
   ASSERT_EQ(str, String(Latin1String("ghijkl")));
   ASSERT_EQ(stream.getPosition(), pdk::pint64(14));
   
   // Seek back and try again
   stream.seek(1);
   ASSERT_EQ(stream.getPosition(), pdk::pint64(1));
   stream >> str;
   ASSERT_EQ(str, String(Latin1String("bcdef")));
   ASSERT_EQ(stream.getPosition(), pdk::pint64(6));
   
   stream.seek(6);
   stream >> str;
   ASSERT_EQ(str, String(Latin1String("ghijkl")));
   ASSERT_EQ(stream.getPosition(), pdk::pint64(14));
}

//TEST_F(TextStreamTest, testgetPosition3LargeFile)
//{
//   {
//      File file(sg_testFileName);
//      file.open(IoDevice::OpenModes(IoDevice::OpenMode::WriteOnly) | IoDevice::OpenMode::Text);
//      TextStream out(&file);
//      // NOTE: The unusual spacing is to ensure non-1-character whitespace.
//      String lineString = Latin1String(" 0  1  2\t3  4\t \t5  6  7  8   9 \n");
//      // Approximate 50kb text file
//      const int NbLines = (50*1024) / lineString.length() + 1;
//      for (int line = 0; line < NbLines; ++line) {
//         out << lineString;
//      } 
//      // File is automatically flushed and closed on destruction.
//   }

//   File file(sg_testFileName);
//   file.open(IoDevice::OpenModes(IoDevice::OpenMode::ReadOnly) | IoDevice::OpenMode::Text);
//   TextStream in(&file);
//   const int testValues[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
//   int value;
//   while (true) {
//      in.getPosition();
//      for ( int i = 0; i < 10; ++i ) {
//         in >> value;
//         if (in.getStatus() != TextStream::Status::Ok) {
//            // End case, i == 0 && eof reached.
//            ASSERT_EQ(i, 0);
//            ASSERT_EQ(in.getStatus(), TextStream::Status::ReadPastEnd);
//            return;
//         }
//         ASSERT_EQ(value, testValues[i]);
//      }
//   }
//}

TEST_F(TextStreamTest, testReadAllFromStdin)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process stdinProcess;
   stdinProcess.start(APP_FILENAME(ReadAllStdinProcessApp), IoDevice::OpenModes(IoDevice::OpenMode::ReadWrite) | IoDevice::OpenMode::Text);
   stdinProcess.setReadChannel(Process::ProcessChannel::StandardError);
   
   TextStream stream(&stdinProcess);
   stream.setCodec("ISO-8859-1");
   stream << "hello world" << pdk::io::flush;
   
   stdinProcess.closeWriteChannel();
   ASSERT_TRUE(stdinProcess.waitForFinished(5000));
   ASSERT_EQ(stream.readAll(), String::fromLatin1("hello world\n"));
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(TextStreamTest, testReadLineFromStdin)
{
   PDKTEST_BEGIN_APP_CONTEXT();
   Process stdinProcess;
   stdinProcess.start(APP_FILENAME(ReadLineStdinProcessApp), IoDevice::OpenModes(IoDevice::OpenMode::ReadWrite) | IoDevice::OpenMode::Text);
   stdinProcess.setReadChannel(Process::ProcessChannel::StandardError);
   stdinProcess.write("abc\n");
   ASSERT_TRUE(stdinProcess.waitForReadyRead(5000));
   ASSERT_EQ(stdinProcess.readAll(), ByteArray("abc"));
   
   stdinProcess.write("def\n");
   ASSERT_TRUE(stdinProcess.waitForReadyRead(5000));
   ASSERT_EQ(stdinProcess.readAll(), ByteArray("def"));
   
   stdinProcess.closeWriteChannel();
   
   ASSERT_TRUE(stdinProcess.waitForFinished(5000));
   PDKTEST_END_APP_CONTEXT();
}

TEST_F(TextStreamTest, testRead)
{
   {
      File::remove(Latin1String("testfile"));
      File file(Latin1String("testfile"));
      file.open(File::OpenMode::WriteOnly);
      file.write("4.15 abc ole");
      file.close();
      
      ASSERT_TRUE(file.open(File::OpenMode::ReadOnly));
      TextStream stream(&file);
      ASSERT_EQ(stream.read(0), String(Latin1String("")));
      ASSERT_EQ(stream.read(4), String(Latin1String("4.15")));
      ASSERT_EQ(stream.read(4), String(Latin1String(" abc")));
      stream.seek(1);
      ASSERT_EQ(stream.read(4), String(Latin1String(".15 ")));
      stream.seek(1);
      ASSERT_EQ(stream.read(4), String(Latin1String(".15 ")));
      stream.seek(2);
      ASSERT_EQ(stream.read(4), String(Latin1String("15 a")));
      // ### add tests for reading \r\n etc..
   }
   {
      // File larger than TEXTSTREAM_BUFFERSIZE
      File::remove(Latin1String("testfile"));
      File file(Latin1String("testfile"));
      file.open(File::OpenMode::WriteOnly);
      for (int i = 0; i < 16384 / 8; ++i) {
         file.write("01234567");
      }
      file.write("0");
      file.close();
      
      ASSERT_TRUE(file.open(File::OpenMode::ReadOnly));
      TextStream stream(&file);
      ASSERT_EQ(stream.read(10), String(Latin1String("0123456701")));
      ASSERT_EQ(stream.read(10), String(Latin1String("2345670123")));
      ASSERT_EQ(stream.readAll().size(), 16385-20);
   }
}

TEST_F(TextStreamTest, testExists)
{
   String s;
   TextStream stream(&s);
   stream << s.contains(String(Latin1String("hei")));
   ASSERT_EQ(s, String(Latin1String("0")));
}

TEST_F(TextStreamTest, testForcePoint)
{
   String str;
   TextStream stream(&str);
   stream << pdk::io::fixed << pdk::io::forcepoint << 1.0 << ' ' << 1 << ' ' << 0 << ' ' << -1.0 << ' ' << -1;
   ASSERT_EQ(str, String(Latin1String("1.000000 1 0 -1.000000 -1")));
   
   str.clear();
   stream.seek(0);
   stream << pdk::io::scientific << pdk::io::forcepoint << 1.0 << ' ' << 1 << ' ' << 0 << ' ' << -1.0 << ' ' << -1;
   ASSERT_EQ(str, String(Latin1String("1.000000e+00 1 0 -1.000000e+00 -1")));
   
   str.clear();
   stream.seek(0);
   stream.setRealNumberNotation(TextStream::RealNumberNotation::SmartNotation);
   stream << pdk::io::forcepoint << 1.0 << ' ' << 1 << ' ' << 0 << ' ' << -1.0 << ' ' << -1;
   ASSERT_EQ(str, String(Latin1String("1.00000 1 0 -1.00000 -1")));
}

TEST_F(TextStreamTest, testForceSign)
{
   String str;
   TextStream stream(&str);
   stream << pdk::io::forcesign << 1.2 << ' ' << -1.2 << ' ' << 0;
   ASSERT_EQ(str, String(Latin1String("+1.2 -1.2 +0")));
}

TEST_F(TextStreamTest, testRead0d0d0a)
{
   File file(sg_task113817File);
   file.open(IoDevice::OpenMode::ReadOnly | IoDevice::OpenMode::Text);
   TextStream stream(&file);
   while (!stream.atEnd()) {
      stream.readLine();
   }
}

namespace {

TextStream &noop(TextStream &s)
{
   return s;
}

using pdk::io::TextStreamFunc;

using NumCaseDataType = std::list<std::tuple<TextStreamFunc, TextStreamFunc, TextStreamFunc, TextStreamFunc, int, String>>;

void init_numeral_case_data(NumCaseDataType &data)
{
   TextStreamFunc noop_ = noop;
   TextStreamFunc bin_  = pdk::io::bin;
   TextStreamFunc oct_  = pdk::io::oct;
   TextStreamFunc hex_  = pdk::io::hex;
   TextStreamFunc base  = pdk::io::showbase;
   TextStreamFunc ucb   = pdk::io::uppercasebase;
   TextStreamFunc lcb   = pdk::io::lowercasebase;
   TextStreamFunc ucd   = pdk::io::uppercasedigits;
   TextStreamFunc lcd   = pdk::io::lowercasedigits;
   
   data.push_back(std::make_tuple(noop_, noop_, noop_, noop_, 31, String(Latin1String("31"))));
   data.push_back(std::make_tuple(noop_, base, noop_, noop_, 31, String(Latin1String("31"))));
   
   data.push_back(std::make_tuple(hex_, noop_, noop_, noop_, 31, String(Latin1String("1f"))));
   data.push_back(std::make_tuple(hex_, noop_, noop_, lcd, 31, String(Latin1String("1f"))));
   data.push_back(std::make_tuple(hex_, noop_, ucb, noop_, 31, String(Latin1String("1f"))));
   data.push_back(std::make_tuple(hex_, noop_, noop_, ucd, 31, String(Latin1String("1F"))));
   data.push_back(std::make_tuple(hex_, noop_, lcb, ucd, 31, String(Latin1String("1F"))));
   data.push_back(std::make_tuple(hex_, noop_, ucb, ucd, 31, String(Latin1String("1F"))));
   data.push_back(std::make_tuple(hex_, base, noop_, noop_, 31, String(Latin1String("0x1f"))));
   data.push_back(std::make_tuple(hex_, base, lcb, lcd, 31, String(Latin1String("0x1f"))));
   data.push_back(std::make_tuple(hex_, base, ucb, noop_, 31, String(Latin1String("0X1f"))));
   data.push_back(std::make_tuple(hex_, base, ucb, lcd, 31, String(Latin1String("0X1f"))));
   data.push_back(std::make_tuple(hex_, base, noop_, ucb, 31, String(Latin1String("0X1f"))));
   data.push_back(std::make_tuple(hex_, base, lcb, ucd, 31, String(Latin1String("0x1F"))));
   data.push_back(std::make_tuple(hex_, base, lcb, ucd, 31, String(Latin1String("0x1F"))));
   
   data.push_back(std::make_tuple(bin_, noop_, noop_, noop_, 31, String(Latin1String("11111"))));
   data.push_back(std::make_tuple(bin_, base, noop_, noop_, 31, String(Latin1String("0b11111"))));
   data.push_back(std::make_tuple(bin_, base, lcb, noop_, 31, String(Latin1String("0b11111"))));
   data.push_back(std::make_tuple(bin_, base, ucb, noop_, 31, String(Latin1String("0B11111"))));
   data.push_back(std::make_tuple(bin_, base, noop_, ucd, 31, String(Latin1String("0b11111"))));
   data.push_back(std::make_tuple(bin_, base, lcb, ucd, 31, String(Latin1String("0b11111"))));
   data.push_back(std::make_tuple(bin_, base, ucb, ucd, 31, String(Latin1String("0B11111"))));
   
   data.push_back(std::make_tuple(oct_, noop_, noop_, noop_, 31, String(Latin1String("37"))));
   data.push_back(std::make_tuple(oct_, base, noop_, noop_, 31, String(Latin1String("037"))));
}

} // anonymous namespace

TEST_F(TextStreamTest, testNumeralCase)
{
   NumCaseDataType data;
   init_numeral_case_data(data);
   for (auto &item : data) {
      TextStreamFunc func1 = std::get<0>(item);
      TextStreamFunc func2 = std::get<1>(item);
      TextStreamFunc func3 = std::get<2>(item);
      TextStreamFunc func4 = std::get<3>(item);
      int value = std::get<4>(item);
      String &expected = std::get<5>(item);
      String str;
      TextStream stream(&str);
      stream << func1 << func2 << func3 << func4 << value;
      ASSERT_EQ(str, expected);
   }
}

TEST_F(TextStreamTest, testNanInf)
{
   String str(Latin1String("nan NAN nAn +nan +NAN +nAn -nan -NAN -nAn"
                           " inf INF iNf +inf +INF +iNf -inf -INF -iNf"));
   TextStream stream(&str);
   double tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isnan(tmpD));
   tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isnan(tmpD));
   tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isnan(tmpD));
   tmpD = 0;
   
   stream >> tmpD;
   ASSERT_TRUE(std::isnan(tmpD));
   tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isnan(tmpD));
   tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isnan(tmpD));
   tmpD = 0;
   
   stream >> tmpD;
   ASSERT_TRUE(std::isnan(tmpD));
   tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isnan(tmpD));
   tmpD = 0;  
   stream >> tmpD;
   ASSERT_TRUE(std::isnan(tmpD));
   tmpD = 0;
   
   stream >> tmpD;
   ASSERT_TRUE(std::isinf(tmpD));
   ASSERT_TRUE(tmpD > 0);
   tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isinf(tmpD));
   ASSERT_TRUE(tmpD > 0);
   tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isinf(tmpD));
   ASSERT_TRUE(tmpD > 0);
   tmpD = 0;
   
   stream >> tmpD;
   ASSERT_TRUE(std::isinf(tmpD));
   ASSERT_TRUE(tmpD > 0);
   tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isinf(tmpD));
   ASSERT_TRUE(tmpD > 0);
   tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isinf(tmpD));
   ASSERT_TRUE(tmpD > 0);
   tmpD = 0;
   
   stream >> tmpD;
   ASSERT_TRUE(std::isinf(tmpD));
   ASSERT_TRUE(tmpD < 0);
   tmpD = 0;
   stream >> tmpD;
   ASSERT_TRUE(std::isinf(tmpD));
   ASSERT_TRUE(tmpD < 0);
   tmpD = 0;  
   stream >> tmpD;
   ASSERT_TRUE(std::isinf(tmpD));
   ASSERT_TRUE(tmpD < 0);
   tmpD = 0;
   
   stream.seek(0);
   float tmpF = 0;
   
   stream >> tmpF;
   ASSERT_TRUE(std::isnan(tmpF));
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isnan(tmpF));
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isnan(tmpF));
   tmpF = 0;
   
   stream >> tmpF;
   ASSERT_TRUE(std::isnan(tmpF));
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isnan(tmpF));
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isnan(tmpF));
   tmpF = 0;
   
   stream >> tmpF;
   ASSERT_TRUE(std::isnan(tmpF));
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isnan(tmpF));
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isnan(tmpF));
   tmpF = 0;
   
   stream >> tmpF;
   ASSERT_TRUE(std::isinf(tmpF));
   ASSERT_TRUE(tmpF > 0);
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isinf(tmpF));
   ASSERT_TRUE(tmpF > 0);
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isinf(tmpF));
   ASSERT_TRUE(tmpF > 0);
   tmpF = 0;
   
   stream >> tmpF;
   ASSERT_TRUE(std::isinf(tmpF));
   ASSERT_TRUE(tmpF > 0);
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isinf(tmpF));
   ASSERT_TRUE(tmpF > 0);
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isinf(tmpF));
   ASSERT_TRUE(tmpF > 0);
   tmpF = 0;
   
   stream >> tmpF;
   ASSERT_TRUE(std::isinf(tmpF));
   ASSERT_TRUE(tmpF < 0);
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isinf(tmpF));
   ASSERT_TRUE(tmpF < 0);
   tmpF = 0;
   stream >> tmpF;
   ASSERT_TRUE(std::isinf(tmpF));
   ASSERT_TRUE(tmpF < 0);
   tmpF = 0;
   
   String s;
   TextStream out(&s);
   out << pdk::inf() << ' ' << -pdk::inf() << ' ' << pdk::qnan()
       << pdk::io::uppercasedigits << ' '
       << pdk::inf() << ' ' << -pdk::inf() << ' ' << pdk::qnan()
       << pdk::io::flush;
}

namespace {

void init_utf8_incomplete_at_buffer_boundary_data(std::list<bool> &data)
{
   data.push_back(false);
   if (String(Character::SpecialCharacter::ReplacementCharacter).toLocal8Bit() == "\xef\xbf\xbd") {
      data.push_back(true);
   }
}

} // anonymous namespace

TEST_F(TextStreamTest, testUtf8IncompleteAtBufferBoundary)
{
   File::remove(sg_testFileName);
   File data(sg_testFileName);
   TextCodec *utf8Codec = TextCodec::codecForMib(106);
   String lineContents = String::fromUtf8("\342\200\223" // U+2013 EN DASH
                                          "\342\200\223"
                                          "\342\200\223"
                                          "\342\200\223"
                                          "\342\200\223"
                                          "\342\200\223");
   data.open(File::OpenModes(File::OpenMode::WriteOnly) | File::OpenMode::Truncate);
   {
      TextStream out(&data);
      out.setCodec(utf8Codec);
      out.setFieldWidth(3);
      
      for (int i = 0; i < 1000; ++i) {
         out << i << lineContents << pdk::io::endl;
      }
   }
   data.close();
   
   std::list<bool> tdata;
   init_utf8_incomplete_at_buffer_boundary_data(tdata);
   for (auto useLocale : tdata) {
      data.open(File::OpenMode::ReadOnly);
      TextStream in(&data);
      if (!useLocale) {
         in.setCodec(utf8Codec); // Utf8Codec
      } else {
         in.setCodec(TextCodec::getCodecForLocale());
      }
      int i = 0;
      do {
         String line = in.readLine().trimmed();
         ++i;
         ASSERT_TRUE(line.endsWith(lineContents)) << String(Latin1String("Line %1: %2")).arg(i).arg(line);
      } while (!in.atEnd());
      data.close();
   }
}

TEST_F(TextStreamTest, testWriteSeekWriteNoBOM)
{
   //First with the default codec (normally either latin-1 or UTF-8)
   Buffer out;
   out.open(IoDevice::OpenMode::WriteOnly);
   TextStream stream(&out);
   int number = 0;
   String sizeStr = Latin1String("Size=")
         + String::number(number).rightJustified(10, Latin1Character('0'));
   stream << sizeStr << pdk::io::endl;
   stream << "Version=" << String::number(14) << pdk::io::endl;
   stream << "blah blah blah" << pdk::io::endl;
   stream.flush();
   
   ASSERT_STREQ(out.getBuffer().getConstRawData(), "Size=0000000000\nVersion=14\nblah blah blah\n");
   
   // Now overwrite the size header item
   number = 42;
   stream.seek(0);
   sizeStr = Latin1String("Size=")
         + String::number(number).rightJustified(10, Latin1Character('0'));
   stream << sizeStr << pdk::io::endl;
   stream.flush();
   
   // Check buffer is still OK
   ASSERT_STREQ(out.getBuffer().getConstRawData(), "Size=0000000042\nVersion=14\nblah blah blah\n");
   
   //Then UTF-16
   
   Buffer out16;
   out16.open(IoDevice::OpenMode::WriteOnly);
   TextStream stream16(&out16);
   stream16.setCodec("UTF-16");
   
   stream16 << "one" << "two" << Latin1String("three");
   stream16.flush();
   
   // save that output
   ByteArray first = out16.getBuffer();
   
   stream16.seek(0);
   stream16 << "one";
   stream16.flush();
   
   ASSERT_EQ(out16.getBuffer(), first);
}

namespace {

void generate_operator_char_data(std::list<std::tuple<ByteArray, Character, char, ByteArray>> &data, bool forString)
{
   // empty
   data.push_back(std::make_tuple(ByteArray(), Character('\0'), '\0', ByteArray("\0", 1)));
   // a
   data.push_back(std::make_tuple(ByteArray("a"), Character('a'), 'a', ByteArray("a")));
   // \\na
   data.push_back(std::make_tuple(ByteArray("\na"), Character('\n'), '\n', ByteArray("\n")));
   // \\0
   data.push_back(std::make_tuple(ByteArray("\0"), Character('\0'), '\0', ByteArray("\0", 1)));
   // \\xff
   data.push_back(std::make_tuple(ByteArray("\xff"), Character('\xff'), '\xff', ByteArray("\xff")));
   // \\xfe
   data.push_back(std::make_tuple(ByteArray("\xfe"), Character('\xfe'), '\xfe', ByteArray("\xfe")));
   if (!forString) {
      // utf16-BE (empty)
      data.push_back(std::make_tuple(ByteArray("\xff\xfe", 2), Character('\0'), '\0', ByteArray("\0", 1)));
      // utf16-BE (a)
      data.push_back(std::make_tuple(ByteArray("\xff\xfe\x61\x00", 4), Character('a'), 'a', ByteArray("a")));
      // utf16-LE (empty)
      data.push_back(std::make_tuple(ByteArray("\xfe\xff", 2), Character('\0'), '\0', ByteArray("\0", 1)));
      // utf16-LE (a)
      data.push_back(std::make_tuple(ByteArray("\xfe\xff\x00\x61", 4), Character('a'), 'a', ByteArray("a")));
   }
}

} // anonymous namespace

TEST_F(TextStreamTest, testCharacterOperatorsFromDevice)
{
   std::list<std::tuple<ByteArray, Character, char, ByteArray>> data;
   generate_operator_char_data(data, false);
   for (auto &item : data) {
      ByteArray &input = std::get<0>(item);
      Character &charOutput = std::get<1>(item);
      ByteArray &writeOutput = std::get<3>(item);
      
      Buffer buf(&input);
      buf.open(Buffer::OpenMode::ReadOnly);
      TextStream stream(&buf);
      stream.setCodec(TextCodec::codecForName("ISO-8859-1"));
      Character tmp;
      stream >> tmp;
      ASSERT_EQ(tmp, charOutput);
      
      Buffer writeBuf;
      writeBuf.open(Buffer::OpenMode::WriteOnly);
      
      TextStream writeStream(&writeBuf);
      writeStream.setCodec(TextCodec::codecForName("ISO-8859-1"));
      writeStream << charOutput;
      writeStream.flush();
      
      ASSERT_EQ(writeBuf.getBuffer().size(), writeOutput.size());
      ASSERT_STREQ(writeBuf.getBuffer().getConstRawData(), writeOutput.getConstRawData());
   }
}

TEST_F(TextStreamTest, testCharOperatorsFromDevice)
{
   std::list<std::tuple<ByteArray, Character, char, ByteArray>> data;
   generate_operator_char_data(data, false);
   for (auto &item : data) {
      ByteArray &input = std::get<0>(item);
      char charOutput = std::get<2>(item);
      ByteArray &writeOutput = std::get<3>(item);
      Buffer buf(&input);
      buf.open(Buffer::OpenMode::ReadOnly);
      TextStream stream(&buf);
      stream.setCodec(TextCodec::codecForName("ISO-8859-1"));
      char tmp;
      stream >> tmp;
      ASSERT_EQ(tmp, charOutput);
      
      Buffer writeBuf;
      writeBuf.open(Buffer::OpenMode::WriteOnly);
      TextStream writeStream(&writeBuf);
      writeStream.setCodec(TextCodec::codecForName("ISO-8859-1"));
      writeStream << charOutput;
      writeStream.flush();
      
      ASSERT_EQ(writeBuf.getBuffer().size(), writeOutput.size());
      ASSERT_STREQ(writeBuf.getBuffer().getConstRawData(), writeOutput.getConstRawData());
   }
}

namespace {

void generate_natural_numbers_data(std::list<std::tuple<ByteArray, pdk::pulonglong>> &data, bool forString)
{
   data.push_back(std::make_tuple(ByteArray(), pdk::pulonglong(0)));
   data.push_back(std::make_tuple(ByteArray("a"), pdk::pulonglong(0)));
   data.push_back(std::make_tuple(ByteArray(" "), pdk::pulonglong(0)));
   data.push_back(std::make_tuple(ByteArray("0"), pdk::pulonglong(0)));
   data.push_back(std::make_tuple(ByteArray("1"), pdk::pulonglong(1)));
   data.push_back(std::make_tuple(ByteArray("12"), pdk::pulonglong(12)));
   data.push_back(std::make_tuple(ByteArray("-12"), pdk::pulonglong(-12)));
   data.push_back(std::make_tuple(ByteArray("-0"), pdk::pulonglong(0)));
   data.push_back(std::make_tuple(ByteArray(" 1"), pdk::pulonglong(1)));
   data.push_back(std::make_tuple(ByteArray(" \r\n\r\n123"), pdk::pulonglong(123)));
   
   data.push_back(std::make_tuple(ByteArray("127"), pdk::pulonglong(127)));
   data.push_back(std::make_tuple(ByteArray("128"), pdk::pulonglong(128)));
   data.push_back(std::make_tuple(ByteArray("129"), pdk::pulonglong(129)));
   data.push_back(std::make_tuple(ByteArray("-127"), pdk::pulonglong(-127)));
   data.push_back(std::make_tuple(ByteArray("-128"), pdk::pulonglong(-128)));
   data.push_back(std::make_tuple(ByteArray("-129"), pdk::pulonglong(-129)));
   
   data.push_back(std::make_tuple(ByteArray("32767"), pdk::pulonglong(32767)));
   data.push_back(std::make_tuple(ByteArray("32768"), pdk::pulonglong(32768)));
   data.push_back(std::make_tuple(ByteArray("32769"), pdk::pulonglong(32769)));
   
   data.push_back(std::make_tuple(ByteArray("-32767"), pdk::pulonglong(-32767)));
   data.push_back(std::make_tuple(ByteArray("-32768"), pdk::pulonglong(-32768)));
   data.push_back(std::make_tuple(ByteArray("-32769"), pdk::pulonglong(-32769)));
   
   data.push_back(std::make_tuple(ByteArray("65537"), pdk::pulonglong(65537)));
   data.push_back(std::make_tuple(ByteArray("65536"), pdk::pulonglong(65536)));
   data.push_back(std::make_tuple(ByteArray("65535"), pdk::pulonglong(65535)));
   
   data.push_back(std::make_tuple(ByteArray("-65537"), pdk::pulonglong(-65537)));
   data.push_back(std::make_tuple(ByteArray("-65536"), pdk::pulonglong(-65536)));
   data.push_back(std::make_tuple(ByteArray("-65535"), pdk::pulonglong(-65535)));
   
   data.push_back(std::make_tuple(ByteArray("2147483646"), pdk::pulonglong(2147483646)));
   data.push_back(std::make_tuple(ByteArray("2147483647"), pdk::pulonglong(2147483647)));
   data.push_back(std::make_tuple(ByteArray("2147483648"), PDK_UINT64_C(2147483648)));
   
   data.push_back(std::make_tuple(ByteArray("-2147483646"), pdk::pulonglong(-2147483646)));
   data.push_back(std::make_tuple(ByteArray("-2147483647"), pdk::pulonglong(-2147483647)));
   data.push_back(std::make_tuple(ByteArray("-2147483648"), pdk::puint64(-2147483648LL)));
   
   data.push_back(std::make_tuple(ByteArray("4294967296"), PDK_UINT64_C(4294967296)));
   data.push_back(std::make_tuple(ByteArray("4294967297"), PDK_UINT64_C(4294967297)));
   data.push_back(std::make_tuple(ByteArray("4294967298"), PDK_UINT64_C(4294967298)));
   
   data.push_back(std::make_tuple(ByteArray("-4294967296"), pdk::puint64(-4294967296)));
   data.push_back(std::make_tuple(ByteArray("-4294967297"), pdk::puint64(-4294967297)));
   data.push_back(std::make_tuple(ByteArray("-4294967298"), pdk::puint64(-4294967298)));
   
   data.push_back(std::make_tuple(ByteArray("9223372036854775807"), PDK_UINT64_C(9223372036854775807)));
   data.push_back(std::make_tuple(ByteArray("9223372036854775808"), PDK_UINT64_C(9223372036854775808)));
   data.push_back(std::make_tuple(ByteArray("9223372036854775809"), PDK_UINT64_C(9223372036854775809)));
   
   data.push_back(std::make_tuple(ByteArray("18446744073709551615"), PDK_UINT64_C(18446744073709551615)));
   data.push_back(std::make_tuple(ByteArray("0"), PDK_UINT64_C(0)));
   data.push_back(std::make_tuple(ByteArray("1"), PDK_UINT64_C(1)));
   
   // hex tests
   data.push_back(std::make_tuple(ByteArray("0x0"), pdk::pulonglong(0)));
   data.push_back(std::make_tuple(ByteArray("0x"), pdk::pulonglong(0)));
   
   data.push_back(std::make_tuple(ByteArray("0x1"), pdk::pulonglong(1)));
   data.push_back(std::make_tuple(ByteArray("0xf"), pdk::pulonglong(15)));
   
   data.push_back(std::make_tuple(ByteArray("0xdeadbeef"), PDK_UINT64_C(3735928559)));
   data.push_back(std::make_tuple(ByteArray("0XDEADBEEF"), PDK_UINT64_C(3735928559)));
   data.push_back(std::make_tuple(ByteArray("0xdeadbeefZzzzz"), PDK_UINT64_C(3735928559)));
   data.push_back(std::make_tuple(ByteArray("  0xdeadbeefZzzzz"), PDK_UINT64_C(3735928559)));
   
   // oct tests
   data.push_back(std::make_tuple(ByteArray("00"), pdk::pulonglong(0)));
   data.push_back(std::make_tuple(ByteArray("0141"), pdk::pulonglong(97)));
   data.push_back(std::make_tuple(ByteArray("01419999"), pdk::pulonglong(97)));
   data.push_back(std::make_tuple(ByteArray("  01419999"), pdk::pulonglong(97)));
   
   data.push_back(std::make_tuple(ByteArray("0b0"), pdk::pulonglong(0)));
   data.push_back(std::make_tuple(ByteArray("0b1"), pdk::pulonglong(1)));
   data.push_back(std::make_tuple(ByteArray("0b10"), pdk::pulonglong(2)));
   data.push_back(std::make_tuple(ByteArray("0B10"), pdk::pulonglong(2)));
   data.push_back(std::make_tuple(ByteArray("0b101010"), pdk::pulonglong(42)));
   data.push_back(std::make_tuple(ByteArray("0b1010102345"), pdk::pulonglong(42)));
   data.push_back(std::make_tuple(ByteArray("  0b1010102345"), pdk::pulonglong(42)));
   
   // utf-16 tests
   if (!forString) {
      // utf16-BE (empty)
      data.push_back(std::make_tuple(ByteArray("\xfe\xff", 2), pdk::pulonglong(0)));
      // utf16-BE (0xdeadbeef)
      data.push_back(std::make_tuple(ByteArray("\xfe\xff"
                                               "\x00\x30\x00\x78\x00\x64\x00\x65\x00\x61\x00\x64\x00\x62\x00\x65\x00\x65\x00\x66", 22), PDK_UINT64_C(3735928559)));
      // utf16-LE (empty)"
      data.push_back(std::make_tuple(ByteArray("\xff\xfe", 2), PDK_UINT64_C(0)));
      // utf16-LE (0xdeadbeef)
      data.push_back(std::make_tuple(ByteArray("\xff\xfe"
                                               "\x30\x00\x78\x00\x64\x00\x65\x00\x61\x00\x64\x00\x62\x00\x65\x00\x65\x00\x66\x00", 22), PDK_UINT64_C(3735928559)));
   }
}

} // anonymous namespace

#define IMPLEMENT_STREAM_RIGHT_INT_OPERATOR_TEST(texttype, type) \
   TEST_F(TextStreamTest, test##texttype##ReadOperatorFromDevice)\
{ \
   std::list<std::tuple<ByteArray, pdk::pulonglong>> data;\
   generate_natural_numbers_data(data, false);\
   for (auto &item : data) {\
      ByteArray &input = std::get<0>(item);\
      pdk::pulonglong output = std::get<1>(item);\
      type sh; \
      TextStream stream(&input); \
      stream >> sh; \
      ASSERT_EQ(sh, (type)output); \
   }\
   }

using pdkplonglong = pdk::plonglong;
using pdkpulonglong = pdk::pulonglong;

IMPLEMENT_STREAM_RIGHT_INT_OPERATOR_TEST(signedShort, signed short)
IMPLEMENT_STREAM_RIGHT_INT_OPERATOR_TEST(unsignedShort, unsigned short)
IMPLEMENT_STREAM_RIGHT_INT_OPERATOR_TEST(signedInt, signed int)
IMPLEMENT_STREAM_RIGHT_INT_OPERATOR_TEST(unsignedInt, unsigned int)
IMPLEMENT_STREAM_RIGHT_INT_OPERATOR_TEST(pdkplonglong, pdk::plonglong)
IMPLEMENT_STREAM_RIGHT_INT_OPERATOR_TEST(pdkpulonglong, pdk::pulonglong)
;

namespace {

void generate_real_numbers_data(std::list<std::tuple<ByteArray, double>> &data, bool forString)
{
   data.push_back(std::make_tuple(ByteArray(), 0.0));
   data.push_back(std::make_tuple(ByteArray("a"), 0.0));
   data.push_back(std::make_tuple(ByteArray("1.0"), 1.0));
   data.push_back(std::make_tuple(ByteArray(" 1"), 1.0));
   data.push_back(std::make_tuple(ByteArray(" \r\n1.2"), 1.2));
   data.push_back(std::make_tuple(ByteArray("3.14"), 3.14));
   data.push_back(std::make_tuple(ByteArray("-3.14"), -3.14));
   data.push_back(std::make_tuple(ByteArray(" -3.14"), -3.14));
   data.push_back(std::make_tuple(ByteArray("314e-02"), 3.14));
   data.push_back(std::make_tuple(ByteArray("314E-02"), 3.14));
   data.push_back(std::make_tuple(ByteArray("314e+02"), 31400.));
   data.push_back(std::make_tuple(ByteArray("314E+02"), 31400.));
   if (!forString) {
      // utf16-BE (empty)
      data.push_back(std::make_tuple(ByteArray("\xff\xfe", 2),  0.0));
      // utf16-LE (empty)
      data.push_back(std::make_tuple(ByteArray("\xfe\xff", 2),  0.0));
   }
}

} // anonymous namespace

#define IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR_TEST(texttype, type) \
   TEST_F(TextStreamTest, test##texttype##ReadOperatorFromDevice)\
{ \
   std::list<std::tuple<ByteArray, double>> data;\
   generate_real_numbers_data(data, false);\
   for (auto &item : data) {\
      ByteArray &input = std::get<0>(item);\
      double output = std::get<1>(item);\
      type sh; \
      TextStream stream(&input); \
      stream >> sh; \
      ASSERT_EQ(sh, (type)output); \
   }\
}

IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR_TEST(float, float)
IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR_TEST(double, double)
;

namespace {

void generate_string_data(std::list<std::tuple<ByteArray, ByteArray, String>> data, bool forString)
{
   data.push_back(std::make_tuple(ByteArray(), ByteArray(), String()));
   data.push_back(std::make_tuple(ByteArray("a"), ByteArray("a"), String(Latin1String("a"))));
   data.push_back(std::make_tuple(ByteArray("a b"), ByteArray("a b"), String(Latin1String("a"))));
   data.push_back(std::make_tuple(ByteArray(" a b"), ByteArray(" a b"), String(Latin1String("a"))));
   data.push_back(std::make_tuple(ByteArray("a1"), ByteArray("a1"), String(Latin1String("a1"))));
   data.push_back(std::make_tuple(ByteArray("a1 b1"), ByteArray("a1"), String(Latin1String("a1"))));
   data.push_back(std::make_tuple(ByteArray(" a1 b1"), ByteArray("a1"), String(Latin1String("a1"))));
   data.push_back(std::make_tuple(ByteArray("\n\n\nole i dole\n"), ByteArray("ole"), String(Latin1String("ole"))));
   
   if (!forString) {
      // utf16-BE (empty)
      data.push_back(std::make_tuple(ByteArray("\xff\xfe", 2), ByteArray(), String()));
      // utf16-BE (corrupt)
      data.push_back(std::make_tuple(ByteArray("\xff", 1), ByteArray("\xff"), String::fromLatin1("\xff")));
      // utf16-LE (empty)
      data.push_back(std::make_tuple(ByteArray("\xfe\xff", 2), ByteArray(), String()));
      // utf16-LE (corrupt)
       data.push_back(std::make_tuple(ByteArray("\xfe", 1), ByteArray("\xfe"), String::fromLatin1("\xfe")));
   }
}

} // anonymous namespace

TEST_F(TextStreamTest, testCharPtrReadOperatorFromDevice)
{
   std::list<std::tuple<ByteArray, ByteArray, String>> data;
   generate_string_data(data, false);
   for (auto &item : data) {
      ByteArray &input = std::get<0>(item);
      ByteArray &arrayOutput = std::get<1>(item);
      Buffer buffer(&input);
      buffer.open(Buffer::OpenMode::ReadOnly);
      TextStream stream(&buffer);
      stream.setCodec(TextCodec::codecForName("ISO-8859-1"));
      stream.setAutoDetectUnicode(true);
      
      char buf[1024];
      stream >> buf;
      
      ASSERT_STREQ((const char *)buf, arrayOutput.getConstRawData());
   }
}

int main(int argc, char **argv)
{
   sg_argc = argc;
   sg_argv = argv;
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
