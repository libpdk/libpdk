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
// Created by softboy on 2018/01/03.

#include "pdk/utils/SharedPointer.h"

#include "gtest/gtest.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/base/os/thread/Atomic.h"
#include <list>
#include <utility>
#include <tuple>
#include <cstdlib>
#include <string>

using pdk::utils::SharedPointer;
using pdk::utils::WeakPointer;
using pdk::utils::internal::ExternalRefCountData;

template <typename T>
static inline ExternalRefCountData *refcount_data(const SharedPointer<T> &b)
{
   struct Dummy
   {
      void *m_value;
      ExternalRefCountData *m_data;
   };
   PDK_STATIC_ASSERT(sizeof(SharedPointer<T>) == sizeof(Dummy));
   PDK_ASSERT(static_cast<const Dummy*>(static_cast<const void*>(&b))->m_value == b.getData());
}

class Data
{
public:
   Data()
      : m_generation(++sm_generatorCounter)
   {}
   
   virtual ~Data()
   {
      if (m_generation <= 0) {
         std::cout << "qsharedpointer: Double deletion!";
      }
      m_generation = 0;
      ++sm_destructorCounter;
   }
   
   void doDelete()
   {
      delete this;
   }
   
   bool alsoDelete()
   {
      doDelete();
      return true;
   }
   
   virtual void virtualDelete()
   {
      delete this;
   }
   
   virtual int classLevel() 
   {
      return 1;
   }
   
   static int sm_destructorCounter;
   static int sm_generatorCounter;
   int m_generation;
};

int Data::sm_generatorCounter = 0;
int Data::sm_destructorCounter = 0;

struct NoDefaultConstructor1
{
   int i;
   NoDefaultConstructor1(int i) : i(i) {}
   NoDefaultConstructor1(uint j) : i(j + 42) {}
};

struct NoDefaultConstructorRef1
{
   int &i;
   NoDefaultConstructorRef1(int &i) : i(i) {}
};

struct NoDefaultConstructor2
{
   void *m_ptr;
   int i;
   NoDefaultConstructor2(void *ptr, int i) : m_ptr(ptr), i(i) {}
};

struct NoDefaultConstructorRef2
{
   std::string m_str;
   int i;
   NoDefaultConstructorRef2(std::string &str, int i) : m_str(str), i(i) {}
};

struct NoDefaultConstructorRRef1
{
   int &i;
   NoDefaultConstructorRRef1(int &&i) : i(i) {}
};

TEST(SharedPointerTest, testBasics)
{
   {
      SharedPointer<Data> ptr;
      WeakPointer<Data> weakref;
      ASSERT_EQ(sizeof(ptr), 2 * sizeof(void *));
      ASSERT_EQ(sizeof(weakref), 2 * sizeof(void *));
   }
   {
      SharedPointer<const Data> ptr;
      WeakPointer<const Data> weakref;
      ASSERT_EQ(sizeof(ptr), 2 * sizeof(void *));
      ASSERT_EQ(sizeof(weakref), 2 * sizeof(void *));
   }
   std::list<bool> data{true, false};
   std::list<bool>::iterator begin = data.begin();
   std::list<bool>::iterator end = data.end();
   while (begin != end) {
      bool isNull = *begin;
      Data *aData = 0;
      if (!isNull) {
         aData = new Data;
      }
      SharedPointer<Data> ptr(aData);
      {
         ASSERT_EQ(ptr.isNull(), isNull);
         ASSERT_EQ(bool(ptr), !isNull);
         ASSERT_EQ(!ptr, isNull);
         ASSERT_EQ(ptr.getData(), aData);
         if (!isNull) {
            Data &dataReference = *ptr;
            ASSERT_EQ(&dataReference, aData);
         }
         ASSERT_TRUE(ptr == aData);
      }
      ++begin;
   }
}
