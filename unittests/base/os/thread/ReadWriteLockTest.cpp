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
// Created by softboy on 2017/01/08.

#include <limits.h>
#include <cstdio>
#include <list>
#include <utility>
#include <tuple>

#include "gtest/gtest.h"
#include "pdk/base/os/thread/ReadWriteLock.h"

using pdk::os::thread::ReadWriteLock;

TEST(ReadWriteLockTest, testConstructDestruct)
{
   {
      ReadWriteLock rwlock;
   }
}

TEST(ReadWriteLockTest, testReadLockUnlock)
{
   ReadWriteLock rwlock;
   rwlock.lockForRead();
   rwlock.unlock();
}

TEST(ReadWriteLockTest, testWriteLockUnlock)
{
   ReadWriteLock rwlock;
   rwlock.lockForWrite();
   rwlock.unlock();
}

TEST(ReadWriteLockTest, testReadLockUnlockLoop)
{
   ReadWriteLock rwlock;
   int runs = 10000;
   for (int i = 0; i < runs; ++i) {
      rwlock.lockForRead();
      rwlock.unlock();
   }
}

TEST(ReadWriteLockTest, testWriteLockUnlockLoop)
{
   ReadWriteLock rwlock;
   int runs = 10000;
   for (int i = 0; i < runs; ++i) {
      rwlock.lockForWrite();
      rwlock.unlock();
   }
}

TEST(ReadWriteLockTest, testReadLockLoop)
{
   ReadWriteLock rwlock;
   int runs = 10000;
   for (int i = 0; i < runs; ++i) {
      rwlock.lockForRead();
   }
   for (int i = 0; i < runs; ++i) {
      rwlock.unlock();
   }
}

TEST(ReadWriteLockTest, testWriteLockLoop)
{
  /*
   * If you include this, the test should print one line
   * and then block.
   */
#if 0
   ReadWriteLock rwlock;
   int runs = 10000;
   for (int i = 0; i < runs; ++i) {
      rwlock.lockForWrite();
   }
#endif
}

TEST(ReadWriteLockTest, testReadWriteLockUnlockLoop)
{
   ReadWriteLock rwlock;
   int runs = 10000;
   for (int i = 0; i < runs; ++i) {
      rwlock.lockForRead();
      rwlock.unlock();
      rwlock.lockForWrite();
      rwlock.unlock();
   }
}


