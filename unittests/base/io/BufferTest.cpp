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
// Created by zzu_softboy on 2018/03/13.

#include "gtest/gtest.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/io/Buffer.h"
#include "pdk/base/io/IoDevice.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/lang/Character.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdktest/PdkTest.h"

#include <list>
#include <tuple>

using pdk::io::Buffer;
using pdk::ds::ByteArray;
using pdk::io::IoDevice;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::Character;
using pdktest::TestEventLoop;

int sg_argc;
char **sg_argv;

TEST(BufferTest, testGetSetCheck)
{
   Buffer obj1;
   ByteArray var1("libpdk text data");
   obj1.setData(var1);
   ASSERT_EQ(var1, obj1.getData());
   obj1.setData(ByteArray());
   ASSERT_EQ(ByteArray(), obj1.getData());
}

TEST(BufferTest, testOpen)
{
   ByteArray data(10, 'f');
   Buffer buffer;
   ASSERT_TRUE(!buffer.open(IoDevice::OpenMode::NotOpen));
   ASSERT_TRUE(!buffer.isOpen());
   buffer.close();

   ASSERT_TRUE(!buffer.open(IoDevice::OpenMode::Text));
   ASSERT_TRUE(!buffer.isOpen());
   buffer.close();

   ASSERT_TRUE(!buffer.open(IoDevice::OpenMode::Unbuffered));
   ASSERT_TRUE(!buffer.isOpen());
   buffer.close();

   ASSERT_TRUE(buffer.open(IoDevice::OpenMode::ReadOnly));
   ASSERT_TRUE(buffer.isReadable());
   buffer.close();

   ASSERT_TRUE(buffer.open(IoDevice::OpenMode::WriteOnly));
   ASSERT_TRUE(buffer.isWritable());
   buffer.close();

   buffer.setData(data);
   ASSERT_TRUE(buffer.open(IoDevice::OpenMode::Append));
   ASSERT_TRUE(buffer.isWritable());
   ASSERT_EQ(buffer.getSize(), pdk::pint64(10));
   ASSERT_EQ(buffer.getPosition(), buffer.getSize());
   buffer.close();

   buffer.setData(data);
   ASSERT_TRUE(buffer.open(IoDevice::OpenMode::Truncate));
   ASSERT_TRUE(buffer.isWritable());
   ASSERT_EQ(buffer.getSize(), pdk::pint64(0));
   ASSERT_EQ(buffer.getPosition(), pdk::pint64(0));
   buffer.close();

   ASSERT_TRUE(buffer.open(IoDevice::OpenMode::ReadWrite));
   ASSERT_TRUE(buffer.isReadable());
   ASSERT_TRUE(buffer.isWritable());
   buffer.close();
}

TEST(BufferTest, testReadBlock)
{
   const int arraySize = 10;
   char array[arraySize];
   Buffer buffer;
   ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(0));
   ASSERT_EQ(buffer.read(array, arraySize), pdk::pint64(-1));
   ASSERT_TRUE(buffer.atEnd());

   ByteArray byteArray;
   byteArray.resize(arraySize);
   buffer.setBuffer(&byteArray);
   ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(arraySize));
   buffer.open(IoDevice::OpenMode::WriteOnly);
   ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(arraySize));
   ASSERT_EQ(buffer.read(array, arraySize), pdk::pint64(-1)); // no read access
   buffer.close();

   buffer.open(IoDevice::OpenMode::ReadOnly);
   ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(arraySize));
   ASSERT_EQ(buffer.read(array, arraySize), pdk::pint64(arraySize));
   ASSERT_TRUE(buffer.atEnd());
   ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(0));

   ASSERT_EQ(buffer.read(array, 1), pdk::pint64(0));
   ASSERT_TRUE(buffer.atEnd());

   buffer.close();
   buffer.open(IoDevice::OpenMode::ReadOnly);
   ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(arraySize));
   ASSERT_EQ(buffer.read(array, arraySize / 2), pdk::pint64(arraySize / 2));
   ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(arraySize / 2));
   ASSERT_EQ(buffer.read(array + arraySize / 2, arraySize - arraySize/2), pdk::pint64(arraySize - arraySize / 2));
   ASSERT_TRUE(buffer.atEnd());
   ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(0));
}

TEST(BufferTest, testReadBlockPastEnd)
{
   ByteArray array(4096 + 3616, 'd');
   Buffer buffer(&array);
   buffer.open(IoDevice::OpenMode::ReadOnly);
   char dummy[4096];
   buffer.read(1);
   ASSERT_EQ(buffer.read(dummy, 4096), pdk::pint64(4096));
   ASSERT_EQ(buffer.read(dummy, 4096), pdk::pint64(3615));
   ASSERT_TRUE(buffer.atEnd());
}

