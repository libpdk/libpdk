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
// Created by softboy on 2017/11/16.

#ifndef PDK_UTILS_REFCOUNT_H
#define PDK_UTILS_REFCOUNT_H

#include "pdk/global/Global.h"
#include <atomic>

namespace pdk {
namespace utils {

class Refcount
{
public:
   inline bool ref() noexcept
   {
      
   }
   
   inline bool deref() noexcept
   {
      
   }
   
#if !defined(PDK_NO_UNSHARABLE_CONTAINERS)
   bool setSharable(bool sharable) noexcept
   {
      
   }
   
   bool isSharable() const noexcept
   {
      
   }
#endif
   
   bool isStatic() const noexcept
   {
      
   }
   
   bool isShared() const noexcept
   {
      
   }
   
private:
   std::atomic<int> m_atomic;
};

} // utils
} // pdk

#endif // PDK_UTILS_REFCOUNT_H
