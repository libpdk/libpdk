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
// Created by softboy on 2018/02/03.

#include "pdk/utils/internal/LocaleToolsPrivate.h"
#include "pdk/utils/internal/LocalePrivate.h"
#include "pdk/utils/internal/DoubleScanPrintPrivate.h"
#include "pdk/global/internal/NumericPrivate.h"
#include "pdk/base/lang/String.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <cmath>
#include <cstdlib>
#include <time.h>

#if defined(PDK_OS_LINUX) && !defined(__UCLIBC__)
#    include <fenv.h>
#endif

// Sizes as defined by the ISO C99 standard - fallback
#ifndef LLONG_MAX
#   define LLONG_MAX PDK_INT64_C(0x7fffffffffffffff)
#endif
#ifndef LLONG_MIN
#   define LLONG_MIN (-LLONG_MAX - PDK_INT64_C(1))
#endif
#ifndef ULLONG_MAX
#   define ULLONG_MAX PDK_UINT64_C(0xffffffffffffffff)
#endif

namespace pdk {
namespace utils {
namespace internal {

PDK_CLOCALE_HOLDER

using pdk::lang::Latin1String;

void double_to_ascii(double d, LocaleData::DoubleForm form, int precision, char *buf, int bufSize,
                     bool &sign, int &length, int &decpt)
{
   if (bufSize == 0) {
      decpt = 0;
      sign = d < 0;
      length = 0;
      return;
   }
   
   // Detect special numbers (nan, +/-inf)
   // We cannot use the high-level API of libdouble-conversion as we need to apply locale-specific
   // formatting, such as decimal points, thousands-separators, etc. Because of this, we have to
   // check for infinity and NaN before calling DoubleToAscii.
   if (std::isinf(d)) {
      sign = d < 0;
      if (bufSize >= 3) {
         buf[0] = 'i';
         buf[1] = 'n';
         buf[2] = 'f';
         length = 3;
      } else {
         length = 0;
      }
      return;
   } else if (std::isnan(d)) {
      if (bufSize >= 3) {
         buf[0] = 'n';
         buf[1] = 'a';
         buf[2] = 'n';
         length = 3;
      } else {
         length = 0;
      }
      return;
   }
   
   if (form == LocaleData::DoubleForm::DFSignificantDigits && precision == 0) {
      precision = 1; // 0 significant digits is silently converted to 1
   }
   
   // Cut the precision at 999, to fit it into the format string. We can't get more than 17
   // significant digits, so anything after that is mostly noise. You do get closer to the "middle"
   // of the range covered by the given double with more digits, so to a degree it does make sense
   // to honor higher precisions. We define that at more than 999 digits that is not the case.
   if (precision > 999) {
      precision = 999;
   } else if (precision == Locale::FloatingPointShortest) {
      precision = LocaleData::DoubleMaxSignificant; // "shortest" mode not supported by snprintf
   }
   if (is_zero(d)) {
      // Negative zero is expected as simple "0", not "-0". We cannot do d < 0, though.
      sign = false;
      buf[0] = '0';
      length = 1;
      decpt = 1;
      return;
   } else if (d < 0) {
      sign = true;
      d = -d;
   } else {
      sign = false;
   }
   const int formatLength = 7; // '%', '.', 3 digits precision, 'f', '\0'
   char format[formatLength];
   format[formatLength - 1] = '\0';
   format[0] = '%';
   format[1] = '.';
   format[2] = char((precision / 100) % 10) + '0';
   format[3] = char((precision / 10) % 10)  + '0';
   format[4] = char(precision % 10)  + '0';
   int extraChars;
   switch (form) {
   case LocaleData::DoubleForm::DFDecimal:
      format[formatLength - 2] = 'f';
      // <anything> '.' <precision> '\0' - optimize for numbers smaller than 512k
      extraChars = (d > (1 << 19) ? LocaleData::DoubleMaxDigitsBeforeDecimal : 6) + 2;
      break;
   case LocaleData::DoubleForm::DFExponent:
      format[formatLength - 2] = 'e';
      // '.', 'e', '-', <exponent> '\0'
      extraChars = 7;
      break;
   case LocaleData::DoubleForm::DFSignificantDigits:
      format[formatLength - 2] = 'g';
      // either the same as in the 'e' case, or '.' and '\0'
      // precision covers part before '.'
      extraChars = 7;
      break;
   }
   
   VarLengthArray<char> target(precision + extraChars);
   length = pdk_double_snprintf(target.getRawData(), target.size(), PDK_CLOCALE, format, d);
   int firstSignificant = 0;
   int decptInTarget = length;
   
   // Find the first significant digit (not 0), and note any '.' we encounter.
   // There is no '-' at the front of target because we made sure d > 0 above.
   while (firstSignificant < length) {
      if (target[firstSignificant] == '.') {
         decptInTarget = firstSignificant;
      } else if (target[firstSignificant] != '0') {
         break;
      }
      ++firstSignificant;
   }
   
   // If no '.' found so far, search the rest of the target buffer for it.
   if (decptInTarget == length) {
      decptInTarget = std::find(target.getRawData() + firstSignificant, target.getRawData() + length, '.') -
            target.getRawData();
   }
   int eSign = length;
   if (form != LocaleData::DoubleForm::DFDecimal) {
      // In 'e' or 'g' form, look for the 'e'.
      eSign = std::find(target.getRawData() + firstSignificant, target.getRawData() + length, 'e') -
            target.getRawData();
      
      if (eSign < length) {
         // If 'e' is found, the final decimal point is determined by the number after 'e'.
         // Mind that the final decimal point, decpt, is the offset of the decimal point from the
         // start of the resulting string in buf. It may be negative or larger than bufSize, in
         // which case the missing digits are zeroes. In the 'e' case decptInTarget is always 1,
         // as variants of snprintf always generate numbers with one digit before the '.' then.
         // This is why the final decimal point is offset by 1, relative to the number after 'e'.
         bool ok;
         const char *endptr;
         decpt = pdk_strtoll(target.getRawData() + eSign + 1, &endptr, 10, &ok) + 1;
         PDK_ASSERT(ok);
         PDK_ASSERT(endptr - target.getRawData() <= length);
      } else {
         // No 'e' found, so it's the 'f' form. Variants of snprintf generate numbers with
         // potentially multiple digits before the '.', but without decimal exponent then. So we
         // get the final decimal point from the position of the '.'. The '.' itself takes up one
         // character. We adjust by 1 below if that gets in the way.
         decpt = decptInTarget - firstSignificant;
      }
   } else {
      // In 'f' form, there can not be an 'e', so it's enough to look for the '.'
      // (and possibly adjust by 1 below)
      decpt = decptInTarget - firstSignificant;
   }
   
   // Move the actual digits from the snprintf target to the actual buffer.
   if (decptInTarget > firstSignificant) {
      // First move the digits before the '.', if any
      int lengthBeforeDecpt = decptInTarget - firstSignificant;
      std::memcpy(buf, target.getRawData() + firstSignificant, std::min(lengthBeforeDecpt, bufSize));
      if (eSign > decptInTarget && lengthBeforeDecpt < bufSize) {
         // Then move any remaining digits, until 'e'
         std::memcpy(buf + lengthBeforeDecpt, target.getRawData() + decptInTarget + 1,
                     std::min(eSign - decptInTarget - 1, bufSize - lengthBeforeDecpt));
         // The final length of the output is the distance between the first significant digit
         // and 'e' minus 1, for the '.', except if the buffer is smaller.
         length = std::min(eSign - firstSignificant - 1, bufSize);
      } else {
         // 'e' was before the decpt or things didn't fit. Don't subtract the '.' from the length.
         length = std::min(eSign - firstSignificant, bufSize);
      }
   } else {
      if (eSign > firstSignificant) {
         // If there are any significant digits at all, they are all after the '.' now.
         // Just copy them straight away.
         std::memcpy(buf, target.getRawData() + firstSignificant, std::min(eSign - firstSignificant, bufSize));
         // The decimal point was before the first significant digit, so we were one off above.
         // Consider 0.1 - buf will be just '1', and decpt should be 0. But
         // "decptInTarget - firstSignificant" will yield -1.
         ++decpt;
         length = std::min(eSign - firstSignificant, bufSize);
      } else {
         // No significant digits means the number is just 0.
         buf[0] = '0';
         length = 1;
         decpt = 1;
      }
   }
   while (length > 1 && buf[length - 1] == '0') {// drop trailing zeroes
      --length;
   }
}

double ascii_to_double(const char *num, int numLen, bool &ok, int &processed,
                       TrailingJunkMode trailingJunkMode)
{
   if (*num == '\0') {
      ok = false;
      processed = 0;
      return 0.0;
   }
   
   ok = true;
   
   // We have to catch NaN before because we need NaN as marker for "garbage" in the
   // libdouble-conversion case and, in contrast to libdouble-conversion or sscanf, we don't allow
   // "-nan" or "+nan"
   if (pdk::strcmp(num, "nan") == 0) {
      processed = 3;
      return pdk::snan();
   } else if ((num[0] == '-' || num[0] == '+') && pdk::strcmp(num + 1, "nan") == 0) {
      processed = 0;
      ok = false;
      return 0.0;
   }
   
   // Infinity values are implementation defined in the sscanf case. In the libdouble-conversion
   // case we need infinity as overflow marker.
   if (pdk::strcmp(num, "+inf") == 0) {
      processed = 4;
      return pdk::inf();
   } else if (pdk::strcmp(num, "inf") == 0) {
      processed = 3;
      return pdk::inf();
   } else if (pdk::strcmp(num, "-inf") == 0) {
      processed = 4;
      return -pdk::inf();
   }
   
   double d = 0.0;
   if (pdk_double_sscanf(num, PDK_CLOCALE, "%lf%n", &d, &processed) < 1) {
      processed = 0;
   }
   if ((trailingJunkMode == TrailingJunkMode::TrailingJunkProhibited && processed != numLen) || std::isnan(d)) {
      // Implementation defined nan symbol or garbage found. We don't accept it.
      processed = 0;
      ok = false;
      return 0.0;
   }
   
   if (!std::isfinite(d)) {
      // Overflow. Check for implementation-defined infinity symbols and reject them.
      // We assume that any infinity symbol has to contain a character that cannot be part of a
      // "normal" number (that is 0-9, ., -, +, e).
      ok = false;
      for (int i = 0; i < processed; ++i) {
         char c = num[i];
         if ((c < '0' || c > '9') && c != '.' && c != '-' && c != '+' && c != 'e') {
            // Garbage found
            processed = 0;
            return 0.0;
         }
      }
      return d;
   }
   // Otherwise we would have gotten NaN or sorted it out above.
   PDK_ASSERT(trailingJunkMode == TrailingJunkMode::TrailingJunkAllowed || processed == numLen);
   
   // Check if underflow has occurred.
   if (is_zero(d)) {
      for (int i = 0; i < processed; ++i) {
         if (num[i] >= '1' && num[i] <= '9') {
            // if a digit before any 'e' is not 0, then a non-zero number was intended.
            ok = false;
            return 0.0;
         } else if (num[i] == 'e' || num[i] == 'E') {
            break;
         }
      }
   }
   return d;
}

unsigned long long
pdk_strtoull(const char * nptr, const char **endptr, int base, bool *ok)
{
   // strtoull accepts negative numbers. We don't.
   // Use a different variable so we pass the original nptr to strtoul
   // (we need that so endptr may be nptr in case of failure)
   const char *begin = nptr;
   while (ascii_isspace(*begin)) {
      ++begin;
   }
   if (*begin == '-') {
      *ok = false;
      return 0;
   }
   
   *ok = true;
   errno = 0;
   char *endptr2 = 0;
   unsigned long long result = std::strtoull(nptr, &endptr2, base);
   if (endptr)
      *endptr = endptr2;
   if ((result == 0 || result == std::numeric_limits<unsigned long long>::max())
       && (errno || endptr2 == nptr)) {
      *ok = false;
      return 0;
   }
   return result;
}

long long
pdk_strtoll(const char * nptr, const char **endptr, int base, bool *ok)
{
   *ok = true;
   errno = 0;
   char *endptr2 = 0;
   long long result = std::strtoll(nptr, &endptr2, base);
   if (endptr) {
      *endptr = endptr2;
   }
   if ((result == 0 || result == std::numeric_limits<long long>::min()
        || result == std::numeric_limits<long long>::max())
       && (errno || nptr == endptr2)) {
      *ok = false;
      return 0;
   }
   return result;
}

String pdk_ulltoa(pdk::pulonglong l, int base, const Character _zero)
{
   ushort buff[65]; // length of MAX_ULLONG in base 2
   ushort *p = buff + 65;
   if (base != 10 || _zero.unicode() == '0') {
      while (l != 0) {
         int c = l % base;
         --p;
         if (c < 10) {
            *p = '0' + c;
         } else {
            *p = c - 10 + 'a';
         }
         l /= base;
      }
   }
   else {
      while (l != 0) {
         int c = l % base;
         *(--p) = _zero.unicode() + c;
         l /= base;
      }
   }
   return String(reinterpret_cast<Character *>(p), 65 - (p - buff));
}

String pdk_lltoa(pdk::plonglong l, int base, const Character zero)
{
   return pdk_ulltoa(l < 0 ? -l : l, base, zero);
}

String &decimal_form(Character zero, Character decimal, Character group,
                     String &digits, int decpt, int precision,
                     PrecisionMode pm,
                     bool always_show_decpt,
                     bool thousands_group)
{
   if (decpt < 0) {
      for (int i = 0; i < -decpt; ++i) {
         digits.prepend(zero);
      }
      decpt = 0;
   }
   else if (decpt > digits.length()) {
      for (int i = digits.length(); i < decpt; ++i) {
         digits.append(zero);
      }
   }
   if (pm == PrecisionMode::PMDecimalDigits) {
      uint decimal_digits = digits.length() - decpt;
      for (int i = decimal_digits; i < precision; ++i)
         digits.append(zero);
   }
   else if (pm == PrecisionMode::PMSignificantDigits) {
      for (int i = digits.length(); i < precision; ++i) {
         digits.append(zero);
      }
   }
   else { // pm == PMChopTrailingZeros
   }
   
   if (always_show_decpt || decpt < digits.length())
      digits.insert(decpt, decimal);
   
   if (thousands_group) {
      for (int i = decpt - 3; i > 0; i -= 3) {
         digits.insert(i, group);
      }
   }
   if (decpt == 0) {
      digits.prepend(zero);
   }
   return digits;
}

String &exponent_form(Character zero, Character decimal, Character exponential,
                      Character group, Character plus, Character minus,
                      String &digits, int decpt, int precision,
                      PrecisionMode pm,
                      bool always_show_decpt,
                      bool leading_zero_in_exponent)
{
   int exp = decpt - 1;
   
   if (pm == PrecisionMode::PMDecimalDigits) {
      for (int i = digits.length(); i < precision + 1; ++i) {
         digits.append(zero);
      }
   }
   else if (pm == PrecisionMode::PMSignificantDigits) {
      for (int i = digits.length(); i < precision; ++i)
         digits.append(zero);
   }
   else { // pm == PMChopTrailingZeros
   }
   
   if (always_show_decpt || digits.length() > 1)
      digits.insert(1, decimal);
   
   digits.append(exponential);
   digits.append(LocaleData::longLongToString(zero, group, plus, minus,
                                              exp, leading_zero_in_exponent ? 2 : 1, 10, -1, 
                                              LocaleData::Flag::AlwaysShowSign));
   
   return digits;
}

double pdk_strtod(const char *s00, const char **se, bool *ok)
{
   const int len = static_cast<int>(strlen(s00));
   PDK_ASSERT(len >= 0);
   return pdk_strntod(s00, len, se, ok);
}

double pdk_strntod(const char *s00, int len, const char **se, bool *ok)
{
   int processed = 0;
   bool nonNullOk = false;
   double d = ascii_to_double(s00, len, nonNullOk, processed, TrailingJunkMode::TrailingJunkAllowed);
   if (se) {
      *se = s00 + processed;
   }
   if (ok) {
      *ok = nonNullOk;
   }
   return d;
}

String pdk_dtoa(double d, int *decpt, int *sign)
{
   bool nonNullSign = false;
   int nonNullDecpt = 0;
   int length = 0;
   
   // Some versions of libdouble-conversion like an extra digit, probably for '\0'
   char result[LocaleData::DoubleMaxSignificant + 1];
   double_to_ascii(d, LocaleData::DoubleForm::DFSignificantDigits, Locale::FloatingPointShortest, result,
                 LocaleData::DoubleMaxSignificant + 1, nonNullSign, length, nonNullDecpt);
   
   if (sign) {
      *sign = nonNullSign ? 1 : 0;
   }
   if (decpt) {
      *decpt = nonNullDecpt;
   }
   return Latin1String(result, length);
}

} // internal
} // utils
} // pdk
