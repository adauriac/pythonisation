#ifndef __NFUTIL_H__
#define __NFUTIL_H__

#include <cstdio>
#include <cstdarg>
#if __cplusplus < 201703L
#include <memory>
#endif
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#pragma once
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <process.h>
#else
#include <limits.h>
#define MAX_PATH PATH_MAX
#endif

#define NFUNUSED(identifier) /* identifier */

#define nfSTATEMENT_MACRO_BEGIN  do {
#define nfSTATEMENT_MACRO_END } while ( (void)0, 0 )

#ifdef __cplusplus
#   ifdef __BORLANDC__
#       define nfUnusedVar(identifier) identifier
#   else
template <class T>
inline void nfUnusedVar(const T& NFUNUSED(t)) {}
#   endif
#endif

namespace nf {

	inline std::string Format(const char *fmt, ...)
	{
		char buf[256];

		va_list args;
		va_start(args, fmt);
		const auto r = std::vsnprintf(buf, sizeof buf, fmt, args);
		va_end(args);

		if (r < 0)
			// conversion failed
			return {};

		const size_t len = r;
		if (len < sizeof buf)
			// we fit in the buffer
			return { buf, len };

#if __cplusplus >= 201703L
		// C++17: Create a string and write to its underlying array
		std::string s(len, '\0');
		va_start(args, fmt);
		std::vsnprintf(s.data(), len + 1, fmt, args);
		va_end(args);

		return s;
#else
		// C++11 or C++14: We need to allocate scratch memory
		auto vbuf = std::unique_ptr<char[]>(new char[len + 1]);
		va_start(args, fmt);
		std::vsnprintf(vbuf.get(), len + 1, fmt, args);
		va_end(args);

		return { vbuf.get(), len };
#endif
	}
}

inline void nfMessageBox(char *text, char *caption, UINT type = MB_OK)
{
	MessageBoxA(NULL, text, caption, type);
}

#define nfMilliSleep(x) Sleep(x);

#endif // __NFUTIL_H__
