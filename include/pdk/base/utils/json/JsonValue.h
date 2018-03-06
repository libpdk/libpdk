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

#ifndef PDK_M_BASE_JSON_JSON_VALUE_H
#define PDK_M_BASE_JSON_JSON_VALUE_H

#include "pdk/global/Global.h"
#include "pdk/base/lang/String.h"
#include <any>

namespace pdk {

// forward declare class with namespace
namespace io {
class Debug;
} // debug

namespace utils {
namespace json {

class JsonArray;
class JsonObject;

using pdk::io::Debug;
using pdk::lang::String;
using pdk::lang::Latin1String;
using pdk::lang::StringData;

// forward declare class with namespace
namespace jsonprivate
{
class Data;
class Base;
class LocalObject;
class Header;
class LocalArray;
class LocalValue;
class LocalEntry;
} // internal

class PDK_CORE_EXPORT JsonValue
{
public:
   enum class Type
   {
      Null =  0x0,
      Bool = 0x1,
      Double = 0x2,
      String = 0x3,
      Array = 0x4,
      Object = 0x5,
      Undefined = 0x80
   };
   
   JsonValue(Type = Type::Null);
   JsonValue(bool b);
   JsonValue(double n);
   JsonValue(int n);
   JsonValue(pdk::pint64 n);
   JsonValue(const String &s);
   JsonValue(Latin1String s);
   JsonValue(const JsonArray &a);
   JsonValue(const JsonObject &o);
   
   ~JsonValue();
   
   JsonValue(const JsonValue &other);
   JsonValue &operator =(const JsonValue &other);
   
   JsonValue(JsonValue &&other) noexcept
      : m_ui(other.m_ui),
        m_d(other.m_d),
        m_type(other.m_type)
   {
      other.m_ui = 0;
      other.m_d = nullptr;
      other.m_type = Type::Null;
   }
   
   JsonValue &operator =(JsonValue &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   void swap(JsonValue &other) noexcept
   {
      std::swap(m_ui, other.m_ui);
      std::swap(m_d, other.m_d);
      std::swap(m_type, other.m_type);
   }
   
   static JsonValue fromStdAny(const std::any &any);
   std::any toStdAny() const;
   
   Type getType() const;
   inline bool isNull() const
   {
      return getType() == Type::Null;
   }
   
   inline bool isBool() const
   {
      return getType() == Type::Bool;
   }
   
   inline bool isDouble() const
   {
      return getType() == Type::Double;
   }
   
   inline bool isString() const
   {
      return getType() == Type::String;
   }
   
   inline bool isArray() const
   {
      return getType() == Type::Array;
   }
   
   inline bool isObject() const
   {
      return getType() == Type::Object;
   }
   
   inline bool isUndefined() const
   {
      return getType() == Type::Undefined;
   }
   
   bool toBool(bool defaultValue = false) const;
   int toInt(int defaultValue = 0) const;
   double toDouble(double defaultValue = 0) const;
   String toString() const;
   String toString(const String &defaultValue) const;
   JsonArray toArray() const;
   JsonArray toArray(const JsonArray &defaultValue) const;
   JsonObject toObject() const;
   JsonObject toObject(const JsonObject &defaultValue) const;
   
   const JsonValue operator[](const String &key) const;
   const JsonValue operator[](Latin1String key) const;
   const JsonValue operator[](int i) const;
   
   bool operator==(const JsonValue &other) const;
   bool operator!=(const JsonValue &other) const;
   
private:
   // avoid implicit conversions from char * to bool
   inline JsonValue(const void *)
   {}
   
   friend class jsonprivate::LocalValue;
   friend class JsonArray;
   friend class JsonObject;
   friend PDK_CORE_EXPORT Debug operator<<(Debug, const JsonValue &);
   
   JsonValue(jsonprivate::Data *data, jsonprivate::Base *base, const jsonprivate::LocalValue& value);
   void stringDataFromStringHelper(const String &string);
   
   void detach();
   
   union {
      pdk::puint64 m_ui;
      bool m_b;
      double m_dbl;
      StringData *m_stringData;
      jsonprivate::Base *m_base;
   };
   jsonprivate::Data *m_d; // needed for Objects and Arrays
   Type m_type;
};

class PDK_CORE_EXPORT JsonValueRef
{
public:
   JsonValueRef(JsonArray *array, int idx)
      : m_array(array),
        m_isObject(false),
        m_index(idx)
   {}
   
