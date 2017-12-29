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
// Created by softboy on 2017/12/29.


#include "gtest/gtest.h"
#include "pdk/utils/ScopedPointer.h"
#include "pdk/base/os/thread/Atomic.h"
#include <list>
#include <utility>
#include <tuple>
#include <cstdlib>

using pdk::utils::ScopedPointer;
using pdk::utils::ScopedArrayPointer;
using pdk::os::thread::AtomicInt;

TEST(ScopedPointerTest, testDefaultConstructor)
{
   ScopedPointer<int> p;
   ASSERT_EQ(p.getData(), static_cast<int *>(nullptr));
}

TEST(ScopedPointerTest, testDataOnDefaultConstructed)
{
   ScopedPointer<int> p;
   ASSERT_EQ(p.getData(), static_cast<int *>(nullptr));
}

class MyClass
{};

class MySubClass : public MyClass
{};

TEST(ScopedPointerTest, testUseSubClassInConstructor)
{
   // Use a syntax which users typically would do.
   ScopedPointer<MyClass> p(new MyClass);   
}

TEST(ScopedPointerTest, testDataOnValue)
{
   int *const rawPointer = new int(5);
   ScopedPointer<int> p(rawPointer);
   ASSERT_EQ(p.getData(), rawPointer);
}

TEST(ScopedPointerTest, testDataSignature)
{
   const ScopedPointer<int> p;
   p.getData();
   
}

TEST(ScopedPointerTest, testReset)
{
   /* Call reset() on a default constructed value. */
   {
      ScopedPointer<int> p;
      p.reset();
      ASSERT_EQ(p.getData(), static_cast<int *>(nullptr));
   }
   /* Call reset() on an active value. */
   {
      ScopedPointer<int> p(new int(2018));
      p.reset();
      ASSERT_EQ(p.getData(), static_cast<int *>(nullptr));
   }
   /* Call reset() with a value, on an active value. */
   {
      ScopedPointer<int> p(new int(2018));
      int *const newPtr = new int(9);
      p.reset(newPtr);
      ASSERT_EQ(*p.getData(), 9);
   }
   /* Call reset() with a value, on default constructed value. */
   {
      ScopedPointer<int> p;
      int *const value = new int(2008);
      p.reset(value);
      ASSERT_EQ(*p.getData(), 2008);
   }
}

class AbstractClass
{
public:
   virtual ~AbstractClass()
   {}
   
   virtual int member() const = 0;
};

class SubClass : public AbstractClass
{
public:
   virtual int member() const
   {
      return 5;
   }
};

TEST(ScopedPointerTest, testDeferenceOperator)
{
   /* Dereference a basic value. */
   {
      ScopedPointer<int> p(new int(2018));
      const int value2 = *p;
      ASSERT_EQ(value2, 2018);
   }
   /* Dereference a pointer to an abstract class. This verifies
    * that the operator returns a reference, when compiling
    * with MSVC 2005. 
    */
   {
      ScopedPointer<AbstractClass> p(new SubClass);
      ASSERT_EQ((*p).member(), 5);
   }
}

TEST(ScopedPointerTest, testDereferenceOperatorSignature)
{
   /* The operator should be const. */
   {
      const ScopedPointer<int> p(new int(2018));
      *p;
   }
   /* A reference should be returned, not a value. */
   {
      const ScopedPointer<int> p(new int(2018));
      PDK_UNUSED(static_cast<int &>(*p));
   }
   /* Instantiated on a const object, the returned object is a const reference. */
   {
      const ScopedPointer<const int> p(new int(2018));
      PDK_UNUSED(static_cast<const int &>(*p));
   }
}

class AnyForm
{
public:
   int value;
};

TEST(ScopedPointerTest, testPointerOperator)
{
   ScopedPointer<AnyForm> p(new AnyForm);
   p->value = 2018;
   ASSERT_EQ(p->value, 2018);
}

TEST(ScopedPointerTest, testPointerOperatorSignature)
{
   /* The operator should be const. */
   const ScopedPointer<AnyForm> p(new AnyForm);
   p->value = 2018;
   ASSERT_TRUE(p->value);
}

TEST(ScopedPointerTest, testNegationOperator)
{
   /* Invoke on default constructed value. */
   {
      ScopedPointer<int> p;
      ASSERT_TRUE(!p);
   }
   /* Invoke on a value. */
   {
      ScopedPointer<int> p(new int(2018));
      ASSERT_EQ(!p, false);
   }
}

TEST(ScopedPointerTest, testNegationOperatorSignature)
{
   /* The signature should be const. */
   const ScopedPointer<int> p;
   !p;
   PDK_UNUSED(static_cast<bool>(!p));  
}

TEST(ScopedPointerTest, testOperatorBool)
{
   // Invoke on default constructed value.
   {
      ScopedPointer<int> p;
      ASSERT_EQ(static_cast<bool>(p), false);
   }
   // Invoke on active value.
   {
      ScopedPointer<int> p(new int(2018));
      ASSERT_EQ(static_cast<bool>(p), true);
   }
}

TEST(ScopedPointerTest, testOperatorBoolSignature)
{
   const ScopedPointer<int> p;
   (void)static_cast<bool>(p);
}