namespace {

void init_write_block_data(std::list<String> &data)
{
   data.push_back(Latin1String("Buffer Test"));
   data.push_back(Latin1String(
                     "The Buffer class is an I/O device that operates on a ByteArray.\n"
                     "Buffer is used to read and write to a memory buffer. It is normally "
                     "used with a TextStream or a DataStream. Buffer has an associated "
                     "ByteArray which holds the buffer data. The size() of the buffer is "
                     "automatically adjusted as data is written.\n"
                     "The constructor Buffer(ByteArray) creates a Buffer using an existing "
                     "byte array. The byte array can also be set with setBuffer(). Writing to "
                     "the Buffer will modify the original byte array because ByteArray is "
                     "explicitly shared.\n"
                     "Use open() to open the buffer before use and to set the mode (read-only, "
                     "write-only, etc.). close() closes the buffer. The buffer must be closed "
                     "before reopening or calling setBuffer().\n"
                     "A common way to use Buffer is through DataStream or TextStream, which "
                     "have constructors that take a Buffer parameter. For convenience, there "
                     "are also DataStream and TextStream constructors that take a ByteArray "
                     "parameter. These constructors create and open an internal Buffer.\n"
                     "Note that TextStream can also operate on a QString (a Unicode string); a "
                     "Buffer cannot.\n"
                     "You can also use Buffer directly through the standard IoDevice functions "
                     "readBlock(), writeBlock() readLine(), at(), getch(), putch() and ungetch().\n"
                     "See also File, DataStream, TextStream, ByteArray, Shared Classes, Collection "
                     "Classes and Input/Output and Networking.\n\n"
                     "The Buffer class is an I/O device that operates on a ByteArray.\n"
                     "Buffer is used to read and write to a memory buffer. It is normally "
                     "used with a TextStream or a DataStream. Buffer has an associated "
                     "ByteArray which holds the buffer data. The size() of the buffer is "
                     "automatically adjusted as data is written.\n"
                     "The constructor Buffer(ByteArray) creates a Buffer using an existing "
                     "byte array. The byte array can also be set with setBuffer(). Writing to "
                     "the Buffer will modify the original byte array because ByteArray is "
                     "explicitly shared.\n"
                     "Use open() to open the buffer before use and to set the mode (read-only, "
                     "write-only, etc.). close() closes the buffer. The buffer must be closed "
                     "before reopening or calling setBuffer().\n"
                     "A common way to use Buffer is through DataStream or TextStream, which "
                     "have constructors that take a Buffer parameter. For convenience, there "
                     "are also DataStream and TextStream constructors that take a ByteArray "
                     "parameter. These constructors create and open an internal Buffer.\n"
                     "Note that TextStream can also operate on a QString (a Unicode string); a "
                     "Buffer cannot.\n"
                     "You can also use Buffer directly through the standard IoDevice functions "
                     "readBlock(), writeBlock() readLine(), at(), getch(), putch() and ungetch().\n"
                     "See also File, DataStream, TextStream, ByteArray, Shared Classes, Collection "
                     "Classes and Input/Output and Networking.\n\n"
                     "The Buffer class is an I/O device that operates on a ByteArray.\n"
                     "Buffer is used to read and write to a memory buffer. It is normally "
                     "used with a TextStream or a DataStream. Buffer has an associated "
                     "ByteArray which holds the buffer data. The size() of the buffer is "
                     "automatically adjusted as data is written.\n"
                     "The constructor Buffer(ByteArray) creates a Buffer using an existing "
                     "byte array. The byte array can also be set with setBuffer(). Writing to "
                     "the Buffer will modify the original byte array because ByteArray is "
                     "explicitly shared.\n"
                     "Use open() to open the buffer before use and to set the mode (read-only, "
                     "write-only, etc.). close() closes the buffer. The buffer must be closed "
                     "before reopening or calling setBuffer().\n"
                     "A common way to use Buffer is through DataStream or TextStream, which "
                     "have constructors that take a Buffer parameter. For convenience, there "
                     "are also DataStream and TextStream constructors that take a ByteArray "
                     "parameter. These constructors create and open an internal Buffer.\n"
                     "Note that TextStream can also operate on a QString (a Unicode string); a "
                     "Buffer cannot.\n"
                     "You can also use Buffer directly through the standard IoDevice functions "
                     "readBlock(), writeBlock() readLine(), at(), getch(), putch() and ungetch().\n"
                     "See also File, DataStream, TextStream, ByteArray, Shared Classes, Collection "
                     "Classes and Input/Output and Networking."
                     ));
}

void init_seek_test_data(std::list<String> &data)
{
   init_write_block_data(data);
}

} // anonymous namespace

