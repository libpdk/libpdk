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
// Created by softboy on 2018/02/23.

#include "pdk/base/io/Debug.h"
#include "pdk/base/io/internal/TextStreamPrivate.h"
#include "pdk/kernel/StringUtils.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/lang/Character.h"
#include "pdk/global/Logging.h"

namespace pdk {
namespace io {

using pdk::lang::Latin1Character;
using pdk::lang::Character;
using internal::TextStreamPrivate;

using pdk::to_hex_upper;
using pdk::from_hex;

Debug::~Debug()
{
   if (!--m_stream->m_ref) {
      if (m_stream->m_space && m_stream->m_buffer.endsWith(Latin1Character(' '))) {
         m_stream->m_buffer.chop(1);
      }
      if (m_stream->m_messageOutput) {
         pdk::message_output(m_stream->m_type,
                             m_stream->m_context,
                             m_stream->m_buffer);
      }
      delete m_stream;
   }
}

void Debug::putUcs4(uint ucs4)
{
   maybeQuote('\'');
   if (ucs4 < 0x20) {
      m_stream->m_ts << "\\x" << hex << ucs4 << reset;
   } else if (ucs4 < 0x80) {
      m_stream->m_ts << char(ucs4);
   } else {
      if (ucs4 < 0x10000) {
         m_stream->m_ts << "\\u" << pdk::io::set_field_width(4);
      } else {
         m_stream->m_ts << "\\U" << pdk::io::set_field_width(8);
      }
      m_stream->m_ts << hex << pdk::io::set_pad_char(Latin1Character('0')) << ucs4 << reset;
   }
   maybeQuote('\'');
}

// These two functions return true if the character should be printed by Debug.
// For ByteArray, this is technically identical to US-ASCII isprint();
// for String, we use Character::isPrint, which requires a full UCS-4 decode.
namespace {

inline bool is_printable(uint ucs4)
{
   return Character::isPrintable(ucs4);
}
inline bool is_printable(ushort uc)
{
   return Character::isPrintable(uc);
}

inline bool is_printable(uchar c)
{
   return c >= ' ' && c < 0x7f;
}

template <typename Char>
inline void put_escaped_string(TextStreamPrivate *d, const Char *begin, int length, bool isUnicode = true)
{
   Character quote(Latin1Character('"'));
   d->write(&quote, 1);
   bool lastWasHexEscape = false;
   const Char *end = begin + length;
   for (const Char *p = begin; p != end; ++p) {
      // check if we need to insert "" to break an hex escape sequence
      if (PDK_UNLIKELY(lastWasHexEscape)) {
         if (from_hex(*p) != -1) {
            // yes, insert it
            Character quotes[] = { Latin1Character('"'), Latin1Character('"') };
            d->write(quotes, 2);
         }
         lastWasHexEscape = false;
      }
      
      if (sizeof(Char) == sizeof(Character)) {
         // Surrogate characters are category Cs (Other_Surrogate), so isPrintable = false for them
         int runLength = 0;
         while (p + runLength != end &&
                is_printable(p[runLength]) && p[runLength] != '\\' && p[runLength] != '"')
            ++runLength;
         if (runLength) {
            d->write(reinterpret_cast<const Character *>(p), runLength);
            p += runLength - 1;
            continue;
         }
      } else if (is_printable(*p) && *p != '\\' && *p != '"') {
         Character c = Latin1Character(*p);
         d->write(&c, 1);
         continue;
      }
      
      // print as an escape sequence (maybe, see below for surrogate pairs)
      int buflen = 2;
      ushort buf[sizeof "\\U12345678" - 1];
      buf[0] = '\\';
      
      switch (*p) {
      case '"':
      case '\\':
         buf[1] = *p;
         break;
      case '\b':
         buf[1] = 'b';
         break;
      case '\f':
         buf[1] = 'f';
         break;
      case '\n':
         buf[1] = 'n';
         break;
      case '\r':
         buf[1] = 'r';
         break;
      case '\t':
         buf[1] = 't';
         break;
      default:
         if (!isUnicode) {
            // print as hex escape
            buf[1] = 'x';
            buf[2] = to_hex_upper(uchar(*p) >> 4);
            buf[3] = to_hex_upper(uchar(*p));
            buflen = 4;
            lastWasHexEscape = true;
            break;
         }
         if (Character::isHighSurrogate(*p)) {
            if ((p + 1) != end && Character::isLowSurrogate(p[1])) {
               // properly-paired surrogates
               uint ucs4 = Character::surrogateToUcs4(*p, p[1]);
               if (is_printable(ucs4)) {
                  buf[0] = *p;
                  buf[1] = p[1];
                  buflen = 2;
               } else {
                  buf[1] = 'U';
                  buf[2] = '0'; // toHexUpper(ucs4 >> 32);
                  buf[3] = '0'; // toHexUpper(ucs4 >> 28);
                  buf[4] = to_hex_upper(ucs4 >> 20);
                  buf[5] = to_hex_upper(ucs4 >> 16);
                  buf[6] = to_hex_upper(ucs4 >> 12);
                  buf[7] = to_hex_upper(ucs4 >> 8);
                  buf[8] = to_hex_upper(ucs4 >> 4);
                  buf[9] = to_hex_upper(ucs4);
                  buflen = 10;
               }
               ++p;
               break;
            }
            // improperly-paired surrogates, fall through
         }
         buf[1] = 'u';
         buf[2] = to_hex_upper(ushort(*p) >> 12);
         buf[3] = to_hex_upper(ushort(*p) >> 8);
         buf[4] = to_hex_upper(*p >> 4);
         buf[5] = to_hex_upper(*p);
         buflen = 6;
      }
      d->write(reinterpret_cast<Character *>(buf), buflen);
   }
   d->write(&quote, 1);
}

} // anonymous namespace 

void Debug::putString(const Character *begin, size_t length)
{
   if (m_stream->testFlag(Stream::NoQuotes)) {
      // no quotes, write the string directly too (no pretty-printing)
      // this respects the QTextStream state, though
      m_stream->m_ts.m_implPtr->putString(begin, int(length));
   } else {
      // we'll reset the QTextStream formatting mechanisms, so save the state
      DebugStateSaver saver(*this);
      m_stream->m_ts.m_implPtr->m_params.reset();
      put_escaped_string(m_stream->m_ts.m_implPtr.getData(), reinterpret_cast<const ushort *>(begin), int(length));
   }
}

void Debug::putByteArray(const char *begin, size_t length, Latin1Content content)
{
   if (m_stream->testFlag(Stream::NoQuotes)) {
      // no quotes, write the string directly too (no pretty-printing)
      // this respects the QTextStream state, though
      String string = content == Latin1Content::ContainsLatin1 
            ? String::fromLatin1(begin, int(length)) 
            : String::fromUtf8(begin, int(length));
      m_stream->m_ts.m_implPtr->putString(string);
   } else {
      // we'll reset the QTextStream formatting mechanisms, so save the state
      DebugStateSaver saver(*this);
      m_stream->m_ts.m_implPtr->m_params.reset();
      put_escaped_string(m_stream->m_ts.m_implPtr.getData(), reinterpret_cast<const uchar *>(begin),
                         int(length), content == Latin1Content::ContainsLatin1);
   }
}

Debug &Debug::resetFormat()
{
   m_stream->m_ts.reset();
   m_stream->m_space = true;
   if (m_stream->m_context.m_version > 1) {
      m_stream->m_flags = 0;
   }
   m_stream->setVerbosity(Stream::DefaultVerbosity);
   return *this;
}

namespace internal
{

class DebugStateSaverPrivate
{
public:
   DebugStateSaverPrivate(Debug &dbg)
      : m_dbg(dbg),
        m_spaces(dbg.autoInsertSpaces()),
        m_flags(0),
        m_streamParams(dbg.m_stream->m_ts.m_implPtr->m_params)
   {
      if (m_dbg.m_stream->m_context.m_version > 1) {
         m_flags = m_dbg.m_stream->m_flags;
      }
   }
   void restoreState()
   {
      const bool currentSpaces = m_dbg.autoInsertSpaces();
      if (currentSpaces && !m_spaces) {
         if (m_dbg.m_stream->m_buffer.endsWith(Latin1Character(' '))) {
            m_dbg.m_stream->m_buffer.chop(1);
         }
      }
      m_dbg.setAutoInsertSpaces(m_spaces);
      m_dbg.m_stream->m_ts.m_implPtr->m_params = m_streamParams;
      if (m_dbg.m_stream->m_context.m_version > 1) {
         m_dbg.m_stream->m_flags = m_flags;
      }
      if (!currentSpaces && m_spaces) {
         m_dbg.m_stream->m_ts << ' ';
      }  
   }
   
   Debug &m_dbg;
   // Debug state
   const bool m_spaces;
   int m_flags;
   // TextStream state
   const TextStreamPrivate::Params m_streamParams;
};
} // internal

DebugStateSaver::DebugStateSaver(Debug &dbg)
   : m_implPtr(new DebugStateSaverPrivate(dbg))
{}

DebugStateSaver::~DebugStateSaver()
{
    m_implPtr->restoreState();
}

void pdk_meta_enum_flag_debug_operator(Debug &debug, size_t sizeofT, int value)
{
   pdk_meta_enum_flag_debug_operator<int>(debug, sizeofT, value);
}

} // io
} // pdk
