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
// Created by softboy on 2018/02/23.

#include "pdk/global/PlatformDefs.h"
#include "pdk/base/io/Debug.h"
#include "pdk/base/io/fs/Settings.h"

#ifndef PDK_NO_SETTINGS

#include "pdk/base/io/fs/internal/SettingsPrivate.h"
#include "pdk/utils/Cache.h"
#include "pdk/base/io/fs/File.h"
#include "pdk/base/io/fs/Dir.h"
#include "pdk/base/io/fs/FileInfo.h"
#include "pdk/global/LibraryInfo.h"
#include "pdk/base/io/fs/TemporaryFile.h"

#endif // PDK_NO_SETTINGS

