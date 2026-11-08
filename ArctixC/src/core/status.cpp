#include "arctixc/core/status.hpp"

extern "C"
{

ARCTIXC_CORE_API const char* ArctixC_StatusToString(int status)
{
    switch (status)
    {
    case ArctixCStatus_Ok:
        return "Ok";
    case ArctixCStatus_InvalidArgument:
        return "InvalidArgument";
    case ArctixCStatus_NotInitialized:
        return "NotInitialized";
    case ArctixCStatus_AlreadyInitialized:
        return "AlreadyInitialized";
    case ArctixCStatus_OutOfMemory:
        return "OutOfMemory";
    case ArctixCStatus_Exhausted:
        return "Exhausted";
    case ArctixCStatus_NotJoinable:
        return "NotJoinable";
    case ArctixCStatus_AlreadyRunning:
        return "AlreadyRunning";
    case ArctixCStatus_WorkerThrew:
        return "WorkerThrew";
    default:
        return "Unknown";
    }
}

} // extern "C"
