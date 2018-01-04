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
#include "ForwardDeclared.h"

#include "gtest/gtest.h"
#include <list>
#include <utility>
#include <tuple>
#include <cstdlib>
#include <string>

using pdk::utils::SharedPointer;
using pdk::utils::WeakPointer;
using pdk::utils::internal::ExternalRefCountData;
using pdk::hash;

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
   return static_cast<const Dummy*>(static_cast<const void*>(&b))->m_data;
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
      Data *otherData = new Data;
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
         ASSERT_TRUE(!(ptr != aData));
         ASSERT_TRUE(aData == ptr);
         ASSERT_TRUE(!(aData != ptr));
         
         ASSERT_TRUE(ptr != otherData);
         ASSERT_TRUE(otherData != ptr);
         ASSERT_TRUE(!(ptr == otherData));
         ASSERT_TRUE(!(otherData == ptr));
      }
      ASSERT_TRUE(!refcount_data(ptr) || refcount_data(ptr)->m_weakRef.load() == 1);
      ASSERT_TRUE(!refcount_data(ptr) || refcount_data(ptr)->m_strongRef.load() == 1);
      
      {
         // create another object:
         SharedPointer<Data> otherCopy(otherData);
         ASSERT_TRUE(ptr != otherCopy);
         ASSERT_TRUE(otherCopy != ptr);
         ASSERT_TRUE(! (ptr == otherCopy));
         ASSERT_TRUE(! (otherCopy == ptr));
         // otherData is deleted here
      }
      ASSERT_TRUE(!refcount_data(ptr) || refcount_data(ptr)->m_weakRef.load() == 1);
      ASSERT_TRUE(!refcount_data(ptr) || refcount_data(ptr)->m_strongRef.load() == 1);
      
      {
         // create a copy:
         SharedPointer<Data> copy(ptr);
         ASSERT_TRUE(copy == ptr);
         ASSERT_TRUE(ptr == copy);
         ASSERT_TRUE(!(copy != ptr));
         ASSERT_TRUE(!(ptr != copy));
         // otherData is deleted here
         ASSERT_TRUE(!refcount_data(ptr) || refcount_data(ptr)->m_weakRef.load() == 2);
         ASSERT_TRUE(!refcount_data(ptr) || refcount_data(ptr)->m_strongRef.load() == 2);
         
      }
      ASSERT_TRUE(!refcount_data(ptr) || refcount_data(ptr)->m_weakRef.load() == 1);
      ASSERT_TRUE(!refcount_data(ptr) || refcount_data(ptr)->m_strongRef.load() == 1);
      // create a weak reference:
      {
         WeakPointer<Data> weak(ptr);
         ASSERT_EQ(weak.isNull(), isNull);
         ASSERT_EQ(!weak, isNull);
         ASSERT_EQ(static_cast<bool>(weak), !isNull);
         
         ASSERT_TRUE(ptr == weak);
         ASSERT_TRUE(weak == ptr);
         ASSERT_TRUE(!(ptr != weak));
         ASSERT_TRUE(!(weak != ptr));
         
         WeakPointer<Data> weak2(weak);
         ASSERT_EQ(weak2.isNull(), isNull);
         ASSERT_EQ(!weak2, isNull);
         ASSERT_TRUE(!(ptr != weak));
         ASSERT_TRUE(!(weak != ptr));
         SharedPointer<Data> strong(weak);
         ASSERT_TRUE(strong == weak);
         ASSERT_TRUE(strong == ptr);
         ASSERT_EQ(strong.getData(), aData);
      }
      ASSERT_TRUE(!refcount_data(ptr) || refcount_data(ptr)->m_weakRef.load() == 1);
      ASSERT_TRUE(!refcount_data(ptr) || refcount_data(ptr)->m_strongRef.load() == 1);
      ++begin;
   }
}

TEST(SharedPointerTest, testOperators)
{
   SharedPointer<char> p1;
   SharedPointer<char> p2(new char);
   pdk::ptrdiff diff = p2.getData() - p1.getData();
   ASSERT_NE(p1.getData(), p2.getData());
   ASSERT_TRUE(diff != 0);
   
   // operator-
   ASSERT_EQ(p2 - p1.getData(), diff);
   ASSERT_EQ(p2.getData() - p1, diff);
   ASSERT_TRUE(p2 - p1 == diff);
   ASSERT_TRUE(p1 - p2 == -diff);
   ASSERT_EQ(p1 - p1, static_cast<pdk::ptrdiff>(0));
   ASSERT_EQ(p2 - p2, static_cast<pdk::ptrdiff>(0));
   
   // operator<
   ASSERT_TRUE(p1 < p2.getData());
   ASSERT_TRUE(p1.getData() < p2);
   ASSERT_TRUE(p1 < p2);
   ASSERT_TRUE(!(p2 < p1));
   ASSERT_TRUE(!(p2 < p2));
   ASSERT_TRUE(!(p1 < p1));
   
   ASSERT_EQ(hash(p1), hash(p1.getData()));
   ASSERT_EQ(hash(p2), hash(p2.getData()));
}

