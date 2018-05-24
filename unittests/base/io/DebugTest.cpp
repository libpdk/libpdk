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
// Created by zzu_softboy on 2018/05/24.

#include "gtest/gtest.h"
#include "pdk/kernel/CoreApplication.h"
#include "pdk/base/io/Debug.h"
#include "pdktest/PdkTest.h"
#include "pdk/base/lang/String.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/base/ds/StringList.h"
#include "pdk/global/Logging.h"

using pdk::io::Debug;
using pdk::lang::String;
using pdk::lang::Latin1Character;
using pdk::ds::ByteArray;
using pdk::lang::Character;
using pdk::lang::Latin1String;
using pdk::io::DebugStateSaver;
using pdk::ds::StringList;
using pdk::lang::StringRef;
using pdk::lang::StringView;

PDKTEST_DECLARE_APP_STARTUP_ARGS();

TEST(DebugTest, testAssignment)
{
   Debug debug1(pdk::MsgType::DebugMsg);
   Debug debug2(pdk::MsgType::WarningMsg);
   debug1 << "foo";
   debug2 << "bar";
   debug1 = debug2;
   debug1 << "1";
   debug2 << "2";
}

static pdk::MsgType sg_msgType;
static String sg_msg;
static ByteArray sg_file;
static int sg_line;
static ByteArray sg_function;

namespace {

void my_message_handler(pdk::MsgType type, const pdk::MessageLogContext &context, const String &msg)
{
   sg_msg = msg;
   sg_msgType = type;
   sg_file = context.m_file;
   sg_line = context.m_line;
   sg_function = context.m_function;
}

class MessageHandlerSetter
{
public:
   MessageHandlerSetter(pdk::MessageHandler newMessageHandler)
      : m_oldMessageHandler(pdk::install_message_handler(newMessageHandler))
   {}
   
   ~MessageHandlerSetter()
   {
      pdk::install_message_handler(m_oldMessageHandler);
   }
   
private:
   pdk::MessageHandler m_oldMessageHandler;
};

} // anonymous namespace

TEST(DebugTest, testCriticalWithoutDebug)
{
   String file;
   String function;
   int line = 0;
   MessageHandlerSetter mhs(my_message_handler);
   { critical_stream() << "A critical_stream() message"; }
#ifndef PDK_NO_MESSAGELOGCONTEXT
   file = Latin1String(__FILE__); line = __LINE__ - 2; function = Latin1String(PDK_FUNC_INFO);
#endif
   ASSERT_EQ(sg_msgType, pdk::MsgType::CriticalMsg);
   ASSERT_EQ(sg_msg, String::fromLatin1("A critical_stream() message"));
   ASSERT_EQ(String::fromLatin1(sg_file), file);
   ASSERT_EQ(sg_line, line);
   ASSERT_EQ(String::fromLatin1(sg_function), function);
}

TEST(DebugTest, testDebugWithBool)
{
   String file, function;
   int line = 0;
   MessageHandlerSetter mhs(my_message_handler);
   { debug_stream() << false << true; }
#ifndef PDK_NO_MESSAGELOGCONTEXT
   file = Latin1String(__FILE__); line = __LINE__ - 2; function = Latin1String(PDK_FUNC_INFO);
#endif
   ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
   ASSERT_EQ(sg_msg, String::fromLatin1("false true"));
   ASSERT_EQ(String::fromLatin1(sg_file), file);
   ASSERT_EQ(sg_line, line);
   ASSERT_EQ(String::fromLatin1(sg_function), function);
}

namespace {

class MyPoint
{
public:
   MyPoint(int val1, int val2)
      : m_v1(val1), m_v2(val2) {}
   int m_v1;
   int m_v2;
};
Debug operator<< (Debug s, const MyPoint& point)
{
   const DebugStateSaver saver(s);
   s.nospace() << "MyPoint(" << point.m_v1 << ", " << point.m_v2 << ")";
   return s;
}

class MyLine
{
public:
   MyLine(const MyPoint& point1, const MyPoint& point2)
      : m_p1(point1), m_p2(point2) {}
   MyPoint m_p1;
   MyPoint m_p2;
};
Debug operator<< (Debug s, const MyLine& line)
{
   const DebugStateSaver saver(s);
   s.nospace();
   s << "MyLine(" << line.m_p1 << ", "<< line.m_p2;
   if (s.verbosity() > 2)
      s << ", Manhattan length=" << (std::abs(line.m_p2.m_v1 - line.m_p1.m_v1) + std::abs(line.m_p2.m_v2 - line.m_p1.m_v2));
   s << ')';
   return s;
}

} // anonymous namespace

