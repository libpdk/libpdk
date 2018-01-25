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
// Created by softboy on 2018/01/25.

#include "pdk/kernel/ElapsedTimer.h"

namespace pdk {
namespace kernel {

static constexpr pdk::pint64 sg_invalidData = PDK_INT64_C(0x8000000000000000);

void ElapsedTimer::invalidate() noexcept
{
     m_t1 = m_t2 = sg_invalidData;
}

bool ElapsedTimer::isValid() const noexcept
{
    return m_t1 != sg_invalidData && m_t2 != sg_invalidData;
}

bool ElapsedTimer::hasExpired(pdk::pint64 timeout) const noexcept
{
    // if timeout is -1, quint64(timeout) is LLINT_MAX, so this will be
    // considered as never expired
    return pdk::puint64(elapsed()) > pdk::puint64(timeout);
}

} // kernel
} // pdk
