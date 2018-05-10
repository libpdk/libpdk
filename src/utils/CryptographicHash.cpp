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
// Created by softboy on 2018/03/05.

#include "pdk/utils/CryptographicHash.h"
#include "pdk/base/io/IoDevice.h"
#include "sha1/sha1.cpp"

//#if !defined(PDK_CRYPTOGRAPHICHASH_ONLY_SHA1)
//#  error "Are you sure you need the other hashing algorithms besides SHA-1?"
//#endif

#ifndef PDK_CRYPTOGRAPHICHASH_ONLY_SHA1
#include "md5/md5.h"
#include "md5/md5.cpp"
#include "md4/md4.h"
#include "md4/md4.cpp"

typedef unsigned char BitSequence;
typedef unsigned long long DataLength;

#include "sha3/KeccakSponge.c"
typedef spongeState hashState;

#include "sha3/KeccakNISTInterface.c"

/*
  This lets us choose between SHA3 implementations at build time.
 */
typedef spongeState SHA3Context;
typedef HashReturn (SHA3Init)(hashState *state, int hashbitlen);
typedef HashReturn (SHA3Update)(hashState *state, const BitSequence *data, DataLength databitlen);
typedef HashReturn (SHA3Final)(hashState *state, BitSequence *hashval);

#if PDK_PROCESSOR_WORDSIZE == 8 // 64 bit version

#include "sha3/KeccakF-1600-opt64.c"

static SHA3Init * const sha3Init = Init;
static SHA3Update * const sha3Update = Update;
static SHA3Final * const sha3Final = Final;

#else // 32 bit optimised fallback

#include "sha3/KeccakF-1600-opt32.c"

static SHA3Init * const sha3Init = Init;
static SHA3Update * const sha3Update = Update;
static SHA3Final * const sha3Final = Final;

#endif

/*
    These #defines replace the typedefs needed by the RFC6234 code. Normally
    the typedefs would come from from stdint.h, but since this header is not
    available on all platforms (MSVC 2008, for example), we #define them to the
    PDK equivalents.
*/

#ifdef uint64_t
#undef uint64_t
#endif

#define uint64_t pdk::puint64

#ifdef uint32_t
#undef uint32_t
#endif

#define uint32_t pdk::puint32

#ifdef uint8_t
#undef uint8_t
#endif

#define uint8_t pdk::puint8

#ifdef int_least16_t
#undef int_least16_t
#endif

#define int_least16_t pdk::pint16

// Header from rfc6234 with 1 modification:
// sha1.h - commented out '#include <stdint.h>' on line 74
#include "rfc6234/sha.h"

/*
    These 2 functions replace macros of the same name in sha224-256.c and
    sha384-512.c. Originally, these macros relied on a global static 'addTemp'
    variable. We do not want this for 2 reasons:
    
    1. since we are including the sources directly, the declaration of the 2 conflict
    
    2. static variables are not thread-safe, we do not want multiple threads
    computing a hash to corrupt one another
*/
extern "C" {
int SHA224_256AddLength(SHA256Context *context, unsigned int length);
int SHA384_512AddLength(SHA512Context *context, unsigned int length);
}

// Sources from rfc6234, with 4 modifications:
// sha224-256.c - commented out 'static uint32_t addTemp;' on line 68
// sha224-256.c - appended 'M' to the SHA224_256AddLength macro on line 70
#include "rfc6234/sha224-256.c"
// sha384-512.c - commented out 'static uint64_t addTemp;' on line 302
// sha384-512.c - appended 'M' to the SHA224_256AddLength macro on line 304
#include "rfc6234/sha384-512.c"

#undef uint64_t
#undef uint32_t
#undef uint68_t
#undef int_least16_t

inline int SHA224_256AddLength(SHA256Context *context, unsigned int length)
{
   pdk::puint32 addTemp;
   return SHA224_256AddLengthM(context, length);
}
inline int SHA384_512AddLength(SHA512Context *context, unsigned int length)
{
   pdk::puint64 addTemp;
   return SHA384_512AddLengthM(context, length);
}

