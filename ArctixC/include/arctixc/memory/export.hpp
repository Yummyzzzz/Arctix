#pragma once

// ArctixC - DLL export / import macro used by the memory headers.
//
// There is no ArctixC.Memory.dll: every memory symbol lives in the
// single ArctixC.dll. CMake defines ARCTIXC_MEMORY_BUILD (together with
// the core/thread defines) when building that DLL, so this macro is
// dllexport inside the build and dllimport for SDK consumers that link
// ArctixC.lib.

#ifdef _WIN32
    #ifdef ARCTIXC_MEMORY_BUILD
        #define ARCTIXC_MEMORY_API __declspec(dllexport)
    #else
        #define ARCTIXC_MEMORY_API __declspec(dllimport)
    #endif
#else
    #define ARCTIXC_MEMORY_API
#endif
