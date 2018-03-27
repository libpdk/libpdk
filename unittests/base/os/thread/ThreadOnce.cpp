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
// Created by softboy on 2018/03/27.

#include "pdk/global/PlatformDefs.h"
#include "pdk/global/GlobalStatic.h"
#include "ThreadOnce.h"
#include <shared_mutex>

namespace pdkunittest {

PDK_GLOBAL_STATIC(std::recursive_mutex, sg_onceInitializationMutex);

enum OnceExtra
{
   MustRunCode = 0x01,
   MustUnlockMutex = 0x02
};

OnceControl::OnceControl(BasicAtomicInt *control)
{
   m_data = 0;
   m_gv = control;
   // check if code has already run once
   if (m_gv->loadAcquire() == 2) {
      // uncontended case: it has already initialized
      // no waiting
      return;
   }
   // acquire the path
   sg_onceInitializationMutex()->lock();
   m_extra = MustUnlockMutex;
   if (m_gv->testAndSetAcquire(0, 1)) {
      // path acquired, we're the first
      m_extra |= MustRunCode;
   }
}

OnceControl::~OnceControl()
{
   if (mustRunCode()) {
      // code wasn't run!
      m_gv->testAndSetRelease(1, 0);  
   }else {
      m_gv->testAndSetRelease(1, 2);
   }
   if (m_extra & MustUnlockMutex) {
      sg_onceInitializationMutex()->unlock();
   }
}

bool OnceControl::mustRunCode()
{
    return m_extra & MustRunCode;
}

void OnceControl::done()
{
    m_extra &= ~MustRunCode;
}


} // pdkunittest
