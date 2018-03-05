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

#ifndef PDK_UTILS_CRYPTO_GRAPHIC_HASH_H
#define PDK_UTILS_CRYPTO_GRAPHIC_HASH_H

#include "pdk/base/ds/ByteArray.h"

namespace pdk {

// forward declare class with namespace
namespace io {
class IoDevice;
} // io

// forward declare class with namespace
namespace ds {
class ByteArray;
} // ds

namespace utils {

// forward declare class with namespace
namespace internal {
class CryptographicHashPrivate;
} // internal

using pdk::io::IoDevice;
using pdk::ds::ByteArray;
using internal::CryptographicHashPrivate;

class PDK_CORE_EXPORT CryptographicHash
{
public:
   enum Algorithm
   {
#ifndef PDK_CRYPTOGRAPHICHASH_ONLY_SHA1
      Md4,
      Md5,
#endif
      Sha1 = 2,
#ifndef PDK_CRYPTOGRAPHICHASH_ONLY_SHA1
      Sha224,
      Sha256,
      Sha384,
      Sha512,
      
      Keccak_224 = 7,
      Keccak_256,
      Keccak_384,
      Keccak_512,
      RealSha3_224 = 11,
      RealSha3_256,
      RealSha3_384,
      RealSha3_512,
#  ifndef PDK_SHA3_KECCAK_COMPAT
      Sha3_224 = RealSha3_224,
      Sha3_256 = RealSha3_256,
      Sha3_384 = RealSha3_384,
      Sha3_512 = RealSha3_512
#  else
      Sha3_224 = Keccak_224,
      Sha3_256 = Keccak_256,
      Sha3_384 = Keccak_384,
      Sha3_512 = Keccak_512
#  endif
#endif
   };
   explicit CryptographicHash(Algorithm method);
   ~CryptographicHash();
   
   void reset();
   
   void addData(const char *data, int length);
   void addData(const ByteArray &data);
   bool addData(IoDevice *device);
   
   ByteArray result() const;
   
   static ByteArray hash(const ByteArray &data, Algorithm method);
private:
   PDK_DISABLE_COPY(CryptographicHash);
   CryptographicHashPrivate *m_implPtr;
};

} // utils
} // pdk

#endif // PDK_UTILS_CRYPTO_GRAPHIC_HASH_H
