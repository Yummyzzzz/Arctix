#pragma once

// ArctixC - DLL export / import macro used by the thread headers.
//
// There is no ArctixC.Thread.dll: every thread symbol lives in the
// single ArctixC.dll. CMake defines ARCTIXC_THREAD_BUILD (together with
// the core/memory defines) when building that DLL, so this macro is
// dllexport inside the build and dllimport for SDK consumers that link
// ArctixC.lib.

#ifdef _WIN32
    #ifdef ARCTIXC_THREAD_BUILD
        #define ARCTIXC_THREAD_API __declspec(dllexport)
    #else
        #define ARCTIXC_THREAD_API __declspec(dllimport)
    #endif
#else
    #define ARCTIXC_THREAD_API
#endif