   JsonValueRef(JsonObject *object, int idx)
      : m_object(object),
        m_isObject(true), 
        m_index(idx)
   {}
   
   inline operator JsonValue() const
   {
      return toValue();
   }
   
   JsonValueRef &operator = (const JsonValue &val);
   JsonValueRef &operator = (const JsonValueRef &val);
   
   std::any toStdAny() const;
   
   inline JsonValue::Type getType() const
   {
      return toValue().getType();
   }
   
   inline bool isNull() const 
   {
      return getType() == JsonValue::Type::Null;
   }
   
   inline bool isBool() const 
   {
      return getType() == JsonValue::Type::Bool;
   }
   
   inline bool isDouble() const
   {
      return getType() == JsonValue::Type::Double;
   }
   
   inline bool isString() const
   {
      return getType() == JsonValue::Type::String;
   }
   
   inline bool isArray() const
   {
      return getType() == JsonValue::Type::Array;
   }
   
   inline bool isObject() const 
   {
      return getType() == JsonValue::Type::Object; }
   
   inline bool isUndefined() const
   {
      return getType() == JsonValue::Type::Undefined;
   }
   
   inline bool toBool() const
   {
      return toValue().toBool();
   }
   
   inline int toInt() const
   {
      return toValue().toInt();
   }
   
   inline double toDouble() const
   {
      return toValue().toDouble();
   }
   
   inline String toString() const
   {
      return toValue().toString();
   }
   
   JsonArray toArray() const;
   JsonObject toObject() const;
   
   inline bool toBool(bool defaultValue) const
   {
      return toValue().toBool(defaultValue);
   }
   
   inline int toInt(int defaultValue) const
   {
      return toValue().toInt(defaultValue);
   }
   
   inline double toDouble(double defaultValue) const 
   {
      return toValue().toDouble(defaultValue);
   }
   
   inline String toString(const String &defaultValue) const
   {
      return toValue().toString(defaultValue);
   }
   
   inline bool operator==(const JsonValue &other) const
   {
      return toValue() == other;
   }
   
   inline bool operator!=(const JsonValue &other) const
   {
      return toValue() != other;
   }
   
private:
   JsonValue toValue() const;
   
   union {
      JsonArray *m_array;
      JsonObject *m_object;
   };
   uint m_isObject : 1;
   uint m_index : 31;
};

class JsonValuePtr
{
   JsonValue value;
public:
   explicit JsonValuePtr(const JsonValue& val)
      : value(val) {}
   
   JsonValue& operator*() { return value; }
   JsonValue* operator->() { return &value; }
};

class JsonValueRefPtr
{
   JsonValueRef valueRef;
public:
   JsonValueRefPtr(JsonArray *array, int idx)
      : valueRef(array, idx) {}
   JsonValueRefPtr(JsonObject *object, int idx)
      : valueRef(object, idx)  {}
   
   JsonValueRef& operator*() { return valueRef; }
   JsonValueRef* operator->() { return &valueRef; }
};


#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_JSON_READONLY)
PDK_CORE_EXPORT Debug operator<<(Debug, const JsonValue &);
#endif

} // json
} // utils
} // pdk

PDK_DECLARE_TYPEINFO(pdk::utils::json::JsonValue, PDK_MOVABLE_TYPE);

#endif // PDK_M_BASE_JSON_JSON_VALUE_H