TEST(SharedPointerTest, testNullPtrOps)
{
   SharedPointer<char> p1(nullptr);
   SharedPointer<char> p2 = nullptr;
   SharedPointer<char> null;
   ASSERT_EQ(p1, nullptr);
   ASSERT_FALSE(p1);
   ASSERT_FALSE(p1.getData());
   ASSERT_EQ(p2, null);
   ASSERT_EQ(p2, nullptr);
   ASSERT_EQ(nullptr, p2);
   ASSERT_FALSE(p2);
   ASSERT_FALSE(p2.getData());
   ASSERT_TRUE(p1 == p2);
   
   SharedPointer<char> p3 = p1;
   ASSERT_EQ(p3, p1);
   ASSERT_EQ(p3, null);
   ASSERT_EQ(p3, nullptr);
   ASSERT_EQ(nullptr, p3);
   ASSERT_FALSE(p3.getData());
   
   p3 = nullptr;
   
   // check for non-ambiguity
   SharedPointer<char> p1Zero(0);
   SharedPointer<char> p2Zero = 0;
   
   p3 = 0;
   SharedPointer<char> p4(new char);
   ASSERT_TRUE(p4);
   ASSERT_TRUE(p4.getData());
   ASSERT_TRUE(p4 != nullptr);
   ASSERT_TRUE(nullptr != p4);
   ASSERT_TRUE(p4 != p1);
   ASSERT_TRUE(p1 != p4);
   ASSERT_TRUE(p4 != p1);
   ASSERT_TRUE(p4 != p2);
   ASSERT_TRUE(p4 != null);
   ASSERT_TRUE(p4 != p3);
}

TEST(SharedPointerTest, testSwap)
{
   SharedPointer<int> p1;
   SharedPointer<int> p2(new int(42));
   SharedPointer<int> control = p2;
   ASSERT_NE(p1, control);
   ASSERT_TRUE(p1.isNull());
   ASSERT_EQ(p2, control);
   ASSERT_FALSE(p2.isNull());
   ASSERT_EQ(*p2, 42);
   
   p1.swap(p2);
   ASSERT_EQ(p1, control);
   ASSERT_FALSE(p1.isNull());
   ASSERT_NE(p2, control);
   ASSERT_TRUE(p2.isNull());
   ASSERT_EQ(*p1, 42);
   
   p1.swap(p2);
   ASSERT_NE(p1, control);
   ASSERT_TRUE(p1.isNull());
   ASSERT_EQ(p2, control);
   ASSERT_FALSE(p2.isNull());
   ASSERT_EQ(*p2, 42);
   
   swap(p1, p2);
   ASSERT_EQ(p1, control);
   ASSERT_FALSE(p1.isNull());
   ASSERT_NE(p2, control);
   ASSERT_TRUE(p2.isNull());
   ASSERT_EQ(*p1, 42);
   
   WeakPointer<int> w1;
   WeakPointer<int> w2 = control;
   ASSERT_TRUE(w1.isNull());
   ASSERT_FALSE(w2.isNull());
   ASSERT_EQ(w2.lock(), control);
   
   w1.swap(w2);
   ASSERT_FALSE(w1.isNull());
   ASSERT_TRUE(w2.isNull());
   ASSERT_EQ(w1.lock(), control);
   ASSERT_FALSE(w2.lock());
   
   swap(w1, w2);
   ASSERT_TRUE(w1.isNull());
   ASSERT_EQ(w2.lock(), control);
   
   p1.reset();
   p2.reset();
   control.reset();
   
   ASSERT_TRUE(w1.isNull());
   ASSERT_TRUE(w2.isNull());
}