TEST(DebugTest, testDebugSpaceHandling)
{
   MessageHandlerSetter mhs(my_message_handler);
   {
      Debug d = debug_stream();
      ASSERT_TRUE(d.autoInsertSpaces());
      d.setAutoInsertSpaces(false);
      ASSERT_TRUE(!d.autoInsertSpaces());
      d << "  ";
      d.setAutoInsertSpaces(true);
      ASSERT_TRUE(d.autoInsertSpaces());
      d << "foo";
      d.nospace();
      d << "key=" << "value";
      d.space();
      d << 1 << 2;
      MyLine line(MyPoint(10, 11), MyPoint (12, 13));
      d << line;
      d << "bar";
      // With the old implementation of MyPoint doing dbg.nospace() << ...; dbg.space() we ended up with
      // MyLine(MyPoint(10, 11) ,  MyPoint(12, 13) )
   }
   ASSERT_EQ(sg_msg, String::fromLatin1("  foo key=value 1 2 MyLine(MyPoint(10, 11), MyPoint(12, 13)) bar"));
   ASSERT_TRUE(debug_stream().autoInsertSpaces());
}

TEST(DebugTest, testDebugNoQuotes)
{
   MessageHandlerSetter mhs(my_message_handler);
   {
      Debug d = debug_stream();
      d << StringLiteral("Hello");
      d.noquote();
      d << StringLiteral("Hello");
      d.quote();
      d << StringLiteral("Hello");
   }
   ASSERT_EQ(sg_msg, String::fromLatin1("\"Hello\" Hello \"Hello\""));
   
   {
      Debug d = debug_stream();
      d << Character('H');
      d << Latin1String("Hello");
      d << ByteArray("Hello");
      d.noquote();
      d << Character('H');
      d << Latin1String("Hello");
      d << ByteArray("Hello");
   }
   ASSERT_EQ(sg_msg, String::fromLatin1("'H' \"Hello\" \"Hello\" H Hello Hello"));
}

TEST(DebugTest, testVerbosity)
{
   MyLine line(MyPoint(10, 11), MyPoint (12, 13));
   String output;
   Debug d(&output);
   d.nospace();
   d << line << '\n';
   const int oldVerbosity = d.verbosity();
   d.setVerbosity(0);
   ASSERT_EQ(d.verbosity(), 0);
   d.setVerbosity(7);
   ASSERT_EQ(d.verbosity(), 7);
   const int newVerbosity = oldVerbosity  + 2;
   d.setVerbosity(newVerbosity);
   ASSERT_EQ(d.verbosity(), newVerbosity);
   d << line << '\n';
   d.setVerbosity(oldVerbosity );
   ASSERT_EQ(d.verbosity(), oldVerbosity);
   d << line;
   const StringList lines = output.split(Latin1Character('\n'));
   ASSERT_EQ(lines.size(), 3u);
   // Verbose should be longer
   ASSERT_TRUE(lines.at(1).size() > lines.at(0).size()) << pdk_printable(lines.join(Latin1Character(',')));
   // Switching back to brief produces same output
   ASSERT_EQ(lines.at(0).size(), lines.at( 2).size());
}

TEST(DebugTest, testStateSaver)
{
   MessageHandlerSetter mhs(my_message_handler);
   {
      Debug d = debug_stream();
      d << 42;
      {
         DebugStateSaver saver(d);
         d << 43;
      }
      d << 44;
   }
   ASSERT_EQ(sg_msg, String::fromLatin1("42 43 44"));
   
   {
      Debug d = debug_stream();
      {
         DebugStateSaver saver(d);
         d.nospace() << pdk::io::hex << pdk::io::right << pdk::io::set_field_width(3) << pdk::io::set_pad_char('0') << 42;
      }
      d << 42;
   }
   ASSERT_EQ(sg_msg, String::fromLatin1("02a 42"));
   {
      Debug d = debug_stream();
      {
         DebugStateSaver saver(d);
         d.nospace().noquote() << StringLiteral("Hello");
      }
      d << StringLiteral("World");
   }
   ASSERT_EQ(sg_msg, String::fromLatin1("Hello \"World\""));
   
   {
      Debug d = debug_stream();
      d.noquote().nospace() << StringLiteral("Hello") << pdk::io::hex << 42;
      {
         DebugStateSaver saver(d);
         d.resetFormat();
         d << StringLiteral("World") << 42;
      }
      d << StringLiteral("!") << 42;
   }
   ASSERT_EQ(sg_msg, String::fromLatin1("Hello2a\"World\" 42!2a"));
}

