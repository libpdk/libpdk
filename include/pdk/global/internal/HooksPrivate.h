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
// Created by softboy on 2018/01/27.

#include "pdk/global/Global.h"

#ifndef PDK_GLOBAL_INTERNAL_HOOKS_H
#define PDK_GLOBAL_INTERNAL_HOOKS_H

namespace pdk {

// forward
namespace kernel {
class Object;
} // kernel

namespace hooks {

using pdk::kernel::Object;

enum HookIndex {
   HookDataVersion = 0,
   HookDataSize = 1,
   PdkVersion = 2,
   AddQObject = 3,
   RemoveQObject = 4,
   Startup = 5,
   TypeInformationVersion = 6,
   LastHookIndex
};

using AddQObjectCallback = void(*)(Object*);
using RemoveQObjectCallback = void(*)(Object*);
using StartupCallback = void(*)();

} // hooks

extern pdk::uintptr PDK_CORE_EXPORT sg_pdkHookData[];

} // pdk

#endif // PDK_GLOBAL_INTERNAL_HOOKS_H
