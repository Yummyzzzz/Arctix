#pragma once

// ArctixC - DLL export / import macro used by the thread headers.
//
// Every thread symbol lives in BOTH arctixc.dll (C++) and arctixc_c.dll
// (C). CMake defines ARCTIXC_THREAD_BUILD (together with the core/memory
// defines) when building either DLL, so this macro is dllexport inside
// the build and dllimport for SDK consumers that link arctixc.lib (C++)
// or arctixc_c.lib (C).

// Static linking: define ARCTIXC_STATIC (done automatically when linking
// the ArctixC_Static CMake target) and all API macros expand to nothing.
// No .lib import stubs, no .dll at runtime.
#ifdef _WIN32
    #if defined(ARCTIXC_STATIC)
        #define ARCTIXC_THREAD_API
    #elif defined(ARCTIXC_THREAD_BUILD)
        #define ARCTIXC_THREAD_API __declspec(dllexport)
    #else
        #define ARCTIXC_THREAD_API __declspec(dllimport)
    #endif
#else
    #define ARCTIXC_THREAD_API
#endif
