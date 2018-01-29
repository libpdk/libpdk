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
// Created by zzu_softboy on 2018/01/29.

#include "gtest/gtest.h"
#include <vector>

#include "pdk/base/ds/internal/RingBufferPrivate.h"
#include "pdk/base/ds/ByteArray.h"

using pdk::ds::internal::RingBuffer;
using pdk::ds::ByteArray;

TEST(RingBufferTest, testConstructing)
{
   RingBuffer ringBuffer;
   const int chunkSize = ringBuffer.getChunkSize();
   ringBuffer.setChunkSize(0);
   ASSERT_EQ(ringBuffer.getChunkSize(), PDK_INT64_C(0));
   ringBuffer.setChunkSize(chunkSize);
   ASSERT_EQ(ringBuffer.getChunkSize(), chunkSize);
   
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(0));
   ASSERT_TRUE(ringBuffer.isEmpty());
   ASSERT_EQ(ringBuffer.nextDataBlockSize(), PDK_INT64_C(0));
   ASSERT_EQ(ringBuffer.readPointer(), nullptr);
   ASSERT_EQ(ringBuffer.skip(5), PDK_INT64_C(0));
   ASSERT_EQ(ringBuffer.read(), ByteArray());
   ASSERT_EQ(ringBuffer.getChar(), -1);
   ASSERT_FALSE(ringBuffer.canReadLine());
   char buf[5];
   ASSERT_EQ(ringBuffer.peek(buf, sizeof(buf)), PDK_INT64_C(0));
}

TEST(RingBufferTest, testUsingInVector)
{
   RingBuffer ringBuffer;
   std::vector<RingBuffer> buffers;
   ringBuffer.reserve(5);
   buffers.push_back(ringBuffer);
   ASSERT_EQ(buffers[0].size(), PDK_INT64_C(5));
}

TEST(RingBufferTest, testSizeWhenReserved)
{
   RingBuffer ringBuffer;
   ringBuffer.reserve(5);
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(5));
}

TEST(RingBufferTest, testSizeWhenReservedAndChopped)
{
   RingBuffer ringBuffer;
   ringBuffer.reserve(31337);
   ringBuffer.chop(31337);
   
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(0));
}

TEST(RingBufferTest, testReadPointerAtPositionReadTooMuch)
{
   RingBuffer ringBuffer;
   pdk::pint64 length;
   const char *buf = ringBuffer.readPointerAtPosition(42, length);
   ASSERT_TRUE(buf == 0);
   ASSERT_EQ(length, PDK_INT64_C(0));
}

TEST(RingBufferTest, testReadPointerAtPositionWithHead)
{
   RingBuffer ringBuffer;
   char *buf = ringBuffer.reserve(4);
   std::memcpy(buf, "0123", 4);
   ringBuffer.free(2);
   // ringBuffer should have stayed the same except
   // its head it had moved to position 2
   pdk::pint64 length;
   const char* buf2 = ringBuffer.readPointerAtPosition(0, length);
   ASSERT_EQ(length, 2);
   ASSERT_EQ(*buf2, '2');
   ASSERT_EQ(*(buf2 + 1), '3');
   
   // advance 2 more, ringBuffer should be empty then
   ringBuffer.free(2);
   buf2 = ringBuffer.readPointerAtPosition(0, length);
   ASSERT_EQ(length, PDK_INT64_C(0));
   ASSERT_TRUE(buf2 == 0);
   
   // check buffer with 2 blocks
   std::memcpy(ringBuffer.reserve(4), "0123", 4);
   ringBuffer.append(ByteArray("45678", 5));
   ringBuffer.free(3);
   buf2 = ringBuffer.readPointerAtPosition(PDK_INT64_C(1), length);
   ASSERT_EQ(length, PDK_INT64_C(5));
}

