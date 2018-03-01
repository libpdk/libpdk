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

#include "pdk/base/utils/CommandLineParser.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/global/Logging.h"
#include <map>
#include <vector>
#if defined(PDK_OS_WIN)
#  include "pdk/global/Windows.h"
#endif

#include <cstdio>
#include <cstdlib>

namespace pdk {

// forward declare function
namespace kernel {
extern void call_post_routines();
} // kernel

namespace utils {

using pdk::kernel::call_post_routines;
using pdk::lang::String;
using pdk::lang::StringList;
using pdk::lang::Latin1Character;
using pdk::lang::Latin1String;
using pdk::lang::Character;

namespace {

String wrap_text(const String &names, int longestOptionNameString, const String &description);

} // anonymous namespace

namespace internal {

using NameMapType = std::map<String, int>;

class CommandLineParserPrivate
{
public:
   inline CommandLineParserPrivate()
      : m_singleDashWordOptionMode(CommandLineParser::SingleDashWordOptionMode::ParseAsCompactedShortOptions),
        m_optionsAfterPositionalArgumentsMode(CommandLineParser::OptionsAfterPositionalArgumentsMode::ParseAsOptions),
        m_builtinVersionOption(false),
        m_builtinHelpOption(false),
        m_needsParsing(true)
   {}
   
