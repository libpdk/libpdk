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

#ifndef PDK_M_BASE_OS_THREAD_THREAD_STORAGE_H
#define PDK_M_BASE_OS_THREAD_THREAD_STORAGE_H

#include "pdk/global/Global.h"

namespace pdk {
namespace os {
namespace thread {

class PDK_CORE_EXPORT ThreadStorageData
{
public:
   explicit ThreadStorageData(void (*func)(void *));
   ~ThreadStorageData();
   
   void** get() const;
   void** set(void* p);
   
   static void finish(void**);
   int m_id;
};

template <typename T>
inline T *&thread_storage_localdata(ThreadStorageData &data, T **)
{
   void **value = data.get();
   if (!value) {
      value = data.set(nullptr);
   }
   return *(reinterpret_cast<T **>(value));
}

template <typename T>
inline T *thread_storage_localdata_const(const ThreadStorageData &data, T **)
{
   void **value = data.get();
   return value ? *(reinterpret_cast<T **>(value)) : nullptr;
}

template <typename T>
inline void thread_storage_set_localdata(ThreadStorageData &data, T **value)
{
   (void) data.set(*value);
}

template <typename T>
inline void thread_storage_delete_data(void *data, T **)
{
   delete static_cast<T *>(data);
}

// value-based specialization
template <typename T>
inline T &thread_storage_localdata(ThreadStorageData &data, T *)
{
   void **value = data.get();
   if (!value) {
      value = data.set(new T());
   }
   return *(reinterpret_cast<T *>(*value));
}

template <typename T>
inline T thread_storage_localdata_const(const ThreadStorageData &data, T *)
{
   void **value = data.get();
   return value ? *(reinterpret_cast<T *>(*value)) : T();
}

template <typename T>
inline void thread_storage_set_localdata(ThreadStorageData &data, T *value)
{
   (void) data.set(new T(*value));
}

template <typename T>
inline void thread_storage_delete_data(void *data, T *)
{
   delete static_cast<T *>(data);
}

template <class T>
class ThreadStorage
{
private:
   ThreadStorageData m_implPtr;
   
   PDK_DISABLE_COPY(ThreadStorage);
   
   static inline void deleteData(void *x)
   {
      thread_storage_delete_data(x, reinterpret_cast<T *>(0));
   }
   
public:
   inline ThreadStorage() 
      : m_implPtr(deleteData)
   {}
   
   inline ~ThreadStorage()
   {}
   
   inline bool hasLocalData() const
   {
      return m_implPtr.get() != nullptr;
   }
   
   inline T& localData()
   {
      return thread_storage_localdata(m_implPtr, reinterpret_cast<T *>(0));
   }
   
   inline T localData() const
   {
      return thread_storage_localdata_const(m_implPtr, reinterpret_cast<T *>(0));
   }
   
   inline void setLocalData(T t)
   {
      thread_storage_set_localdata(m_implPtr, &t);
   }
};

} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_THREAD_STORAGE_H
