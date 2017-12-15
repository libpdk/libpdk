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
// Created by softboy on 2017/12/15.

#ifndef PDK_M_BASE_DS_BYTE_ARRAY_H
#define PDK_M_BASE_DS_BYTE_ARRAY_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <string>
#include <iterator>

#include "pdk/utils/RefCount.h"
#include "pdk/base/ds/internal/ArrayData.h"

#ifdef truncate
#error ByteArray.h must be included before any header file that defines truncate
#endif

#if defined(PDK_OS_DARWIN)
PDK_FORWARD_DECLARE_CF_TYPE(CFData);
PDK_FORWARD_DECLARE_OBJC_CLASS(NSData);
#endif

namespace pdk {
namespace ds {

using ByteArrayData = internal::ArrayData;

template <int N>
struct StaticByteArrayData
{
   ByteArrayData m_header;
   char m_data[N + 1];
   
   ByteArrayData *getDataPtr() const
   {
      PDK_ASSERT(m_header.m_ref.isStatic());
      return const_cast<ByteArrayData *>(&m_header);
   }
};

struct ByteArrayDataPtr
{
   ByteArrayData *m_ptr; 
};

#define PDK_STATIC_BYTE_DATA_HEADER_INITIALIZER_WITH_OFFSET(size, offset) \
   PDK_STATIC_ARRAY_HEADER_INITIALIZER_WITH_OFFSET(size, offset)

#define PDK_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(size) \
   PDK_STATIC_BYTE_DATA_HEADER_INITIALIZER_WITH_OFFSET(size)

#define ByteArrayLiteral(str) \
   ([]()-> pdk::ds::ByteArray {\
      enum { Size = sizeof(str) - 1 };\
      static const StaticByteArrayData<Size> byteArrayLiteral = {\
         PDK_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(Size),\
         str\
      };\
      ByteArrayDataPtr holder = { byteArrayLiteral.getDataPtr() };\
      const ByteArray byteArray(holder);\
      return byteArray;\
   }())

class PDK_CORE_EXPORT ByteArray
{
   
};



} // ds
} // pdk

#endif // PDK_M_BASE_DS_BYTE_ARRAY_H
