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
// Created by softboy on 2018/02/05.

#ifndef PDK_M_BASE_IO_BUFFER_H
#define PDK_M_BASE_IO_BUFFER_H

#include "pdk/base/io/IoDevice.h"
#include "pdk/base/ds/ByteArray.h"

namespace pdk {

// forward declare class with namespace
namespace kernel {
class Object;
} // kernel

namespace io {

// forward declare class with namespace
namespace internal {
class BufferPrivate;
} // internal

using internal::BufferPrivate;
using pdk::kernel::Object;

class PDK_CORE_EXPORT Buffer : public IoDevice
{  
public:
   explicit Buffer(Object *parent = nullptr);
   Buffer(ByteArray *buf, Object *parent = nullptr);
   ~Buffer();
   
   ByteArray &getBuffer();
   const ByteArray &getBuffer() const;
   void setBuffer(ByteArray *a);
   
   void setData(const ByteArray &data);
   inline void setData(const char *data, int len);
   const ByteArray &getData() const;
   
   bool open(OpenModes openMode) override;
   
   void close() override;
   pdk::pint64 getSize() const override;
   pdk::pint64 getPosition() const override;
   bool seek(pdk::pint64 off) override;
   bool atEnd() const override;
   bool canReadLine() const override;
   
protected:
   void connectNotify(pdk::puint32 signal) override;
   void disconnectNotify(pdk::puint32 signal) override;
   pdk::pint64 readData(char *data, pdk::pint64 maxlen) override;
   pdk::pint64 writeData(const char *data, pdk::pint64 len) override;
   void emitSignals();
private:
   PDK_DECLARE_PRIVATE(Buffer);
   PDK_DISABLE_COPY(Buffer);
};

inline void Buffer::setData(const char *data, int len)
{
   setData(ByteArray(data, len));
}

} // io
} // pdk

#endif // PDK_M_BASE_IO_BUFFER_H
