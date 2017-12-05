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
// Created by softboy on 2017/12/05.

#ifndef PDK_KERNEL_MATH_H
#define PDK_KERNEL_MATH_H

#ifndef PDK_MATH_E
#  define PDK_MATH_E (2.7182818284590452354)
#endif

#ifndef PDK_MATH_LOG2E
#  define PDK_MATH_LOG2E (1.4426950408889634074)
#endif

#ifndef PDK_MATH_LOG10E
#  define PDK_MATH_LOG10E (0.43429448190325182765)
#endif

#ifndef PDK_MATH_LN2
#  define PDK_MATH_LN2 (0.69314718055994530942)
#endif

#ifndef PDK_MATH_LN10
#  define PDK_MATH_LN10 (2.30258509299404568402)
#endif

#ifndef PDK_MATH_PI
#  define PDK_MATH_PI (3.14159265358979323846)
#endif

#define PDK_SINE_TABLE_SIZE 256

namespace pdk {
namespace kernel {

extern const double pdk_sine_table[PDK_SINE_TABLE_SIZE];

inline double fast_sin(double x)
{
   int si = static_cast<int>(x * (0.5 * PDK_SINE_TABLE_SIZE / PDK_MATH_PI));
   double d = x - si * (2.0 * PDK_MATH_PI / PDK_SINE_TABLE_SIZE);
   int ci = si + PDK_SINE_TABLE_SIZE / 4;
   si &= PDK_SINE_TABLE_SIZE - 1;
   ci &= PDK_SINE_TABLE_SIZE - 1;
   return pdk_sine_table[si] + (pdk_sine_table[ci] - 0.5 * pdk_sine_table[si] * d) * d;
}

inline double fast_cos(double x)
{
   int ci = static_cast<int>(x * (0.5 * PDK_SINE_TABLE_SIZE / PDK_MATH_PI));
   double d = x - ci * (2.0 * PDK_MATH_PI / PDK_SINE_TABLE_SIZE);
   int si = ci + PDK_SINE_TABLE_SIZE / 4;
   si &= PDK_SINE_TABLE_SIZE - 1;
   ci &= PDK_SINE_TABLE_SIZE - 1;
   return pdk_sine_table[si] - (pdk_sine_table[ci] + 0.5 * pdk_sine_table[si] * d) * d;
}

constexpr inline float degrees_to_radians(float degrees)
{
   return degrees * static_cast<float>(PDK_MATH_PI / 180 );
}

constexpr inline double degrees_to_radians(double degrees)
{
   return degrees * PDK_MATH_PI / 180;
}

constexpr inline float radians_to_degrees(float radians)
{
   return radians * static_cast<float>(180 / PDK_MATH_PI);
}

constexpr inline float radians_to_degrees(double radians)
{
   return radians * 180 / PDK_MATH_PI;
}

} // kernel
} // pdk

#endif // PDK_KERNEL_MATH_H
