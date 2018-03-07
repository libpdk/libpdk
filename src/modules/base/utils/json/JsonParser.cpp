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

#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/utils/json/internal/JsonParserPrivate.h"
#include "pdk/base/utils/json/internal/JsonPrivate.h"
#include "pdk/base/text/codecs/internal/UtfCodecPrivate.h"
#include "pdk/base/lang/Character.h"

//#define PARSER_DEBUG
#ifdef PARSER_DEBUG
static int indent = 0;
#define BEGIN warning_stream() << ByteArray(4*indent++, ' ').getConstRawData() << "pos=" << current
#define END --indent
#define DEBUG warning_stream() << ByteArray(4*indent, ' ').getConstRawData()
#else
#define BEGIN if (1) ; else warning_stream()
#define END do {} while (0)
#define DEBUG if (1) ; else warning_stream()
#endif

static const int sg_nestingLimit = 1024;

// error strings for the JSON parser
#define JSONERR_OK          PDK_TRANSLATE_NOOP("JsonParseError", "no error occurred")
#define JSONERR_UNTERM_OBJ  PDK_TRANSLATE_NOOP("JsonParseError", "unterminated object")
#define JSONERR_MISS_NSEP   PDK_TRANSLATE_NOOP("JsonParseError", "missing name separator")
#define JSONERR_UNTERM_AR   PDK_TRANSLATE_NOOP("JsonParseError", "unterminated array")
#define JSONERR_MISS_VSEP   PDK_TRANSLATE_NOOP("JsonParseError", "missing value separator")
#define JSONERR_ILLEGAL_VAL PDK_TRANSLATE_NOOP("JsonParseError", "illegal value")
#define JSONERR_END_OF_NUM  PDK_TRANSLATE_NOOP("JsonParseError", "invalid termination by number")
#define JSONERR_ILLEGAL_NUM PDK_TRANSLATE_NOOP("JsonParseError", "illegal number")
#define JSONERR_STR_ESC_SEQ PDK_TRANSLATE_NOOP("JsonParseError", "invalid escape sequence")
#define JSONERR_STR_UTF8    PDK_TRANSLATE_NOOP("JsonParseError", "invalid UTF8 string")
#define JSONERR_UTERM_STR   PDK_TRANSLATE_NOOP("JsonParseError", "unterminated string")
#define JSONERR_MISS_OBJ    PDK_TRANSLATE_NOOP("JsonParseError", "object is missing after a comma")
#define JSONERR_DEEP_NEST   PDK_TRANSLATE_NOOP("JsonParseError", "too deeply nested document")
#define JSONERR_DOC_LARGE   PDK_TRANSLATE_NOOP("JsonParseError", "too large document")
#define JSONERR_GARBAGEEND  PDK_TRANSLATE_NOOP("JsonParseError", "garbage at the end of the document")

