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
// Created by softboy on 2017/12/19.

#include "pdk/base/lang/String.h"
#include "pdk/kernel/StringUtils.h"
#include "pdk/pal/kernel/Simd.h"

namespace pdk {
namespace lang {

namespace
{

#if !defined(__OPTIMIZE_SIZE__)

template <uint MaxCount>
struct UnrollTailLoop
{
   template <typename RetType, typename Functor1, typename Functor2>
   static inline RetType exec(int count, RetType returnIfExited, 
                              Functor1 loopCheck, Functor2 returnIfFailed, int i = 0)
   {
      /* equivalent to:
       *   while (count--) {
       *       if (loopCheck(i))
       *           return returnIfFailed(i);
       *   }
       *   return returnIfExited;
       */
      if (!count) {
         return returnIfExited;
      }
      bool check = loopCheck(i);
      if (check) {
         const RetType &retval = returnIfFailed(i);
         return retval;
      }
      return UnrollTailLoop<MaxCount - 1>::exec(count - 1, returnIfExited, loopCheck, returnIfFailed, i + 1);
   }
   
   template <typename Functor>
   static inline void exec(int count, Functor code)
   {
      /* equivalent to:
       *   for (int i = 0; i < count; ++i)
       *       code(i);
       */
      exec(count, 0, [=](int i) -> bool{
         code(i);
         return false; 
      },  [](int) { return 0; });
   }
};
template <>
template <typename RetType, typename Functor1, typename Functor2>
inline RetType UnrollTailLoop<0>::exec(int, RetType returnIfExited, Functor1, Functor2, int)
{
   return returnIfExited;
}

#endif

}

String::Data *String::fromLatin1Helper(const char *str, int size)
{
   Data *dptr;
   if (!str) {
      dptr = Data::getSharedNull();
   } else if (size == 0 || (!*str && size < 0)) {
      dptr = Data::allocate(0);
   } else {
      if (size < 0) {
         size = pdk::strlen(str);
      }
      dptr = Data::allocate(size + 1);
      PDK_CHECK_ALLOC_PTR(dptr);
      dptr->m_size = size;
      dptr->getData()[size] = '\0';
      char16_t *dest = dptr->getData();
      internal::utf16_from_latin1(dest, str, static_cast<uint>(size));
   }
   return dptr;
}

namespace internal {

void utf16_from_latin1(char16_t *dest, const char *str, size_t size) noexcept
{
   /* SIMD:
    * Unpacking with SSE has been shown to improve performance on recent CPUs
    * The same method gives no improvement with NEON.
    */
#if defined(__SSE2__)
   const char *end = str + size;
   pdk::ptrdiff offset = 0;
   // we're going to read str[offset..offset+15] (16 bytes)
   for (; str + offset + 15 < end; offset += 16) {
      const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(str + offset));
#  ifdef __AVX2__
      // zero extend to an YMM register
      const __m256i extended = _mm256_cvtepu8_epi16(chunk);
      // store
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(dest + offset), extended);
#  else
      const __m128i nullMask = _mm_set1_epi32(0);
      const __m128i firstHalf = _mm_unpacklo_epi8(chunk, nullMask);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + offset), firstHalf);
      const __m128i secondHalf = _mm_unpackhi_epi8(chunk, nullMask);
      _mm_storeu_si128(reinterpret_cast<__m128i *>(dest + offset + 8), secondHalf);
#  endif
   }
   size = size % 16;
   dest += offset;
   str += offset;
#  if !defined(__OPTIMIZE_SIZE__)
   return UnrollTailLoop<15>::exec(static_cast<int>(size), [=](int i) { dest[i] = static_cast<uchar>(str[i]); });
#  endif
#endif
   while (--size) {
      *dest++ = static_cast<uchar>(*str++);
   }
}

} // internal

String::String(const Character *unicode, int size)
{
   if (!unicode) {
      m_data = Data::getSharedNull();
   } else {
      if (size < 0) {
         size = 0;
         while (!unicode[size].isNull()) {
            ++size;
         }
      }
      if (!size) {
         m_data = Data::allocate(0);
      } else {
         m_data = Data::allocate(0);
         PDK_CHECK_ALLOC_PTR(m_data);
         m_data->m_size = size;
         std::memcpy(m_data->getData(), unicode, size * sizeof(Character));
         m_data->getData()[size] = '\0';
      }
   }
}

} // lang
} // pdk
