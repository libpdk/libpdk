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

#if defined(__mips_dsp)
#error "asdasda"
#endif

namespace pdk {
namespace lang {

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
   }
   return dptr;
}

} // lang
} // pdk
