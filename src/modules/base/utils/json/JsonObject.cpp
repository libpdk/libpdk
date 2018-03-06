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

#include "pdk/base/utils/json/JsonObject.h"
#include "pdk/base/utils/json/JsonValue.h"
#include "pdk/base/utils/json/JsonArray.h"
#include "pdk/base/utils/json/internal/JsonPrivate.h"
#include "pdk/base/utils/json/internal/JsonWriterPrivate.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/base/io/Debug.h"
#include <vector>

namespace pdk {
namespace utils {
namespace json {

using pdk::io::Debug;
using pdk::io::DebugStateSaver;
using pdk::lang::String;

JsonObject::JsonObject()
   : m_data(nullptr), 
     m_object(nullptr)
{
}

JsonObject::JsonObject(jsonprivate::Data *data, jsonprivate::LocalObject *object)
   : m_data(data),
     m_object(object)
{
   PDK_ASSERT(m_data);
   PDK_ASSERT(m_object);
   m_data->m_ref.ref();
}

void JsonObject::initialize()
{
   m_data = nullptr;
   m_object = nullptr;
}

JsonObject::~JsonObject()
{
   if (m_data && !m_data->m_ref.deref()) {
      delete m_data;
   }
}

JsonObject::JsonObject(const JsonObject &other)
{
   m_data = other.m_data;
   m_object = other.m_object;
   if (m_data) {
      m_data->m_ref.ref();
   }
}

JsonObject &JsonObject::operator =(const JsonObject &other)
{
   if (m_data != other.m_data) {
      if (m_data && !m_data->m_ref.deref()) {
         delete m_data;
      }
      m_data = other.m_data;
      if (m_data) {
         m_data->m_ref.ref();
      }
   }
   m_object = other.m_object;
   return *this;
}

JsonObject JsonObject::fromAnyMap(const AnyMap &map)
{
   JsonObject object;
   if (map.empty()) {
      return object;
   }
   object.detach(1024);
   std::vector<jsonprivate::offset> offsets;
   jsonprivate::offset currentOffset;
   currentOffset = sizeof(jsonprivate::Base);
   
   // the map is already sorted, so we can simply append one entry after the other and
   // write the offset table at the end
   for (AnyMap::const_iterator iter = map.cbegin(); iter != map.cend(); ++iter) {
      String key = iter->first;
      JsonValue val = JsonValue::fromStdAny(iter->second);
      bool latinOrIntValue;
      int valueSize = jsonprivate::LocalValue::requiredStorage(val, &latinOrIntValue);
      bool latinKey = jsonprivate::use_compressed(key);
      int valueOffset = sizeof(jsonprivate::LocalEntry) + jsonprivate::string_size(key, latinKey);
      int requiredSize = valueOffset + valueSize;
      if (!object.detach(requiredSize + sizeof(jsonprivate::offset))) {// offset for the new index entry
         return JsonObject();
      }
      jsonprivate::LocalEntry *entry = reinterpret_cast<jsonprivate::LocalEntry *>(reinterpret_cast<char *>(object.m_object) + currentOffset);
      entry->m_value.m_type = pdk::as_integer<JsonValue::Type>(val.m_type);
      entry->m_value.m_latinKey = latinKey;
      entry->m_value.m_latinOrIntValue = latinOrIntValue;
      entry->m_value.m_value = jsonprivate::LocalValue::valueToStore(val, (char *)entry - (char *)object.m_object + valueOffset);
      jsonprivate::copy_string((char *)(entry + 1), key, latinKey);
      if (valueSize) {
         jsonprivate::LocalValue::copyData(val, (char *)entry + valueOffset, latinOrIntValue);
      }
      offsets.push_back(currentOffset);
      currentOffset += requiredSize;
      object.m_object->m_size = currentOffset;
   }
   
   // write table
   object.m_object->m_tableOffset = currentOffset;
   if (!object.detach(sizeof(jsonprivate::offset)*offsets.size())) {
      return JsonObject();
   }
   memcpy(object.m_object->getTable(), offsets.data(), offsets.size() * sizeof(uint));
   object.m_object->m_length = offsets.size();
   object.m_object->m_size = currentOffset + sizeof(jsonprivate::offset) * offsets.size();
   return object;
}

AnyMap JsonObject::toAnyMap() const
{
   AnyMap map;
   if (m_object) {
      for (uint i = 0; i < m_object->m_length; ++i) {
         jsonprivate::LocalEntry *e = m_object->entryAt(i);
         map[e->getKey()] = JsonValue(m_data, m_object, e->m_value).toStdAny();
      }
   }
   return map;
}

StringList JsonObject::getKeys() const
{
   StringList keys;
   if (m_object) {
      keys.resize(m_object->m_length);
      for (uint i = 0; i < m_object->m_length; ++i) {
         jsonprivate::LocalEntry *e = m_object->entryAt(i);
         keys.push_back(e->getKey());
      }
   }
   return keys;
}

int JsonObject::getSize() const
{
   if (!m_data) {
      return 0;
   }
   return m_object->m_length;
}

bool JsonObject::isEmpty() const
{
   if (!m_data) {
      return true;
   }
   return !m_object->m_length;
}

JsonValue JsonObject::getValue(const String &key) const
{
   if (!m_data) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   bool keyExists;
   int i = m_object->indexOf(key, &keyExists);
   if (!keyExists) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   return JsonValue(m_data, m_object, m_object->entryAt(i)->m_value);
}

JsonValue JsonObject::getValue(Latin1String key) const
{
   if (!m_data) {
      return JsonValue(JsonValue::Type::Undefined);
   }      
   bool keyExists;
   int i = m_object->indexOf(key, &keyExists);
   if (!keyExists) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   return JsonValue(m_data, m_object, m_object->entryAt(i)->m_value);
}

JsonValue JsonObject::operator [](const String &key) const
{
   return getValue(key);
}

JsonValueRef JsonObject::operator [](const String &key)
{
   // ### somewhat inefficient, as we lookup the key twice if it doesn't yet exist
   bool keyExists = false;
   int index = m_object ? m_object->indexOf(key, &keyExists) : -1;
   if (!keyExists) {
      iterator iter = insert(key, JsonValue());
      index = iter.m_index;
   }
   return JsonValueRef(this, index);
}

JsonValueRef JsonObject::operator [](Latin1String key)
{
   // ### optimize me
   return operator[](String(key));
}

JsonObject::iterator JsonObject::insert(const String &key, const JsonValue &value)
{
   if (value.m_type == JsonValue::Type::Undefined) {
      remove(key);
      return end();
   }
   JsonValue val = value;
   
   bool latinOrIntValue;
   int valueSize = jsonprivate::LocalValue::requiredStorage(val, &latinOrIntValue);
   
   bool latinKey = jsonprivate::use_compressed(key);
   int valueOffset = sizeof(jsonprivate::LocalEntry) + jsonprivate::string_size(key, latinKey);
   int requiredSize = valueOffset + valueSize;
   if (!detach(requiredSize + sizeof(jsonprivate::offset))) {// offset for the new index entry
      return iterator();
   }
   if (!m_object->m_length) {
      m_object->m_tableOffset = sizeof(jsonprivate::LocalObject);
   }
   bool keyExists = false;
   int pos = m_object->indexOf(key, &keyExists);
   if (keyExists) {
      ++m_data->m_compactionCounter;
   }
   uint off = m_object->reserveSpace(requiredSize, pos, 1, keyExists);
   if (!off) {
      return end();
   }
   jsonprivate::LocalEntry *entry = m_object->entryAt(pos);
   entry->m_value.m_type = pdk::as_integer<JsonValue::Type>(val.m_type);
   entry->m_value.m_latinKey = latinKey;
   entry->m_value.m_latinOrIntValue = latinOrIntValue;
   entry->m_value.m_value = jsonprivate::LocalValue::valueToStore(val, (char *)entry - (char *)m_object + valueOffset);
   jsonprivate::copy_string((char *)(entry + 1), key, latinKey);
   if (valueSize) {
      jsonprivate::LocalValue::copyData(val, (char *)entry + valueOffset, latinOrIntValue);
   }
   if (m_data->m_compactionCounter > 32u && m_data->m_compactionCounter >= unsigned(m_object->m_length) / 2u) {
      compact();
   }
   return iterator(this, pos);
}

void JsonObject::remove(const String &key)
{
   if (!m_data) {
      return;
   }
   bool keyExists;
   int index = m_object->indexOf(key, &keyExists);
   if (!keyExists) {
      return;
   }
   detach();
   m_object->removeItems(index, 1);
   ++m_data->m_compactionCounter;
   if (m_data->m_compactionCounter > 32u && m_data->m_compactionCounter >= unsigned(m_object->m_length) / 2u) {
      compact();
   }
}

JsonValue JsonObject::take(const String &key)
{
   if (!m_object) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   bool keyExists;
   int index = m_object->indexOf(key, &keyExists);
   if (!keyExists) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   JsonValue v(m_data, m_object, m_object->entryAt(index)->m_value);
   detach();
   m_object->removeItems(index, 1);
   ++m_data->m_compactionCounter;
   if (m_data->m_compactionCounter > 32u && m_data->m_compactionCounter >= unsigned(m_object->m_length) / 2u) {
      compact();
   }
   return v;
}

bool JsonObject::contains(const String &key) const
{
   if (!m_object) {  
      return false;
   }
   bool keyExists;
   m_object->indexOf(key, &keyExists);
   return keyExists;
}

bool JsonObject::contains(Latin1String key) const
{
   if (!m_object) {
      return false;
   }
   bool keyExists;
   m_object->indexOf(key, &keyExists);
   return keyExists;
}

bool JsonObject::operator==(const JsonObject &other) const
{
   if (m_object == other.m_object) {
      return true;
   }
   if (!m_object) {
      return !other.m_object->m_length;
   }
   if (!other.m_object) {
      return !m_object->m_length;
   }
   if (m_object->m_length != other.m_object->m_length) {
      return false;
   }
   for (uint i = 0; i < m_object->m_length; ++i) {
      jsonprivate::LocalEntry *entry = m_object->entryAt(i);
      JsonValue v(m_data, m_object, entry->m_value);
      if (other.getValue(entry->getKey()) != v) {
         return false;
      }
   }
   return true;
}

bool JsonObject::operator!=(const JsonObject &other) const
{
   return !(*this == other);
}

JsonObject::iterator JsonObject::erase(JsonObject::iterator iter)
{
   PDK_ASSERT(m_data && m_data->m_ref.load() == 1);
   if (iter.m_object != this || iter.m_index < 0 || iter.m_index >= (int)m_object->m_length) {
      return iterator(this, m_object->m_length);
   }
   int index = iter.m_index;
   m_object->removeItems(index, 1);
   ++m_data->m_compactionCounter;
   if (m_data->m_compactionCounter > 32u && m_data->m_compactionCounter >= unsigned(m_object->m_length) / 2u) {
      compact();
   }
   // iterator hasn't changed
   return iter;
}

JsonObject::iterator JsonObject::find(const String &key)
{
   bool keyExists = false;
   int index = m_object ? m_object->indexOf(key, &keyExists) : 0;
   if (!keyExists) {
      return end();
   }
   detach();
   return iterator(this, index);
}

JsonObject::iterator JsonObject::find(Latin1String key)
{
   bool keyExists = false;
   int index = m_object ? m_object->indexOf(key, &keyExists) : 0;
   if (!keyExists) {
      return end();
   }
   detach();
   return iterator(this, index);
}

JsonObject::const_iterator JsonObject::constFind(const String &key) const
{
   bool keyExists = false;
   int index = m_object ? m_object->indexOf(key, &keyExists) : 0;
   if (!keyExists) {
      return end();
   }
   return const_iterator(this, index);
}

JsonObject::const_iterator JsonObject::constFind(Latin1String key) const
{
   bool keyExists = false;
   int index = m_object ? m_object->indexOf(key, &keyExists) : 0;
   if (!keyExists) {
      return end();
   }
   return const_iterator(this, index);
}

bool JsonObject::detach(uint reserve)
{
   if (!m_data) {
      if (reserve >= jsonprivate::LocalValue::MaxSize) {
         warning_stream("Json: Document too large to store in data structure");
         return false;
      }
      m_data = new jsonprivate::Data(reserve, JsonValue::Type::Object);
      m_object = static_cast<jsonprivate::LocalObject *>(m_data->m_header->getRoot());
      m_data->m_ref.ref();
      return true;
   }
   if (reserve == 0 && m_data->m_ref.load() == 1) {
      return true;
   }
   jsonprivate::Data *x = m_data->clone(m_object, reserve);
   if (!x) {
      return false;
   }
   x->m_ref.ref();
   if (!m_data->m_ref.deref()) {
      delete m_data;
   }
   m_data = x;
   m_object = static_cast<jsonprivate::LocalObject *>(m_data->m_header->getRoot());
   return true;
}

void JsonObject::compact()
{
   if (!m_data || !m_data->m_compactionCounter) {
      return;
   }
   detach();
   m_data->compact();
   m_object = static_cast<jsonprivate::LocalObject *>(m_data->m_header->getRoot());
}

String JsonObject::keyAt(int i) const
{
   PDK_ASSERT(m_object && i >= 0 && i < (int)m_object->m_length);
   jsonprivate::LocalEntry *entry = m_object->entryAt(i);
   return entry->getKey();
}

JsonValue JsonObject::valueAt(int i) const
{
   if (!m_object || i < 0 || i >= (int)m_object->m_length) {
      return JsonValue(JsonValue::Type::Undefined);
   }
   jsonprivate::LocalEntry *entry = m_object->entryAt(i);
   return JsonValue(m_data, m_object, entry->m_value);
}

void JsonObject::setValueAt(int i, const JsonValue &val)
{
   PDK_ASSERT(m_object && i >= 0 && i < (int)m_object->m_length);
   jsonprivate::LocalEntry *entry = m_object->entryAt(i);
   insert(entry->getKey(), val);
}

#if !defined(PDK_NO_DEBUG_STREAM) && !defined(PDK_JSON_READONLY)
Debug operator<<(Debug dbg, const JsonObject &object)
{
   DebugStateSaver saver(dbg);
   if (!object.m_object) {
      dbg << "JsonObject()";
      return dbg;
   }
   ByteArray json;
   jsonprivate::Writer::objectToJson(object.m_object, json, 0, true);
   dbg.nospace() << "JsonObject("
                 << json.getConstRawData() // print as utf-8 string without extra quotation marks
                 << ")";
   return dbg;
}
#endif

} // json
} // utils
} // pdk