TEST(SharedPointerTest, testMoveSemantics)
{
   SharedPointer<int> p1;
   SharedPointer<int> p2(new int(42));
   SharedPointer<int> control = p2;
   ASSERT_NE(p1, control);
   ASSERT_TRUE(p1.isNull());
   ASSERT_EQ(p2, control);
   ASSERT_FALSE(p2.isNull());
   ASSERT_EQ(*p2, 42);
   
   // move assignment
   p1 = std::move(p2);
   ASSERT_EQ(p1, control);
   ASSERT_FALSE(p1.isNull());
   ASSERT_NE(p2, control);
   ASSERT_TRUE(p2.isNull());
   ASSERT_EQ(*p1, 42);
   
   // move construction
   SharedPointer<int> p3 = std::move(p1);
   ASSERT_NE(p1, control);
   ASSERT_TRUE(p1.isNull());
   ASSERT_EQ(p3, control);
   ASSERT_FALSE(p3.isNull());
   ASSERT_EQ(*p3, 42);
   
   WeakPointer<int> w1;
   WeakPointer<int> w2 = control;
   ASSERT_TRUE(w1.isNull());
   ASSERT_FALSE(w2.isNull());
   ASSERT_FALSE(w1.lock());
   
   // move assignment
   w1 = std::move(w2);
   ASSERT_TRUE(w2.isNull());
   ASSERT_FALSE(w1.isNull());
   ASSERT_EQ(w1.lock(), control);
   ASSERT_FALSE(w2.lock());
   
   // move construction
   WeakPointer<int> w3 = std::move(w1);
   ASSERT_TRUE(w1.isNull());
   ASSERT_FALSE(w3.isNull());
   ASSERT_EQ(w3.lock(), control);
   ASSERT_FALSE(w1.lock());
   
   p1.reset();
   p2.reset();
   p3.reset();
   control.reset();
   ASSERT_TRUE(w1.isNull());
   ASSERT_TRUE(w2.isNull());
   ASSERT_TRUE(w3.isNull());
}

TEST(SharedPointerTest, testUseOfForwardDeclared)
{
   SharedPointer<ForwardDeclared> ptr;
   SharedPointer<ForwardDeclared> ptr2 = ptr;
   SharedPointer<ForwardDeclared> ptr3;
   ptr3 = ptr;
   SharedPointer<ForwardDeclared> ptr4;
   ptr4 = std::move(ptr);
   SharedPointer<ForwardDeclared> ptr5 = std::move(ptr2);
   ptr4.swap(ptr3);
   ptr3.swap(ptr4);
}

TEST(SharedPointerTest, testMemoryManagement)
{
   int generation = Data::sm_generatorCounter + 1;
   int destructorCounter = Data::sm_destructorCounter;
   SharedPointer<Data> ptr = SharedPointer<Data>(new Data);
   ASSERT_EQ(ptr->m_generation, generation);
   ASSERT_EQ(Data::sm_destructorCounter, destructorCounter);
   ASSERT_EQ(Data::sm_generatorCounter, generation);
   
   ptr = ptr;
   ASSERT_EQ(ptr->m_generation, generation);
   ASSERT_EQ(Data::sm_destructorCounter, destructorCounter);
   ASSERT_EQ(Data::sm_generatorCounter, generation);
   
   {
      SharedPointer<Data> copy = ptr;
      ASSERT_EQ(ptr->m_generation, generation);
      ASSERT_EQ(copy->m_generation, generation);
   }
   ASSERT_EQ(ptr->m_generation, generation);
   ASSERT_EQ(Data::sm_destructorCounter, destructorCounter);
   ASSERT_EQ(Data::sm_generatorCounter, generation);
   {
      WeakPointer<Data> weak = ptr;
      weak = ptr;
      ASSERT_EQ(ptr->m_generation, generation);
      ASSERT_EQ(Data::sm_destructorCounter, destructorCounter);
      ASSERT_EQ(Data::sm_generatorCounter, generation);
      weak = weak;
      ASSERT_EQ(ptr->m_generation, generation);
      ASSERT_EQ(Data::sm_destructorCounter, destructorCounter);
      ASSERT_EQ(Data::sm_generatorCounter, generation);
      
      SharedPointer<Data> strong = weak;
      ASSERT_EQ(ptr->m_generation, generation);
      ASSERT_EQ(strong->m_generation, generation);
      ASSERT_EQ(Data::sm_destructorCounter, destructorCounter);
      ASSERT_EQ(Data::sm_generatorCounter, generation);
   }
   ASSERT_EQ(ptr->m_generation, generation);
   ASSERT_EQ(Data::sm_destructorCounter, destructorCounter);
   ASSERT_EQ(Data::sm_generatorCounter, generation);
   
   WeakPointer<Data> weak = ptr;
   ptr = SharedPointer<Data>();
   
   // destructor must have been called
   ASSERT_EQ(Data::sm_destructorCounter, destructorCounter + 1);
   ASSERT_TRUE(ptr.isNull());
   ASSERT_TRUE(weak.isNull());
   
   // if we create a strong pointer from the weak, it must still be null
   ptr = weak;
   ASSERT_TRUE(ptr.isNull());
   ASSERT_TRUE(ptr == nullptr);
   ASSERT_EQ(ptr.getData(), reinterpret_cast<Data *>(0));
}

