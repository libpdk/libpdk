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
// Created by softboy on 2018/03/26.

#ifndef PDK_TESTLIB_GLOBAL_H
#define PDK_TESTLIB_GLOBAL_H

#include "pdk/global/Global.h"

#if defined(PDK_STATIC)
# define PDK_TESTLIB_EXPORT
#else
# ifdef PDK_BUILD_TESTLIB_LIB
#  define PDK_TESTLIB_EXPORT PDK_DECL_EXPORT
# else
#  define PDK_TESTLIB_EXPORT PDK_DECL_IMPORT
# endif
#endif

#if (defined PDK_CC_HPACC) && (defined __ia64)
# ifdef PDK_TESTLIB_EXPORT
#  undef PDK_TESTLIB_EXPORT
# endif
# define PDK_TESTLIB_EXPORT
#endif

#define PDK_TRY_LOOP_IMPL(expr, timeoutValue, step) \
   if (!(expr)) {\
      pdktest::wait(0);\
   } \
   int pdkTestIndex = 0;\
   for (; pdkTestIndex < timeoutValue && !(expr); pdkTestIndex += step) {\
      pdktest::wait(step);\
   }


#define PDK_TRY_TIMEOUT_DEBUG_IMPL(expr, timeoutValue, step)\
    if (!(expr)) { \
        PDK_TRY_LOOP_IMPL((expr), (2 * timeoutValue), step);\
        if (expr) { \
            pdk::lang::String msg = pdk::lang::String::fromUtf8("pdktest: This test case check (\"%1\") failed because the requested timeout (%2 ms) was too short, %3 ms would have been sufficient this time."); \
            msg = msg.arg(pdk::lang::String::fromUtf8(#expr)).arg(timeoutValue).arg(timeoutValue + pdkTestIndex); \
            FAIL() << pdk_printable(msg); \
        } \
    }

#define PDK_TRY_IMPL(expr, timeout)\
    const int pdkTestStep = 50; \
    const int pdkTestTimeoutValue = timeout; \
    PDK_TRY_LOOP_IMPL((expr), pdkTestTimeoutValue, pdkTestStep); \
    PDK_TRY_TIMEOUT_DEBUG_IMPL((expr), pdkTestTimeoutValue, pdkTestStep)\
   
#define PDK_TRY_VERIFY_WITH_TIMEOUT(expr, timeout) \
do { \
    PDK_TRY_IMPL((expr), timeout);\
    ASSERT_TRUE(expr); \
} while (false)

#define PDK_TRY_VERIFY(expr) PDK_TRY_VERIFY_WITH_TIMEOUT((expr), 5000)

#define PDK_TRY_VERIFY2_WITH_TIMEOUT(expr, messageExpression, timeout) \
do { \
    PDK_TRY_IMPL((expr), timeout);\
    ASSERT_TRUE(expr) << messageExpression; \
} while (false)

#define PDK_TRY_VERIFY2(expr, messageExpression) PDK_TRY_VERIFY2_WITH_TIMEOUT((expr), (messageExpression), 5000)

#define PDK_TRY_COMPARE_WITH_TIMEOUT(expr, expected, timeout) \
do { \
    PDK_TRY_IMPL(((expr) == (expected)), timeout);\
    ASSERT_EQ((expr), expected); \
} while (false)

#define PDK_TRY_COMPARE(expr, expected) PDK_TRY_COMPARE_WITH_TIMEOUT((expr), expected, 5000)

namespace pdktest {

}

#endif // PDK_TESTLIB_GLOBAL_H