TEST(DebugTest, testVeryLongWarningMessage)
{
   String file, function;
   int line = 0;
   MessageHandlerSetter mhs(my_message_handler);
   String test;
   {
      String part(Latin1String("0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\n"));
      for (int i = 0; i < 1000; ++i) {
         test.append(part);
      }
      warning_stream("Test output:\n%s\nend", pdk_printable(test));
   }
#ifndef PDK_NO_MESSAGELOGCONTEXT
   file = Latin1String(__FILE__); line = __LINE__ - 3; function = Latin1String(PDK_FUNC_INFO);
#endif
   ASSERT_EQ(sg_msgType, pdk::MsgType::WarningMsg);
   ASSERT_EQ(sg_msg, String::fromLatin1("Test output:\n") + test + String::fromLatin1("\nend"));
   ASSERT_EQ(String::fromLatin1(sg_file), file);
   ASSERT_EQ(sg_line, line);
   ASSERT_EQ(String::fromLatin1(sg_function), function);
}

TEST(DebugTest, testDebugCharacter)
{
   String file, function;
   int line = 0;
   MessageHandlerSetter mhs(my_message_handler);
   {
      Debug d = debug_stream();
      d << Character('f') << Character(Latin1Character('\xE4')); // f, ä
      d.nospace().noquote() << Character('o') << Character('o')  << Character(Latin1Character('\xC4')); // o, o, Ä
   }
#ifndef PDK_NO_MESSAGELOGCONTEXT
   file = Latin1String(__FILE__); line = __LINE__ - 5; function = Latin1String(PDK_FUNC_INFO);
#endif
   ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
   ASSERT_EQ(sg_msg, String::fromLatin1("'f' '\\u00e4' oo\\u00c4"));
   ASSERT_EQ(String::fromLatin1(sg_file), file);
   ASSERT_EQ(sg_line, line);
   ASSERT_EQ(String::fromLatin1(sg_function), function);
}

