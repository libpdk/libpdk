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
// Created by softboy on 2018/01/28.

#include "pdk/kernel/Object.h"
#include "pdk/kernel/internal/ObjectPrivate.h"
#include "pdk/kernel/AbstractEventDispatcher.h"
#include "pdk/kernel/internal/AbstractEventDispatcherPrivate.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/kernel/internal/CoreApplicationPrivate.h"
#include "pdk/base/os/thread/Thread.h"
#include "pdk/base/os/thread/internal/ThreadPrivate.h"
#include "pdk/base/os/thread/Semaphore.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/os/thread/OrderedMutexLocker.h"
#include "pdk/global/internal/HooksPrivate.h"

#include <utility>
#include <memory>
#include <set>
#include <new>
#include <ctype.h>
#include <limits.h>

namespace pdk {
namespace kernel {

ObjectData::~ObjectData()
{}

namespace internal {

ObjectPrivate::ObjectPrivate(int version)
   : m_currentChildBeingDeleted(0),
     m_threadData(0)
{
   PDK_UNUSED(version);
   m_apiPtr = nullptr;
   m_parent = nullptr;                                 // no parent yet. It is set by setParent()
   m_isWidget = false;                           // assume not a widget object
   m_wasDeleted = false;                         // double-delete catcher
   m_isDeletingChildren = false;                 // set by deleteChildren()
   m_sendChildEvents = true;                     // if we should send ChildAdded and ChildRemoved events to parent
   m_receiveChildEvents = true;
   m_postedEvents = 0;
   m_extraData = 0;
   m_isWindow = false;
   m_deleteLaterCalled = false;
}

ObjectPrivate::~ObjectPrivate()
{
}

void delete_in_event_handler(Object *object)
{
   delete object;
}

} // internal


Object::Object(Object *parent)
   : m_implPtr(new ObjectPrivate)
{
}

Object::Object(ObjectPrivate &dd, Object *parent)
   : m_implPtr(&dd)
{
}

Object::~Object()
{
}

String Object::getObjectName() const
{
   PDK_D(const Object);
   return implPtr->m_extraData ? implPtr->m_extraData->m_objectName : String();
}

void Object::setObjectName(const String &name)
{
   PDK_D(Object);
   if (!implPtr->m_extraData) {
      implPtr->m_extraData = new internal::ObjectPrivate::ExtraData;
   }
   if (implPtr->m_extraData->m_objectName != name) {
      implPtr->m_extraData->m_objectName = name;
      // emit objectNameChanged
   }
}

bool Object::event(Event *e)
{
}

void Object::timerEvent(TimerEvent *)
{
}

void Object::childEvent(ChildEvent * /* event */)
{
}

void Object::customEvent(Event * /* event */)
{
}

bool Object::eventFilter(Object * /* watched */, Event * /* event */)
{
   return false;
}

Thread *Object::getThread() const
{
}

void Object::moveToThread(Thread *targetThread)
{
}

int Object::startTimer(int interval, pdk::TimerType timerType)
{
}

void Object::setParent(Object *parent)
{
}

void Object::installEventFilter(Object *obj)
{
}


void Object::removeEventFilter(Object *obj)
{
}

void Object::deleteLater()
{
}

} // kernel
} // pdk