TEST(BufferTest, testWriteBlock)
{
   std::list<String> data;
   init_write_block_data(data);
   for (const String &text : data) {
      ByteArray byteArray;
      Buffer buffer(&byteArray);
      buffer.open(IoDevice::OpenMode::ReadWrite);
      ByteArray data = text.toLatin1();
      ASSERT_EQ(buffer.write(data.getConstRawData(), data.size()), pdk::pint64(data.size()));
      ASSERT_EQ(buffer.getData(), text.toLatin1());
   }
}

TEST(BufferTest, testSeek1)
{
   Buffer buffer;
   buffer.open(IoDevice::OpenMode::WriteOnly);
   ASSERT_EQ(buffer.getSize(), pdk::pint64(0));
   ASSERT_EQ(buffer.getPosition(),  pdk::pint64(0));
   const pdk::pint64 pos = 10;
   ASSERT_TRUE(buffer.seek(pos));
   ASSERT_EQ(buffer.getSize(), pos);
}

#define DO_VALID_SEEK(position) \
{\
   char c;\
   ASSERT_TRUE(buffer.seek(pdk::pint64(position)));\
   ASSERT_EQ(buffer.getPosition(), pdk::pint64(position));\
   ASSERT_TRUE(buffer.getChar(&c));\
   ASSERT_EQ(Character(c), text.at(pdk::pint64(position)));\
   }

#define DO_INVALID_SEEK(position) \
{\
   pdk::pint64 prevPos = buffer.getPosition();\
   ASSERT_TRUE(!buffer.seek(pdk::pint64(position)));\
   ASSERT_EQ(buffer.getPosition(), prevPos);\
   }

TEST(BufferTest, testSeek2)
{
   std::list<String> items;
   init_seek_test_data(items);
   for (const String &text: items) {
      ByteArray byteArray;
      Buffer buffer(&byteArray);
      ASSERT_EQ(buffer.getPosition(), pdk::pint64(0));

      buffer.open(IoDevice::OpenMode::ReadWrite);
      ASSERT_EQ(buffer.getPosition(), pdk::pint64(0));
      ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(0));

      ByteArray data = text.toLatin1();
      ASSERT_EQ(buffer.write(data.getConstRawData(), data.size()), pdk::pint64(data.size()));
      ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(0));
      ASSERT_EQ(buffer.getSize(), pdk::pint64(data.size()));

      DO_INVALID_SEEK(-1);
      DO_VALID_SEEK(0);
      DO_VALID_SEEK(text.size() - 1);
      ASSERT_TRUE(buffer.atEnd());
      DO_VALID_SEEK(text.size() / 2);

      // Special case: valid to seek one position past the buffer.
      // Its then legal to write, but not read.
      {
         char c = 'a';
         ASSERT_TRUE(buffer.seek(pdk::pint64(text.size())));
         ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(0));
         ASSERT_EQ(buffer.read(&c, pdk::pint64(1)), pdk::pint64(0));
         ASSERT_EQ(c, 'a');
         ASSERT_EQ(buffer.write(&c, pdk::pint64(1)), pdk::pint64(1));
      }

      // Special case 2: seeking to an arbitrary position beyond the buffer auto-expands it
      {
         char c;
         const int offset = 1; // any positive integer will do
         const pdk::pint64 pos = buffer.getSize() + offset;
         ASSERT_TRUE(buffer.seek(pos));
         ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(0));
         ASSERT_EQ(buffer.getPosition(), pos);
         ASSERT_TRUE(!buffer.getChar(&c));
         ASSERT_TRUE(buffer.seek(pos - 1));
         ASSERT_TRUE(buffer.getChar(&c));
         ASSERT_EQ(c, buffer.getData().at(pos - 1));
         ASSERT_TRUE(buffer.seek(pos));
         ASSERT_TRUE(buffer.putChar(c));
      }
   }
}