TEST(DebugTest, testDebugString)
{
   /* Use a basic string. */
   {
      String file, function;
      int line = 0;
      const String in(Latin1String("input"));
      const StringRef inRef(&in);
      
      MessageHandlerSetter mhs(my_message_handler);
      { debug_stream() << inRef; }
#ifndef PDK_NO_MESSAGELOGCONTEXT
      file = Latin1String(__FILE__); line = __LINE__ - 2; function = Latin1String(PDK_FUNC_INFO);
#endif
      ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
      ASSERT_EQ(sg_msg, String::fromLatin1("\"input\""));
      ASSERT_EQ(String::fromLatin1(sg_file), file);
      ASSERT_EQ(sg_line, line);
      ASSERT_EQ(String::fromLatin1(sg_function), function);
   }
   
   /* simpler tests from now on */
   MessageHandlerSetter mhs(my_message_handler);
   
   String string = Latin1String("Hello");
   debug_stream() << string;
   ASSERT_EQ(sg_msg, String(Latin1String("\"Hello\"")));
   
   debug_stream().noquote().nospace() << string;
   ASSERT_EQ(sg_msg, string);
   
   debug_stream().noquote().nospace() << pdk::io::set_field_width(8) << string;
   ASSERT_EQ(sg_msg, Latin1String("   ") + string);
   
   string = String::fromLocal8Bit("Sm\xc3\xb8rg\xc3\xa5sbord "                               // Latin script
                                  "\xce\x91\xce\xb8\xce\xae\xce\xbd\xce\xb1 "                // Greek script
                                  "\xd0\x9c\xd0\xbe\xd1\x81\xd0\xba\xd0\xb2\xd0\xb0");        // Cyrillic script
   debug_stream().noquote().nospace() << string;
   ASSERT_EQ(sg_msg, string);
   
   // This string only contains printable characters
   debug_stream() << string;
   ASSERT_EQ(sg_msg, '"' + string + '"');
   
   string = String::fromLocal8Bit("\n\t\\\"");
   debug_stream().noquote().nospace() << string;
   ASSERT_EQ(sg_msg, string);
   
   // This string only contains characters that must be escaped
   debug_stream() << string;
   ASSERT_EQ(sg_msg, String(Latin1String("\"\\n\\t\\\\\\\"\"")));
   
   // Unicode escapes, BMP
   string = String::fromLocal8Bit("\1"                           // U+0001: START OF HEADING (category Cc)
                                  "\x7f"                         // U+007F: DELETE (category Cc)
                                  "\xc2\xad"                     // U+00AD: SOFT HYPHEN (category Cf)
                                  "\xef\xbb\xbf");                // U+FEFF: ZERO WIDTH NO-BREAK SPACE / BOM (category Cf)
   debug_stream() << string;
   ASSERT_EQ(sg_msg, String(Latin1String("\"\\u0001\\u007F\\u00AD\\uFEFF\"")));
   
   // Unicode printable non-BMP
   string = String::fromLocal8Bit("\xf0\x90\x80\x80");            // U+10000: LINEAR B SYLLABLE B008 A (category Lo)
   debug_stream() << string;
   ASSERT_EQ(sg_msg, '"' + string + '"');
   
   // non-BMP and non-printable
   string = String::fromLocal8Bit("\xf3\xa0\x80\x81 "            // U+E0001: LANGUAGE TAG (category Cf)
                                  "\xf4\x80\x80\x80");            // U+100000: Plane 16 Private Use (category Co)
   debug_stream() << string;
   ASSERT_EQ(sg_msg, String(Latin1String("\"\\U000E0001 \\U00100000\"")));
   
   // broken surrogate pairs
   ushort utf16[] = { 0xDC00, 0xD800, 'x', 0xD800, 0 };
   string = String::fromUtf16(utf16);
   debug_stream() << string;
   ASSERT_EQ(sg_msg, String(Latin1String("\"\\uDC00\\uD800x\\uD800\"")));
}

TEST(DebugTest, testDebugStringRef)
{
   /* Use a basic string. */
   {
      String file, function;
      int line = 0;
      const String in(Latin1String("input"));
      const StringRef inRef(&in);
      
      MessageHandlerSetter mhs(my_message_handler);
      { debug_stream() << inRef; }
#ifndef PDK_NO_MESSAGELOGCONTEXT
      file = Latin1String(__FILE__); line = __LINE__ - 2; function = Latin1String(PDK_FUNC_INFO);
#endif
      ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
      ASSERT_EQ(sg_msg, String::fromLatin1("\"input\""));
      ASSERT_EQ(String::fromLatin1(sg_file), file);
      ASSERT_EQ(sg_line, line);
      ASSERT_EQ(String::fromLatin1(sg_function), function);
   }
   
   /* Use a null StringRef. */
   {
      String file, function;
      int line = 0;
      
      const StringRef inRef;
      
      MessageHandlerSetter mhs(my_message_handler);
      { debug_stream() << inRef; }
#ifndef PDK_NO_MESSAGELOGCONTEXT
      file = Latin1String(__FILE__); line = __LINE__ - 2; function = Latin1String(PDK_FUNC_INFO);
#endif
      ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
      ASSERT_EQ(sg_msg, String::fromLatin1("\"\""));
      ASSERT_EQ(String::fromLatin1(sg_file), file);
      ASSERT_EQ(sg_line, line);
      ASSERT_EQ(String::fromLatin1(sg_function), function);
   }
}

