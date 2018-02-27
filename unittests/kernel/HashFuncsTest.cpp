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
// Created by softboy on 2017/12/30.

#include "gtest/gtest.h"
#include "pdk/kernel/HashFuncs.h"
#include <list>
#include <algorithm>
#include <utility>
#include <tuple>

TEST(HashFuncsTest, testHash)
{
   {
      std::pair<int, int> p12(1, 2);
      std::pair<int, int> p21(2, 1);
      ASSERT_TRUE(pdk::pdk_hash(p12) == pdk::pdk_hash(p12));
      ASSERT_TRUE(pdk::pdk_hash(p21) == pdk::pdk_hash(p21));
      ASSERT_FALSE(pdk::pdk_hash(p12) == pdk::pdk_hash(p21));
      std::pair<int, int> pA(0x12345678, 0x12345678);
      std::pair<int, int> pB(0x12345675, 0x12345675);
      ASSERT_FALSE(pdk::pdk_hash(pA) == pdk::pdk_hash(pB));
   }
}
