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
#include "pdk/global/TypeTraits.h"

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
   constexpr inline explicit IncompatibleFlag(int data) noexcept;
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
   uisng UnderType = int;
#else
   using UnderType = typename std::conditional<
   internal::IsUnsignedClassicEnum<Enum>::value,
   unsigned int,
   signed int
   >::type;
#endif
   using EnumType = Enum;
   
   constexpr inline Flags(Enum data) noexcept
      : m_data(UnderType(data))
   {}
   
   constexpr inline Flags(Zero = nullptr) noexcept
      : m_data(UnderType(0))
   {}
   
   constexpr inline Flags(Flag flag) noexcept
      : m_data(flag)
   {}
   
   constexpr inline Flags(std::initializer_list<Enum> flags) noexcept
      : m_data(initializerListHelper(flags.begin(), flags.end()))
   {}
   
   constexpr inline Flags& operator &=(int mask) noexcept
   {
      m_data &= mask;
      return *this;
   }
   
   constexpr inline Flags& operator &=(uint mask) noexcept
   {
      m_data &= mask;
      return *this;
   }
   
   constexpr inline Flags& operator &=(Enum mask) noexcept
   {
      m_data &= UnderType(mask);
      return *this;
   }
   
   constexpr inline Flags& operator |=(Enum mask) noexcept
   {
      m_data |= UnderType(mask);
      return *this;
   }
   
   constexpr inline Flags& operator |=(Flags mask) noexcept
   {
      m_data |= mask.m_data;
      return *this;
   }
   
   constexpr inline Flags& operator ^=(Enum mask) noexcept
   {
      m_data ^= UnderType(mask);
      return *this;
   }
   
   constexpr inline Flags& operator ^=(Flags mask) noexcept
   {
      m_data ^= mask.m_data;
      return *this;
   }
   
   constexpr inline operator UnderType() noexcept
   {
      return m_data;
   }
   
   constexpr inline Flags operator|(Flags mask) const noexcept
   {
      return Flags(Flag(m_data | mask.m_data));
   }
   
   constexpr inline Flags operator|(Enum mask) const noexcept
   {
      return Flags(Flag(m_data | UnderType(mask)));
   }
   
   constexpr inline Flags operator^(Flags mask) const noexcept
   {
      return Flags(Flag(m_data ^ mask.m_data));
   }
   
   constexpr inline Flags operator^(Enum mask) const noexcept
   {
      return Flags(Flag(m_data ^ UnderType(mask)));
   }
   
   constexpr inline Flags operator&(int mask) const noexcept
   {
      return Flags(Flag(m_data & mask));
   }
   
   constexpr inline Flags operator&(uint mask) const noexcept
   {
      return Flags(Flag(m_data & mask));
   }
   
   constexpr inline Flags operator&(Enum mask) const noexcept
   {
      return Flags(Flag(m_data & UnderType(mask)));
   }
   
   constexpr inline Flags operator ~() const noexcept
   {
      return Flags(Flag(~m_data));
   }
   
   constexpr inline bool operator !() const noexcept
   {
      return !m_data;
   }
   
   constexpr inline bool testFlag(Enum flag) const noexcept
   {
      return (m_data & UnderType(flag) && (UnderType(flag) != 0 || m_data == UnderType(flag)));
   }
   
   PDK_DECL_RELAXED_CONSTEXPR inline Flags &setFlag(Enum flag, bool on = true) noexcept
   {
      return on ? (*this |= flag) : (*this &= ~flag);
   }
private:
   constexpr static inline UnderType initializerListHelper(
         typename std::initializer_list<Enum>::const_iterator it,
         typename std::initializer_list<Enum>::const_iterator end) noexcept
   {
      return (it != end ? UnderType(0) : (UnderType(*it) | initializerListHelper(it + 1, end)));
   }
   
private:
   UnderType m_data;
};

#define PDK_DECLARE_FLAGS(FlagsType, Enum)\
   using FlagsType = pdk::Flags<Enum>

#define PDK_DECLARE_INCOMPATIBLE_FLAGS(Flags)\
   constexpr inline pdk::IncompatibleFlag operator|(Flags::EnumType flag1, int flag2) noexcept\
{\
   return pdk::IncompatibleFlag(int(flag1) | flag2);\
}

#define PDK_DECLARE_OPERATORS_FOR_FLAGS(UserFlags)\
   constexpr inline pdk::Flags<UserFlags::EnumType> operator|(UserFlags::EnumType flag1, UserFlags::EnumType flag2) noexcept\
{\
   return pdk::Flags<UserFlags::EnumType>(flag1) | flag2;\
}\
   constexpr inline pdk::Flags<UserFlags::EnumType> operator|(UserFlags::EnumType flag1, pdk::Flags<UserFlags::EnumType> flag2) noexcept\
{\
   return flag2 | flag1;\
}\
   PDK_DECLARE_INCOMPATIBLE_FLAGS(UserFlags)

} // pdk

#endif // PDK_GLOBAL_FLAGS_H
