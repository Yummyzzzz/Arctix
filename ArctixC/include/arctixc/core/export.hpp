#pragma once

// ArctixC - DLL export / import macro shared by arctixc.dll (C++)
// and arctixc_c.dll (C).
//
// Build either DLL with ARCTIXC_CORE_BUILD defined (done by CMake).
// C++ consumers link arctixc.lib, C consumers link arctixc_c.lib, and
// include the SDK headers WITHOUT defining ARCTIXC_CORE_BUILD.
//
// All subsystems (core, memory, thread) ship in each DLL, so CMake
// defines ARCTIXC_CORE_BUILD, ARCTIXC_MEMORY_BUILD and
// ARCTIXC_THREAD_BUILD together when building either one. The three
// macros therefore always agree (all dllexport in the DLL, all dllimport
// for consumers); they are kept separate so headers stay self-describing.

#ifdef _WIN32
    #ifdef ARCTIXC_CORE_BUILD
        #define ARCTIXC_CORE_API __declspec(dllexport)
    #else
        #define ARCTIXC_CORE_API __declspec(dllimport)
    #endif
#else
    #define ARCTIXC_CORE_API
#endif
