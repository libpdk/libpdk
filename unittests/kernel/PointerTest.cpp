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
// Created by softboy on 2018/04/03.

#include "gtest/gtest.h"
#include "pdk/kernel/Pointer.h"
#include "pdk/kernel/Object.h"
#include "pdk/base/os/thread/Runnable.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/ThreadPool.h"

using pdk::kernel::Pointer;
using pdk::kernel::Object;
using pdk::os::thread::Runnable;
using pdk::os::thread::Thread;
using pdk::os::thread::ThreadPool;

TEST(PointerTest, testConstructors)
{ 
   Object temp;
   Pointer<Object> p1;
   Pointer<Object> p2(&temp);
   Pointer<Object> p3(p2);
   EXPECT_EQ(p1, Pointer<Object>(nullptr));
   EXPECT_EQ(p2, Pointer<Object>(&temp));
   EXPECT_EQ(p3, Pointer<Object>(&temp));
}

TEST(PointerTest, testDestructor)
{
   // Make two Pointer's to the same object
   Object *object = new Object;
   Pointer<Object> p1 = object;
   Pointer<Object> p2 = object;
   // Check that they point to the correct object
   EXPECT_EQ(p1, Pointer<Object>(object));
   EXPECT_EQ(p2, Pointer<Object>(object));
   EXPECT_EQ(p1, p2);
   // Destroy the guarded object
   delete object;
   // Check that both pointers were zeroed
   EXPECT_EQ(p1, Pointer<Object>(nullptr));
   EXPECT_EQ(p2, Pointer<Object>(nullptr));
   EXPECT_EQ(p1, p2);
}

class DerivedObject : public Object
{
public:
   inline DerivedObject *getMePtr() const
   {
      return const_cast<DerivedObject *>(this);
   }
};

TEST(PointerTest, testAssignmentOperators)
{
   Pointer<Object> p1;
   Pointer<Object> p2;
   DerivedObject *derivedObject = new DerivedObject;
   // Test assignment with a Object-derived object pointer
   p1 = derivedObject;
   p2 = p1;
   EXPECT_EQ(p1, Pointer<Object>(derivedObject));
   EXPECT_EQ(p2, Pointer<Object>(derivedObject));
   EXPECT_EQ(p1, Pointer<Object>(p2));
   delete derivedObject;
   // Test assignment with a null pointer
   p1 = 0;
   p2 = p1;
   EXPECT_EQ(p1, Pointer<Object>(nullptr));
   EXPECT_EQ(p2, Pointer<Object>(nullptr));
   EXPECT_EQ(p1, Pointer<Object>(p2));
   
   // Test assignment with a real Object pointer
   Object *object = new Object;
   
   p1 = object;
   p2 = p1;
   EXPECT_EQ(p1, Pointer<Object>(object));
   EXPECT_EQ(p2, Pointer<Object>(object));
   EXPECT_EQ(p1, Pointer<Object>(p2));
   
   // Test assignment with the same pointer that's already guarded
   p1 = object;
   p2 = p1;
   EXPECT_EQ(p1, Pointer<Object>(object));
   EXPECT_EQ(p2, Pointer<Object>(object));
   EXPECT_EQ(p1, Pointer<Object>(p2));
   
   // Cleanup
   delete object;
}

TEST(PointerTest, testEqualityOperators)
{
   Pointer<Object> p1;
   Pointer<Object> p2;
   
   EXPECT_TRUE(p1 == p2);
   
   Object *object = nullptr;
   p1 = object;
   EXPECT_TRUE(p1 == p2);
   EXPECT_TRUE(p1 == object);
   p2 = object;
   EXPECT_TRUE(p2 == p1);
   EXPECT_TRUE(p2 == object);
   Object *temp = new Object;
   p1 = temp;
   EXPECT_TRUE(p1 != p2);
   p2 = p1;
   EXPECT_TRUE(p1 == p2);
   
   // compare to zero
   p1 = nullptr;
   EXPECT_TRUE(p1 == nullptr);
   EXPECT_TRUE(nullptr == p1);
   EXPECT_TRUE(p2 != nullptr);
   EXPECT_TRUE(nullptr != p2);
   EXPECT_TRUE(p1 == object);
   EXPECT_TRUE(object == p1);
   EXPECT_TRUE(p2 != object);
   EXPECT_TRUE(object != p2);
   delete temp;
}

TEST(PointerTest, testSwap)
{
   Pointer<Object> c1, c2;
   {
      Object o;
      c1 = &o;
      EXPECT_TRUE(c2.isNull());
      EXPECT_EQ(c1.getData(), &o);
      c1.swap(c2);
      EXPECT_TRUE(c1.isNull());
      EXPECT_EQ(c2.getData(), &o);
   }
   EXPECT_TRUE(c1.isNull());
   EXPECT_TRUE(c2.isNull());
}

