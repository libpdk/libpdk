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
// Created by softboy on 2018/01/29.

#include "pdk/base/os/thread/ThreadStorage.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/global/GlobalStatic.h"
#include <mutex>
#include <string>

namespace pdk {
namespace os {
namespace thread {

static std::mutex sg_destructorsMutex;
typedef std::vector<void (*)(void *)> DestructorMap;
PDK_GLOBAL_STATIC(DestructorMap, sg_destructors);

ThreadStorageData::ThreadStorageData(void (*func)(void *))
{
   std::scoped_lock locker(sg_destructorsMutex);
   DestructorMap *destr = sg_destructors();
   if (!destr) {
      // the destructors vector has already been destroyed, yet a new
      // ThreadStorage is being allocated. this can only happen during global
      // destruction, at which point we assume that there is only one thread.
      // in order to keep ThreadStorage working, we need somewhere to store
      // the data, best place we have in this situation is at the tail of the
      // current thread's tls vector. the destructor is ignored, since we have
      // no where to store it, and no way to actually call it.
      internal::ThreadData *data = internal::ThreadData::current();
      m_id = data->m_tls.size();
      // DEBUG_MSG("ThreadStorageData: Allocated id %d, destructor %p cannot be stored", id, func);
      return;
   }
   for (m_id = 0; static_cast<size_t>(m_id) < destr->size(); ++m_id) {
      if (destr->at(m_id) == 0) {
         break;
      }
   }
   if (static_cast<size_t>(m_id) == destr->size()) {
      destr->push_back(func);
   } else {
      (*destr)[m_id] = func;
   }
   // DEBUG_MSG("ThreadStorageData: Allocated id %d, destructor %p", m_id, func);
}

ThreadStorageData::~ThreadStorageData()
{
   // DEBUG_MSG("ThreadStorageData: Released id %d", m_id);
   std::scoped_lock locker(sg_destructorsMutex);
   if (sg_destructors()) {
      (*sg_destructors())[m_id] = nullptr;
   }
}

void **ThreadStorageData::get() const
{
   internal::ThreadData *data = internal::ThreadData::current();
   if (!data) {
      // qWarning("ThreadStorage::get: ThreadStorage can only be used with threads started with Thread");
      return 0;
   }
   std::vector<void *> &tls = data->m_tls;
   if (tls.size() <= static_cast<size_t>(m_id)) {
      tls.resize(m_id + 1);
   }
   void **value = &tls[m_id];
   
   //   DEBUG_MSG("ThreadStorageData: Returning storage %d, data %p, for thread %p",
   //             m_id,
   //             *v,
   //             data->thread.load());
   
   return *value ? value : nullptr;
}

void **ThreadStorageData::set(void *p)
{
   internal::ThreadData *data = internal::ThreadData::current();
   if (!data) {
      // qWarning("ThreadStorage::set: ThreadStorage can only be used with threads started with Thread");
      return 0;
   }
   std::vector<void *> &tls = data->m_tls;
   if (tls.size() <= static_cast<size_t>(m_id)) {
      tls.resize(m_id + 1);
   }
   void *&value = tls[m_id];
   // delete any previous data
   if (value != 0) {
      //      DEBUG_MSG("QThreadStorageData: Deleting previous storage %d, data %p, for thread %p",
      //                m_id,
      //                value,
      //                data->thread.load());
      
      std::unique_lock locker(sg_destructorsMutex);
      DestructorMap *destr = sg_destructors();
      void (*destructor)(void *) = destr ? destr->at(m_id) : 0;
      locker.unlock();
      
      void *q = value;
      value = 0;
      if (destructor) {
         destructor(q);
      }
         
   }
   // store new data
   value = p;
   // DEBUG_MSG("ThreadStorageData: Set storage %d for thread %p to %p", m_id, data->m_thread.load(), p);
   return &value;
}

void ThreadStorageData::finish(void **p)
{
   std::vector<void *> *tls = reinterpret_cast<std::vector<void *> *>(p);
   if (!tls || tls->empty() || !sg_destructors()) {
      return; // nothing to do
   }
   
   // DEBUG_MSG("QThreadStorageData: Destroying storage for thread %p", QThread::currentThread());
   while (!tls->empty()) {
      void *&value = tls->back();
      void *q = value;
      value = 0;
      int i = tls->size() - 1;
      tls->resize(i);
      if (!q) {
         // data already deleted
         continue;
      }
      
      std::unique_lock locker(sg_destructorsMutex);
      void (*destructor)(void *) = sg_destructors()->at(i);
      locker.unlock();
      
      if (!destructor) {
         if (Thread::getCurrentThread()) {
            //                qWarning("QThreadStorage: Thread %p exited after QThreadStorage %d destroyed",
            //                         QThread::currentThread(), i);
            continue;
         }
      }
      destructor(q); //crash here might mean the thread exited after threadstorage was destroyed
      if (tls->size() > i) {
         //re reset the tls in case it has been recreated by its own destructor.
         (*tls)[i] = 0;
      }
   }
   tls->clear();
}

} // thread
} // os
} // pdk
