#include "stdafx.h"

#include <stdarg.h>

#if WINDOWS
#define LOGOUTPUT(x) OutputDebugStringA(x);
#else
#define LOGOUTPUT(x)
#endif

#define LOGCONSOLE(...)           \
	memset(buffer, 0, size);      \
	sprintf(buffer, __VA_ARGS__); \
	printf(buffer);               \
	LOGOUTPUT(buffer)
#define LOGCONSOLEVA(x, p_va_list) \
	memset(buffer, 0, size);       \
	vsprintf(buffer, p_va_list);   \
	printf(buffer);                \
	LOGOUTPUT(buffer)

namespace Logger
{
	int mPrintIndent = 0;
	std::string mLogCat;
};

void Logger::LogMessage(const char *aMessage, ...)
{
	const unsigned int size = 1024 * 16;
	static char buffer[size] = {0};
	memset(buffer, 0, size);

	// Category
	if (!mLogCat.empty())
	{
		LOGCONSOLE("%s%s", mLogCat.c_str(), ":\t")
	}

	memset(buffer, 0, size);

	// indent
	std::string indent;
	for (int i = 0; i < mPrintIndent; ++i)
	{
		indent += '\t';
	}
	LOGCONSOLE(indent.c_str());

	// message/arguments
	va_list argptr;
	va_start(argptr, aMessage);
	vprintf(aMessage, argptr);

	vsprintf(buffer, aMessage, argptr);
	LOGOUTPUT(buffer);

	va_end(argptr);
}
