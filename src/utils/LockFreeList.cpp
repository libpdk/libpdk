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
// Created by softboy on 2018/01/05.

#include "pdk/utils/internal/LockFreeListPrivate.h"

namespace pdk {
namespace utils {
namespace internal {

enum {
   Offset0 = 0x00000000,
   Offset1 = 0x00008000,
   Offset2 = 0x00080000,
   Offset3 = 0x00800000,
   
   Size0 = Offset1  - Offset0,
   Size1 = Offset2  - Offset1,
   Size2 = Offset3  - Offset2,
   Size3 = LockFreeListDefaultConstants::MaxIndex - Offset3
};

const int LockFreeListDefaultConstants::sm_sizes[LockFreeListDefaultConstants::BlockCount] = {
   Size0,
   Size1,
   Size2,
   Size3
};

} // internal
} // utils
} // pdk
