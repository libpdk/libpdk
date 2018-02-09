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
// Created by softboy on 2018/02/02.

#ifndef PDK_GLOBAL_SYSINFO_H
#define PDK_GLOBAL_SYSINFO_H

#include "pdk/global/Global.h"

namespace pdk {

/*
   System information
*/

/*
 * GCC (5-7) has a regression that causes it to emit wrong deprecated warnings:
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77849
 *
 * Try to work around it by defining our own macro.
 */
#ifdef PDK_SYSINFO_DEPRECATED_X
#error "PDK_SYSINFO_DEPRECATED_X already defined"
#endif

#ifdef Q_CC_GNU
#define PDK_SYSINFO_DEPRECATED_X(x)
#else
#define PDK_SYSINFO_DEPRECATED_X(x) PDK_DEPRECATED_X(x)
#endif

// forward declare class with namespace
namespace lang {
class String;
} // lang

using pdk::lang::String;

class PDK_CORE_EXPORT SysInfo
{
public:
   enum Sizes {
      WordSize = (sizeof(void *) << 3)
   };
   
#if defined(PDK_BYTE_ORDER)
   enum Endian {
      BigEndian,
      LittleEndian
#  if PDK_BYTE_ORDER == PDK_BIG_ENDIAN
      ,ByteOrder = BigEndian
#  elif PDK_BYTE_ORDER == PDK_LITTLE_ENDIAN
      ,ByteOrder = LittleEndian
#  else
#     error "Undefined byte order"
#  endif
   };
#endif
   static String getMachineHostName();
};
   
} // pdk

#endif // PDK_GLOBAL_SYSINFO_H
