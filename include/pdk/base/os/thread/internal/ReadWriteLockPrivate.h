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
// Created by softboy on 2018/01/05.

#ifndef PDK_M_BASE_OS_THREAD_INTERNAL_READWRITE_LOCK_PRIVATE_H
#define PDK_M_BASE_OS_THREAD_INTERNAL_READWRITE_LOCK_PRIVATE_H

#include "pdk/global/Global.h"
#include <mutex>
#include <condition_variable>
#include <map>
#include <thread>

namespace pdk {
namespace os {
namespace thread {
namespace internal {

class ReadWriteLockPrivate
{
public:
   ReadWriteLockPrivate(bool isRecursive = false)
      : m_readerCount(0),
        m_writerCount(0),
        m_waitingReaders(0),
        m_waitingWriters(0),
        m_recursive(isRecursive),
        m_id(0)
   {}
   
   bool lockForWrite(int timeout, std::unique_lock<std::mutex> &mutexLocker);
   bool lockForRead(int timeout, std::unique_lock<std::mutex> &mutexLocker);
   void unlock(std::unique_lock<std::mutex> &mutexLocker);
   
   bool recursiveLockForWrite(int timeout);
   bool recursiveLockForRead(int timeout);
   void recursiveUnlock();
   
   void release();
   static ReadWriteLockPrivate *allocate();
   
   std::mutex m_mutex;
   std::condition_variable m_writerCond;
   std::condition_variable m_readerCond;
   int m_readerCount;
   int m_writerCount;
   int m_waitingReaders;
   int m_waitingWriters;
   const bool m_recursive;
   int m_id;
   std::thread::id m_currentWriter;
   std::map<std::thread::id, int> m_currentReaders;
};

} // internal
} // thread
} // os
} // pdk

#endif // PDK_M_BASE_OS_THREAD_INTERNAL_READWRITE_LOCK_PRIVATE_H
