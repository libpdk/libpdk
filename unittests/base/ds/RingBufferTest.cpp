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
}
