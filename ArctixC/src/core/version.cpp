#include "arctixc/core/version.hpp"

extern "C"
{

ARCTIXC_CORE_API const char* ArctixC_GetVersionString(void)
{
    return ARCTIXC_VERSION_STRING;
}

ARCTIXC_CORE_API void ArctixC_GetVersion(int* major, int* minor, int* patch)
{
    if (major != nullptr)
    {
        *major = ARCTIXC_VERSION_MAJOR;
    }
    if (minor != nullptr)
    {
        *minor = ARCTIXC_VERSION_MINOR;
    }
    if (patch != nullptr)
    {
        *patch = ARCTIXC_VERSION_PATCH;
    }
}

} // extern "C"
