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

#ifndef PDK_M_BASE_JSON_INTERNAL_JSON_PARSER_PRIVATE_H
#define PDK_M_BASE_JSON_INTERNAL_JSON_PARSER_PRIVATE_H

#include "pdk/global/Global.h"
#include "pdk/base/utils/json/JsonDocument.h"
#include "pdk/base/ds/VarLengthArray.h"
#include <vector>

namespace pdk {
namespace utils {
namespace json {
namespace jsonprivate {

class Parser
{
public:
   Parser(const char *json, int length);
   
   JsonDocument parse(JsonParseError *error);
   
   class ParsedObject
   {
   public:
      ParsedObject(Parser *parser, int pos) 
         : m_parser(parser),
           m_objectPosition(pos)
      {
         m_offsets.reserve(64);
      }
      void insert(uint offset);
      
      Parser *m_parser;
      int m_objectPosition;
      std::vector<uint> m_offsets;
      
      inline jsonprivate::Entry *entryAt(int i) const
      {
         return reinterpret_cast<jsonprivate::LocalEntry *>(m_parser->m_data + m_objectPosition + m_offsets[i]);
      }
   };
private:
   inline void eatBOM();
   inline bool eatSpace();
   inline char nextToken();
   
   bool parseObject();
   bool parseArray();
   bool parseMember(int baseOffset);
   bool parseString(bool *latin1);
   bool parseValue(jsonprivate::Value *val, int baseOffset);
   bool parseNumber(jsonprivate::Value *val, int baseOffset);
   const char *m_head;
   const char *m_json;
   const char *m_end;
   
   char *m_data;
   int m_dataLength;
   int m_current;
   int m_nestingLevel;
   JsonParseError::ParseError m_lastError;
   
   inline int reserveSpace(int space)
   {
      if (m_current + space >= m_dataLength) {
         m_dataLength = 2 * m_dataLength + space;
         char *newData = (char *)realloc(m_data, m_dataLength);
         if (!newData) {
            m_lastError = JsonParseError::ParseError::DocumentTooLarge;
            return -1;
         }
         m_data = newData;
      }
      int pos = m_current;
      m_current += space;
      return pos;
   }
};

} // jsonprivate
} // json
} // utils
} // pdk

#endif // PDK_M_BASE_JSON_INTERNAL_JSON_PARSER_PRIVATE_H
