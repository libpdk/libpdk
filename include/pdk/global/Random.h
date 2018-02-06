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
// Created by softboy on 2018/02/07.

#ifndef PDK_GLOBAL_RANDOM_H
#define PDK_GLOBAL_RANDOM_H

#include "pdk/global/Global.h"
#include <algorithm>
#include <random>

#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif

namespace pdk {

class RandomGenerator
{
   // restrict the template parameters to unsigned integers 32 bits wide or larger
   template <typename UInt> 
   using IfValidUInt =
   typename std::enable_if<std::is_unsigned<UInt>::value && sizeof(UInt) >= sizeof(uint), bool>::type;
public:
   RandomGenerator(pdk::puint32 seedValue = 1)
      : RandomGenerator(&seedValue, 1)
   {}
   
   template <pdk::sizetype N> RandomGenerator(const pdk::puint32 (&seedBuffer)[N])
      : RandomGenerator(seedBuffer, seedBuffer + N)
   {}
   
   RandomGenerator(const pdk::puint32 *seedBuffer, pdk::sizetype len)
      : RandomGenerator(seedBuffer, seedBuffer + len)
   {}
   
   PDK_CORE_EXPORT RandomGenerator(std::seed_seq &sseq) noexcept;
   PDK_CORE_EXPORT RandomGenerator(const pdk::puint32 *begin, const pdk::puint32 *end);
   
   // copy constructor & assignment operator (move unnecessary)
   PDK_CORE_EXPORT RandomGenerator(const RandomGenerator &other);
   PDK_CORE_EXPORT RandomGenerator &operator=(const RandomGenerator &other);
   
   friend PDK_CORE_EXPORT bool operator==(const RandomGenerator &rng1, const RandomGenerator &rng2);
   friend bool operator!=(const RandomGenerator &rng1, const RandomGenerator &rng2)
   {
      return !(rng1 == rng2);
   }
   
   pdk::puint32 generate()
   {
      pdk::puint32 ret;
      fillRange(&ret, 1);
      return ret;
   }
   
   pdk::puint64 generate64()
   {
      pdk::puint32 buf[2];
      fillRange(buf);
      return buf[0] | (pdk::puint64(buf[1]) << 32);
   }
   
   double generateDouble()
   {
      // IEEE 754 double precision has:
      //   1 bit      sign
      //  10 bits     exponent
      //  53 bits     mantissa
      // In order for our result to be normalized in the range [0, 1), we
      // need exactly 53 bits of random data. Use generate64() to get enough.
      pdk::puint64 x = generate64();
      pdk::puint64 limit = PDK_UINT64_C(1) << std::numeric_limits<double>::digits;
      x >>= std::numeric_limits<pdk::puint64>::digits - std::numeric_limits<double>::digits;
      return double(x) / double(limit);
   }
   
   double bounded(double highest)
   {
      return generateDouble() * highest;
   }
   
   pdk::puint32 bounded(pdk::puint32 highest)
   {
      pdk::puint64 value = generate();
      value *= highest;
      value /= (max)() + pdk::puint64(1);
      return pdk::puint32(value);
   }
   
   int bounded(int highest)
   {
      return int(bounded(pdk::puint32(highest)));
   }
   
   pdk::puint32 bounded(pdk::puint32 lowest, pdk::puint32 highest)
   {
      return bounded(highest - lowest) + lowest;
   }
   
   int bounded(int lowest, int highest)
   {
      return bounded(highest - lowest) + lowest;
   }
   
   template <typename UInt, IfValidUInt<UInt> = true>
   void fillRange(UInt *buffer, pdk::sizetype count)
   {
      doFillRange(buffer, buffer + count);
   }
   
   template <typename UInt, size_t N, IfValidUInt<UInt> = true>
   void fillRange(UInt (&buffer)[N])
   {
      doFillRange(buffer, buffer + N);
   }
   
   // API like std::seed_seq
   template <typename ForwardIterator>
   void generate(ForwardIterator begin, ForwardIterator end)
   {
      std::generate(begin, end, [this]() { return generate(); });
   }
   
   void generate(pdk::puint32 *begin, pdk::puint32 *end)
   {
      doFillRange(begin, end);
   }
   
   // API like std:: random engines
   using result_type = pdk::puint32;
   result_type operator()()
   {
      return generate();
   }
   
   void seed(pdk::puint32 s = 1)
   {
      *this = { s };
   }
   
