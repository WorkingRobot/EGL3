#pragma once

#ifdef EGL3_DISABLE_ASSERTS

#define EGL3_ASSERT(condition, message)

#else

#include <stdio.h>
#include <stdlib.h>

// TODO: call some sort of logging function instead of just printf
__forceinline bool _EGL3_Assert(const char* Condition, const char* Message, const char* Filename, unsigned Line) {
	printf("ASSERTION FAILED - %s @ %u - %s - %s\n", Filename, Line, Condition, Message);
	abort();
}

#define EGL3_ASSERT(condition, message) (void)((!!(condition)) || (_EGL3_Assert(#condition, message, __FILE__, __LINE__)))

#endif