// @TODO Test incomplete
TEST(BufferTest, testReadRawdata)
{
   static const unsigned char mydata[] = {
      0x01, 0x00, 0x03, 0x84, 0x78, 0x9c, 0x3b, 0x76,
      0xec, 0x18, 0xc3, 0x31, 0x0a, 0xf1, 0xcc, 0x99,
      0x6d, 0x5b
   };

   ByteArray data = ByteArray::fromRawData((const char *)mydata, sizeof(mydata));
   Buffer buffer(&data);
   buffer.open(IoDevice::OpenMode::ReadOnly);
}

TEST(BufferTest, testIsSequential)
{
   Buffer buffer;
   ASSERT_TRUE(!buffer.isSequential());
}

namespace {

void init_signal_test_data(std::list<ByteArray> &data)
{
   //   data.push_back(ByteArray());
   data.push_back(ByteArray("1"));
   //   data.push_back(ByteArray("11"));
   //   data.push_back(ByteArray(100, '1'));
}

} // anonymous namespace

static pdk::pint64 sg_totalBytesWritten = 0;
static bool sg_gotReadyRead = false;

TEST(BufferTest, testSignalEmit)
{
   std::list<ByteArray> data;
   init_signal_test_data(data);
   for (const ByteArray &sample: data) {
      sg_totalBytesWritten = 0;
      Buffer buffer;
      PDKTEST_BEGIN_APP_CONTEXT();
      buffer.open(IoDevice::OpenMode::WriteOnly);
      buffer.getBuffer().resize(sample.size()*10);
      buffer.connectReadyReadSignal([]() {
         sg_gotReadyRead = true;
         TestEventLoop::instance().exitLoop();
      }, PDK_RETRIEVE_APP_INSTANCE());
      buffer.connectBytesWrittenSignal([](pdk::pint64 written) {
         sg_totalBytesWritten += written;
      }, PDK_RETRIEVE_APP_INSTANCE());
      for (int i = 0; i < 10; ++i) {
         sg_gotReadyRead = false;
         ASSERT_EQ(buffer.write(sample), pdk::pint64(sample.size()));
         if (sample.size() > 0) {
            TestEventLoop::instance().enterLoop(5);
            if (TestEventLoop::instance().getTimeout()) {
               FAIL() << "Timed out when waiting for readyRead()";
            }
            ASSERT_EQ(sg_totalBytesWritten, pdk::pint64(sample.size() * (i + 1)));
            ASSERT_TRUE(sg_gotReadyRead);
         } else {
            ASSERT_EQ(sg_totalBytesWritten, pdk::pint64(0));
            ASSERT_TRUE(!sg_gotReadyRead);
         }
      }
      PDKTEST_END_APP_CONTEXT();
   }
}

TEST(BufferTest, testIsClosedAfterClose)
{
   Buffer buffer;
   buffer.open(Buffer::OpenMode::ReadOnly);
   ASSERT_TRUE(buffer.isOpen());
   buffer.close();
   ASSERT_TRUE(!buffer.isOpen());
}

namespace {

void init_readline_data(std::list<std::tuple<ByteArray, int, ByteArray>> &data)
{
   data.push_back(std::make_tuple("line1\nline2\n", 1024, "line1\n"));
   data.push_back(std::make_tuple("hi there", 1024, "hi there"));
   data.push_back(std::make_tuple("l\n", 3, "l\n"));
   data.push_back(std::make_tuple("l\n", 2, "l"));
}

void init_can_readline_data(std::list<std::tuple<ByteArray, bool>> &data)
{
   data.push_back(std::make_tuple("no newline", false));
   data.push_back(std::make_tuple("two \n lines\n", true));
   data.push_back(std::make_tuple("\n", true));
   data.push_back(std::make_tuple(ByteArray(), false));
}

} // anonymous namespace

TEST(BufferTest, testReadLine)
{
   std::list<std::tuple<ByteArray, int, ByteArray>> data;
   init_readline_data(data);
   for (auto &item: data) {
      ByteArray &src = std::get<0>(item);
      int maxLength = std::get<1>(item);
      ByteArray &expected = std::get<2>(item);
      Buffer buffer;
      buffer.setBuffer(&src);
      char *result = new char[maxLength + 1];
      result[maxLength] = '\0';

      ASSERT_TRUE(buffer.open(IoDevice::OpenMode::ReadOnly));
      pdk::pint64 bytesRead = buffer.readLine(result, maxLength);
      ASSERT_EQ(bytesRead, pdk::pint64(expected.size()));
      ASSERT_EQ(ByteArray(result), expected);

      buffer.close();
      delete[] result;
   }
}