   void seed(std::seed_seq &sseq) noexcept
   {
      *this = { sseq };
   }
   
   PDK_CORE_EXPORT void discard(unsigned long long z);
   static PDK_DECL_CONSTEXPR result_type min()
   {
      return std::numeric_limits<result_type>::min();
   }
   
   static PDK_DECL_CONSTEXPR result_type max()
   {
      return std::numeric_limits<result_type>::max();
   }
   
   static inline PDK_DECL_CONST_FUNCTION RandomGenerator *system();
   static inline PDK_DECL_CONST_FUNCTION RandomGenerator *global();
   static inline RandomGenerator securelySeeded();
   
protected:
   enum System {};
   RandomGenerator(System);
   
private:
   PDK_CORE_EXPORT void doFillRange(void *buffer, void *bufferEnd);
   
   friend class RandomGenerator64;
   struct SystemGenerator;
   struct SystemAndGlobalGenerators;
   using RandomEngine = std::mersenne_twister_engine<pdk::puint32,
   32,624,397,31,0x9908b0df,11,0xffffffff,7,0x9d2c5680,15,0xefc60000,18,1812433253>;
   
   union Storage {
      uint m_dummy;
#ifdef PDK_COMPILER_UNRESTRICTED_UNIONS
      RandomEngine m_twister;
      RandomEngine &engine()
      {
         return m_twister;
      }
      const RandomEngine &engine() const
      {
         return m_twister; }
#else
      std::aligned_storage<sizeof(RandomEngine), PDK_ALIGNOF(RandomEngine)>::type m_buffer;
      RandomEngine &engine()
      {
         return reinterpret_cast<RandomEngine &>(m_buffer);
      }
      
      const RandomEngine &engine() const
      {
         return reinterpret_cast<const RandomEngine &>(m_buffer);
      }
#endif
      
      PDK_STATIC_ASSERT_X(std::is_trivially_destructible<RandomEngine>::value,
                        "std::mersenne_twister not trivially destructible as expected");
      constexpr Storage();
   };
   uint m_type;
   Storage m_storage;
};

class RandomGenerator64 : public RandomGenerator
{
   RandomGenerator64(System);
public:
   // unshadow generate() overloads, since we'll override.
   using RandomGenerator::generate;
   pdk::puint64 generate()
   {
      return generate64();
   }
   
   using result_type = pdk::puint64;
   result_type operator()()
   {
      return generate64();
   }

   RandomGenerator64(pdk::puint32 seedValue = 1)
      : RandomGenerator(seedValue)
   {}
   
   template <pdk::sizetype N> RandomGenerator64(const pdk::puint32 (&seedBuffer)[N])
      : RandomGenerator(seedBuffer)
   {}
   
   RandomGenerator64(const pdk::puint32 *seedBuffer, pdk::sizetype len)
      : RandomGenerator(seedBuffer, len)
   {}
   
   RandomGenerator64(std::seed_seq &sseq) noexcept
      : RandomGenerator(sseq)
   {}
   
   RandomGenerator64(const pdk::puint32 *begin, const pdk::puint32 *end)
      : RandomGenerator(begin, end)
   {}
   
   RandomGenerator64(const RandomGenerator &other) : RandomGenerator(other) {}
   
   void discard(unsigned long long z)
   {
      PDK_ASSERT_X(z * 2 > z, "RandomGenerator64::discard",
                 "Overflow. Are you sure you want to skip over 9 pdk::puintillion samples?");
      RandomGenerator::discard(z * 2);
   }
   
   static PDK_DECL_CONSTEXPR result_type min()
   {
      return std::numeric_limits<result_type>::min();
   }
   
   static PDK_DECL_CONSTEXPR result_type max()
   {
      return std::numeric_limits<result_type>::max();
   }
   static PDK_DECL_CONST_FUNCTION PDK_CORE_EXPORT RandomGenerator64 *system();
   static PDK_DECL_CONST_FUNCTION PDK_CORE_EXPORT RandomGenerator64 *global();
   static PDK_CORE_EXPORT RandomGenerator64 securelySeeded();
};

inline RandomGenerator *RandomGenerator::system()
{
   return RandomGenerator64::system();
}

inline RandomGenerator *RandomGenerator::global()
{
   return RandomGenerator64::global();
}

RandomGenerator RandomGenerator::securelySeeded()
{
   return RandomGenerator64::securelySeeded();
}

} // pdk

#endif // PDK_GLOBAL_RANDOM_H