namespace pdk {
namespace utils {
namespace json {

namespace codecsinternal = pdk::text::codecs::internal;
namespace utf8funcs = codecsinternal::Utf8Functions;

using pdk::lang::Character;
using pdk::lang::String;
using pdk::kernel::CoreApplication;

String JsonParseError::getErrorString() const
{
   const char *sz = "";
   switch (m_error) {
   case ParseError::NoError:
      sz = JSONERR_OK;
      break;
   case ParseError::UnterminatedObject:
      sz = JSONERR_UNTERM_OBJ;
      break;
   case ParseError::MissingNameSeparator:
      sz = JSONERR_MISS_NSEP;
      break;
   case ParseError::UnterminatedArray:
      sz = JSONERR_UNTERM_AR;
      break;
   case ParseError::MissingValueSeparator:
      sz = JSONERR_MISS_VSEP;
      break;
   case ParseError::IllegalValue:
      sz = JSONERR_ILLEGAL_VAL;
      break;
   case ParseError::TerminationByNumber:
      sz = JSONERR_END_OF_NUM;
      break;
   case ParseError::IllegalNumber:
      sz = JSONERR_ILLEGAL_NUM;
      break;
   case ParseError::IllegalEscapeSequence:
      sz = JSONERR_STR_ESC_SEQ;
      break;
   case ParseError::IllegalUTF8String:
      sz = JSONERR_STR_UTF8;
      break;
   case ParseError::UnterminatedString:
      sz = JSONERR_UTERM_STR;
      break;
   case ParseError::MissingObject:
      sz = JSONERR_MISS_OBJ;
      break;
   case ParseError::DeepNesting:
      sz = JSONERR_DEEP_NEST;
      break;
   case ParseError::DocumentTooLarge:
      sz = JSONERR_DOC_LARGE;
      break;
   case ParseError::GarbageAtEnd:
      sz = JSONERR_GARBAGEEND;
      break;
   }
   return CoreApplication::translate("JsonParseError", sz);
}

namespace jsonprivate {

Parser::Parser(const char *json, int length)
   : m_head(json),
     m_json(json),
     m_data(nullptr),
     m_dataLength(0),
     m_current(0),
     m_nestingLevel(0),
     m_lastError(JsonParseError::ParseError::NoError)
{
   m_end = json + length;
}

/*
  
begin-array     = ws %x5B ws  ; [ left square bracket

begin-object    = ws %x7B ws  ; { left curly bracket

end-array       = ws %x5D ws  ; ] right square bracket

end-object      = ws %x7D ws  ; } right curly bracket

name-separator  = ws %x3A ws  ; : colon

value-separator = ws %x2C ws  ; , comma

Insignificant whitespace is allowed before or after any of the six
structural characters.

ws = *(
          %x20 /              ; Space
          %x09 /              ; Horizontal tab
          %x0A /              ; Line feed or New line
          %x0D                ; Carriage return
      )
      
*/
enum {
   Space = 0x20,
   Tab = 0x09,
   LineFeed = 0x0a,
   Return = 0x0d,
   BeginArray = 0x5b,
   BeginObject = 0x7b,
   EndArray = 0x5d,
   EndObject = 0x7d,
   NameSeparator = 0x3a,
   ValueSeparator = 0x2c,
   Quote = 0x22
};

void Parser::eatBOM()
{
   // eat UTF-8 byte order mark
   uchar utf8bom[3] = { 0xef, 0xbb, 0xbf };
   if (m_end - m_json > 3 &&
       (uchar)m_json[0] == utf8bom[0] &&
       (uchar)m_json[1] == utf8bom[1] &&
       (uchar)m_json[2] == utf8bom[2])
      m_json += 3;
}

bool Parser::eatSpace()
{
   while (m_json < m_end) {
      if (*m_json > Space) {
         break;
      }
      if (*m_json != Space &&
          *m_json != Tab &&
          *m_json != LineFeed &&
          *m_json != Return)
         break;
      ++m_json;
   }
   return (m_json < m_end);
}

char Parser::nextToken()
{
   if (!eatSpace()) {
      return 0;
   }
   char token = *m_json++;
   switch (token) {
   case BeginArray:
   case BeginObject:
   case NameSeparator:
   case ValueSeparator:
   case EndArray:
   case EndObject:
   case Quote:
      break;
   default:
      token = 0;
      break;
   }
   return token;
}

/*
    JSON-text = object / array
*/
JsonDocument Parser::parse(JsonParseError *error)
{
#ifdef PARSER_DEBUG
   indent = 0;
   warning_stream(">>>>> parser begin");
#endif
   // allocate some space
   m_dataLength = std::max(m_end - m_json, (ptrdiff_t) 256);
   m_data = (char *)malloc(m_dataLength);
   // fill in Header data
   jsonprivate::Header *h = (jsonprivate::Header *)m_data;
   h->m_tag = JsonDocument::BinaryFormatTag;
   h->m_version = 1u;   
   m_current = sizeof(jsonprivate::Header);
   eatBOM();
   char token = nextToken();
   DEBUG << pdk::io::hex << (uint)token;
   if (token == BeginArray) {
      if (!parseArray()) {
         goto error;
      } 
   } else if (token == BeginObject) {
      if (!parseObject()) {
         goto error;
      } 
   } else {
      m_lastError = JsonParseError::ParseError::IllegalValue;
      goto error;
   }
   eatSpace();
   if (m_json < m_end) {
      m_lastError = JsonParseError::ParseError::GarbageAtEnd;
      goto error;
   }
   
   END;
   {
      if (error) {
         error->m_offset = 0;
         error->m_error = JsonParseError::ParseError::NoError;
      }
      jsonprivate::Data *d = new jsonprivate::Data(m_data, m_current);
      return JsonDocument(d);
   }
   
error:
#ifdef PARSER_DEBUG
   warning_stream(">>>>> parser error");
#endif
   if (error) {
      error->m_offset = m_json - m_head;
      error->m_error  = m_lastError;
   }
   free(m_data);
   return JsonDocument();
}

void Parser::ParsedObject::insert(uint offset) {
   const jsonprivate::LocalEntry *newEntry = 
         reinterpret_cast<const jsonprivate::LocalEntry *>(m_parser->m_data + m_objectPosition + offset);
   size_t min = 0;
   int n = m_offsets.size();
   while (n > 0) {
      int half = n >> 1;
      int middle = min + half;
      if (*entryAt(middle) >= *newEntry) {
         n = half;
      } else {
         min = middle + 1;
         n -= half + 1;
      }
   }
   if (min < m_offsets.size() && *entryAt(min) == *newEntry) {
      m_offsets[min] = offset;
   } else {
      m_offsets.push_back(offset);
      // m_offsets[min] = offset;
   }
}

/*
    object = begin-object [ member *( value-separator member ) ]
    end-object
*/
bool Parser::parseObject()
{
   if (++m_nestingLevel > sg_nestingLimit) {
      m_lastError = JsonParseError::ParseError::DeepNesting;
      return false;
   }
   int objectOffset = reserveSpace(sizeof(jsonprivate::LocalObject));
   if (objectOffset < 0) {
      return false;
   }
   BEGIN << "parseObject pos=" << objectOffset << m_current << m_json;
   ParsedObject parsedObject(this, objectOffset);
   char token = nextToken();
   while (token == Quote) {
      int off = m_current - objectOffset;
      if (!parseMember(objectOffset)) {
         return false;
      }
      parsedObject.insert(off);
      token = nextToken();
      if (token != ValueSeparator) {
         break;
      }
      token = nextToken();
      if (token == EndObject) {
         m_lastError = JsonParseError::ParseError::MissingObject;
         return false;
      }
   }
   
   DEBUG << "end token=" << token;
   if (token != EndObject) {
      m_lastError = JsonParseError::ParseError::UnterminatedObject;
      return false;
   }
   
   DEBUG << "numEntries" << parsedObject.m_offsets.size();
   int table = objectOffset;
   // finalize the object
   if (parsedObject.m_offsets.size()) {
      int tableSize = parsedObject.m_offsets.size() * sizeof(uint);
      table = reserveSpace(tableSize);
      if (table < 0) {
         return false;
      }
#if PDK_BYTE_ORDER == PDK_LITTLE_ENDIAN
      memcpy(m_data + table, parsedObject.m_offsets.data(), tableSize);
#else
      offset *o = (offset *)(m_data + table);
      for (int i = 0; i < parsedObject.m_offsets.size(); ++i) {
         o[i] = parsedObject.m_offsets[i];
      } 
#endif
   }
   
   jsonprivate::LocalObject *o = (jsonprivate::LocalObject *)(m_data + objectOffset);
   o->m_tableOffset = table - objectOffset;
   o->m_size = m_current - objectOffset;
   o->m_isObject = true;
   o->m_length = parsedObject.m_offsets.size();
   DEBUG << "current=" << m_current;
   END;
   --m_nestingLevel;
   return true;
}

/*
    member = string name-separator value
*/
bool Parser::parseMember(int baseOffset)
{
   int entryOffset = reserveSpace(sizeof(jsonprivate::LocalEntry));
   if (entryOffset < 0) {
      return false;
   }
   BEGIN << "parseMember pos=" << entryOffset;
   bool latin1;
   if (!parseString(&latin1)) {
      return false;
   }
   char token = nextToken();
   if (token != NameSeparator) {
      m_lastError = JsonParseError::ParseError::MissingNameSeparator;
      return false;
   }
   if (!eatSpace()) {
      m_lastError = JsonParseError::ParseError::UnterminatedObject;
      return false;
   }
   jsonprivate::LocalValue val;
   if (!parseValue(&val, baseOffset)) {
      return false;
   }
   // finalize the entry
   jsonprivate::LocalEntry *e = (jsonprivate::LocalEntry *)(m_data + entryOffset);
   e->m_value = val;
   e->m_value.m_latinKey = latin1;
   END;
   return true;
}

namespace {
struct ValueArray {
   static const int sm_prealloc = 128;
   ValueArray()
      : m_data(m_stackValues),
        m_alloc(sm_prealloc),
        m_size(0)
   {}
   