// NVD for "non-virtual destructor"
struct NVDData
{
   static int sm_destructorCounter;
   ~NVDData() 
   { 
      ++sm_destructorCounter;
   }
   
   int m_dummy;
};

int NVDData::sm_destructorCounter;

struct NVDDerivedData : NVDData
{
   static int sm_destructorCounter;
   ~NVDDerivedData()
   {
      ++sm_destructorCounter;
   }
};

int NVDDerivedData::sm_destructorCounter;

TEST(SharedPointerTest, testNonVirtualDestructors)
{
   NVDData::sm_destructorCounter = NVDDerivedData::sm_destructorCounter = 0;
   {
      SharedPointer<NVDData> ptr(new NVDData);
   }
   ASSERT_EQ(NVDData::sm_destructorCounter, 1);
   ASSERT_EQ(NVDDerivedData::sm_destructorCounter, 0);
   NVDData::sm_destructorCounter = NVDDerivedData::sm_destructorCounter = 0;
   {
      SharedPointer<NVDDerivedData> ptr(new NVDDerivedData);
   }
   ASSERT_EQ(NVDData::sm_destructorCounter, 1);
   ASSERT_EQ(NVDDerivedData::sm_destructorCounter, 1);
   NVDData::sm_destructorCounter = NVDDerivedData::sm_destructorCounter = 0;
   {
      SharedPointer<NVDData> bptr;
      SharedPointer<NVDDerivedData> ptr(new NVDDerivedData);
      bptr = ptr;
   }
   ASSERT_EQ(NVDData::sm_destructorCounter, 1);
   ASSERT_EQ(NVDDerivedData::sm_destructorCounter, 1);
   
   NVDData::sm_destructorCounter = NVDDerivedData::sm_destructorCounter = 0;
   {
      SharedPointer<NVDData> ptr(new NVDDerivedData);
   }
   ASSERT_EQ(NVDData::sm_destructorCounter, 1);
   ASSERT_EQ(NVDDerivedData::sm_destructorCounter, 1);
}

TEST(SharedPointerTest, testLock)
{
   SharedPointer<int> ptr = SharedPointer<int>::create();
   ASSERT_TRUE(ptr);
   WeakPointer<int> weakPtr = ptr;
   ASSERT_TRUE(ptr == weakPtr);
   ASSERT_TRUE(ptr == weakPtr.lock());
   ASSERT_TRUE(ptr == weakPtr.toStrongRef());
   
   ptr.reset();
   ASSERT_TRUE(!weakPtr);
   ASSERT_TRUE(ptr != weakPtr);
   ASSERT_TRUE(ptr == weakPtr.lock());
   ASSERT_TRUE(ptr == weakPtr.toStrongRef());
}

class DerivedData: public Data
{
public:
   static int sm_derivedDestructorCounter;
   int m_moreData;
   DerivedData() 
      : m_moreData(0) 
   { }
   ~DerivedData()
   {
      ++sm_derivedDestructorCounter;
   }
   
   virtual void virtualDelete()
   {
      delete this;
   }
   
   virtual int classLevel()
   { 
      return 2;
   }
};
int DerivedData::sm_derivedDestructorCounter = 0;

class Stuffing
{
public:
   char m_buffer[16];
   Stuffing() 
   {
      for (uint i = 0; i < sizeof m_buffer; ++i) {
         m_buffer[i] = 16 - i;
      } 
   }
   virtual ~Stuffing()
   {}
};

class DiffPtrDerivedData: public Stuffing, public Data
{
public:
   virtual int classLevel() 
   { 
      return 3;
   }
};

class VirtualDerived: virtual public Data
{
public:
   int m_moreData;
   
   VirtualDerived() 
      : m_moreData(0xc0ffee)
   {}
   virtual int classLevel() 
   { 
      return 4;
   }
};

