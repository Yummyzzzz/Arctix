#pragma once

// ArctixC foundation - shared status / error codes.
// Types + function declarations. Implementation in src/core/status.cpp.
//
// Memory keeps its historical bool return values for compatibility.
// New subsystems (starting with Thread) report detailed failures with
// ArctixCStatus so callers can distinguish e.g. "already running" from
// "out of memory" without guessing.
//
// The enum lives at global scope (not inside a namespace) so the same
// header compiles from both C and C++. From C++, the arctixc::Status
// alias may be used for brevity.

#include "arctixc/core/export.hpp"

typedef enum ArctixCStatus
{
    ArctixCStatus_Ok = 0,
    ArctixCStatus_InvalidArgument = 1,
    ArctixCStatus_NotInitialized = 2,
    ArctixCStatus_AlreadyInitialized = 3,
    ArctixCStatus_OutOfMemory = 4,
    ArctixCStatus_Exhausted = 5,
    ArctixCStatus_NotJoinable = 6,
    ArctixCStatus_AlreadyRunning = 7,
    ArctixCStatus_WorkerThrew = 8,
    ArctixCStatus_Unknown = 9
} ArctixCStatus;

#ifdef __cplusplus
namespace arctixc
{
using Status = ::ArctixCStatus;
} // namespace arctixc

extern "C"
{
#endif

// Returns a short human-readable string for a status code, e.g. "Ok".
// Never returns NULL. Unknown codes map to "Unknown".
ARCTIXC_CORE_API const char* ArctixC_StatusToString(int status);

#ifdef __cplusplus
} // extern "C"
#endif