TEST(DebugTest, testDebugStreamStringView)
{
   /* Use a basic string. */
   {
      Latin1String file, function;
      int line = 0;
      const StringView inView = StringViewLiteral("input");
      
      MessageHandlerSetter mhs(my_message_handler);
      { debug_stream() << inView; }
#ifndef PDK_NO_MESSAGELOGCONTEXT
      file = Latin1String(__FILE__); line = __LINE__ - 2; function = Latin1String(PDK_FUNC_INFO);
#endif
      ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
      ASSERT_EQ(sg_msg, Latin1String("\"input\""));
      ASSERT_EQ(Latin1String(sg_file), file);
      ASSERT_EQ(sg_line, line);
      ASSERT_EQ(Latin1String(sg_function), function);
   }
   
   /* Use a null StringView. */
   {
      String file, function;
      int line = 0;
      
      const StringView inView;
      
      MessageHandlerSetter mhs(my_message_handler);
      { debug_stream() << inView; }
#ifndef PDK_NO_MESSAGELOGCONTEXT
      file = Latin1String(__FILE__); line = __LINE__ - 2; function = Latin1String(PDK_FUNC_INFO);
#endif
      ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
      ASSERT_EQ(sg_msg, Latin1String("\"\""));
      ASSERT_EQ(Latin1String(sg_file), file);
      ASSERT_EQ(sg_line, line);
      ASSERT_EQ(Latin1String(sg_function), function);
   }
}

TEST(DebugTest, testDebugStreamLatin1String)
{
   String file, function;
   int line = 0;
   MessageHandlerSetter mhs(my_message_handler);
   {
      Debug d = debug_stream();
      d << Latin1String("foo") << Latin1String("") << Latin1String("barbaz", 3);
      d.nospace().noquote() << Latin1String("baz");
   }
#ifndef PDK_NO_MESSAGELOGCONTEXT
   file = Latin1String(__FILE__); line = __LINE__ - 5; function = Latin1String(PDK_FUNC_INFO);
#endif
   ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
   ASSERT_EQ(sg_msg, String::fromLatin1("\"foo\" \"\" \"bar\" baz"));
   ASSERT_EQ(String::fromLatin1(sg_file), file);
   ASSERT_EQ(sg_line, line);
   ASSERT_EQ(String::fromLatin1(sg_function), function);
   
   /* simpler tests from now on */
   Latin1String string("\"Hello\"");
   debug_stream() << string;
   ASSERT_EQ(sg_msg, String(Latin1String("\"\\\"Hello\\\"\"")));
   
   debug_stream().noquote().nospace() << string;
   ASSERT_EQ(sg_msg, String(string));
   
   debug_stream().noquote().nospace() << pdk::io::set_field_width(8) << string;
   ASSERT_EQ(sg_msg, Latin1String(" ") + String(string));
   
   string = Latin1String("\nSm\xF8rg\xE5sbord\\");
   debug_stream().noquote().nospace() << string;
   ASSERT_EQ(sg_msg, String(string));
   
   debug_stream() << string;
   ASSERT_EQ(sg_msg, String(Latin1String("\"\\nSm\\u00F8rg\\u00E5sbord\\\\\"")));
}

TEST(DebugTest, testDebugStreamByteArray)
{
   String file, function;
   int line = 0;
   MessageHandlerSetter mhs(my_message_handler);
   {
      Debug d = debug_stream();
      d << ByteArrayLiteral("foo") << ByteArrayLiteral("") << ByteArray("barbaz", 3);
      d.nospace().noquote() << ByteArrayLiteral("baz");
   }
#ifndef PDK_NO_MESSAGELOGCONTEXT
   file = Latin1String(__FILE__); line = __LINE__ - 5; function = Latin1String(PDK_FUNC_INFO);
#endif
   ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
   ASSERT_EQ(sg_msg, String::fromLatin1("\"foo\" \"\" \"bar\" baz"));
   ASSERT_EQ(String::fromLatin1(sg_file), file);
   ASSERT_EQ(sg_line, line);
   ASSERT_EQ(String::fromLatin1(sg_function), function);
   
   /* simpler tests from now on */
   ByteArray ba = "\"Hello\"";
   debug_stream() << ba;
   ASSERT_EQ(sg_msg, String(Latin1String("\"\\\"Hello\\\"\"")));
   
   debug_stream().noquote().nospace() << ba;
   ASSERT_EQ(sg_msg, String::fromLatin1(ba));
   
   debug_stream().noquote().nospace() << pdk::io::set_field_width(8) << ba;
   ASSERT_EQ(sg_msg, Latin1String(" ") + String::fromLatin1(ba));
   
   ba = "\nSm\xC3\xB8rg\xC3\xA5sbord\\";
   debug_stream().noquote().nospace() << ba;
   ASSERT_EQ(sg_msg, String::fromUtf8(ba));
   
   debug_stream() << ba;
   ASSERT_EQ(sg_msg, String(Latin1String("\"\\nSm\\xC3\\xB8rg\\xC3\\xA5sbord\\\\\"")));
   
   // ensure that it closes hex escape sequences correctly
   debug_stream() << ByteArray("\377FFFF");
   ASSERT_EQ(sg_msg, String(Latin1String("\"\\xFF\"\"FFFF\"")));
}

