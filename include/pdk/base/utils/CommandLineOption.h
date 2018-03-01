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

#ifndef PDK_M_BASE_UTILS_COMMANDLINE_OPTION_H
#define PDK_M_BASE_UTILS_COMMANDLINE_OPTION_H

#include "pdk/base/ds/StringList.h"
#include "pdk/utils/SharedData.h"

namespace pdk {
namespace utils {

namespace internal {
class CommandLineOptionPrivate;
} // internal

using pdk::ds::StringList;
using pdk::ds::String;
using internal::CommandLineOptionPrivate;

class PDK_CORE_EXPORT CommandLineOption
{
public:
   enum class Flag {
      HiddenFromHelp = 0x1,
      ShortOptionStyle = 0x2
   };
   PDK_DECLARE_FLAGS(Flags, Flag);
   
   explicit CommandLineOption(const String &name);
   explicit CommandLineOption(const StringList &names);
   CommandLineOption(const String &name, const String &description,
                     const String &valueName = String(),
                     const String &defaultValue = String());
   CommandLineOption(const StringList &names, const String &description,
                     const String &valueName = String(),
                     const String &defaultValue = String());
   CommandLineOption(const CommandLineOption &other);
   
   ~CommandLineOption();
   
   CommandLineOption &operator=(const CommandLineOption &other);
   CommandLineOption &operator=(CommandLineOption &&other) noexcept
   {
      swap(other);
      return *this;
   }
   
   void swap(CommandLineOption &other) noexcept
   {
      std::swap(m_implPtr, other.m_implPtr);
   }
   
   StringList getNames() const;
   
   void setValueName(const String &name);
   String getValueName() const;
   
   void setDescription(const String &description);
   String getDescription() const;
   
   void setDefaultValue(const String &defaultValue);
   void setDefaultValues(const StringList &defaultValues);
   StringList getDefaultValues() const;
   
   Flags getFlags() const;
   void setFlags(Flags flags);
private:
   SharedDataPointer<CommandLineOptionPrivate> m_implPtr;
};

PDK_DECLARE_OPERATORS_FOR_FLAGS(CommandLineOption::Flags)

} // utils
} // pdk

PDK_DECLARE_SHARED(pdk::utils::CommandLineOption)

#endif // PDK_M_BASE_UTILS_COMMANDLINE_OPTION_H
