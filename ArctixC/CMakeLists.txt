cmake_minimum_required(VERSION 3.20)

# Single source of truth for the version: the generated
# arctixc/core/version.hpp macros AND both DLL VERSIONINFO
# resources (ProductVersion etc.) are configured from this.
project(ArctixC VERSION 0.2.0 LANGUAGES CXX C)

# Every C++ standard Arctix supports. The code is written against C++17,
# so it also compiles cleanly under C++20 and C++23 with no changes.
# Consumers pick one at configure time:
#   cmake -DARCTIXC_CXX_STANDARD=20 ...
set(ARCTIXC_CXX_STANDARD 17 CACHE STRING "C++ standard for ArctixC (17, 20 or 23)")
set_property(CACHE ARCTIXC_CXX_STANDARD PROPERTY STRINGS 17 20 23)
if(NOT ARCTIXC_CXX_STANDARD MATCHES "^(17|20|23)$")
    message(FATAL_ERROR "ARCTIXC_CXX_STANDARD must be 17, 20 or 23 (got '${ARCTIXC_CXX_STANDARD}')")
endif()

set(CMAKE_CXX_STANDARD ${ARCTIXC_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(ARCTIXC_BUILD_TESTS "Build ArctixC tests" ON)
option(ARCTIXC_BUILD_EXAMPLES "Build ArctixC examples" ON)

if(MSVC)
    add_compile_options(/W4 /permissive-)
endif()

# Generated version header (single-source version: project VERSION above).
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/arctixc/core/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc/core/version.hpp
    @ONLY
)

# Generated Windows VERSIONINFO resources: embed CompanyName,
# FileDescription, FileVersion, InternalName, OriginalFilename,
# ProductName and LegalCopyright into each DLL. Version numbers track
# project VERSION above. One configured copy per binary (the C++ DLL
# and the C DLL carry their own file names).
set(ARCTIXC_RC_INTERNAL_NAME "arctixc")
set(ARCTIXC_RC_ORIGINAL_FILENAME "arctixc.dll")
set(ARCTIXC_RC_PRODUCT_NAME "arctixc.dll")
set(ARCTIXC_RC_FILE_DESCRIPTION "ArctixC systems SDK, C++ API (memory, buffers, virtual memory, threads)")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/arctixc.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc.rc
    @ONLY
)

set(ARCTIXC_RC_INTERNAL_NAME "arctixc_c")
set(ARCTIXC_RC_ORIGINAL_FILENAME "arctixc_c.dll")
set(ARCTIXC_RC_PRODUCT_NAME "arctixc_c.dll")
set(ARCTIXC_RC_FILE_DESCRIPTION "ArctixC systems SDK, C API (memory, buffers, virtual memory, threads)")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/arctixc.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc_c.rc
    @ONLY
)

if(WIN32)
    set(ARCTIXC_VERSION_RESOURCE ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc.rc)
    set(ARCTIXC_C_VERSION_RESOURCE ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc_c.rc)
endif()

# ---------------------------------------------------------------------------
# Shared implementation sources. Both DLLs compile the same core + C++
# implementation (same namespace arctix::, same headers, same behavior);
# only the C ABI wrappers differ. Each DLL is self-contained: ship
# arctixc.dll for C++ consumers, arctixc_c.dll for C consumers.
# ---------------------------------------------------------------------------
set(ARCTIXC_CORE_SOURCES
    src/core/version.cpp
    src/core/status.cpp
)

set(ARCTIXC_IMPL_SOURCES
    src/memory/allocator.cpp
    src/memory/memory_pool.cpp
    src/memory/memory_manager.cpp
    src/memory/statistics_manager.cpp
    src/memory/windows_memory.cpp
    src/memory/buffer.cpp
    src/memory/virtual_memory.cpp
    src/thread/thread.cpp
    src/thread/thread_pool.cpp
)

set(ARCTIXC_C_API_SOURCES
    src/memory/c_api_memory.cpp
    src/thread/c_api_thread.cpp
)

# ---------------------------------------------------------------------------
# ArctixC - C++ DLL (arctixc.dll + arctixc.lib, all lowercase).
# Self-contained: same core + same C++ implementation + the C ABI
# wrappers, compiled from the same sources as arctixc_c.dll below.
# C++ consumers link arctixc.lib; the C ABI is also present so mixed
# C/C++ test binaries link and run against either DLL.
# ---------------------------------------------------------------------------
add_library(ArctixC SHARED
    ${ARCTIXC_CORE_SOURCES}
    ${ARCTIXC_IMPL_SOURCES}
    ${ARCTIXC_C_API_SOURCES}
    ${ARCTIXC_VERSION_RESOURCE}
)
add_library(ArctixC::ArctixC ALIAS ArctixC)

target_compile_definitions(ArctixC PRIVATE
    ARCTIXC_CORE_BUILD
    ARCTIXC_MEMORY_BUILD
    ARCTIXC_THREAD_BUILD
)

