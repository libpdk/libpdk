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
// Created by softboy on 2018/05/10.

#include "pdk/base/os/thread/Atomic.h"

#define FORKFD_NO_SPAWNFD

#if defined(PDK_NO_DEBUG) && !defined(NDEBUG)
#  define NDEBUG
#endif

typedef pdk::os::thread::BasicAtomicInt ffd_atomic_int;
#define ffd_atomic_pointer(type)  pdk::os::thread::BasicAtomicPointer<type>

#define FFD_ATOMIC_INIT(val)    PDK_BASIC_ATOMIC_INITIALIZER(val)

#define FFD_ATOMIC_RELAXED  Relaxed
#define FFD_ATOMIC_ACQUIRE  Acquire
#define FFD_ATOMIC_RELEASE  Release
#define loadRelaxed         load
#define storeRelaxed        store

#define FFD_CONCAT(x, y)    x ## y

#define ffd_atomic_load(ptr,order)      (ptr)->FFD_CONCAT(load, order)()
#define ffd_atomic_store(ptr,val,order) (ptr)->FFD_CONCAT(store, order)(val)
#define ffd_atomic_exchange(ptr,val,order) (ptr)->FFD_CONCAT(fetchAndStore, order)(val)
#define ffd_atomic_compare_exchange(ptr,expected,desired,order1,order2) \
    (ptr)->FFD_CONCAT(testAndSet, order1)(*expected, desired, *expected)
#define ffd_atomic_add_fetch(ptr,val,order) ((ptr)->FFD_CONCAT(fetchAndAdd, order)(val) + val)

extern "C" {
#include "../../../../thirdparty/forkfd/forkfd.c"
}
