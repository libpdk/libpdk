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