   ~ValueArray()
   {
      if (m_data != m_stackValues) {
         free(m_data);
      }
   }
   
   inline bool grow()
   {
      m_alloc *= 2;
      if (m_data == m_stackValues) {
         jsonprivate::LocalValue *newValues = static_cast<jsonprivate::LocalValue *>(malloc(m_alloc * sizeof(jsonprivate::LocalValue)));
         if (!newValues) {
            return false;
         }
         memcpy(newValues, m_data, m_size * sizeof(jsonprivate::LocalValue));
         m_data = newValues;
      } else {
         void *newValues = realloc(m_data, m_alloc * sizeof(jsonprivate::LocalValue));
         if (!newValues) {
            return false;
         }
         m_data = static_cast<jsonprivate::LocalValue *>(newValues);
      }
      return true;
   }
   
   bool append(const jsonprivate::LocalValue &v)
   {
      if (m_alloc == m_size && !grow()) {
         return false;
      }
      m_data[m_size] = v;
      ++m_size;
      return true;
   }
   
   jsonprivate::LocalValue m_stackValues[sm_prealloc];
   jsonprivate::LocalValue *m_data;
   int m_alloc;
   int m_size;
};
} // anonymous namespace

/*
    array = begin-array [ value *( value-separator value ) ] end-array
*/
bool Parser::parseArray()
{
   BEGIN << "parseArray";
   if (++m_nestingLevel > sg_nestingLimit) {
      m_lastError = JsonParseError::ParseError::DeepNesting;
      return false;
   }
   int arrayOffset = reserveSpace(sizeof(jsonprivate::LocalArray));
   if (arrayOffset < 0) {
      return false;
   }
   ValueArray values;
   if (!eatSpace()) {
      m_lastError = JsonParseError::ParseError::UnterminatedArray;
      return false;
   }
   if (*m_json == EndArray) {
      nextToken();
   } else {
      while (1) {
         if (!eatSpace()) {
            m_lastError = JsonParseError::ParseError::UnterminatedArray;
            return false;
         }
         jsonprivate::LocalValue val;
         if (!parseValue(&val, arrayOffset))
            return false;
         if (!values.append(val)) {
            m_lastError = JsonParseError::ParseError::DocumentTooLarge;
            return false;
         }
         char token = nextToken();
         if (token == EndArray)
            break;
         else if (token != ValueSeparator) {
            if (!eatSpace()) {
               m_lastError = JsonParseError::ParseError::UnterminatedArray;
            } else {
               m_lastError = JsonParseError::ParseError::MissingValueSeparator;
            }
            return false;
         }
      }
   }
   
   DEBUG << "size =" << values.m_size;
   int table = arrayOffset;
   // finalize the object
   if (values.m_size) {
      int tableSize = values.m_size * sizeof(jsonprivate::LocalValue);
      table = reserveSpace(tableSize);
      if (table < 0) {
         return false;
      }
      memcpy(m_data + table, values.m_data, tableSize);
   }
   jsonprivate::LocalArray *a = (jsonprivate::LocalArray *)(m_data + arrayOffset);
   a->m_tableOffset = table - arrayOffset;
   a->m_size = m_current - arrayOffset;
   a->m_isObject = false;
   a->m_length = values.m_size;
   DEBUG << "current=" << m_current;
   END;
   --m_nestingLevel;
   return true;
}

/*
value = false / null / true / object / array / number / string

*/
bool Parser::parseValue(jsonprivate::LocalValue *val, int baseOffset)
{
   BEGIN << "parse Value" << m_json;
   val->m_dummy = 0;
   switch (*m_json++) {
   case 'n':
      if (m_end - m_json < 4) {
         m_lastError = JsonParseError::ParseError::IllegalValue;
         return false;
      }
      if (*m_json++ == 'u' &&
          *m_json++ == 'l' &&
          *m_json++ == 'l') {
         val->m_type = pdk::as_integer<JsonValue::Type>(JsonValue::Type::Null);
         DEBUG << "value: null";
         END;
         return true;
      }
      m_lastError = JsonParseError::ParseError::IllegalValue;
      return false;
   case 't':
      if (m_end - m_json < 4) {
         m_lastError = JsonParseError::ParseError::IllegalValue;
         return false;
      }
      if (*m_json++ == 'r' &&
          *m_json++ == 'u' &&
          *m_json++ == 'e') {
         val->m_type = pdk::as_integer<JsonValue::Type>(JsonValue::Type::Bool);
         val->m_value = true;
         DEBUG << "value: true";
         END;
         return true;
      }
      m_lastError = JsonParseError::ParseError::IllegalValue;
      return false;
   case 'f':
      if (m_end - m_json < 5) {
         m_lastError = JsonParseError::ParseError::IllegalValue;
         return false;
      }
      if (*m_json++ == 'a' &&
          *m_json++ == 'l' &&
          *m_json++ == 's' &&
          *m_json++ == 'e') {
         val->m_type = pdk::as_integer<JsonValue::Type>(JsonValue::Type::Bool);
         val->m_value = false;
         DEBUG << "value: false";
         END;
         return true;
      }
      m_lastError = JsonParseError::ParseError::IllegalValue;
      return false;
   case Quote: {
      val->m_type = pdk::as_integer<JsonValue::Type>(JsonValue::Type::String);
      if (m_current - baseOffset >= LocalValue::MaxSize) {
         m_lastError = JsonParseError::ParseError::DocumentTooLarge;
         return false;
      }
      val->m_value = m_current - baseOffset;
      bool latin1;
      if (!parseString(&latin1)) {
         return false;
      }
      val->m_latinOrIntValue = latin1;
      DEBUG << "value: string";
      END;
      return true;
   }
   case BeginArray:
      val->m_type = pdk::as_integer<JsonValue::Type>(JsonValue::Type::Array);
      if (m_current - baseOffset >= LocalValue::MaxSize) {
         m_lastError = JsonParseError::ParseError::DocumentTooLarge;
         return false;
      }
      val->m_value = m_current - baseOffset;
      if (!parseArray()) {
         return false;
      }
      DEBUG << "value: array";
      END;
      return true;
   case BeginObject:
      val->m_type = pdk::as_integer<JsonValue::Type>(JsonValue::Type::Object);
      if (m_current - baseOffset >= LocalValue::MaxSize) {
         m_lastError = JsonParseError::ParseError::DocumentTooLarge;
         return false;
      }
      val->m_value = m_current - baseOffset;
      if (!parseObject()) {
         return false;
      }
      DEBUG << "value: object";
      END;
      return true;
   case ValueSeparator:
      // Essentially missing value, but after a colon, not after a comma
      // like the other MissingObject errors.
      m_lastError = JsonParseError::ParseError::IllegalValue;
      return false;
   case EndObject:
   case EndArray:
      m_lastError = JsonParseError::ParseError::MissingObject;
      return false;
   default:
      --m_json;
      if (!parseNumber(val, baseOffset)) {
         return false;
      }
      DEBUG << "value: number";
      END;
   }
   
   return true;
}

/*
        number = [ minus ] int [ frac ] [ exp ]
        decimal-point = %x2E       ; .
        digit1-9 = %x31-39         ; 1-9
        e = %x65 / %x45            ; e E
        exp = e [ minus / plus ] 1*DIGIT
        frac = decimal-point 1*DIGIT
        int = zero / ( digit1-9 *DIGIT )
        minus = %x2D               ; -
        plus = %x2B                ; +
        zero = %x30                ; 0
        
*/
bool Parser::parseNumber(jsonprivate::LocalValue *val, int baseOffset)
{
   BEGIN << "parseNumber" << m_json;
   val->m_type = pdk::as_integer<JsonValue::Type>(JsonValue::Type::Double);
   const char *start = m_json;
   bool isInt = true;
   // minus
   if (m_json < m_end && *m_json == '-') {
      ++m_json;
   }
   // int = zero / ( digit1-9 *DIGIT )
   if (m_json < m_end && *m_json == '0') {
      ++m_json;
   } else {
      while (m_json < m_end && *m_json >= '0' && *m_json <= '9') {
         ++m_json;
      }
   }
   // frac = decimal-point 1*DIGIT
   if (m_json < m_end && *m_json == '.') {
      isInt = false;
      ++m_json;
      while (m_json < m_end && *m_json >= '0' && *m_json <= '9') {
         ++m_json;
      }
   }
   // exp = e [ minus / plus ] 1*DIGIT
   if (m_json < m_end && (*m_json == 'e' || *m_json == 'E')) {
      isInt = false;
      ++m_json;
      if (m_json < m_end && (*m_json == '-' || *m_json == '+')) {
         ++m_json;
      }
      while (m_json < m_end && *m_json >= '0' && *m_json <= '9') {
         ++m_json;
      }
      
   }
   if (m_json >= m_end) {
      m_lastError = JsonParseError::ParseError::TerminationByNumber;
      return false;
   }
   ByteArray number(start, m_json - start);
   DEBUG << "numberstring" << number;
   if (isInt) {
      bool ok;
      int n = number.toInt(&ok);
      if (ok && n < (1<<25) && n > -(1<<25)) {
         val->m_intValue = n;
         val->m_latinOrIntValue = true;
         END;
         return true;
      }
   }
   bool ok;
   union {
      pdk::puint64 m_ui;
      double m_d;
   };
   m_d = number.toDouble(&ok);
   if (!ok) {
      m_lastError = JsonParseError::ParseError::IllegalNumber;
      return false;
   }
   int pos = reserveSpace(sizeof(double));
   if (pos < 0) {
      return false;
   }
   pdk::to_little_endian(m_ui, m_data + pos);
   if (m_current - baseOffset >= LocalValue::MaxSize) {
      m_lastError = JsonParseError::ParseError::DocumentTooLarge;
      return false;
   }
   val->m_value = pos - baseOffset;
   val->m_latinOrIntValue = false;
   END;
   return true;
}

namespace {
/*
  
        string = quotation-mark *char quotation-mark
        
        char = unescaped /
               escape (
                   %x22 /          ; "    quotation mark  U+0022
                   %x5C /          ; \    reverse solidus U+005C
                   %x2F /          ; /    solidus         U+002F
                   %x62 /          ; b    backspace       U+0008
                   %x66 /          ; f    form feed       U+000C
                   %x6E /          ; n    line feed       U+000A
                   %x72 /          ; r    carriage return U+000D
                   %x74 /          ; t    tab             U+0009
                   %x75 4HEXDIG )  ; uXXXX                U+XXXX
                   
        escape = %x5C              ; \
        
        quotation-mark = %x22      ; "
        
        unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
 */
inline bool add_hex_digit(char digit, uint *result)
{
   *result <<= 4;
   if (digit >= '0' && digit <= '9') {
      *result |= (digit - '0');
   } else if (digit >= 'a' && digit <= 'f') {
      *result |= (digit - 'a') + 10;
   } else if (digit >= 'A' && digit <= 'F') {
      *result |= (digit - 'A') + 10;
   } else {
      return false;
   }
   return true;
}

inline bool scan_escape_sequence(const char *&json, const char *end, uint *ch)
{
   ++json;
   if (json >= end) {
      return false;
   }
   DEBUG << "scan escape" << (char)*json;
   uint escaped = *json++;
   switch (escaped) {
   case '"':
      *ch = '"'; break;
   case '\\':
      *ch = '\\'; break;
   case '/':
      *ch = '/'; break;
   case 'b':
      *ch = 0x8; break;
   case 'f':
      *ch = 0xc; break;
   case 'n':
      *ch = 0xa; break;
   case 'r':
      *ch = 0xd; break;
   case 't':
      *ch = 0x9; break;
   case 'u': {
      *ch = 0;
      if (json > end - 4)
         return false;
      for (int i = 0; i < 4; ++i) {
         if (!add_hex_digit(*json, ch)) {
            return false;
         }
         ++json;
      }
      return true;
   }
   default:
      // this is not as strict as one could be, but allows for more Json files
      // to be parsed correctly.
      *ch = escaped;
      return true;
   }
   return true;
}

inline bool scan_utf8_char(const char *&json, const char *end, uint *result)
{
   const uchar *&src = reinterpret_cast<const uchar *&>(json);
   const uchar *uend = reinterpret_cast<const uchar *>(end);
   uchar b = *src++;
   int res = utf8funcs::fromUtf8<codecsinternal::Utf8BaseTraits>(b, result, src, uend);
   if (res < 0) {
      // decoding error, backtrack the character we read above
      --json;
      return false;
   }   
   return true;
}

} // anonymous namespace

bool Parser::parseString(bool *latin1)
{
   *latin1 = true;
   
   const char *start = m_json;
   int outStart = m_current;
   // try to write out a latin1 string
   int stringPos = reserveSpace(2);
   if (stringPos < 0) {
      return false;
   }
   BEGIN << "parse string stringPos=" << stringPos << m_json;
   while (m_json < m_end) {
      uint ch = 0;
      if (*m_json == '"') {
         break;
      } else if (*m_json == '\\') {
         if (!scan_escape_sequence(m_json, m_end, &ch)) {
            m_lastError = JsonParseError::ParseError::IllegalEscapeSequence;
            return false;
         }
      } else {
         if (!scan_utf8_char(m_json, m_end, &ch)) {
            m_lastError = JsonParseError::ParseError::IllegalUTF8String;
            return false;
         }
      }
      // bail out if the string is not pure latin1 or too long to hold as a latin1string (which has only 16 bit for the length)
      if (ch > 0xff || m_json - start >= 0x8000) {
         *latin1 = false;
         break;
      }
      int pos = reserveSpace(1);
      if (pos < 0) {
         return false;
      }
      DEBUG << "  " << ch << (char)ch;
      m_data[pos] = (uchar)ch;
   }
   ++m_json;
   DEBUG << "end of string";
   if (m_json >= m_end) {
      m_lastError = JsonParseError::ParseError::UnterminatedString;
      return false;
   }
   // no unicode string, we are done
   if (*latin1) {
      // write string length
      *(jsonprivate::ple_ushort *)(m_data + stringPos) = ushort(m_current - outStart - sizeof(ushort));
      int pos = reserveSpace((4 - m_current) & 3);
      if (pos < 0) {
         return false;
      }
      while (pos & 3) {
         m_data[pos++] = 0;
      }
      END;
      return true;
   }
   *latin1 = false;
   DEBUG << "not latin";
   m_json = start;
   m_current = outStart + sizeof(int);
   while (m_json < m_end) {
      uint ch = 0;
      if (*m_json == '"') {
         break;
      } else if (*m_json == '\\') {
         if (!scan_escape_sequence(m_json, m_end, &ch)) {
            m_lastError = JsonParseError::ParseError::IllegalEscapeSequence;
            return false;
         }
      } else {
         if (!scan_utf8_char(m_json, m_end, &ch)) {
            m_lastError = JsonParseError::ParseError::IllegalUTF8String;
            return false;
         }
      }
      if (Character::requiresSurrogates(ch)) {
         int pos = reserveSpace(4);
         if (pos < 0) {
            return false;
         }
         *(jsonprivate::ple_ushort *)(m_data + pos) = Character::getHighSurrogate(ch);
         *(jsonprivate::ple_ushort *)(m_data + pos + 2) = Character::getLowSurrogate(ch);
      } else {
         int pos = reserveSpace(2);
         if (pos < 0) {
            return false;
         }            
         *(jsonprivate::ple_ushort *)(m_data + pos) = (ushort)ch;
      }
   }
   ++m_json;
   if (m_json >= m_end) {
      m_lastError = JsonParseError::ParseError::UnterminatedString;
      return false;
   }
   // write string length
   *(jsonprivate::ple_int *)(m_data + stringPos) = (m_current - outStart - sizeof(int))/2;
   int pos = reserveSpace((4 - m_current) & 3);
   if (pos < 0) {
      return false;
   }
   while (pos & 3) {
      m_data[pos++] = 0;
   }
   END;
   return true;
}

} // jsonprivate
} // json
} // utils
} // pdk

