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

#include "pdk/base/utils/json/internal/JsonPrivate.h"
#include <algorithm>

namespace pdk {
namespace utils {
namespace json {
namespace jsonprivate {

static constexpr Base sg_emptyArray  = { { ple_uint(sizeof(Base)) }, { 0 }, { ple_uint(0) } };
static constexpr Base sg_emptyObject = { { ple_uint(sizeof(Base)) }, { 0 }, { ple_uint(0) } };

void Data::compact()
{
   PDK_ASSERT(sizeof(LocalValue) == sizeof(offset));
   
   if (!m_compactionCounter) {
      return;
   }
   Base *base = m_header->getRoot();
   int reserve = 0;
   if (base->m_isObject) {
      LocalObject *o = static_cast<LocalObject *>(base);
      for (int i = 0; i < (int)o->m_length; ++i) {
         reserve += o->entryAt(i)->getUsedStorage(o);
      }
   } else {
      LocalArray *a = static_cast<LocalArray *>(base);
      for (int i = 0; i < (int)a->m_length; ++i) {
         reserve += (*a)[i].getUsedStorage(a);
      }
   }
   
   int size = sizeof(Base) + reserve + base->m_length * sizeof(offset);
   int alloc = sizeof(Header) + size;
   Header *h = (Header *) malloc(alloc);
   h->m_tag = JsonDocument::BinaryFormatTag;
   h->m_version = 1;
   Base *b = h->getRoot();
   b->m_size = size;
   b->m_isObject = m_header->getRoot()->m_isObject;
   b->m_length = base->m_length;
   b->m_tableOffset = reserve + sizeof(LocalArray);
   int offset = sizeof(Base);
   if (b->m_isObject) {
      LocalObject *o = static_cast<LocalObject *>(base);
      LocalObject *no = static_cast<LocalObject *>(b);      
      for (int i = 0; i < (int)o->m_length; ++i) {
         no->getTable()[i] = offset;
         const LocalEntry *e = o->entryAt(i);
         LocalEntry *ne = no->entryAt(i);
         int s = e->getSize();
         memcpy(ne, e, s);
         offset += s;
         int dataSize = e->m_value.getUsedStorage(o);
         if (dataSize) {
            memcpy((char *)no + offset, e->m_value.getData(o), dataSize);
            ne->m_value.m_value = offset;
            offset += dataSize;
         }
      }
   } else {
      LocalArray *a = static_cast<LocalArray *>(base);
      LocalArray *na = static_cast<LocalArray *>(b);
      for (int i = 0; i < (int)a->m_length; ++i) {
         const LocalValue &v = (*a)[i];
         LocalValue &nv = (*na)[i];
         nv = v;
         int dataSize = v.getUsedStorage(a);
         if (dataSize) {
            memcpy((char *)na + offset, v.getData(a), dataSize);
            nv.m_value = offset;
            offset += dataSize;
         }
      }
   }
   PDK_ASSERT(offset == (int)b->m_tableOffset);
   
   free(m_header);
   m_header = h;
   m_alloc = alloc;
   m_compactionCounter = 0;
}

bool Data::valid() const
{
   if (m_header->m_tag != JsonDocument::BinaryFormatTag || m_header->m_version != 1u) {
      return false;
   }
   bool res = false;
   Base *root = m_header->getRoot();
   int maxSize = m_alloc - sizeof(Header);
   if (root->m_isObject) {
      res = static_cast<LocalObject *>(root)->isValid(maxSize);
   } else {
      res = static_cast<LocalArray *>(root)->isValid(maxSize);
   }
   return res;
}

int Base::reserveSpace(uint dataSize, int posInTable, uint numItems, bool replace)
{
   PDK_ASSERT(posInTable >= 0 && posInTable <= (int)m_length);
   if (m_size + dataSize >= LocalValue::MaxSize) {
      warning_stream("Json: Document too large to store in data structure %d %d %d", (uint)m_size, dataSize, LocalValue::MaxSize);
      return 0;
   }
   offset off = m_tableOffset;
   // move table to new position
   if (replace) {
      memmove((char *)(getTable()) + dataSize, getTable(), m_length * sizeof(offset));
   } else {
      memmove((char *)(getTable() + posInTable + numItems) + dataSize, getTable() + posInTable, (m_length - posInTable)*sizeof(offset));
      memmove((char *)(getTable()) + dataSize, getTable(), posInTable*sizeof(offset));
   }
   m_tableOffset += dataSize;
   for (int i = 0; i < (int)numItems; ++i) {
      getTable()[posInTable + i] = off;
   } 
   m_size += dataSize;
   if (!replace) {
      m_length += numItems;
      m_size += numItems * sizeof(offset);
   }
   return off;
}

void Base::removeItems(int pos, int numItems)
{
   PDK_ASSERT(pos >= 0 && pos <= (int)m_length);
   if (pos + numItems < (int)m_length) {
      memmove(getTable() + pos, getTable() + pos + numItems, (m_length - pos - numItems)*sizeof(offset));
   }
   m_length -= numItems;
}

int LocalObject::indexOf(const String &key, bool *exists) const
{
   int min = 0;
   int n = m_length;
   while (n > 0) {
      int half = n >> 1;
      int middle = min + half;
      if (*entryAt(middle) >= key) {
         n = half;
      } else {
         min = middle + 1;
         n -= half + 1;
      }
   }
   if (min < (int)m_length && *entryAt(min) == key) {
      *exists = true;
      return min;
   }
   *exists = false;
   return min;
}

int LocalObject::indexOf(Latin1String key, bool *exists) const
{
   int min = 0;
   int n = m_length;
   while (n > 0) {
      int half = n >> 1;
      int middle = min + half;
      if (*entryAt(middle) >= key) {
         n = half;
      } else {
         min = middle + 1;
         n -= half + 1;
      }
   }
   if (min < (int)m_length && *entryAt(min) == key) {
      *exists = true;
      return min;
   }
   *exists = false;
   return min;
}

bool LocalObject::isValid(int maxSize) const
{
   if (m_size > (uint)maxSize || m_tableOffset + m_length * sizeof(offset) > m_size) {
      return false;
   }
   String lastKey;
   for (uint i = 0; i < m_length; ++i) {
      offset entryOffset = getTable()[i];
      if (entryOffset + sizeof(LocalEntry) >= m_tableOffset) {
         return false;
      }
      LocalEntry *e = entryAt(i);
      if (!e->isValid(m_tableOffset - getTable()[i]))
         return false;
      String key = e->getKey();
      if (key < lastKey) {
         return false;
      } 
      if (!e->m_value.isValid(this)) {
         return false;
      }
      lastKey = key;
   }
   return true;
}

bool LocalArray::isValid(int maxSize) const
{
   if (m_size > (uint)maxSize || m_tableOffset + m_length * sizeof(offset) > m_size) {
      return false;
   }
   for (uint i = 0; i < m_length; ++i) {
      if (!at(i).isValid(this)) {
         return false;
      }
   }
   return true;
}

bool LocalEntry::operator ==(const String &key) const
{
   if (m_value.m_latinKey) {
      return (getShallowLatin1Key() == key);
   } else {
      return (getShallowKey() == key);
   }
}

bool LocalEntry::operator==(Latin1String key) const
{
   if (m_value.m_latinKey) {
      return getShallowLatin1Key() == key;
   } else {
      return getShallowKey() == key;
   } 
}

bool LocalEntry::operator ==(const LocalEntry &other) const
{
   if (m_value.m_latinKey) {
      if (other.m_value.m_latinKey) {
         return getShallowLatin1Key() == other.getShallowLatin1Key();
      } 
      return getShallowLatin1Key() == other.getShallowKey();
   } else if (other.m_value.m_latinKey) {
      return getShallowKey() == other.getShallowLatin1Key();
   }
   return getShallowKey() == other.getShallowKey();
}

bool LocalEntry::operator >=(const LocalEntry &other) const
{
   if (m_value.m_latinKey) {
      if (other.m_value.m_latinKey) {
         return getShallowLatin1Key() >= other.getShallowLatin1Key();
      }
      return getShallowLatin1Key() >= other.getShallowKey();
   } else if (other.m_value.m_latinKey) {
      return getShallowKey() >= other.getShallowLatin1Key();
   }
   return getShallowKey() >= other.getShallowKey();
}

int LocalValue::getUsedStorage(const Base *b) const
{
   int s = 0;
   switch (JsonValue::Type(static_cast<uint>(m_type))) {
   case JsonValue::Type::Double:
      if (m_latinOrIntValue) {
         break;
      }
      s = sizeof(double);
      break;
   case JsonValue::Type::String: {
      char *d = getData(b);
      if (m_latinOrIntValue) {
         s = sizeof(ushort) + pdk::from_little_endian(*(ushort *)d);
      } else {
         s = sizeof(int) + sizeof(ushort) * pdk::from_little_endian(*(int *)d);
      }
      break;
   }
   case JsonValue::Type::Array:
   case JsonValue::Type::Object:
      s = base(b)->m_size;
      break;
   case JsonValue::Type::Null:
   case JsonValue::Type::Bool:
   default:
      break;
   }
   return aligned_size(s);
}

bool LocalValue::isValid(const Base *b) const
{
   int offset = 0;
   switch (JsonValue::Type(static_cast<uint>(m_type))) {
   case JsonValue::Type::Double:
      if (m_latinOrIntValue) {
         break;
      }  
      PDK_FALLTHROUGH();
   case JsonValue::Type::String:
   case JsonValue::Type::Array:
   case JsonValue::Type::Object:
      offset = m_value;
      break;
   case JsonValue::Type::Null:
   case JsonValue::Type::Bool:
   default:
      break;
   }
   if (!offset) {
      return true;
   }
   if (offset + sizeof(uint) > b->m_tableOffset) {
      return false;
   }
   int s = getUsedStorage(b);
   if (!s) {
      return true;
   }
   if (s < 0 || s > (int)b->m_tableOffset - offset) {
      return false;
   }
   
   if (m_type == pdk::as_integer<JsonValue::Type>(JsonValue::Type::Array)) {
      return static_cast<LocalArray *>(base(b))->isValid(s);
   }
   if (m_type == pdk::as_integer<JsonValue::Type>(JsonValue::Type::Object)) {
      return static_cast<LocalObject *>(base(b))->isValid(s);
   }
   return true;
}

int LocalValue::requiredStorage(JsonValue &v, bool *compressed)
{
   *compressed = false;
   switch (v.m_type) {
   case JsonValue::Type::Double:
      if (jsonprivate::compressed_number(v.m_dbl) != INT_MAX) {
         *compressed = true;
         return 0;
      }
      return sizeof(double);
   case JsonValue::Type::String: {
      String s = v.toString();
      *compressed = jsonprivate::use_compressed(s);
      return jsonprivate::string_size(s, *compressed);
   }
   case JsonValue::Type::Array:
   case JsonValue::Type::Object:
      if (v.m_data && v.m_data->m_compactionCounter) {
         v.detach();
         v.m_data->compact();
         v.m_base = static_cast<jsonprivate::Base *>(v.m_data->m_header->getRoot());
      }
      return v.m_base ? uint(v.m_base->m_size) : sizeof(jsonprivate::Base);
   case JsonValue::Type::Undefined:
   case JsonValue::Type::Null:
   case JsonValue::Type::Bool:
      break;
   }
   return 0;
}

uint LocalValue::valueToStore(const JsonValue &v, uint offset)
{
   switch (v.m_type) {
   case JsonValue::Type::Undefined:
   case JsonValue::Type::Null:
      break;
   case JsonValue::Type::Bool:
      return v.m_b;
   case JsonValue::Type::Double: {
      int c = jsonprivate::compressed_number(v.m_dbl);
      if (c != INT_MAX) {
         return c;
      }
   }
      PDK_FALLTHROUGH();
   case JsonValue::Type::String:
   case JsonValue::Type::Array:
   case JsonValue::Type::Object:
      return offset;
   }
   return 0;
}

void LocalValue::copyData(const JsonValue &v, char *dest, bool compressed)
{
   switch (v.m_type) {
   case JsonValue::Type::Double:
      if (!compressed) {
         pdk::to_little_endian(v.m_ui, dest);
      }
      break;
   case JsonValue::Type::String: {
      String str = v.toString();
      jsonprivate::copy_string(dest, str, compressed);
      break;
   }
   case JsonValue::Type::Array:
   case JsonValue::Type::Object: {
      const jsonprivate::Base *b = v.m_base;
      if (!b) {
         b = (v.m_type == JsonValue::Type::Array ? &sg_emptyArray : &sg_emptyObject);
      }
      memcpy(dest, b, b->m_size);
      break;
   }
   default:
      break;
   }
}

} // jsonprivate
} // json
} // utils
} // pdk
