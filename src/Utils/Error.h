#pragma once

#include <cassert>

/* macros */
#ifndef NDEBUG
#define GL_CALL(x) \
   glClearError(); \
   x; \
   assert(glLogError(#x, __FILE__, __LINE__));

#else
#define GL_CALL(x) x
#endif

/* forward declarations */
void glClearError();
bool glLogError(const char* call, const char* file, int line);