TEST(ScopedPointerTest, testIsNull)
{
   /* Invoke on default constructed value. */
   {
      ScopedPointer<int> p;
      ASSERT_TRUE(p.isNull());
      ASSERT_TRUE(p == nullptr);
      ASSERT_TRUE(nullptr == p);
   }
   /* Invoke on a set value. */
   {
      ScopedPointer<int> p(new int(2018));
      ASSERT_TRUE(!p.isNull());
      ASSERT_TRUE(p != nullptr);
      ASSERT_TRUE(nullptr != p);
   }
}

TEST(ScopedPointerTest, testIsNullSignature)
{
   const ScopedPointer<int> p(new int(2018));
   ASSERT_FALSE(static_cast<bool>(p.isNull()));
}

TEST(ScopedPointerTest, testObjectSize)
{
   ASSERT_EQ(sizeof(ScopedPointer<int>), sizeof(void *));
}

struct RefCounted
{
   RefCounted()
      : m_ref(0)
   {
      sm_instanceCount.ref();
   }
   
   RefCounted(const RefCounted &)
      : m_ref(0)
   {
      sm_instanceCount.ref();
   }
   
   ~RefCounted()
   {
      PDK_ASSERT(m_ref.load() == 0);
      sm_instanceCount.deref();
   }
   
   RefCounted &operator =(const RefCounted &)
   {
      return *this;
   }
   
   AtomicInt m_ref;
   static AtomicInt sm_instanceCount;
};

AtomicInt RefCounted::sm_instanceCount = 0;

template <typename A1, typename A2, typename B>
void scoped_pointer_comparison(const A1 &a1, const A2 &a2, const B &b)
{
   // test equality on equal pointers
   ASSERT_EQ(a1, a2);
   ASSERT_EQ(a2, a1);
   
   // test inequality on equal pointers
   ASSERT_TRUE(!(a1 != a2));
   ASSERT_TRUE(!(a2 != a1));
   
   // test equality on unequal pointers
   ASSERT_TRUE(!(a1 == b));
   ASSERT_TRUE(!(a2 == b));
   ASSERT_TRUE(!(b == a1));
   ASSERT_TRUE(!(b == a2));
   
   // test inequality on unequal pointers
   ASSERT_TRUE(b != a1);
   ASSERT_TRUE(b != a2);
   ASSERT_TRUE(a1 != b);
   ASSERT_TRUE(a2 != b);
}

TEST(ScopedPointerTest, testComparison)
{
   ASSERT_EQ(RefCounted::sm_instanceCount.load(), 0);
   {
      RefCounted *a = new RefCounted;
      RefCounted *b = new RefCounted;
      ASSERT_EQ(RefCounted::sm_instanceCount.load(), 2);
      ScopedPointer<RefCounted> pa1(a);
      ScopedPointer<RefCounted> pa2(a);
      ScopedPointer<RefCounted> pb(b);
      scoped_pointer_comparison(pa1, pa1, pb);
      scoped_pointer_comparison(pa2, pa2, pb);
      scoped_pointer_comparison(pa1, pa2, pb);
      pa2.take();
      ASSERT_EQ(RefCounted::sm_instanceCount.load(), 2);
   }
   ASSERT_EQ(RefCounted::sm_instanceCount.load(), 0);
   {
      RefCounted *a = new RefCounted[42];
      RefCounted *b = new RefCounted[43];
      ASSERT_EQ(RefCounted::sm_instanceCount.load(), 85);
      ScopedArrayPointer<RefCounted> pa1(a);
      ScopedArrayPointer<RefCounted> pa2(a);
      ScopedArrayPointer<RefCounted> pb(b);
      scoped_pointer_comparison(pa1, pa1, pb);
      scoped_pointer_comparison(pa2, pa2, pb);
      scoped_pointer_comparison(pa1, pa2, pb);
      pa2.take();
      ASSERT_EQ(RefCounted::sm_instanceCount.load(), 85);
   }
   ASSERT_EQ(RefCounted::sm_instanceCount.load(), 0);
   {
//      RefCounted *a = new RefCounted;
//      RefCounted *b = new RefCounted;
//      ASSERT_EQ(RefCounted::sm_instanceCount.load(), 2);
      // @TODO test SharedDataPointer
   }
   ASSERT_EQ(RefCounted::sm_instanceCount.load(), 0);
}

TEST(ScopedPointerTest, testArray)
{
   int instCount = RefCounted::sm_instanceCount.load();
   {
      ScopedArrayPointer<RefCounted> array;
      array.reset(new RefCounted[42]);
      ASSERT_EQ(RefCounted::sm_instanceCount.load(), 42 + instCount);
   }
   ASSERT_EQ(RefCounted::sm_instanceCount.load(), instCount);
   {
      ScopedArrayPointer<RefCounted> array(new RefCounted[42]);
      ASSERT_EQ(instCount + 42, RefCounted::sm_instanceCount.load());
      array.reset(new RefCounted[28]);
      ASSERT_EQ(instCount + 28, RefCounted::sm_instanceCount.load());
      array.reset(0);
      ASSERT_EQ(instCount, RefCounted::sm_instanceCount.load());
   }
   ASSERT_EQ(instCount, RefCounted::sm_instanceCount.load());
}
