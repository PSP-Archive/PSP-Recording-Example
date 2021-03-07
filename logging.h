#ifndef logging_H_
#define logging_H_

#include <stdlib.h>
#include <string.h>

#define LOG_ENABLED

#ifdef LOG_ENABLED
	#define printLog pspDebugScreenPrintf
#else
	#define printLog dummy
#endif

void dummy(const char* str, ...);

#endif
