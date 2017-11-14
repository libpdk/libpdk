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

#ifndef PDK_GLOBAL_FLAGS_H
#define PDK_GLOBAL_FLAGS_H

#include <initializer_list>
#include "pdk/global/Global.h"

namespace pdk
{

class Flag
{
private:
   int m_data;
public:
   constexpr inline Flag(int data) noexcept
      : m_data(data)
   {}
   
   constexpr inline operator int() const noexcept
   {
      return m_data;
   }
   
#if !defined(PDK_CC_MSVC)
   // Microsoft Visual Studio has buggy behavior when it comes to
   // unsigned enums: even if the enum is unsigned, the enum tags are
   // always signed
#  if !defined(__LP64__)
   constexpr inline Flag(long data) noexcept
      : m_data(int(data))
   {}
   constexpr inline Flag(ulong data) noexcept
      : m_data(int(long(data)))
   {}
#  endif
   constexpr inline Flag(uint data) noexcept
      : m_data(int(data))
   {}
   
   constexpr inline Flag(short data) noexcept
      : m_data(int(data))
   {}
   
   constexpr inline Flag(ushort data) noexcept
      : m_data(int(uint(data)))
   {}
   
   constexpr inline operator uint() const noexcept
   {
      return uint(m_data);
   }
#endif
};

class IncompatibleFlag
{
private:
   int m_data;
public:
   constexpr inline explicit IncompatibleFlag(data) noexcept;
   constexpr inline operator int() const noexcept
   {
      return m_data;
   }
};

constexpr inline IncompatibleFlag::IncompatibleFlag(int data) noexcept
   : m_data(data)
{}

template <typename Enum>
class Flags
{
   PDK_STATIC_ASSERT_X((sizeof(Enum) <= sizeof(int)), 
                       "pdk::Flags uses an int as storage, so an enum with underlying "
                       "long long will overflow");
   struct Private;
   typedef int (Private::*Zero);
public:
#if defined(PDK_CC_MSVC)
   uisng Int = int;
#else
   
#endif
};

} // pdk

#endif // PDK_GLOBAL_FLAGS_H