TEST(BufferTest, testCanReadLine)
{
   std::list<std::tuple<ByteArray, bool>> data;
   init_can_readline_data(data);
   for (auto &item: data) {
      ByteArray &src = std::get<0>(item);
      bool expected = std::get<1>(item);
      Buffer buffer;
      buffer.setBuffer(&src);
      ASSERT_TRUE(!buffer.canReadLine());
      ASSERT_TRUE(buffer.open(IoDevice::OpenMode::ReadOnly));
      ASSERT_EQ(buffer.canReadLine(), expected);
   }
}

TEST(BufferTest, testAtEnd)
{
   Buffer buffer;
   buffer.open(Buffer::OpenMode::Append);
   buffer.write("heisann");
   buffer.close();

   buffer.open(Buffer::OpenMode::ReadOnly);
   buffer.seek(buffer.getSize());
   char c;
   ASSERT_TRUE(!buffer.getChar(&c));
   ASSERT_EQ(buffer.read(&c, 1), pdk::pint64(0));
}

TEST(BufferTest, testReadLineBoundaries)
{
   ByteArray line = "This is a line\n";
   Buffer buffer;
   buffer.open(IoDevice::OpenMode::ReadWrite);
   while (buffer.getSize() < 16384) {
      buffer.write(line);
   }
   buffer.seek(0);
   ByteArray lineByLine;
   while (!buffer.atEnd()) {
      lineByLine.append(buffer.readLine());      
   }

   buffer.seek(0);
   ASSERT_EQ(buffer.getBytesAvailable(), lineByLine.size());

   ByteArray all = buffer.readAll();
   ASSERT_EQ(all.size(), lineByLine.size());
   ASSERT_EQ(all, lineByLine);
}

TEST(BufferTest, testGetAndUngetChar)
{
   // Create some data in a buffer
   ByteArray line = "This is a line\n";
   Buffer buffer;
   buffer.open(IoDevice::OpenMode::ReadWrite);
   while (buffer.getSize() < 16384) {
      buffer.write(line);
   }
   // Take a copy of the data held in the buffer
   buffer.seek(0);
   ASSERT_EQ(buffer.getBytesAvailable(), buffer.getSize());
   ByteArray data = buffer.readAll();
   ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(0));

   // Get and unget each character in order
   for (pdk::pint64 i = 0; i < buffer.getSize(); ++i) {
      buffer.seek(i);
      char c;
      ASSERT_TRUE(buffer.getChar(&c));
      ASSERT_EQ(c, data.at((uint)i));
      buffer.ungetChar(c);
   }

   // Get and unget each character in reverse order
   for (pdk::pint64 i = buffer.getSize() - 1; i >= 0; --i) {
      buffer.seek(i);
      char c;
      ASSERT_TRUE(buffer.getChar(&c));
      ASSERT_EQ(c, data.at((uint)i));
      buffer.ungetChar(c);
   }

   // Verify that the state of the buffer still matches the original data.
   buffer.seek(0);
   ASSERT_EQ(buffer.getBytesAvailable(), data.size());
   ASSERT_EQ(buffer.readAll(), data);
   ASSERT_EQ(buffer.getBytesAvailable(), pdk::pint64(0));
}

TEST(BufferTest, testWriteAfterByteArrayResize)
{
   Buffer buffer;
   ASSERT_TRUE(buffer.open(IoDevice::OpenMode::WriteOnly));
   
   buffer.write(ByteArray().fill('a', 1000));
   ASSERT_EQ(buffer.getBuffer().size(), 1000);
   
   // resize the ByteArray behind Buffer's back
   buffer.getBuffer().clear();
   buffer.seek(0);
   ASSERT_EQ(buffer.getBuffer().size(), 0);
   
   buffer.write(ByteArray().fill('b', 1000));
   ASSERT_EQ(buffer.getBuffer().size(), 1000);
}

TEST(BufferTest, testReadNull)
{
   ByteArray buffer;
   buffer.resize(32000);
   for (int i = 0; i < buffer.size(); ++i) {
      buffer[i] = char(i & 0xff);
   }
      
   Buffer in(&buffer);
   in.open(IoDevice::OpenMode::ReadOnly);
   ByteArray chunk;
   chunk.resize(16380);
   in.read(chunk.getRawData(), 16380);
   ASSERT_EQ(chunk, buffer.mid(0, chunk.size()));
   in.read(chunk.getRawData(), 0);
   chunk.resize(8);
   in.read(chunk.getRawData(), chunk.size());
   ASSERT_EQ(chunk, buffer.mid(16380, chunk.size()));
}

int main(int argc, char **argv)
{
   sg_argc = argc;
   sg_argv = argv;
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
