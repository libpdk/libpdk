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

#include "pdk/utils/MemoryHelper.h"

namespace pdk {
namespace utils {

size_t calculate_block_size(size_t elementCount, size_t elementSize, size_t headerSize) noexcept
{
   unsigned count = static_cast<unsigned int>(elementCount);
   unsigned size = static_cast<unsigned int>(elementSize);
   unsigned header = static_cast<unsigned int>(headerSize);
   PDK_ASSERT(elementSize);
   PDK_ASSERT(size == elementSize);
   PDK_ASSERT(header == headerSize);
   if (PDK_UNLIKELY(count != elementCount)) {
      return std::numeric_limits<size_t>::max();
   }
   
}

CalculateGrowingBlockSizeResult calculate_growing_block_size(size_t elementCount, size_t elementSize, 
                                                             size_t headerSize) noexcept
{
   
}

} // utils
} // pdk
