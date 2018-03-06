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
// Created by softboy on 2018/03/05.

#ifndef PDK_GLOBAL_INTERNAL_ENDIAN_PRIVATE_H
#define PDK_GLOBAL_INTERNAL_ENDIAN_PRIVATE_H

#include "pdk/global/Endian.h"

namespace pdk {
namespace internal {

using pdk::LittleEndianStorageType;
using pdk::BigEndianStorageType;

// Note if using multiple of these bitfields in a union; the underlying storage type must
// match. Since we always use an unsigned storage type, unsigned and signed versions may
// be used together, but different bit-widths may not.
template<class S, int pos, int width>
class SpecialIntegerBitfield
{
protected:
   using T = typename S::StorageType;
   using UT = typename std::make_unsigned<T>::type ;
   
   static constexpr UT mask()
   {
      return ((UT(1) << width) - 1) << pos;
   }
   
public:
   // FIXME: val is public until qtdeclarative is fixed to not access it directly.
   UT m_val;
   
   SpecialIntegerBitfield &operator =(T t)
   {
      UT i = S::fromSpecial(m_val);
      i &= ~mask();
      i |= (UT(t) << pos) & mask();
      m_val  = S::toSpecial(i);
      return *this;
   }
   
   operator T() const
   {
      if (std::is_signed<T>::value) {
         UT i = S::fromSpecial(m_val);
         i <<= (sizeof(T) * 8) - width - pos;
         T t = T(i);
         t >>= (sizeof(T) * 8) - width;
         return t;
      }
      return (S::fromSpecial(m_val) & mask()) >> pos;
   }
   
   bool operator !() const
   {
      return !(m_val & S::toSpecial(mask()));
   }
   
   bool operator ==(SpecialIntegerBitfield<S, pos, width> i) const
   {
      return ((m_val ^ i.m_val) & S::toSpecial(mask())) == 0;
   }
   
   bool operator !=(SpecialIntegerBitfield<S, pos, width> i) const
   {
      return ((m_val ^ i.m_val) & S::toSpecial(mask())) != 0;
   }
   
   SpecialIntegerBitfield &operator +=(T i)
   {
      return (*this = (T(*this) + i));
   }
   
   SpecialIntegerBitfield &operator -=(T i)
   {
      return (*this = (T(*this) - i));
   }
   
   SpecialIntegerBitfield &operator *=(T i)
   {
      return (*this = (T(*this) * i));
   }
   
   SpecialIntegerBitfield &operator /=(T i)
   {
      return (*this = (T(*this) / i));
   }
   
   SpecialIntegerBitfield &operator %=(T i)
   {
      return (*this = (T(*this) % i));
   }
   
   SpecialIntegerBitfield &operator |=(T i)
   {
      return (*this = (T(*this) | i));
   }
   
   SpecialIntegerBitfield &operator &=(T i)
   {
      return (*this = (T(*this) & i));
   }
   
   SpecialIntegerBitfield &operator ^=(T i)
   {
      return (*this = (T(*this) ^ i));
   }
   
   SpecialIntegerBitfield &operator >>=(T i)
   {
      return (*this = (T(*this) >> i));
   }
   
   SpecialIntegerBitfield &operator <<=(T i)
   {
      return (*this = (T(*this) << i));
   }
};

template<typename T, int pos, int width>
using LEIntegerBitfield = SpecialIntegerBitfield<LittleEndianStorageType<T>, pos, width>;

template<typename T, int pos, int width>
using BEIntegerBitfield = SpecialIntegerBitfield<BigEndianStorageType<T>, pos, width>;

template<int pos, int width>
using pint32_le_bitfield = LEIntegerBitfield<int, pos, width>;

template<int pos, int width>
using puint32_le_bitfield = LEIntegerBitfield<uint, pos, width>;

template<int pos, int width>
using pint32_be_bitfield = BEIntegerBitfield<int, pos, width>;

template<int pos, int width>
using puint32_be_bitfield = BEIntegerBitfield<uint, pos, width>;

} // internal
} // pdk

#endif // PDK_GLOBAL_INTERNAL_ENDIAN_PRIVATE_H