TEST(SharedPointerTest, testDownCast)
{
   {
      SharedPointer<DerivedData> ptr = SharedPointer<DerivedData>(new DerivedData);
      SharedPointer<Data> basePtr = pdk::utils::shared_pointer_cast<Data>(ptr);
      SharedPointer<Data> other;
      
      ASSERT_TRUE(ptr == basePtr);
      ASSERT_TRUE(basePtr == ptr);
      ASSERT_TRUE(!(ptr != basePtr));
      ASSERT_TRUE(!(basePtr != ptr));
      
      ASSERT_TRUE(ptr != other);
      ASSERT_TRUE(other != ptr);
      ASSERT_TRUE(!(ptr == other));
      ASSERT_TRUE(!(other == ptr));
   }
   {
      SharedPointer<DerivedData> ptr = SharedPointer<DerivedData>(new DerivedData);
      SharedPointer<Data> baseptr = ptr;
   }
   int destructorCount;
   destructorCount = DerivedData::sm_derivedDestructorCounter;
   {
      SharedPointer<Data> baseptr;
      {
         SharedPointer<DerivedData> ptr = SharedPointer<DerivedData>(new DerivedData);
         baseptr = ptr;
         ASSERT_EQ(baseptr, ptr);
      }
   }
   ASSERT_EQ(DerivedData::sm_derivedDestructorCounter, destructorCount + 1);
   destructorCount = DerivedData::sm_derivedDestructorCounter;
   {
      SharedPointer<DerivedData> ptr = SharedPointer<DerivedData>(new DerivedData);
      WeakPointer<Data> baseptr = ptr;
      ASSERT_EQ(baseptr, ptr);
      ptr = SharedPointer<DerivedData>();
      ASSERT_TRUE(baseptr.isNull());
   }
   ASSERT_EQ(DerivedData::sm_derivedDestructorCounter, destructorCount + 1);
   destructorCount = DerivedData::sm_derivedDestructorCounter;
   {
      SharedPointer<DerivedData> ptr = SharedPointer<DerivedData>(new DerivedData);
      WeakPointer<Data> weakptr(ptr);
      SharedPointer<Data> baseptr = weakptr;
      ASSERT_EQ(baseptr, ptr);
      WeakPointer<Data> baseweakptr = weakptr;
      ASSERT_EQ(baseweakptr, ptr);
   }
   ASSERT_EQ(DerivedData::sm_derivedDestructorCounter, destructorCount + 1);
}

void function_data_by_value(SharedPointer<Data> ptr) 
{ 
   PDK_UNUSED(ptr);
}
void function_data_by_ref(const SharedPointer<Data> &ptr)
{ 
   PDK_UNUSED(ptr);
}

TEST(SharedPointerTest, testFunctionCallDownCast)
{
   SharedPointer<DerivedData> p(new DerivedData());
   function_data_by_ref(p);
   function_data_by_value(p);
}

TEST(SharedPointerTest, testUpCast)
{
   SharedPointer<Data> baseptr = SharedPointer<Data>(new DerivedData);
   {
      SharedPointer<DerivedData> derivedptr = pdk::utils::shared_pointer_cast<DerivedData>(baseptr);
      ASSERT_TRUE(baseptr == derivedptr);
      ASSERT_EQ(static_cast<Data *>(derivedptr.getData()), baseptr.getData());
      ASSERT_EQ(int(refcount_data(baseptr)->m_weakRef.load()), 2);
      ASSERT_EQ(int(refcount_data(baseptr)->m_strongRef.load()), 2);
   }
   ASSERT_EQ(int(refcount_data(baseptr)->m_weakRef.load()), 1);
   ASSERT_EQ(int(refcount_data(baseptr)->m_strongRef.load()), 1);
   {
      WeakPointer<DerivedData> derivedptr = pdk::utils::weak_pointer_cast<DerivedData>(baseptr);
      ASSERT_TRUE(baseptr == derivedptr);
   }
   ASSERT_EQ(int(refcount_data(baseptr)->m_weakRef.load()), 1);
   ASSERT_EQ(int(refcount_data(baseptr)->m_strongRef.load()), 1);
   {
      WeakPointer<Data> weakptr = baseptr;
      SharedPointer<DerivedData> derivedptr = pdk::utils::shared_pointer_cast<DerivedData>(weakptr);
      ASSERT_TRUE(baseptr == derivedptr);
      ASSERT_EQ(static_cast<Data *>(derivedptr.getData()), baseptr.getData());
   }
   ASSERT_EQ(int(refcount_data(baseptr)->m_weakRef.load()), 1);
   ASSERT_EQ(int(refcount_data(baseptr)->m_strongRef.load()), 1);
   
   {
      SharedPointer<DerivedData> derivedptr = baseptr.staticCast<DerivedData>();
      ASSERT_TRUE(baseptr == derivedptr);
      ASSERT_EQ(static_cast<Data *>(derivedptr.getData()), baseptr.getData());
   }
   ASSERT_EQ(int(refcount_data(baseptr)->m_weakRef.load()), 1);
   ASSERT_EQ(int(refcount_data(baseptr)->m_strongRef.load()), 1);
}
