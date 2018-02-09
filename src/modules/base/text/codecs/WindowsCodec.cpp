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
// Created by softboy on 2018/02/01.

#include "pdk/base/text/codecs/internal/WindowsCodecPrivate.h"
#include "pdk/base/ds/VarLengthArray.h"
#include "pdk/base/ds/ByteArray.h"
#include "pdk/global/Windows.h"

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

using pdk::ds::ByteArray;
using pdk::ds::VarLengthArray;
using pdk::lang::String;
using pdk::lang::Character;
using pdk::lang::Latin1Character;

WindowsLocalCodec::WindowsLocalCodec()
{
}

WindowsLocalCodec::~WindowsLocalCodec()
{
}

String WindowsLocalCodec::convertToUnicode(const char *chars, int length, ConverterState *state) const
{
   const char *mb = chars;
   int mblen = length;
   
   if (!mb || !mblen)
      return String();
   
   VarLengthArray<wchar_t, 4096> wc(4096);
   int len;
   String sp;
   bool prepend = false;
   char stateData = 0;
   int remainingChars = 0;
   
   //save the current state information
   if (state) {
      stateData = (char)state->m_stateData[0];
      remainingChars = state->m_remainingChars;
   }
   
   //convert the pending charcter (if available)
   if (state && remainingChars) {
      char prev[3] = {0};
      prev[0] = stateData;
      prev[1] = mb[0];
      remainingChars = 0;
      len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                prev, 2, wc.getRawData(), wc.length());
      if (len) {
         prepend = true;
         sp.push_back(Character(wc[0]));
         mb++;
         mblen--;
         wc[0] = 0;
      }
   }
   
   while (!(len=MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS,
                                    mb, mblen, wc.getRawData(), wc.length()))) {
      int r = GetLastError();
      if (r == ERROR_INSUFFICIENT_BUFFER) {
         const int wclen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                                               mb, mblen, 0, 0);
         wc.resize(wclen);
      } else if (r == ERROR_NO_UNICODE_TRANSLATION) {
         //find the last non NULL character
         while (mblen > 1  && !(mb[mblen-1]))
            mblen--;
         //check whether,  we hit an invalid character in the middle
         if ((mblen <= 1) || (remainingChars && stateData))
            return convertToUnicodeCharByChar(chars, length, state);
         //Remove the last character and try again...
         stateData = mb[mblen-1];
         remainingChars = 1;
         mblen--;
      } else {
         // Fail.
         //warning_stream("MultiByteToWideChar: Cannot convert multibyte text");
         break;
      }
   }
   
   if (len <= 0)
      return String();
   
   if (wc[len-1] == 0) // len - 1: we don't want terminator
      --len;
   
   //save the new state information
   if (state) {
      state->m_stateData[0] = (char)stateData;
      state->m_remainingChars = remainingChars;
   }
   String s((Character*)wc.getRawData(), len);
   if (prepend) {
      return sp+s;
   }
   return s;
}

String WindowsLocalCodec::convertToUnicodeCharByChar(const char *chars, int length, ConverterState *state) const
{
   if (!chars || !length)
      return String();
   
   int copyLocation = 0;
   int extra = 2;
   if (state && state->m_remainingChars) {
      copyLocation = state->m_remainingChars;
      extra += copyLocation;
   }
   int newLength = length + extra;
   char *mbcs = new char[newLength];
   //ensure that we have a NULL terminated string
   mbcs[newLength-1] = 0;
   mbcs[newLength-2] = 0;
   memcpy(&(mbcs[copyLocation]), chars, length);
   if (copyLocation) {
      //copy the last character from the state
      mbcs[0] = (char)state->m_stateData[0];
      state->m_remainingChars = 0;
   }
   const char *mb = mbcs;
#if !defined(Q_OS_WINRT)
   const char *next = 0;
   String s;
   while ((next = CharNextExA(CP_ACP, mb, 0)) != mb) {
      wchar_t wc[2] ={0};
      int charlength = next - mb;
      int len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED|MB_ERR_INVALID_CHARS, mb, charlength, wc, 2);
      if (len>0) {
         s.append(Character(wc[0]));
      } else {
         int r = GetLastError();
         //check if the character being dropped is the last character
         if (r == ERROR_NO_UNICODE_TRANSLATION && mb == (mbcs+newLength -3) && state) {
            state->m_remainingChars = 1;
            state->m_stateData[0] = (char)*mb;
         }
      }
      mb = next;
   }
#else
   String s;
   size_t size = mbstowcs(NULL, mb, length);
   if (size < 0) {
      Q_ASSERT("Error in CE TextCodec");
      return String();
   }
   wchar_t* ws = new wchar_t[size + 2];
   ws[size +1] = 0;
   ws[size] = 0;
   size = mbstowcs(ws, mb, length);
   for (size_t i = 0; i < size; i++)
      s.append(QChar(ws[i]));
   delete [] ws;
#endif
   delete [] mbcs;
   return s;
}

ByteArray WindowsLocalCodec::convertFromUnicode(const Character *ch, int uclen, ConverterState *) const
{
   if (!ch)
      return ByteArray();
   if (uclen == 0)
      return ByteArray("");
   BOOL usedDef;
   ByteArray mb(4096, 0);
   int len;
   while (!(len=WideCharToMultiByte(CP_ACP, 0, (const wchar_t*)ch, uclen,
                                    mb.getRawData(), mb.size()-1, 0, &usedDef)))
   {
      int r = GetLastError();
      if (r == ERROR_INSUFFICIENT_BUFFER) {
         mb.resize(1+WideCharToMultiByte(CP_ACP, 0,
                                         (const wchar_t*)ch, uclen,
                                         0, 0, 0, &usedDef));
         // and try again...
      } else {
         // Fail.  Probably can't happen in fact (dwFlags is 0).
#ifndef QT_NO_DEBUG
         // Can't use warning_stream(), as it'll recurse to handle %ls
         fprintf(stderr,
                 "WideCharToMultiByte: Cannot convert multibyte text (error %d): %ls\n",
                 r, reinterpret_cast<const wchar_t*>(String(ch, uclen).utf16()));
#endif
         break;
      }
   }
   mb.resize(len);
   return mb;
}


ByteArray WindowsLocalCodec::name() const
{
   return "System";
}

int WindowsLocalCodec::mibEnum() const
{
   return 0;
}

} // internal
} // codecs
} // text
} // pdk
