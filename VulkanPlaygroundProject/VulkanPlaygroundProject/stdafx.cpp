#include "stdafx.h"

#include <Windows.h>
#include <stdarg.h> 

namespace Logger {
	int mPrintIndent = 0;
	std::string mLogCat;
};

void Logger::LogMessage(const char* aMessage, ...) {
	const unsigned int size = 1024 * 16;
	static char buffer[size] = { 0 };
	FillMemory(buffer, size, 0);

	//Category
	if (!mLogCat.empty()) {
		sprintf_s(buffer, "%s%s", mLogCat.c_str(), ":\t");
		printf(buffer);
		OutputDebugStringA(buffer);

	}

	FillMemory(buffer, size, 0);

	//indent
	std::string indent;
	for (int i = 0; i < mPrintIndent; ++i) {
		indent += '\t';
	}
	printf(indent.c_str());
	OutputDebugStringA(indent.c_str());

	//message/arguments
	va_list argptr;
	va_start(argptr, aMessage);
	vprintf(aMessage, argptr);


	vsprintf_s(buffer, aMessage, argptr);
	OutputDebugStringA(buffer);

	va_end(argptr);


}
