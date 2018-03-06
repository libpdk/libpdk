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

#ifndef PDK_M_BASE_JSON_JSON_DOCUMENT_H
#define PDK_M_BASE_JSON_JSON_DOCUMENT_H

#include "pdk/base/utils/json/JsonValue.h"
#include <any>

namespace pdk {

// forward declare with namespace
namespace io {
class Debug;
} // io

// forward declare with namespace
namespace lang {
class String;
class Latin1String;
} // lang

// forward declare with namespace
namespace ds {
class ByteArray;
} // ds

namespace utils {
namespace json {

namespace jsonprivate {
class Parser;
}

using pdk::io::Debug;
using pdk::ds::ByteArray;
using pdk::lang::String;
using pdk::lang::Latin1String;

struct PDK_CORE_EXPORT JsonParseError
{
   enum class ParseError
   {
      NoError = 0,
      UnterminatedObject,
      MissingNameSeparator,
      UnterminatedArray,
      MissingValueSeparator,
      IllegalValue,
      TerminationByNumber,
      IllegalNumber,
      IllegalEscapeSequence,
      IllegalUTF8String,
      UnterminatedString,
      MissingObject,
      DeepNesting,
      DocumentTooLarge,
      GarbageAtEnd
   };   
   String getErrorString() const;
   int m_offset;
   ParseError m_error;
};

class PDK_CORE_EXPORT JsonDocument
{
public:
#ifdef PDK_LITTLE_ENDIAN
   static const uint BinaryFormatTag = ('p') | ('b' << 8) | ('j' << 16) | ('s' << 24);
#else
   static const uint BinaryFormatTag = ('p' << 24) | ('b' << 16) | ('j' << 8) | ('s');
#endif
   
   JsonDocument();
   explicit JsonDocument(const JsonObject &object);
   explicit JsonDocument(const JsonArray &array);
   ~JsonDocument();
   
   JsonDocument(const JsonDocument &other);
   JsonDocument &operator =(const JsonDocument &other);
   
   JsonDocument(JsonDocument &&other) noexcept
      : m_data(other.m_data)
   {
      other.m_data = nullptr;
   }
   
   JsonDocument &operator =(JsonDocument &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   void swap(JsonDocument &other) noexcept
   {
      std::swap(m_data, other.m_data);
   }
   
   enum class DataValidation
   {
      Validate,
      BypassValidation
   };
   
   static JsonDocument fromRawData(const char *data, int size, 
                                   DataValidation validation = DataValidation::Validate);
   const char *getRawData(int *size) const;
   
   static JsonDocument fromBinaryData(const ByteArray &data,
                                      DataValidation validation  = DataValidation::Validate);
   ByteArray toBinaryData() const;
   
   static JsonDocument fromAny(const std::any &anyValue);
   std::any toAny() const;
   
   enum class JsonFormat
   {
      Indented,
      Compact
   };
   
   static JsonDocument fromJson(const ByteArray &json, JsonParseError *error = nullptr);
   
#if !defined(PDK_JSON_READONLY)
   ByteArray toJson() const;
   ByteArray toJson(JsonFormat format) const;
#endif
   
   bool isEmpty() const;
   bool isArray() const;
   bool isObject() const;
   
   JsonObject getObject() const;
   JsonArray getArray() const;
   
   void setObject(const JsonObject &object);
   void setArray(const JsonArray &array);
   
   const JsonValue operator[](const String &key) const;
   const JsonValue operator[](Latin1String key) const;
   const JsonValue operator[](int i) const;
   
   bool operator==(const JsonDocument &other) const;
   bool operator!=(const JsonDocument &other) const
   {
      return !(*this == other);
   }
   
   bool isNull() const;
private:
   friend class JsonValue;
   friend class jsonprivate::Data;
   friend class jsonprivate::Parser;
   friend PDK_CORE_EXPORT Debug operator<<(Debug, const JsonDocument &);
   
   JsonDocument(jsonprivate::Data *data);
   jsonprivate::Data *m_data;
};

#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_JSON_READONLY)
PDK_CORE_EXPORT Debug operator<<(Debug, const JsonDocument &);
#endif

} // json
} // utils
} // pdk

PDK_DECLARE_TYPEINFO(pdk::utils::json::JsonDocument, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_JSON_JSON_DOCUMENT_H