TEST(RingBufferTest, testReadPointerAtPositionEmptyRead)
{
   RingBuffer ringBuffer;
   pdk::pint64 length;
   const char *buf = ringBuffer.readPointerAtPosition(0, length);
   ASSERT_TRUE(buf == 0);
   ASSERT_EQ(length, PDK_INT64_C(0));
}

TEST(RingBufferTest, testReadPointerAtPositionWriteRead)
{
   
}

TEST(RingBufferTest, testFree)
{
   RingBuffer ringBuffer;
   // make three byte arrays with different sizes
   ringBuffer.reserve(4096);
   ringBuffer.reserve(2048);
   ringBuffer.append(ByteArray("01234", 5));
   ringBuffer.free(1);
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(4095) + 2048 + 5);
   ringBuffer.free(4096);
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(2047) + 5);
   ringBuffer.free(48);
   ringBuffer.free(2000);
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(4));
   ASSERT_TRUE(std::memcmp(ringBuffer.readPointer(), "1234", 4) == 0);
}

TEST(RingBufferTest, testReserveAndRead)
{
   RingBuffer ringBuffer;
   for (int i = 1; i < 256; ++i) {
      ByteArray ba(i, char(i));
      char *ringPos = ringBuffer.reserve(i);
      ASSERT_TRUE(ringPos);
      std::memcpy(ringPos, ba.getConstRawData(), i);
   }
   
   // readback and check stored data
   for (int i = 1; i < 256; ++i) {
      ByteArray ba;
      ba.resize(i);
      pdk::pint64 thisRead = ringBuffer.read(ba.getRawData(), i);
      ASSERT_EQ(thisRead, static_cast<pdk::pint64>(i));
      ASSERT_EQ(ba.count(char(i)), i);
   }
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(0));
}

TEST(RingBufferTest, testReserveAndReadInPacketMode)
{
   RingBuffer ringBuffer(0);
   // try to allocate 255 buffers
   for (int i = 1; i < 256; ++i) {
      char *ringPos = ringBuffer.reserve(i);
      ASSERT_TRUE(ringPos);
   }
   // count and check the size of stored buffers
   int buffersCount = 0;
   while (!ringBuffer.isEmpty()) {
      ByteArray ba = ringBuffer.read();
      ++buffersCount;
      ASSERT_EQ(ba.size(), buffersCount);
   }
   ASSERT_EQ(buffersCount, 255);
}

TEST(RingBufferTest, testReserveFrontAndRead)
{
   RingBuffer ringBuffer;
   // fill buffer with an arithmetic progression
   for (int i = 1; i < 256; ++i) {
      ByteArray ba(i, char(i));
      char *ringPos = ringBuffer.reserveFront(i);
      ASSERT_TRUE(ringPos);
      std::memcpy(ringPos, ba.getConstRawData(), i);
   }
   // readback and check stored data
   for (int i = 255; i > 0; --i) {
      ByteArray ba;
      ba.resize(i);
      pdk::pint64 thisRead = ringBuffer.read(ba.getRawData(), i);
      ASSERT_EQ(thisRead, static_cast<pdk::pint64>(i));
      ASSERT_EQ(ba.count(char(i)), i);
   }
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(0));
}

TEST(RingBufferTest, testChop)
{
   RingBuffer ringBuffer;
   // make three byte arrays with different sizes
   ringBuffer.append(ByteArray("01234", 5));
   ringBuffer.reserve(2048);
   ringBuffer.reserve(4096);
   
   ringBuffer.chop(1);
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(5) + 2048 + 4095);
   ringBuffer.chop(4096);
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(5) + 2047);
   ringBuffer.chop(48);
   ringBuffer.chop(2000);
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(4));
   ASSERT_TRUE(std::memcmp(ringBuffer.readPointer(), "0123", 4) == 0);
}

TEST(RingBufferTest, testUngetChar)
{
   RingBuffer ringBuffer(16);
   for (int i = 1; i < 32; ++i) {
      ringBuffer.putChar(char(i));
   }
   for (int i = 1; i < 31; ++i) {
      int c = ringBuffer.getChar();
      ASSERT_EQ(c, 1);
      ringBuffer.getChar();
      ringBuffer.ungetChar(char(c)); // unget first char
   }
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(1));
}

