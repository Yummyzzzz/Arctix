#pragma once

// ArctixC - DLL export / import macro used by the memory headers.
//
// Every memory symbol lives in BOTH arctixc.dll (C++) and arctixc_c.dll
// (C). CMake defines ARCTIXC_MEMORY_BUILD (together with the core/thread
// defines) when building either DLL, so this macro is dllexport inside
// the build and dllimport for SDK consumers that link arctixc.lib (C++)
// or arctixc_c.lib (C).

#ifdef _WIN32
    #ifdef ARCTIXC_MEMORY_BUILD
        #define ARCTIXC_MEMORY_API __declspec(dllexport)
    #else
        #define ARCTIXC_MEMORY_API __declspec(dllimport)
    #endif
#else
    #define ARCTIXC_MEMORY_API
#endif
