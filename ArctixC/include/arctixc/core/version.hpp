#pragma once

// ArctixC foundation - version information.
// Types + function declarations. Implementation in src/core/version.cpp.
//
// The ARCTIX_* macros are kept as aliases so code written against
// Arctix Core v0.1.0 keeps compiling against the new header layout.

#define ARCTIXC_VERSION_MAJOR 0
#define ARCTIXC_VERSION_MINOR 3
#define ARCTIXC_VERSION_PATCH 0

#define ARCTIXC_VERSION_STRING "0.3.0"
#define ARCTIXC_VERSION_NAME "ArctixC"

// Backwards-compatible aliases for the v0.1.0 header layout.
#define ARCTIX_VERSION_MAJOR ARCTIXC_VERSION_MAJOR
#define ARCTIX_VERSION_MINOR ARCTIXC_VERSION_MINOR
#define ARCTIX_VERSION_PATCH ARCTIXC_VERSION_PATCH
#define ARCTIX_VERSION_STRING ARCTIXC_VERSION_STRING
#define ARCTIX_VERSION_NAME ARCTIXC_VERSION_NAME

#include "arctixc/core/export.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

// Returns e.g. "0.3.0". Never returns NULL.
ARCTIXC_CORE_API const char* ArctixC_GetVersionString(void);

// Writes major/minor/patch into the non-NULL out parameters.
// NULL parameters are ignored.
ARCTIXC_CORE_API void ArctixC_GetVersion(int* major, int* minor, int* patch);

#ifdef __cplusplus
} // extern "C"
#endif