   bool parse(const StringList &args);
   void checkParsed(const char *method);
   StringList aliases(const String &name) const;
   String getHelpText() const;
   bool registerFoundOption(const String &optionName);
   bool parseOptionValue(const String &optionName, const String &argument,
                         StringList::const_iterator *argumentIterator,
                         StringList::const_iterator argsEnd);
   String m_errorText;
   std::vector<CommandLineOption> m_commandLineOptionList;
   NameMapType m_nameHash;
   std::map<int, StringList> m_optionValuesHash;
   StringList m_optionNames;
   StringList m_positionalArgumentList;
   StringList m_unknownOptionNames;
   String m_description;
   struct PositionalArgumentDefinition
   {
      String m_name;
      String m_description;
      String m_syntax;
   };
   std::vector<PositionalArgumentDefinition> m_positionalArgumentDefinitions;
   CommandLineParser::SingleDashWordOptionMode m_singleDashWordOptionMode;
   CommandLineParser::OptionsAfterPositionalArgumentsMode m_optionsAfterPositionalArgumentsMode;
   bool m_builtinVersionOption;
   bool m_builtinHelpOption;
   bool m_needsParsing;
};

StringList CommandLineParserPrivate::aliases(const String &optionName) const
{
   const NameMapType::const_iterator iter = std::as_const(m_nameHash).find(optionName);
   if (iter == m_nameHash.cend()) {
      warning_stream("CommandLineParser: option not defined: \"%s\"", pdk_printable(optionName));
      return StringList();
   }
   return m_commandLineOptionList.at(iter->second).getNames();
}

void CommandLineParserPrivate::checkParsed(const char *method)
{
   if (m_needsParsing) {
      warning_stream("CommandLineParser: call process() or parse() before %s", method);
   }
}

bool CommandLineParserPrivate::registerFoundOption(const String &optionName)
{
   if (m_nameHash.find(optionName) != m_nameHash.end()) {
      m_optionNames.push_back(optionName);
      return true;
   } else {
      m_unknownOptionNames.push_back(optionName);
      return false;
   }
}

bool CommandLineParserPrivate::parseOptionValue(const String &optionName, const String &argument,
                                                StringList::const_iterator *argumentIterator, StringList::const_iterator argsEnd)
{
   const Latin1Character assignChar('=');
   const NameMapType::const_iterator nameHashIter = std::as_const(m_nameHash).find(optionName);
   if (nameHashIter != m_nameHash.cend()) {
      const int assignPos = argument.indexOf(assignChar);
      const NameMapType::mapped_type optionOffset = nameHashIter->second;
      const bool withValue = !m_commandLineOptionList.at(optionOffset).getValueName().isEmpty();
      if (withValue) {
         if (assignPos == -1) {
            ++(*argumentIterator);
            if (*argumentIterator == argsEnd) {
               m_errorText = CommandLineParser::tr("Missing value after '%1'.").arg(argument);
               return false;
            }
            m_optionValuesHash[optionOffset].push_back(*(*argumentIterator));
         } else {
            m_optionValuesHash[optionOffset].push_back(argument.substring(assignPos + 1));
         }
      } else {
         if (assignPos != -1) {
            m_errorText = CommandLineParser::tr("Unexpected value after '%1'.").arg(argument.left(assignPos));
            return false;
         }
      }
   }
   return true;
}

bool CommandLineParserPrivate::parse(const StringList &args)
{
   m_needsParsing = false;
   bool error = false;
   const String     doubleDashString(StringLiteral("--"));
   const Latin1Character dashChar('-');
   const Latin1Character assignChar('=');
   
   bool forcePositional = false;
   m_errorText.clear();
   m_positionalArgumentList.clear();
   m_optionNames.clear();
   m_unknownOptionNames.clear();
   m_optionValuesHash.clear();
   
   if (args.empty()) {
      warning_stream("CommandLineParser: argument list cannot be empty, it should contain at least the executable name");
      return false;
   }
   
   StringList::const_iterator argumentIterator = args.begin();
   ++argumentIterator; // skip executable name
   
   for (; argumentIterator != args.end() ; ++argumentIterator) {
      String argument = *argumentIterator;
      
      if (forcePositional) {
         m_positionalArgumentList.push_back(argument);
      } else if (argument.startsWith(doubleDashString)) {
         if (argument.length() > 2) {
            String optionName = argument.substring(2).section(assignChar, 0, 0);
            if (registerFoundOption(optionName)) {
               if (!parseOptionValue(optionName, argument, &argumentIterator, args.end()))
                  error = true;
            } else {
               error = true;
            }
         } else {
            forcePositional = true;
         }
      } else if (argument.startsWith(dashChar)) {
         if (argument.size() == 1) { // single dash ("stdin")
            m_positionalArgumentList.push_back(argument);
            continue;
         }
         switch (m_singleDashWordOptionMode) {
         case CommandLineParser::SingleDashWordOptionMode::ParseAsCompactedShortOptions:
         {
            String optionName;
            bool valueFound = false;
            for (int pos = 1 ; pos < argument.size(); ++pos) {
               optionName = argument.substring(pos, 1);
               if (!registerFoundOption(optionName)) {
                  error = true;
               } else {
                  const NameMapType::const_iterator nameHashIter = std::as_const(m_nameHash).find(optionName);
                  PDK_ASSERT(nameHashIter != m_nameHash.cend()); // checked by registerFoundOption
                  const NameMapType::mapped_type optionOffset = nameHashIter->second;
                  const bool withValue = !m_commandLineOptionList.at(optionOffset).getValueName().isEmpty();
                  if (withValue) {
                     if (pos + 1 < argument.size()) {
                        if (argument.at(pos + 1) == assignChar) {
                           ++pos;
                        }
                        m_optionValuesHash[optionOffset].push_back(argument.substring(pos + 1));
                        valueFound = true;
                     }
                     break;
                  }
                  if (pos + 1 < argument.size() && argument.at(pos + 1) == assignChar) {
                     break;
                  }
               }
            }
            if (!valueFound && !parseOptionValue(optionName, argument, &argumentIterator, args.end()))
               error = true;
            break;
         }
         case CommandLineParser::SingleDashWordOptionMode::ParseAsLongOptions:
         {
            if (argument.size() > 2) {
               const String possibleShortOptionStyleName = argument.substring(1, 1);
               const auto shortOptionIter = std::as_const(m_nameHash).find(possibleShortOptionStyleName);
               if (shortOptionIter != m_nameHash.cend()) {
                  const auto &arg = m_commandLineOptionList.at(shortOptionIter->second);
                  if (arg.getFlags() & CommandLineOption::Flag::ShortOptionStyle) {
                     registerFoundOption(possibleShortOptionStyleName);
                     m_optionValuesHash[shortOptionIter->second].push_back(argument.substring(2));
                     break;
                  }
               }
            }
            const String optionName = argument.substring(1).section(assignChar, 0, 0);
            if (registerFoundOption(optionName)) {
               if (!parseOptionValue(optionName, argument, &argumentIterator, args.end()))
                  error = true;
            } else {
               error = true;
            }
            break;
         }
         }
      } else {
         m_positionalArgumentList.push_back(argument);
         if (m_optionsAfterPositionalArgumentsMode == CommandLineParser::OptionsAfterPositionalArgumentsMode::ParseAsPositionalArguments){
            forcePositional = true;
         } 
      }
      if (argumentIterator == args.end()) {
         break;
      }
   }
   return !error;
}

String CommandLineParserPrivate::getHelpText() const
{
   const Latin1Character nl('\n');
   String text;
   String usage;
   usage += CoreApplication::getInstance()->getArguments().front(); // executable name
   if (!m_commandLineOptionList.empty()) {
      usage += Latin1Character(' ') + CommandLineParser::tr("[options]");
   }
   for (const PositionalArgumentDefinition &arg : m_positionalArgumentDefinitions) {
      usage += Latin1Character(' ') + arg.m_syntax;
   }
   text += CommandLineParser::tr("Usage: %1").arg(usage) + nl;
   if (!m_description.isEmpty()) {
      text += m_description + nl;
   }
   text += nl;
   if (!m_commandLineOptionList.empty()) {
      text += CommandLineParser::tr("Options:") + nl;
   }
   
   StringList optionNameList;
   optionNameList.resize(m_commandLineOptionList.size());
   int longestOptionNameString = 0;
   for (const CommandLineOption &option : m_commandLineOptionList) {
      if (option.getFlags() & CommandLineOption::Flag::HiddenFromHelp)
         continue;
      const StringList optionNames = option.getNames();
      String optionNamesString;
      for (const String &optionName : optionNames) {
         const int numDashes = optionName.length() == 1 ? 1 : 2;
         optionNamesString += Latin1String("--", numDashes) + optionName + Latin1String(", ");
      }
      if (!m_optionNames.empty()) {
         optionNamesString.chop(2); // remove trailing ", "
      }
      const auto valueName = option.getValueName();
      if (!valueName.isEmpty()) {
         optionNamesString += Latin1String(" <") + valueName + Latin1Character('>');
      }        
      optionNameList.push_back(optionNamesString);
      longestOptionNameString = std::max(longestOptionNameString, optionNamesString.length());
   }
   ++longestOptionNameString;
   auto optionNameIterator = optionNameList.cbegin();
   for (const CommandLineOption &option : m_commandLineOptionList) {
      if (option.getFlags() & CommandLineOption::Flag::HiddenFromHelp) {
         continue;
      }
      text += wrap_text(*optionNameIterator, longestOptionNameString, option.getDescription());
      ++optionNameIterator;
   }
   if (!m_positionalArgumentDefinitions.empty()) {
      if (!m_commandLineOptionList.empty()) {
         text += nl;
      }
      text += CommandLineParser::tr("Arguments:") + nl;
      for (const PositionalArgumentDefinition &arg : m_positionalArgumentDefinitions) {
         text += wrap_text(arg.m_name, longestOptionNameString, arg.m_description);
      }
   }
   return text;
}


} // internal

CommandLineParser::CommandLineParser()
   : m_implPtr(new CommandLineParserPrivate)
{}

CommandLineParser::~CommandLineParser()
{
   delete m_implPtr;
}

void CommandLineParser::setSingleDashWordOptionMode(CommandLineParser::SingleDashWordOptionMode singleDashWordOptionMode)
{
   m_implPtr->m_singleDashWordOptionMode = singleDashWordOptionMode;
}

void CommandLineParser::setOptionsAfterPositionalArgumentsMode(CommandLineParser::OptionsAfterPositionalArgumentsMode parsingMode)
{
   m_implPtr->m_optionsAfterPositionalArgumentsMode = parsingMode;
}

bool CommandLineParser::addOption(const CommandLineOption &option)
{
   const StringList optionNames = option.getNames();
   
   if (!optionNames.empty()) {
      for (const String &name : optionNames) {
         if (m_implPtr->m_nameHash.find(name) != m_implPtr->m_nameHash.end()) {
            return false;
         }            
      }
      m_implPtr->m_commandLineOptionList.push_back(option);
      const int offset = m_implPtr->m_commandLineOptionList.size() - 1;
      for (const String &name : optionNames) {
         m_implPtr->m_nameHash[name] = offset;
      }
      return true;
   }
   
   return false;
}

bool CommandLineParser::addOptions(const std::list<CommandLineOption> &options)
{
   // should be optimized (but it's no worse than what was possible before)
   bool result = true;
   for (std::list<CommandLineOption>::const_iterator iter = options.begin(), end = options.end(); iter != end; ++iter) {
      result &= addOption(*iter);
   }
   return result;
}

CommandLineOption CommandLineParser::addVersionOption()
{
   CommandLineOption opt(StringList() << StringLiteral("v") << StringLiteral("version"), tr("Displays version information."));
   addOption(opt);
   m_implPtr->m_builtinVersionOption = true;
   return opt;
}

CommandLineOption CommandLineParser::addHelpOption()
{
   CommandLineOption opt(StringList()
                      #ifdef PDK_OS_WIN
                         << StringLiteral("?")
                      #endif
                         << StringLiteral("h")
                         << StringLiteral("help"), tr("Displays this help."));
   addOption(opt);
   m_implPtr->m_builtinHelpOption = true;
   return opt;
}

void CommandLineParser::setAppDescription(const String &description)
{
   m_implPtr->m_description = description;
}

String CommandLineParser::getAppDescription() const
{
   return m_implPtr->m_description;
}

void CommandLineParser::addPositionalArgument(const String &name, const String &description, const String &syntax)
{
   CommandLineParserPrivate::PositionalArgumentDefinition arg;
   arg.m_name = name;
   arg.m_description = description;
   arg.m_syntax = syntax.isEmpty() ? name : syntax;
   m_implPtr->m_positionalArgumentDefinitions.push_back(arg);
}

void CommandLineParser::clearPositionalArguments()
{
   m_implPtr->m_positionalArgumentDefinitions.clear();
}

bool CommandLineParser::parse(const StringList &arguments)
{
   return m_implPtr->parse(arguments);
}

String CommandLineParser::getErrorText() const
{
   if (!m_implPtr->m_errorText.isEmpty())
      return m_implPtr->m_errorText;
   if (m_implPtr->m_unknownOptionNames.size() == 1) {
      return tr("Unknown option '%1'.").arg(m_implPtr->m_unknownOptionNames.front());
   }
   if (m_implPtr->m_unknownOptionNames.size() > 1) {
      return tr("Unknown options: %1.").arg(m_implPtr->m_unknownOptionNames.join(StringLiteral(", ")));
   }
   return String();
}

enum MessageType { UsageMessage, ErrorMessage };

namespace {

#if defined(PDK_OS_WIN)
// Return whether to use a message box. Use handles if a console can be obtained
// or we are run with redirected handles (for example, by QProcess).
inline bool display_message_box()
{
   if (GetConsoleWindow()) {
      return false;
   } 
   STARTUPINFO startupInfo;
   startupInfo.cb = sizeof(STARTUPINFO);
   GetStartupInfo(&startupInfo);
   return !(startupInfo.dwFlags & STARTF_USESTDHANDLES);
}
#endif // PDK_OS_WIN

void show_parser_message(const String &message, MessageType type)
{
#if defined(PDK_OS_WIN)
   if (display_message_box()) {
      const UINT flags = MB_OK | MB_TOPMOST | MB_SETFOREGROUND
            | (type == UsageMessage ? MB_ICONINFORMATION : MB_ICONERROR);
      String title;
      if (CoreApplication::getInstance()) {
         title = CoreApplication::getInstance()->getAppDisplayName.toString();
      }
      
      if (title.isEmpty()) {
         title = CoreApplication::getAppName();
      }
      MessageBoxW(0, reinterpret_cast<const wchar_t *>(message.utf16()),
                  reinterpret_cast<const wchar_t *>(title.utf16()), flags);
      return;
   }
#endif // PDK_OS_WIN
   fputs(pdk_printable(message), type == UsageMessage ? stdout : stderr);
}

String wrap_text(const String &names, int longestOptionNameString, const String &description)
{
   const Latin1Character nl('\n');
   String text = Latin1String("  ") + names.leftJustified(longestOptionNameString) + Latin1Character(' ');
   const int indent = text.length();
   int lineStart = 0;
   int lastBreakable = -1;
   const int max = 79 - indent;
   int x = 0;
   const int len = description.length();
   
   for (int i = 0; i < len; ++i) {
      ++x;
      const Character c = description.at(i);
      if (c.isSpace()) {
         lastBreakable = i;
      }
      int breakAt = -1;
      int nextLineStart = -1;
      if (x > max && lastBreakable != -1) {
         // time to break and we know where
         breakAt = lastBreakable;
         nextLineStart = lastBreakable + 1;
      } else if ((x > max - 1 && lastBreakable == -1) || i == len - 1) {
         // time to break but found nowhere [-> break here], or end of last line
         breakAt = i + 1;
         nextLineStart = breakAt;
      } else if (c == nl) {
         // forced break
         breakAt = i;
         nextLineStart = i + 1;
      }
      
      if (breakAt != -1) {
         const int numChars = breakAt - lineStart;
         //debug_warning() << "breakAt=" << m_description.at(breakAt) << "breakAtSpace=" << breakAtSpace << lineStart << "to" << breakAt << m_description.substring(lineStart, numChars);
         if (lineStart > 0) {
            text += String(indent, Latin1Character(' '));
         }
         text += description.substringRef(lineStart, numChars) + nl;
         x = 0;
         lastBreakable = -1;
         lineStart = nextLineStart;
         if (lineStart < len && description.at(lineStart).isSpace()) {
            ++lineStart; // don't start a line with a space
         }
         i = lineStart;
      }
   }
   
   return text;
}

} // anonymous namespace

void CommandLineParser::process(const StringList &arguments)
{
   if (!m_implPtr->parse(arguments)) {
      show_parser_message(getErrorText() + Latin1Character('\n'), ErrorMessage);
      call_post_routines();
      ::exit(EXIT_FAILURE);
   }
   
   if (m_implPtr->m_builtinVersionOption && isSet(StringLiteral("version"))) {
      showVersion();
   }
   
   if (m_implPtr->m_builtinHelpOption && isSet(StringLiteral("help"))) {
      showHelp(EXIT_SUCCESS);
   }
}

void CommandLineParser::process(const CoreApplication &app)
{
   // CoreApplication::getArguments() is static, but the app instance must exist so we require it as parameter
   PDK_UNUSED(app);
   process(CoreApplication::getArguments());
}



bool CommandLineParser::isSet(const String &name) const
{
   m_implPtr->checkParsed("isSet");
   if (m_implPtr->m_optionNames.contains(name)) {
      return true;
   }
   const StringList aliases = m_implPtr->aliases(name);
   for (const String &optionName : std::as_const(m_implPtr->m_optionNames)) {
      if (aliases.contains(optionName))
         return true;
   }
   return false;
}

String CommandLineParser::getValue(const String &optionName) const
{
   m_implPtr->checkParsed("value");
   const StringList valueList = getValues(optionName);
   if (!valueList.empty()) {
      return valueList.back();
   }
   return String();
}

StringList CommandLineParser::getValues(const String &optionName) const
{
   m_implPtr->checkParsed("values");
   const internal::NameMapType::const_iterator iter = std::as_const(m_implPtr->m_nameHash).find(optionName);
   if (iter != m_implPtr->m_nameHash.cend()) {
      const int optionOffset = iter->second;
      StringList values = m_implPtr->m_optionValuesHash.at(optionOffset);
      if (values.empty()) {
         values = m_implPtr->m_commandLineOptionList.at(optionOffset).getDefaultValues();
      }
      return values;
   }
   warning_stream("CommandLineParser: option not defined: \"%s\"", pdk_printable(optionName));
   return StringList();
}

bool CommandLineParser::isSet(const CommandLineOption &option) const
{
   // option.names() might be empty if the constructor failed
   const auto names = option.getNames();
   return !names.empty() && isSet(names.front());
}

String CommandLineParser::value(const CommandLineOption &option) const
{
   return getValue(option.getNames().front());
}

StringList CommandLineParser::values(const CommandLineOption &option) const
{
   return getValues(option.getNames().front());
}

StringList CommandLineParser::positionalArguments() const
{
   m_implPtr->checkParsed("positionalArguments");
   return m_implPtr->m_positionalArgumentList;
}

StringList CommandLineParser::optionNames() const
{
   m_implPtr->checkParsed("optionNames");
   return m_implPtr->m_optionNames;
}

StringList CommandLineParser::unknownOptionNames() const
{
   m_implPtr->checkParsed("unknownOptionNames");
   return m_implPtr->m_unknownOptionNames;
}

PDK_NORETURN void CommandLineParser::showVersion()
{
   show_parser_message(CoreApplication::getAppName() + Latin1Character(' ')
                       + CoreApplication::getAppVersion() + Latin1Character('\n'),
                       UsageMessage);
   call_post_routines();
   ::exit(EXIT_SUCCESS);
}

PDK_NORETURN void CommandLineParser::showHelp(int exitCode)
{
   show_parser_message(m_implPtr->getHelpText(), UsageMessage);
   call_post_routines();
   ::exit(exitCode);
}

String CommandLineParser::getHelpText() const
{
   return m_implPtr->getHelpText();
}

} // utils
} // pdk

PDK_DECLARE_TYPEINFO(pdk::utils::internal::CommandLineParserPrivate::PositionalArgumentDefinition, PDK_MOVABLE_TYPE);
