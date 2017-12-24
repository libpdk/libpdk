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
// Created by softboy on 2017/12/15.

#ifndef PDK_KERNEL_STRING_UTILS_H
#define PDK_KERNEL_STRING_UTILS_H

#include <string>
#include "pdk/global/Global.h"

namespace pdk
{

// forward declare class with namespace
namespace ds {
class ByteArray;
}

extern const uchar latin1Lowercased[256];
extern const uchar latin1Uppercased[256];

PDK_CORE_EXPORT char *strdup(const char *str);

inline uint strlen(const char *str)
{
   return str ? static_cast<uint>(std::strlen(str)) : 0;
}

inline uint strnlen(const char *str, uint maxLen)
{
   uint length = 0;
   if (str) {
      while (length < maxLen && *str++) {
         ++length;
      }
   }
   return length;
}

PDK_CORE_EXPORT char *strcopy(char *dest, const char *src);
PDK_CORE_EXPORT char *strncopy(char *dest, const char *src, uint length);

PDK_CORE_EXPORT int strcmp(const char *lhs, const char *rhs);
PDK_CORE_EXPORT int strcmp(const ds::ByteArray &lhs, const ds::ByteArray &rhs);
PDK_CORE_EXPORT int strcmp(const ds::ByteArray &lhs, const char *rhs);
PDK_CORE_EXPORT int strcmp(const char *lhs, const ds::ByteArray &rhs);

inline int strncmp(const char *lhs, const char *rhs, uint length)
{
   return (lhs && rhs) ? std::strncmp(lhs, rhs, length)
                       : (lhs ? 1 : (rhs ? -1 : 0));
}

PDK_CORE_EXPORT int stricmp(const char *lhs, const char *rhs);
PDK_CORE_EXPORT int strnicmp(const char *lhs, const char *rhs, uint length);

PDK_CORE_EXPORT int vsnprintf(char *str, size_t size, const char *format, va_list ap);
PDK_CORE_EXPORT int snprintf(char *str, size_t size, const char *format, ...);

PDK_CORE_EXPORT pdk::puint16 checksum(const char *str, uint length);

constexpr const uint ASCII_SPACE_MASK =  (1 << (' ' - 1)) |
      (1 << ('\t' - 1)) |   // 9: HT - horizontal tab
      (1 << ('\n' - 1)) |   // 10: LF - line feed
      (1 << ('\v' - 1)) |   // 11: VT - vertical tab
      (1 << ('\f' - 1)) |   // 12: FF - form feed
      (1 << ('\r' - 1));  // 13: CR - carriage return

constexpr inline bool ascii_isspace(uchar c)
{
   return c >= 1U && c <= 32U && ASCII_SPACE_MASK >> static_cast<uint>(c - 1) & 1U;
}


PDK_STATIC_ASSERT(ascii_isspace(' '));
PDK_STATIC_ASSERT(ascii_isspace('\t'));
PDK_STATIC_ASSERT(ascii_isspace('\n'));
PDK_STATIC_ASSERT(ascii_isspace('\v'));
PDK_STATIC_ASSERT(ascii_isspace('\f'));
PDK_STATIC_ASSERT(ascii_isspace('\r'));
PDK_STATIC_ASSERT(!ascii_isspace('\0'));
PDK_STATIC_ASSERT(!ascii_isspace('\a'));
PDK_STATIC_ASSERT(!ascii_isspace('a'));
PDK_STATIC_ASSERT(!ascii_isspace('\177'));
PDK_STATIC_ASSERT(!ascii_isspace(uchar('\200')));
PDK_STATIC_ASSERT(!ascii_isspace(uchar('\xA0')));
PDK_STATIC_ASSERT(!ascii_isspace(uchar('\377')));

constexpr inline char to_hex_upper(uint value) noexcept
{
   return "0123456789ABCDEF"[value & 0xF];
}

constexpr inline char to_hex_lower(uint value) noexcept
{
   return "0123456789abcdef"[value & 0xF];
}

constexpr inline int from_hex(uint c) noexcept
{
   return ((c >= '0') && (c <= '9')) 
         ? static_cast<int>(c - '0') 
         : ((c >= 'A') && (c <= 'F'))
           ? static_cast<int>(c - 'A' + 10)
           : ((c >= 'a') && (c <= 'f'))
             ? static_cast<int>(c - 'a' + 10)
             : -1;
}

constexpr inline char to_oct(uint value) noexcept
{
   return '0' + static_cast<char>(value & 0x7);
}

constexpr inline int from_oct(uint c) noexcept
{
   return ((c >= '0') && (c <= '7')) ? static_cast<int>(c - '0') : -1;
}

namespace internal {

void utf16_from_latin1(char16_t *dest, const char *str, size_t size) noexcept;

} // internal

} // pdk

#endif // PDK_KERNEL_STRING_UTILS_H
