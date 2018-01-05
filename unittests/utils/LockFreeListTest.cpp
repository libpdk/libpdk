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

#include "gtest/gtest.h"
#include "pdk/utils/internal/LockFreeListPrivate.h"
#include <list>
#include <utility>
#include <tuple>
#include <cstdlib>
#include <thread>
#include <chrono>

using pdk::utils::internal::LockFreeList;
using pdk::utils::internal::LockFreeListDefaultConstants;

TEST(LockFreeListTest, testBasicTest)
{
   {
      LockFreeList<void> voidFreeList;
      int zero = voidFreeList.next();
      int one  = voidFreeList.next();
      int two = voidFreeList.next();
      ASSERT_EQ(zero, 0);
      ASSERT_EQ(one, 1);
      ASSERT_EQ(two, 2);
      voidFreeList[zero];
      voidFreeList[one];
      voidFreeList[two];
      voidFreeList.at(zero);
      voidFreeList.at(one);
      voidFreeList.at(two);
      voidFreeList.release(one);
      int next = voidFreeList.next();
      ASSERT_EQ(next, 1);
      voidFreeList[next];
      voidFreeList.at(next);
   }
   {
      LockFreeList<int> voidFreeList;
      int zero = voidFreeList.next();
      int one  = voidFreeList.next();
      int two = voidFreeList.next();
      ASSERT_EQ(zero, 0);
      ASSERT_EQ(one, 1);
      ASSERT_EQ(two, 2);
      voidFreeList[zero] = zero;
      voidFreeList[one] = one;
      voidFreeList[two] = two;
      ASSERT_EQ(voidFreeList.at(zero), zero);
      ASSERT_EQ(voidFreeList.at(one), one);
      ASSERT_EQ(voidFreeList.at(two), two);
      voidFreeList.release(one);
      int next = voidFreeList.next();
      ASSERT_EQ(next, 1);
      ASSERT_EQ(voidFreeList.at(next), one);
      voidFreeList.at(next);
      voidFreeList[next] = -one;
      ASSERT_EQ(voidFreeList.at(next), -one);
   }
}

struct CustomFreeListConstants : public LockFreeListDefaultConstants
{
   enum {
      InitialNextValue = 50,
      BlockCount = 10
   };
   
   static const int sm_sizes[10];
};

const int CustomFreeListConstants::sm_sizes[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 16777216 - 1 - 2 - 3 - 4 - 5 - 6 - 7 - 8 - 9 };

TEST(LockFreeListTest, testCustomized)
{
   LockFreeList<void, CustomFreeListConstants> customFreeList;
   int next = customFreeList.next();
   ASSERT_EQ(next, int(CustomFreeListConstants::InitialNextValue));
   customFreeList[next];
   customFreeList.at(next);
   customFreeList.release(next);
}

enum { TimeLimit = 3000 };

TEST(LockFreeListTest, testThreaded)
{
   LockFreeList<void> freelist;
   const int ThreadCount = std::thread::hardware_concurrency();
   std::list<std::thread *> threads;
   for (int i = 0; i < ThreadCount; ++i) {
      threads.push_back(new std::thread([](LockFreeList<void> &list){
                           std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
                           std::list<int> needToRelease;
                           do {
                              int i = list.next();
                              int j = list.next();
                              int k = list.next();
                              int l = list.next();
                              list.release(k);
                              int n = list.next();
                              int m = list.next();
                              list.release(l);
                              list.release(m);
                              list.release(n);
                              list.release(j);
                              needToRelease.push_back(i);
                           } while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() < TimeLimit);
                           for (int x : needToRelease) {
                              list.release(x);
                           }
                           
                        }, std::ref(freelist)));
   }
   for(auto& thread: threads) {
      thread->join();
   }
   for(auto& thread: threads) {
      delete thread;
   }
}
