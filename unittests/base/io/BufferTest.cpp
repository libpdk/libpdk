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

using pdk::io::Buffer;
using pdk::ds::ByteArray;
using pdk::io::IoDevice;

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
