#pragma once

// ArctixC - DLL export / import macro used by the memory headers.
//
// Every memory symbol lives in BOTH arctixc.dll (C++) and arctixc_c.dll
// (C). CMake defines ARCTIXC_MEMORY_BUILD (together with the core/thread
// defines) when building either DLL, so this macro is dllexport inside
// the build and dllimport for SDK consumers that link arctixc.lib (C++)
// or arctixc_c.lib (C).

// Static linking: define ARCTIXC_STATIC (done automatically when linking
// the ArctixC_Static CMake target) and all API macros expand to nothing.
// No .lib import stubs, no .dll at runtime.
#ifdef _WIN32
    #if defined(ARCTIXC_STATIC)
        #define ARCTIXC_MEMORY_API
    #elif defined(ARCTIXC_MEMORY_BUILD)
        #define ARCTIXC_MEMORY_API __declspec(dllexport)
    #else
        #define ARCTIXC_MEMORY_API __declspec(dllimport)
    #endif
#else
    #define ARCTIXC_MEMORY_API
#endif
