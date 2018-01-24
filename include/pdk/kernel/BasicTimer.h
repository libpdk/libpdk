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
// Created by softboy on 2017/01/23.

#ifndef PDK_KERNEL_BASIC_TIMER_H
#define PDK_KERNEL_BASIC_TIMER_H

#include "pdk/global/Global.h"

namespace pdk {
namespace kernel {

class Object;

class PDK_CORE_EXPORT BasicTimer
{
public:
   inline BasicTimer()
      : m_id(0)
   {}
   
   inline ~BasicTimer()
   {
      if (m_id) {
         stop();
      }
   }
   
   inline bool isActive() const
   {
      return m_id != 0;
   }
   
   inline int getTimerId()
   {
      return m_id;
   }
   
   void start(int msec, Object *obj);
   void start(int msec, pdk::TimerType timerType, Object *obj);
   void stop();
private:
   int m_id;
};

} // kernel
} // pdk

PDK_DECLARE_TYPEINFO(pdk::kernel::BasicTimer, PDK_MOVABLE_TYPE);

#endif // PDK_KERNEL_BASIC_TIMER_H
