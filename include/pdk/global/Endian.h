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
// Created by softboy on 2017/12/25.

#ifndef PDK_GLOBAL_ENDIAN_H
#define PDK_GLOBAL_ENDIAN_H

#include "pdk/global/Global.h"

namespace pdk
{

// Used to implement a type-safe and alignment-safe copy operation
// If you want to avoid the memcpy, you must write specializations for these functions
template <typename T>
PDK_ALWAYS_INLINE void to_unaligned(void *dest, const T src)
{
   // Using sizeof(T) inside memcpy function produces internal compiler error with
   // MSVC2008/ARM in tst_endian -> use extra indirection to resolve size of T.
   const size_t size = sizeof(T);
#if PDK_HAS_BUILTIN(__builtin_memcpy)
   __builtin_memcpy
      #else
   std::memcpy
      #endif
         (dest, &src, size);
}

} // pdk

#endif // PDK_GLOBAL_ENDIAN_H