TEST(RingBufferTest, testIndexOf)
{
   RingBuffer ringBuffer(16);
   for (int i = 1; i < 256; ++i) {
      ringBuffer.putChar(char(i));
   }
   for (int i = 1; i < 256; ++i) {
      pdk::pint64 index = ringBuffer.indexOf(char(i));
      ASSERT_EQ(index, static_cast<pdk::pint64>(i - 1));
      ASSERT_EQ(ringBuffer.indexOf(char(i), i, i >> 1), index);
      ASSERT_EQ(ringBuffer.indexOf(char(i), 256, i), PDK_INT64_C(-1));
      ASSERT_EQ(ringBuffer.indexOf(char(i), i - 1), -1); // test for absent char
   }
}

TEST(RingBufferTest, testAppendAndRead)
{
   RingBuffer ringBuffer;
   ByteArray ba1("Hello world!");
   ByteArray ba2("Test string.");
   ByteArray ba3("0123456789");
   ringBuffer.append(ba1);
   ringBuffer.append(ba2);
   ringBuffer.append(ba3);
   
   ASSERT_EQ(ringBuffer.read(), ba1);
   ASSERT_EQ(ringBuffer.read(), ba2);
   ASSERT_EQ(ringBuffer.read(), ba3);
}

TEST(RingBufferTest, testPeek)
{
   RingBuffer ringBuffer;
   ByteArray testBuffer;
   // fill buffer with an arithmetic progression
   for (int i = 1; i < 256; ++i) {
      char *ringPos = ringBuffer.reserve(i);
      ASSERT_TRUE(ringPos);
      std::memset(ringPos, i, i);
      testBuffer.append(ringPos, i);
   }
   
   // check stored data
   ByteArray resultBuffer;
   int peekPosition = testBuffer.size();
   for (int i = 1; i < 256; ++i) {
      ByteArray ba(i, 0);
      peekPosition -= i;
      pdk::pint64 thisPeek = ringBuffer.peek(ba.getRawData(), i, peekPosition);
      ASSERT_EQ(thisPeek, static_cast<pdk::pint64>(i));
      resultBuffer.prepend(ba);
   }
   ASSERT_EQ(resultBuffer, testBuffer);
}

TEST(RingBufferTest, testReadLine)
{
   RingBuffer ringBuffer;
   ByteArray ba1("Hello world!\n", 13);
   ByteArray ba2("\n", 1);
   ByteArray ba3("Test string.", 12);
   ByteArray ba4("0123456789", 10);
   ringBuffer.append(ba1);
   ringBuffer.append(ba2);
   ringBuffer.append(ba3 + ba4 + ba2);
   
   char stringBuf[102];
   stringBuf[101] = 0; // non-crash terminator
   ASSERT_EQ(ringBuffer.readLine(stringBuf, sizeof(stringBuf) - 2), static_cast<pdk::pint64>(ba1.size()));
   ASSERT_EQ(ByteArray(stringBuf, int(strlen(stringBuf))), ba1);
   
   // check first empty string reading
   stringBuf[0] = char(0xFF);
   ASSERT_EQ(ringBuffer.readLine(stringBuf, int(sizeof(stringBuf)) - 2), static_cast<pdk::pint64>(ba2.size()));
   ASSERT_EQ(stringBuf[0], ba2.at(0));
   
   ASSERT_EQ(ringBuffer.readLine(stringBuf, int(sizeof(stringBuf)) - 2),
            static_cast<pdk::pint64>(ba3.size() + ba4.size() + ba2.size()));
   ASSERT_EQ(ByteArray(stringBuf, int(strlen(stringBuf))), ba3 + ba4 + ba2);
   ASSERT_EQ(ringBuffer.size(), PDK_INT64_C(0));
}
