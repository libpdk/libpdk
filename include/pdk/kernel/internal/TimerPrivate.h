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
// Created by softboy on 2018/01/25.

#ifndef PDK_KERNEL_INTERNAL_TIMER_PRIVATE_H
#define PDK_KERNEL_INTERNAL_TIMER_PRIVATE_H

#include "pdk/kernel/internal/ObjectPrivate.h"

namespace pdk {
namespace kernel {
namespace internal {

class TimerPrivate : public ObjectPrivate
{
public:
   bool m_singleShot;
   int m_interval;
   int m_remainingTime;
   pdk::TimerType m_timerType;
   bool m_active;
   int m_id;
   int m_inter;
   int m_del;
   uint m_single : 1;
   uint m_nulltimer : 1;
   uint m_type : 2;
   // reserved : 28
};

} // pdk
} // kernel
} // pdk

#endif // PDK_KERNEL_INTERNAL_TIMER_PRIVATE_H