enum TestEnum {
   Flag1 = 0x1,
   Flag2 = 0x10
};

PDK_DECLARE_FLAGS(TestFlags, TestEnum);

TEST(DebugTest, testDebugFlags)
{
   String file;
   String function;
   int line = 0;
   pdk::Flags<TestEnum> flags(Flag1 | Flag2);
   
   MessageHandlerSetter mhs(my_message_handler);
   { debug_stream() << flags; }
#ifndef PDK_NO_MESSAGELOGCONTEXT
   file = Latin1String(__FILE__); line = __LINE__ - 2; function = Latin1String(PDK_FUNC_INFO);
#endif
   ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
   ASSERT_EQ(sg_msg, String::fromLatin1("Flags(0x1|0x10)"));
   ASSERT_EQ(String::fromLatin1(sg_file), file);
   ASSERT_EQ(sg_line, line);
   ASSERT_EQ(String::fromLatin1(sg_function), function);
}

TEST(DebugTest, testTextStreamModifiers)
{
   String file, function;
   int line = 0;
   MessageHandlerSetter mhs(my_message_handler);
   { debug_stream() << pdk::io::hex << short(0xf) << int(0xf) << unsigned(0xf) << long(0xf) << pdk::pint64(0xf) << pdk::puint64(0xf); }
#ifndef PDK_NO_MESSAGELOGCONTEXT
   file = Latin1String(__FILE__); line = __LINE__ - 2; function = Latin1String(PDK_FUNC_INFO);
#endif
   ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
   ASSERT_EQ(sg_msg, String::fromLatin1("f f f f f f"));
   ASSERT_EQ(String::fromLatin1(sg_file), file);
   ASSERT_EQ(sg_line, line);
   ASSERT_EQ(String::fromLatin1(sg_function), function);
}

TEST(DebugTest, testResetFormat)
{
   String file, function;
   int line = 0;
   MessageHandlerSetter mhs(my_message_handler);
   {
      Debug d = debug_stream();
      d.nospace().noquote() << pdk::io::hex <<  int(0xf);
      d.resetFormat() << int(0xf) << StringLiteral("foo");
   }
#ifndef PDK_NO_MESSAGELOGCONTEXT
   file = Latin1String(__FILE__); line = __LINE__ - 5; function = Latin1String(PDK_FUNC_INFO);
#endif
   ASSERT_EQ(sg_msgType, pdk::MsgType::DebugMsg);
   ASSERT_EQ(sg_msg, String::fromLatin1("f15 \"foo\""));
   ASSERT_EQ(String::fromLatin1(sg_file), file);
   ASSERT_EQ(sg_line, line);
   ASSERT_EQ(String::fromLatin1(sg_function), function);
}

TEST(DebugTest, testDefaultMessagehandler)
{
   MessageHandlerSetter mhs(0); // set 0, should set default handler
   pdk::MessageHandler defaultMessageHandler1 = pdk::install_message_handler((pdk::MessageHandler)nullptr); // set 0, should set and return default handler
   ASSERT_TRUE(defaultMessageHandler1);
   pdk::MessageHandler defaultMessageHandler2 = pdk::install_message_handler(my_message_handler); // set myMessageHandler and return default handler
   bool same = (*defaultMessageHandler1 == *defaultMessageHandler2);
   ASSERT_TRUE(same);
   pdk::MessageHandler messageHandler = pdk::install_message_handler((pdk::MessageHandler)nullptr); // set 0, should set default and return myMessageHandler
   same = (*messageHandler == *my_message_handler);
   ASSERT_TRUE(same);
}

// @TODO 
TEST(DebugTest, testThreadSafety)
{
   
}

class TestClassA {};
class TestClassB {};

template <typename T>
TestClassA& operator<< (TestClassA& s, T&)
{
   return s;
}

template<>
TestClassA& operator<< <TestClassB>(TestClassA& s, TestClassB& l);
