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

#ifndef PDK_M_BASE_UTILS_COMMANDLINE_PARSER_H
#define PDK_M_BASE_UTILS_COMMANDLINE_PARSER_H

#include "pdk/base/ds/StringList.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/utils/CommandLineOption.h"

namespace pdk {

// forward declare class with namespace
namespace kernel {
class CoreApplication;
} // kernel

namespace utils {

namespace internal {
class CommandLineParserPrivate;
} // internal

using internal::CommandLineParserPrivate;
using pdk::kernel::CoreApplication;

class PDK_CORE_EXPORT CommandLineParser
{
    PDK_DECLARE_TR_FUNCTIONS(CommandLineParser)
public:
    CommandLineParser();
    ~CommandLineParser();

    enum class SingleDashWordOptionMode
    {
        ParseAsCompactedShortOptions,
        ParseAsLongOptions
    };
    void setSingleDashWordOptionMode(SingleDashWordOptionMode parsingMode);

    enum class OptionsAfterPositionalArgumentsMode
    {
        ParseAsOptions,
        ParseAsPositionalArguments
    };
    void setOptionsAfterPositionalArgumentsMode(OptionsAfterPositionalArgumentsMode mode);

    bool addOption(const CommandLineOption &commandLineOption);
    bool addOptions(const std::list<CommandLineOption> &options);

    CommandLineOption addVersionOption();
    CommandLineOption addHelpOption();
    void setAppDescription(const String &description);
    String getAppDescription() const;
    void addPositionalArgument(const String &name, const String &description, const String &syntax = String());
    void clearPositionalArguments();

    void process(const StringList &arguments);
    void process(const CoreApplication &app);

    bool parse(const StringList &arguments);
    String getErrorText() const;

    bool isSet(const String &name) const;
    String getValue(const String &name) const;
    StringList getValues(const String &name) const;

    bool isSet(const CommandLineOption &option) const;
    String value(const CommandLineOption &option) const;
    StringList values(const CommandLineOption &option) const;

    StringList positionalArguments() const;
    StringList optionNames() const;
    StringList unknownOptionNames() const;

    PDK_NORETURN void showVersion();
    PDK_NORETURN void showHelp(int exitCode = 0);
    String getHelpText() const;

private:
    PDK_DISABLE_COPY(CommandLineParser);
    CommandLineParserPrivate * const m_implPtr;
};

} // utils
} // pdk

#endif // PDK_M_BASE_UTILS_COMMANDLINE_PARSER_H
