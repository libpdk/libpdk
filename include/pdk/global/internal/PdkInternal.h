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
// Created by softboy on 2018/03/04.

#ifndef PDK_GLOBAL_INTERNAL_PDK_INTERNAL_H
#define PDK_GLOBAL_INTERNAL_PDK_INTERNAL_H

#include "pdk/global/Global.h"

namespace pdk {

using InternalCallback = bool (*)(void **);

class PDK_CORE_EXPORT Internal {
public:
   enum class PaintDeviceFlags : uint
   {
      UnknownDevice = 0x00,
      Widget        = 0x01,
      Pixmap        = 0x02,
      Image         = 0x03,
      Printer       = 0x04,
      Picture       = 0x05,
      Pbuffer       = 0x06,    // GL pbuffer
      FramebufferObject = 0x07, // GL framebuffer object
      CustomRaster  = 0x08,
      MacQuartz     = 0x09,
      PaintBuffer   = 0x0a,
      OpenGL        = 0x0b
   };
   
   enum class RelayoutType : uint
   {
      RelayoutNormal,
      RelayoutDragging,
      RelayoutDropped
   };
   
   enum class DockPosition : uint
   {
      LeftDock,
      RightDock,
      TopDock,
      BottomDock,
      DockCount
   };
   
   enum class Callback : uint
   {
      EventNotifyCallback,
      LastCallback
   };
   
   static bool registerCallback(Callback, InternalCallback);
   static bool unregisterCallback(Callback, InternalCallback);
   static bool activateCallbacks(Callback, void **);
};

} // pdk

#endif // PDK_GLOBAL_INTERNAL_PDK_INTERNAL_H
