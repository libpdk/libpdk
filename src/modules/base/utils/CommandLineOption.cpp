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
// Created by softboy on 2018/03/01.


#include "pdk/base/utils/CommandLineOption.h"
#include "pdk/global/Logging.h"
#include <set>

namespace pdk {
namespace utils {

using pdk::ds::String;
using pdk::ds::StringList;
using pdk::lang::Latin1Character;
using pdk::lang::Character;

namespace internal {

class CommandLineOptionPrivate : public SharedData
{
public:
   PDK_NEVER_INLINE
   explicit CommandLineOptionPrivate(const String &name)
      : m_names(removeInvalidNames(StringList(name)))
   {}
   
   PDK_NEVER_INLINE
   explicit CommandLineOptionPrivate(const StringList &names)
      : m_names(removeInvalidNames(names))
   {}
   
   static StringList removeInvalidNames(StringList nameList);
   StringList m_names;
   String m_valueName;
   String m_description;
   StringList m_defaultValues;
   CommandLineOption::Flags m_flags;
};
} // internal

CommandLineOption::CommandLineOption(const String &name)
   : m_implPtr(new CommandLineOptionPrivate(name))
{}

CommandLineOption::CommandLineOption(const StringList &names)
   : m_implPtr(new CommandLineOptionPrivate(names))
{
}

CommandLineOption::CommandLineOption(const String &name, const String &description,
                                     const String &valueName,
                                     const String &defaultValue)
   : m_implPtr(new CommandLineOptionPrivate(name))
{
   setValueName(valueName);
   setDescription(description);
   setDefaultValue(defaultValue);
}

CommandLineOption::CommandLineOption(const StringList &names, const String &description,
                                     const String &valueName,
                                     const String &defaultValue)
   : m_implPtr(new CommandLineOptionPrivate(names))
{
   setValueName(valueName);
   setDescription(description);
   setDefaultValue(defaultValue);
}

CommandLineOption::CommandLineOption(const CommandLineOption &other)
   : m_implPtr(other.m_implPtr)
{}

CommandLineOption::~CommandLineOption()
{}

CommandLineOption &CommandLineOption::operator=(const CommandLineOption &other)
{
   m_implPtr = other.m_implPtr;
   return *this;
}

StringList CommandLineOption::getNames() const
{
   return m_implPtr->m_names;
}

namespace {

struct IsInvalidName
{
   typedef bool result_type;
   typedef String argument_type;
   
   PDK_NEVER_INLINE
   result_type operator()(const String &name) const noexcept
   {
      if (PDK_UNLIKELY(name.isEmpty()))
         return warn("be empty");
      
      const Character c = name.at(0);
      if (PDK_UNLIKELY(c == Latin1Character('-'))) {
         return warn("start with a '-'");
      }
      if (PDK_UNLIKELY(c == Latin1Character('/'))) {
         return warn("start with a '/'");
      }
      if (PDK_UNLIKELY(name.contains(Latin1Character('=')))) {
         return warn("contain a '='");
      }
      return false;
   }
   
   PDK_NEVER_INLINE
   static bool warn(const char *what) noexcept
   {
      warning_stream("CommandLineOption: Option names cannot %s", what);
      return true;
   }
};

} // anonymous namespace

namespace internal {

StringList CommandLineOptionPrivate::removeInvalidNames(StringList nameList)
{
   if (PDK_UNLIKELY(nameList.empty())) {
      warning_stream("CommandLineOption: Options must have at least one name");
   } else {
      nameList.erase(std::remove_if(nameList.begin(), nameList.end(), IsInvalidName()),
                     nameList.end());
   }
   return nameList;
}

} // internal

void CommandLineOption::setValueName(const String &valueName)
{
   m_implPtr->m_valueName = valueName;
}

String CommandLineOption::getValueName() const
{
   return m_implPtr->m_valueName;
}

void CommandLineOption::setDescription(const String &description)
{
   m_implPtr->m_description = description;
}

String CommandLineOption::getDescription() const
{
   return m_implPtr->m_description;
}

void CommandLineOption::setDefaultValue(const String &defaultValue)
{
   StringList newDefaultValues;
   if (!defaultValue.isEmpty()) {
      newDefaultValues.resize(1);
      newDefaultValues << defaultValue;
   }
   // commit:
   m_implPtr->m_defaultValues.swap(newDefaultValues);
}

void CommandLineOption::setDefaultValues(const StringList &defaultValues)
{
   m_implPtr->m_defaultValues = defaultValues;
}

StringList CommandLineOption::getDefaultValues() const
{
   return m_implPtr->m_defaultValues;
}

CommandLineOption::Flags CommandLineOption::getFlags() const
{
   return m_implPtr->m_flags;
}

void CommandLineOption::setFlags(Flags flags)
{
   m_implPtr->m_flags = flags;
}

} // utils
} // pdk
