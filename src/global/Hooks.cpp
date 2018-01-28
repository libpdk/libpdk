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

#include "pdk/global/internal/HooksPrivate.h"

namespace pdk {

// Only add to the end, and bump version if you do.
pdk::uintptr PDK_CORE_EXPORT sg_pdkHookData[] = {
   3, // hook data version
   hooks::LastHookIndex, // size of qtHookData
   PDK_VERSION,
   
   // AddQObject, void(*)(Object*), called for every constructed Object
   // Note: this is called from the Object constructor, ie. the sub-class
   // constructors haven't run yet.
   0,
   
   // RemoveQObject, void(*)(Object*), called for every destructed Object
   // Note: this is called from the Object destructor, ie. the object
   // you get as an argument is already largely invalid.
   0,
   
   // Startup, void(*)(), called once CoreApplication is operational
   0,
   
   // TypeInformationVersion, an integral value, bumped whenever private
   // object sizes or member offsets that are used in Qt Creator's
   // data structure "pretty printing" change.
   //
   // The required sizes and offsets are tested in tests/auto/other/toolsupport.
   // When this fails and the change was intentional, adjust the test and
   // adjust this value here.
   15
};

PDK_STATIC_ASSERT(hooks::LastHookIndex == sizeof(sg_pdkHookData) / sizeof(sg_pdkHookData[0]));

} // pdk