TEST(PointerTest, testIsNull)
{
   Pointer<Object> p1;
   EXPECT_TRUE(p1.isNull());
   Object *temp = new Object;
   p1 = temp;
   EXPECT_TRUE(!p1.isNull());
   p1 = nullptr;
   EXPECT_TRUE(p1.isNull());
   delete temp;
}

TEST(PointerTest, testDereferenceOperators)
{
   DerivedObject *temp = new DerivedObject;
   Pointer<DerivedObject> p1 = temp;
   Pointer<DerivedObject> p2;
   
   // operator->() -- only makes sense if not null
   Object *object = p1->getMePtr();
   EXPECT_EQ(object, temp);
   
   // operator*() -- only makes sense if not null
   Object &ref = *p1;
   EXPECT_EQ(&ref, temp);
   
   // operator T*()
   EXPECT_EQ(static_cast<Object *>(p1), temp);
   EXPECT_EQ(static_cast<Object *>(p2), static_cast<Object *>(nullptr));
   
   // getData()
   EXPECT_EQ(p1.getData(), temp);
   EXPECT_EQ(p2.getData(), static_cast<Object *>(nullptr));
   delete temp;
}

TEST(PointerTest, testDisconnect)
{
   // Verify that pointer remains guarded when all signals are disconencted.
   Pointer<Object> p1 = new Object;
   EXPECT_TRUE(!p1.isNull());
   delete static_cast<Object *>(p1);
   EXPECT_TRUE(p1.isNull());
}

class ChildObject : public Object
{
   Pointer<Object> m_guardedPointer;
   
public:
   ChildObject(Object *parent)
      : Object(parent),
        m_guardedPointer(parent)
   {}
   
   void test();
   ~ChildObject();
};

ChildObject::~ChildObject()
{
   test();
}

void ChildObject::test()
{
   EXPECT_EQ(static_cast<Object *>(m_guardedPointer), static_cast<Object *>(nullptr));
}

class DerivedChild;

class DerivedParent : public Object
{
   DerivedChild *m_derivedChild;
   
public:
   DerivedParent();
   ~DerivedParent();
};

class DerivedChild : public Object
{
   
   DerivedParent *m_parentPointer;
   Pointer<DerivedParent> m_guardedParentPointer;
   
public:
   DerivedChild(DerivedParent *parent)
      : Object(parent),
        m_parentPointer(parent),
        m_guardedParentPointer(parent)
   {}
   void test();
   ~DerivedChild();
};

DerivedParent::DerivedParent()
{
   m_derivedChild = new DerivedChild(this);
}

DerivedParent::~DerivedParent()
{
   delete m_derivedChild;
}

void DerivedChild::test()
{
   EXPECT_EQ(static_cast<DerivedParent *>(m_guardedParentPointer), m_parentPointer);
}

DerivedChild::~DerivedChild()
{
   test();
}

TEST(PointerTest, testCastDuringDestruction)
{
   {
      Object *parentObject = new Object();
      (void) new ChildObject(parentObject);
      delete parentObject;
   }
   
   {
      delete new DerivedParent();
   }
}

class TestRunnable : public Object, public Runnable {
   void run()
   {
      Pointer<Object> obj1 = new Object;
      Pointer<Object> obj2 = new Object;
      obj1->moveToThread(getThread()); // this is the owner thread
      obj1->deleteLater(); // the delete will happen in the owner thread
      obj2->moveToThread(getThread()); // this is the owner thread
      obj2->deleteLater(); // the delete will happen in the owner thread
   }
};

TEST(PointerTest, testThreadSafety)
{
   Thread owner;
   owner.start();
   ThreadPool pool;
   for (int i = 0; i < 300; i++) {
      Pointer<TestRunnable> task = new TestRunnable;
      task->setAutoDelete(true);
      task->moveToThread(&owner);
      pool.start(task);
   }
   pool.waitForDone();
   owner.quit();
   owner.wait();
}

TEST(PointerTest, testConstPointer)
{
   // Compile-time test that Pointer<const T> works.
   Pointer<const Object> fp = new Object;
   delete fp.getData();
}

TEST(PointerTest, testConstQPointer)
{
   // Check that const Pointers work. It's a bit weird to mark a pointer
   // const if its value can change, but the shallow-const principle in C/C++
   // allows this, and people use it, so document it with a test.
   //
   // It's unlikely that this test will fail in and out of itself, but it
   // presents the use-case to static and dynamic checkers that can raise
   // a warning (hopefully) should this become an issue.
   Object *temp = new Object;
   Object *o = new Object(temp);
   const Pointer<Object> p = o;
   delete o;
   delete temp;
   ASSERT_TRUE(!p);
}
