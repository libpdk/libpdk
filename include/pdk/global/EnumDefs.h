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
// Created by softboy on 2017/11/13.

#ifndef PDK_GLOBAL_ENUM_DEFS_H
#define PDK_GLOBAL_ENUM_DEFS_H

#include "pdk/global/Global.h"

namespace pdk {

enum class Initialization
{
   Uninitialized
};

static constexpr PDK_DECL_UNUSED Initialization Uninitialized = Initialization::Uninitialized;

enum class CaseSensitivity
{
   Insensitive,
   Sensitive
};

enum class TimerType
{
   PreciseTimer,
   CoarseTimer,
   VeryCoarseTimer
};

enum class EventPriority
{
   HighEventPriority = 1,
   NormalEventPriority = 0,
   LowEventPriority = -1
};

enum class DateFormat
{
   TextDate,      // default
   ISODate,       // ISO 8601
   SystemLocaleShortDate,
   SystemLocaleLongDate,
   DefaultLocaleShortDate,
   DefaultLocaleLongDate,
   RFC2822Date,        // RFC 2822 (+ 850 and 1036 during parsing)
   ISODateWithMs
};

enum class TimeSpec
{
   LocalTime,
   UTC,
   OffsetFromUTC,
   TimeZone
};

enum class DayOfWeek
{
   Monday = 1,
   Tuesday = 2,
   Wednesday = 3,
   Thursday = 4,
   Friday = 5,
   Saturday = 6,
   Sunday = 7
};

enum class LayoutDirection
{
   LeftToRight,
   RightToLeft,
   LayoutDirectionAuto
};

enum class MsgType
{
   DebugMsg,
   WarningMsg,
   CriticalMsg,
   FatalMsg,
   InfoMsg,
   SystemMsg = CriticalMsg
};

enum class  AppAttribute
{
   AA_ImmediateWidgetCreation = 0,
   AA_MSWindowsUseDirect3DByDefault = 1, // Win only
   AA_DontShowIconsInMenus = 2,
   AA_NativeWindows = 3,
   AA_DontCreateNativeWidgetSiblings = 4,
   AA_PluginApplication = 5,
   AA_DontUseNativeMenuBar = 6,
   AA_MacDontSwapCtrlAndMeta = 7,
   AA_Use96Dpi = 8,
   AA_X11InitThreads = 9,
   AA_SynthesizeTouchForUnhandledMouseEvents = 10,
   AA_SynthesizeMouseForUnhandledTouchEvents = 11,
   AA_UseHighDpiPixmaps = 12,
   AA_ForceRasterWidgets = 13,
   AA_UseDesktopOpenGL = 14,
   AA_UseOpenGLES = 15,
   AA_UseSoftwareOpenGL = 16,
   AA_ShareOpenGLContexts = 17,
   AA_SetPalette = 18,
   AA_EnableHighDpiScaling = 19,
   AA_DisableHighDpiScaling = 20,
   AA_DontUseNativeDialogs = 21,
   AA_SynthesizeMouseForUnhandledTabletEvents = 22,
   AA_CompressHighFrequencyEvents = 23,
   AA_DontCheckOpenGLContextThreadAffinity = 24,
   AA_DisableShaderDiskCache = 24,
   AA_DontShowShortcutsInContextMenus = 26,
   AA_CompressTabletEvents = 27,
   // Add new attributes before this line
   AA_AttributeCount
};

} // pdk

#endif // PDK_GLOBAL_ENUM_DEFS_H
