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
// Created by softboy on 2017/12/04.

#include "pdk/base/ds/internal/ArrayData.h"
#include <climits>

namespace pdk {
namespace ds {
namespace internal {

const ArrayData ArrayData::sm_sharedNull[2] = {
   {PDK_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, sizeof(ArrayData)}
};

namespace 
{

static const ArrayData pdkArray[3] = {
   {PDK_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, sizeof(ArrayData)}, // shared empty
   {{PDK_BASIC_ATOMIC_INITIALIZER(0)}, 0, 0, 0, sizeof(ArrayData)} // unsharable empty
};

static const ArrayData &pdkArrayEmpty = pdkArray[0];
static const ArrayData &pdkArrayUnsharableEmpty = pdkArray[1];

}

ArrayData *ArrayData::allocate(size_t objectSize, size_t alignment, 
                               size_t capacity, AllocationOptions options)
{
   PDK_ASSERT(alignment >= alignof(ArrayData)
              && !(alignment & alignment - 1));
   if (!(options & RawData) && !capacity) {
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
      if (options & Unsharable) {
         return const_cast<ArrayData *>(&pdkArrayUnsharableEmpty);
      }
#endif
      return const_cast<ArrayData *>(&pdkArrayEmpty);
   }
   size_t headerSize = sizeof(ArrayData);
   // Allocate extra (alignment - Q_ALIGNOF(QArrayData)) padding bytes so we
   // can properly align the data array. This assumes malloc is able to
   // provide appropriate alignment for the header -- as it should!
   // Padding is skipped when allocating a header for RawData.
   if (!(options & RawData)) {
      headerSize += (alignment - alignof(ArrayData));
   }
   if (headerSize > INT_MAX) {
      return 0;
   }
   // Calculate the byte size
   // allocSize = objectSize * capacity + headerSize, but checked for overflow
   // plus padded to grow in size
   size_t allocSize;
   if (options & Grow) {
      
   }
}

} // internal
} // ds
} // pdk