target_include_directories(ArctixC
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(ArctixC PROPERTIES
    OUTPUT_NAME "arctixc"
    CXX_STANDARD ${ARCTIXC_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

# ---------------------------------------------------------------------------
# ArctixC_C - C DLL (arctixc_c.dll + arctixc_c.lib, all lowercase).
# Same core + same C++ implementation + same C ABI wrappers, compiled
# from the same sources as arctixc.dll above. C consumers link
# arctixc_c.lib and include arctixc/c_api/*.h from pure C.
# Same namespace, same headers, same behavior - only the binary file
# differs. Both DLLs land in the same folder (CMAKE_RUNTIME_OUTPUT_
# DIRECTORY is untouched, so both drop next to the test/example exes).
# ---------------------------------------------------------------------------
add_library(ArctixC_C SHARED
    ${ARCTIXC_CORE_SOURCES}
    ${ARCTIXC_IMPL_SOURCES}
    ${ARCTIXC_C_API_SOURCES}
    ${ARCTIXC_C_VERSION_RESOURCE}
)
add_library(ArctixC::ArctixC_C ALIAS ArctixC_C)

target_compile_definitions(ArctixC_C PRIVATE
    ARCTIXC_CORE_BUILD
    ARCTIXC_MEMORY_BUILD
    ARCTIXC_THREAD_BUILD
)

target_include_directories(ArctixC_C
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(ArctixC_C PROPERTIES
    OUTPUT_NAME "arctixc_c"
    CXX_STANDARD ${ARCTIXC_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

# ---------------------------------------------------------------------------
# Tests. Every suite runs twice: once against the C++ DLL, once against
# the C DLL (same sources, same expectations, _C_ in the exe name).
# CApiCheck proves both C headers compile from pure C; CApiTests runs the
# C ABI end to end as a real executable (pure C, links the C DLL).
# ---------------------------------------------------------------------------
if(ARCTIXC_BUILD_TESTS)
    enable_testing()

    add_executable(ArctixC_MemoryTests tests/test_memory.cpp)
    target_link_libraries(ArctixC_MemoryTests PRIVATE ArctixC::ArctixC)
    add_test(NAME MemoryTests COMMAND ArctixC_MemoryTests)

    add_executable(ArctixC_ThreadTests tests/test_thread.cpp)
    target_link_libraries(ArctixC_ThreadTests PRIVATE ArctixC::ArctixC)
    add_test(NAME ThreadTests COMMAND ArctixC_ThreadTests)

    add_executable(ArctixC_ExtendedTests tests/test_extended.cpp)
    target_link_libraries(ArctixC_ExtendedTests PRIVATE ArctixC::ArctixC)
    add_test(NAME ExtendedTests COMMAND ArctixC_ExtendedTests)

    # Same suites, linked against the C DLL (proves arctixc_c.dll carries
    # the full implementation, not just the C wrappers).
    add_executable(ArctixC_C_MemoryTests tests/test_memory.cpp)
    target_link_libraries(ArctixC_C_MemoryTests PRIVATE ArctixC::ArctixC_C)
    add_test(NAME C_MemoryTests COMMAND ArctixC_C_MemoryTests)

    add_executable(ArctixC_C_ThreadTests tests/test_thread.cpp)
    target_link_libraries(ArctixC_C_ThreadTests PRIVATE ArctixC::ArctixC_C)
    add_test(NAME C_ThreadTests COMMAND ArctixC_C_ThreadTests)

    add_executable(ArctixC_C_ExtendedTests tests/test_extended.cpp)
    target_link_libraries(ArctixC_C_ExtendedTests PRIVATE ArctixC::ArctixC_C)
    add_test(NAME C_ExtendedTests COMMAND ArctixC_C_ExtendedTests)

    # Verifies both C ABI headers compile from pure C (not just C++).
    # Links the C DLL, where the C ABI actually lives now.
    add_library(ArctixC_CApiCheck STATIC tests/c_api_check.c)
    target_link_libraries(ArctixC_CApiCheck PRIVATE ArctixC::ArctixC_C)
    add_test(NAME CApiCheckBuild COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ArctixC_CApiCheck)

    # Real end-to-end C test: pure C main() exercising the C ABI in
    # arctixc_c.dll (allocator, pool, buffer, virtual pages, thread, pool).
    add_executable(ArctixC_CApiTests tests/c_api_tests.c)
    target_link_libraries(ArctixC_CApiTests PRIVATE ArctixC::ArctixC_C)
    add_test(NAME CApiTests COMMAND ArctixC_CApiTests)
endif()

# ---------------------------------------------------------------------------
# Examples (C++ examples link the C++ DLL; the C demo links the C DLL)
# ---------------------------------------------------------------------------
if(ARCTIXC_BUILD_EXAMPLES)
    add_executable(Example_MemoryOnly examples/memory_only.cpp)
    target_link_libraries(Example_MemoryOnly PRIVATE ArctixC::ArctixC)

    add_executable(Example_ThreadOnly examples/thread_only.cpp)
    target_link_libraries(Example_ThreadOnly PRIVATE ArctixC::ArctixC)

    add_executable(Example_MemoryAndThread examples/memory_and_thread.cpp)
    target_link_libraries(Example_MemoryAndThread PRIVATE ArctixC::ArctixC)

    add_executable(Example_Extended examples/extended.cpp)
    target_link_libraries(Example_Extended PRIVATE ArctixC::ArctixC)

    # Pure C demo (compiles as C, links arctixc_c.lib).
    add_executable(Example_CApiDemo examples/c_api_demo.c)
    target_link_libraries(Example_CApiDemo PRIVATE ArctixC::ArctixC_C)
endif()

# ---------------------------------------------------------------------------
# Install + export rules (lets ArctixI package the SDK later)
# ---------------------------------------------------------------------------
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/arctixc
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/arctixc
    PATTERN "version.hpp.in" EXCLUDE
)

# Generated single-source version header installs where version.hpp is expected.
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc/core/version.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/arctixc/core
)

install(
    TARGETS ArctixC ArctixC_C
    EXPORT ArctixCTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    EXPORT ArctixCTargets
    NAMESPACE ArctixC::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ArctixC
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ArctixCConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ArctixCConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ArctixC
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ArctixCConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ArctixCConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ArctixCConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ArctixC
)
