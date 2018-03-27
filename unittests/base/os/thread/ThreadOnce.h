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

#ifndef PDK_UNITTEST_OS_THREAD_THREAD_ONCE_H
#define PDK_UNITTEST_OS_THREAD_THREAD_ONCE_H

#include "pdk/global/Global.h"
#include "pdk/base/os/thread/Atomic.h"

namespace pdkunittest {

using pdk::os::thread::BasicAtomicInt;

class OnceControl
{
public:
   OnceControl(BasicAtomicInt *);
   ~OnceControl();
   
   bool mustRunCode();
   void done();
   
private:
   BasicAtomicInt *m_gv;
   union {
      pdk::pint32 m_extra;
      void *m_data;
   };
};

#define PDK_ONCE_GV_NAME2(prefix, line) prefix ## line
#define PDK_ONCE_GV_NAME(prefix, line) PDK_ONCE_GV_NAME2(prefix, line)
#define PDK_ONCE_GV PDK_ONCE_GV_NAME(_pdk_onece, __LINE__)

#define PDK_ONCE \
   static BasicAtomicInt PDK_ONCE_GV = PDK_BASIC_ATOMIC_INITIALIZER(0);   \
   if (0){} else                                                       \
   for (pdkunittest::OnceControl _control_(&PDK_ONCE_GV); _control_.mustRunCode(); _control_.done())

template <typename T>
class Singleton
{
   struct Destructor
   {
      T *&m_pointer;
      Destructor(T *&ptr) : m_pointer(ptr)
      {}
      
      ~Destructor()
      {
         delete m_pointer;
      }
   };
   
public:
   T *_pdk_value;
   BasicAtomicInt _pdk_guard;
   
   inline T *getValue()
   {
      for (OnceControl control(&_pdk_guard); control.mustRunCode(); control.done()) {
         _pdk_value = new T();
         static Destructor cleanup(_pdk_value);
      }
      return _pdk_value;
   }
   
   inline T& operator*()
   {
      return *getValue();
   }
   
   inline T* operator->()
   {
      return getValue();
   }
   
   inline operator T*()
   {
      return getValue();
   }
};

} // pdkunittest

#endif // PDK_UNITTEST_OS_THREAD_THREAD_ONCE_H