namespace pdk {
namespace utils {

using pdk::internal::Sha1State;
using pdk::internal::MD5Context;
using pdk::internal::md4_context;
using pdk::io::IoDevice;

namespace internal {

class CryptographicHashPrivate
{
public:
   CryptographicHash::Algorithm m_method;
   union {
      Sha1State m_sha1Context;
#ifndef PDK_CRYPTOGRAPHICHASH_ONLY_SHA1
      MD5Context m_md5Context;
      md4_context m_md4Context;
      SHA224Context m_sha224Context;
      SHA256Context m_sha256Context;
      SHA384Context m_sha384Context;
      SHA512Context m_sha512Context;
      SHA3Context m_sha3Context;
#endif
   };
#ifndef PDK_CRYPTOGRAPHICHASH_ONLY_SHA1
   enum class Sha3Variant
   {
      Sha3,
      Keccak
   };
   void sha3Finish(int bitCount, Sha3Variant sha3Variant);
#endif
   ByteArray m_result;
};

#ifndef PDK_CRYPTOGRAPHICHASH_ONLY_SHA1
void CryptographicHashPrivate::sha3Finish(int bitCount, Sha3Variant sha3Variant)
{
   /*
        FIPS 202 §6.1 defines SHA-3 in terms of calculating the Keccak function
        over the original message with the two-bit suffix "01" appended to it.
        This variable stores that suffix (and it's fed into the calculations
        when the hash is returned to users).
        
        Only 2 bits of this variable are actually used (see the call to sha3Update
        below). The Keccak implementation we're using will actually use the
        *leftmost* 2 bits, and interpret them right-to-left. In other words, the
        bits must appear in order of *increasing* significance; and as the two most
        significant bits of the byte -- the rightmost 6 are ignored. (Yes, this
        seems self-contradictory, but it's the way it is...)
        
        Overall, this means:
        * the leftmost two bits must be "10" (not "01"!);
        * we don't care what the other six bits are set to (they can be set to
        any value), but we arbitrarily set them to 0;
        
        and for an unsigned char this gives us 0b10'00'00'00, or 0x80.
    */
   static const unsigned char sha3FinalSuffix = 0x80;
   m_result.resize(bitCount / 8);
   SHA3Context copy = m_sha3Context;
   switch (sha3Variant) {
   case Sha3Variant::Sha3:
      sha3Update(&copy, reinterpret_cast<const BitSequence *>(&sha3FinalSuffix), 2);
      break;
   case Sha3Variant::Keccak:
      break;
   }   
   sha3Final(&copy, reinterpret_cast<BitSequence *>(m_result.getRawData()));
}
#endif

} // internal

CryptographicHash::CryptographicHash(Algorithm method)
   : m_implPtr(new CryptographicHashPrivate)
{
   m_implPtr->m_method = method;
   reset();
}

CryptographicHash::~CryptographicHash()
{
   delete m_implPtr;
}

void CryptographicHash::reset()
{
   switch (m_implPtr->m_method) {
   case Sha1:
      sha1InitState(&m_implPtr->m_sha1Context);
      break;
#ifdef PDK_CRYPTOGRAPHICHASH_ONLY_SHA1
   default:
      PDK_ASSERT_X(false, "CryptographicHash", "Method not compiled in");
      PDK_UNREACHABLE();
      break;
#else
   case Md4:
      md4_init(&m_implPtr->m_md4Context);
      break;
   case Md5:
      MD5Init(&m_implPtr->m_md5Context);
      break;
   case Sha224:
      SHA224Reset(&m_implPtr->m_sha224Context);
      break;
   case Sha256:
      SHA256Reset(&m_implPtr->m_sha256Context);
      break;
   case Sha384:
      SHA384Reset(&m_implPtr->m_sha384Context);
      break;
   case Sha512:
      SHA512Reset(&m_implPtr->m_sha512Context);
      break;
   case RealSha3_224:
   case Keccak_224:
      sha3Init(&m_implPtr->m_sha3Context, 224);
      break;
   case RealSha3_256:
   case Keccak_256:
      sha3Init(&m_implPtr->m_sha3Context, 256);
      break;
   case RealSha3_384:
   case Keccak_384:
      sha3Init(&m_implPtr->m_sha3Context, 384);
      break;
   case RealSha3_512:
   case Keccak_512:
      sha3Init(&m_implPtr->m_sha3Context, 512);
      break;
#endif
   }
   m_implPtr->m_result.clear();
}

void CryptographicHash::addData(const char *data, int length)
{
   switch (m_implPtr->m_method) {
   case Sha1:
      sha1Update(&m_implPtr->m_sha1Context, (const unsigned char *)data, length);
      break;
#ifdef PDK_CRYPTOGRAPHICHASH_ONLY_SHA1
   default:
      PDK_ASSERT_X(false, "CryptographicHash", "Method not compiled in");
      PDK_UNREACHABLE();
      break;
#else
   case Md4:
      md4_update(&m_implPtr->m_md4Context, (const unsigned char *)data, length);
      break;
   case Md5:
      MD5Update(&m_implPtr->m_md5Context, (const unsigned char *)data, length);
      break;
   case Sha224:
      SHA224Input(&m_implPtr->m_sha224Context, reinterpret_cast<const unsigned char *>(data), length);
      break;
   case Sha256:
      SHA256Input(&m_implPtr->m_sha256Context, reinterpret_cast<const unsigned char *>(data), length);
      break;
   case Sha384:
      SHA384Input(&m_implPtr->m_sha384Context, reinterpret_cast<const unsigned char *>(data), length);
      break;
   case Sha512:
      SHA512Input(&m_implPtr->m_sha512Context, reinterpret_cast<const unsigned char *>(data), length);
      break;
   case RealSha3_224:
   case Keccak_224:
      sha3Update(&m_implPtr->m_sha3Context, reinterpret_cast<const BitSequence *>(data), length*8);
      break;
   case RealSha3_256:
   case Keccak_256:
      sha3Update(&m_implPtr->m_sha3Context, reinterpret_cast<const BitSequence *>(data), length*8);
      break;
   case RealSha3_384:
   case Keccak_384:
      sha3Update(&m_implPtr->m_sha3Context, reinterpret_cast<const BitSequence *>(data), length*8);
      break;
   case RealSha3_512:
   case Keccak_512:
      sha3Update(&m_implPtr->m_sha3Context, reinterpret_cast<const BitSequence *>(data), length*8);
      break;
#endif
   }
   m_implPtr->m_result.clear();
}

void CryptographicHash::addData(const ByteArray &data)
{
   addData(data.getConstRawData(), data.length());
}

bool CryptographicHash::addData(IoDevice *device)
{
   if (!device->isReadable()) {
      return false;
   }
   if (!device->isOpen()) {
      return false;
   }
   
   char buffer[1024];
   int length;
   
   while ((length = device->read(buffer,sizeof(buffer))) > 0) {
      addData(buffer,length);
   }
   
   
   return device->atEnd();
}

ByteArray CryptographicHash::result() const
{
   if (!m_implPtr->m_result.isEmpty())
      return m_implPtr->m_result;
   
   switch (m_implPtr->m_method) {
   case Sha1: {
      Sha1State copy = m_implPtr->m_sha1Context;
      m_implPtr->m_result.resize(20);
      sha1FinalizeState(&copy);
      sha1ToHash(&copy, (unsigned char *)m_implPtr->m_result.getRawData());
      break;
   }
#ifdef PDK_CRYPTOGRAPHICHASH_ONLY_SHA1
   default:
      PDK_ASSERT_X(false, "CryptographicHash", "Method not compiled in");
      PDK_UNREACHABLE();
      break;
#else
   case Md4: {
      md4_context copy = m_implPtr->m_md4Context;
      m_implPtr->m_result.resize(MD4_RESULTLEN);
      md4_final(&copy, (unsigned char *)m_implPtr->m_result.getRawData());
      break;
   }
   case Md5: {
      MD5Context copy = m_implPtr->m_md5Context;
      m_implPtr->m_result.resize(16);
      MD5Final(&copy, (unsigned char *)m_implPtr->m_result.getRawData());
      break;
   }
   case Sha224: {
      SHA224Context copy = m_implPtr->m_sha224Context;
      m_implPtr->m_result.resize(SHA224HashSize);
      SHA224Result(&copy, reinterpret_cast<unsigned char *>(m_implPtr->m_result.getRawData()));
      break;
   }
   case Sha256:{
      SHA256Context copy = m_implPtr->m_sha256Context;
      m_implPtr->m_result.resize(SHA256HashSize);
      SHA256Result(&copy, reinterpret_cast<unsigned char *>(m_implPtr->m_result.getRawData()));
      break;
   }
   case Sha384:{
      SHA384Context copy = m_implPtr->m_sha384Context;
      m_implPtr->m_result.resize(SHA384HashSize);
      SHA384Result(&copy, reinterpret_cast<unsigned char *>(m_implPtr->m_result.getRawData()));
      break;
   }
   case Sha512:{
      SHA512Context copy = m_implPtr->m_sha512Context;
      m_implPtr->m_result.resize(SHA512HashSize);
      SHA512Result(&copy, reinterpret_cast<unsigned char *>(m_implPtr->m_result.getRawData()));
      break;
   }
   case RealSha3_224: {
      m_implPtr->sha3Finish(224, CryptographicHashPrivate::Sha3Variant::Sha3);
      break;
   }
   case RealSha3_256: {
      m_implPtr->sha3Finish(256, CryptographicHashPrivate::Sha3Variant::Sha3);
      break;
   }
   case RealSha3_384: {
      m_implPtr->sha3Finish(384, CryptographicHashPrivate::Sha3Variant::Sha3);
      break;
   }
   case RealSha3_512: {
      m_implPtr->sha3Finish(512, CryptographicHashPrivate::Sha3Variant::Sha3);
      break;
   }
   case Keccak_224: {
      m_implPtr->sha3Finish(224, CryptographicHashPrivate::Sha3Variant::Keccak);
      break;
   }
   case Keccak_256: {
      m_implPtr->sha3Finish(256, CryptographicHashPrivate::Sha3Variant::Keccak);
      break;
   }
   case Keccak_384: {
      m_implPtr->sha3Finish(384, CryptographicHashPrivate::Sha3Variant::Keccak);
      break;
   }
   case Keccak_512: {
      m_implPtr->sha3Finish(512, CryptographicHashPrivate::Sha3Variant::Keccak);
      break;
   }
#endif
   }
   return m_implPtr->m_result;
}

ByteArray CryptographicHash::hash(const ByteArray &data, Algorithm method)
{
   CryptographicHash hash(method);
   hash.addData(data);
   return hash.result();
}

} // utils
} // pdk

#endif // PDK_CRYPTOGRAPHICHASH_ONLY_SHA1
