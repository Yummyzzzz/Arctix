#pragma once

// ArctixC - DLL export / import macro used by the thread headers.
//
// Every thread symbol lives in BOTH arctixc.dll (C++) and arctixc_c.dll
// (C). CMake defines ARCTIXC_THREAD_BUILD (together with the core/memory
// defines) when building either DLL, so this macro is dllexport inside
// the build and dllimport for SDK consumers that link arctixc.lib (C++)
// or arctixc_c.lib (C).

#ifdef _WIN32
    #ifdef ARCTIXC_THREAD_BUILD
        #define ARCTIXC_THREAD_API __declspec(dllexport)
    #else
        #define ARCTIXC_THREAD_API __declspec(dllimport)
    #endif
#else
    #define ARCTIXC_THREAD_API
#endif
