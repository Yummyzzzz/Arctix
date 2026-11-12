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
# ArctixC_Static - static library (arctixc_static.lib, no .dll).
# Same core + same C++ implementation + same C ABI, compiled from the
# same sources as the DLLs above. Consumers link this ONE file, define
# ARCTIXC_STATIC (propagated automatically as a PUBLIC definition so the
# API macros expand to nothing), and ship a single .exe - no DLL to
# stage, no runtime extraction.
#
# Built with the static MSVC runtime (/MT) so it links cleanly into
# standalone /MT executables (e.g. Arctix_bench).
# ---------------------------------------------------------------------------
option(ARCTIXC_BUILD_STATIC "Build the ArctixC static library" ON)

if(ARCTIXC_BUILD_STATIC)
    add_library(ArctixC_Static STATIC
        ${ARCTIXC_CORE_SOURCES}
        ${ARCTIXC_IMPL_SOURCES}
        ${ARCTIXC_C_API_SOURCES}
    )
    add_library(ArctixC::ArctixC_Static ALIAS ArctixC_Static)

    target_compile_definitions(ArctixC_Static PUBLIC ARCTIXC_STATIC)

    target_include_directories(ArctixC_Static
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    set_target_properties(ArctixC_Static PROPERTIES
        OUTPUT_NAME "arctixc_static"
        CXX_STANDARD ${ARCTIXC_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        VERSION ${PROJECT_VERSION}
        SOVERSION 0
    )
endif()

# ---------------------------------------------------------------------------
# Single-header amalgamations: the public API in ONE file each, generated
# at configure time by concatenating headers in dependency order
# (internal arctixc/* includes are inlined, #pragma once guards stripped,
# system includes kept).
#   arctixc.hpp     - C++ API (version, status, memory, threads)
#   arctixc_c.h     - C ABI only (version, status, c_api/*) - pure C safe
#   arctixc_all.hpp - EVERYTHING (C++ API + C ABI, for C++ consumers)
# Consumers drop one header next to the matching binary instead of the
# whole include/ tree. All three work with EITHER the static lib
# (define ARCTIXC_STATIC) or the DLL import libs (default: dllimport).
# ---------------------------------------------------------------------------
if(ARCTIXC_BUILD_STATIC)
    # Export macros first: every header below (starting with the version
    # block) uses ARCTIXC_*_API, so these must be defined before anything.
    set(ARCTIXC_AMALGAMATED_EXPORT_HEADERS
        core/export.hpp
        memory/export.hpp
        thread/export.hpp
    )
    set(ARCTIXC_AMALGAMATED_CPP_HEADERS
        core/status.hpp
        memory/config.hpp
        memory/statistics.hpp
        memory/mem_utils.hpp
        memory/allocator.hpp
        memory/memory_pool.hpp
        memory/buffer.hpp
        memory/virtual_memory.hpp
        thread/thread.hpp
        thread/thread_pool.hpp
    )
    set(ARCTIXC_AMALGAMATED_C_HEADERS
        core/status.hpp
        c_api/memory.h
        c_api/thread.h
    )

    # Appends one public header (pragma/include stripped) to the file in
    # ARCTIXC_AMALGAMATED_OUT. Uses ARCTIXC_AMALGAMATED_CHUNK as scratch.
    # Headers already added to the CURRENT file are skipped, so shared
    # headers (e.g. status.hpp in the universal file) appear exactly once.
    macro(ARCTIXC_AMALGAMATE_APPEND header basedir)
        get_property(ARCTIXC_AMALGAMATED_DONE GLOBAL PROPERTY ARCTIXC_AMALGAMATED_DONE_HEADERS)
        if(NOT "${basedir}/${header}" IN_LIST ARCTIXC_AMALGAMATED_DONE)
            set_property(GLOBAL APPEND PROPERTY ARCTIXC_AMALGAMATED_DONE_HEADERS "${basedir}/${header}")
            file(READ ${basedir}/${header} ARCTIXC_AMALGAMATED_CHUNK)
            string(REGEX REPLACE "#pragma once[^\n]*\n" "" ARCTIXC_AMALGAMATED_CHUNK "${ARCTIXC_AMALGAMATED_CHUNK}")
            string(REGEX REPLACE "#include \"arctixc/[^\"]*\"[^\n]*\n" "" ARCTIXC_AMALGAMATED_CHUNK "${ARCTIXC_AMALGAMATED_CHUNK}")
            file(APPEND ${ARCTIXC_AMALGAMATED_OUT} "\n${ARCTIXC_AMALGAMATED_CHUNK}")
        endif()
    endmacro()

    # Writes banner + export macros + configured version block, then
    # appends each header list variable named in ARGN.
    macro(ARCTIXC_AMALGAMATE out_file description)
        set(ARCTIXC_AMALGAMATED_OUT ${out_file})
        set_property(GLOBAL PROPERTY ARCTIXC_AMALGAMATED_DONE_HEADERS "")
        file(WRITE ${ARCTIXC_AMALGAMATED_OUT}
            "#pragma once\n"
            "\n"
            "// ArctixC - single-header amalgamation (GENERATED, do not edit).\n"
            "// ${description}, generated by ArctixC/CMakeLists.txt\n"
            "// from the public headers + the configured version below.\n"
            "// Static lib: define ARCTIXC_STATIC. DLL: link the import\n"
            "// .lib and stage the .dll (default, no defines needed).\n"
            "// ArctixC version ${PROJECT_VERSION}.\n"
        )
        foreach(ARCTIXC_AMALGAMATED_HEADER IN LISTS ARCTIXC_AMALGAMATED_EXPORT_HEADERS)
            ARCTIXC_AMALGAMATE_APPEND(${ARCTIXC_AMALGAMATED_HEADER} ${CMAKE_CURRENT_SOURCE_DIR}/include/arctixc)
        endforeach()
        # Version block (generated header: read from the build tree).
        ARCTIXC_AMALGAMATE_APPEND(core/version.hpp ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc)
        foreach(ARCTIXC_AMALGAMATED_LIST IN ITEMS ${ARGN})
            foreach(ARCTIXC_AMALGAMATED_HEADER IN LISTS ${ARCTIXC_AMALGAMATED_LIST})
                ARCTIXC_AMALGAMATE_APPEND(${ARCTIXC_AMALGAMATED_HEADER} ${CMAKE_CURRENT_SOURCE_DIR}/include/arctixc)
            endforeach()
        endforeach()
    endmacro()

    ARCTIXC_AMALGAMATE(
        ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc.hpp
        "C++ API in one file"
        ARCTIXC_AMALGAMATED_CPP_HEADERS
    )
    ARCTIXC_AMALGAMATE(
        ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc_c.h
        "C ABI in one file (pure C safe)"
        ARCTIXC_AMALGAMATED_C_HEADERS
    )
    ARCTIXC_AMALGAMATE(
        ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc_all.hpp
        "Full C++ API + C ABI in one file (C++ consumers)"
        ARCTIXC_AMALGAMATED_CPP_HEADERS
        ARCTIXC_AMALGAMATED_C_HEADERS
    )
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc.hpp
            ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc_c.h
            ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc_all.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

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

    # Static-link proof: full extended suite against arctixc_static.lib
    # (no DLL involved - proves the static flavor carries everything).
    if(ARCTIXC_BUILD_STATIC)
        add_executable(ArctixC_StaticSmoke tests/test_extended.cpp)
        target_link_libraries(ArctixC_StaticSmoke PRIVATE ArctixC::ArctixC_Static)
        # Same static runtime as the lib itself (the rest of the repo builds /MD).
        set_target_properties(ArctixC_StaticSmoke PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )
        add_test(NAME StaticSmoke COMMAND ArctixC_StaticSmoke)

        # DLL proof: the SAME single universal header (arctixc_all.hpp,
        # no include/ tree) linked against the C++ DLL import lib.
        # Proves one header serves static AND dynamic consumers.
        add_executable(ArctixC_DllAmalgam tests/test_amalgam_dll.cpp)
        target_include_directories(ArctixC_DllAmalgam PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
        target_link_libraries(ArctixC_DllAmalgam PRIVATE ArctixC::ArctixC)
        add_test(NAME DllAmalgam COMMAND ArctixC_DllAmalgam)
    endif()
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

if(ARCTIXC_BUILD_STATIC)
    install(
        TARGETS ArctixC_Static
        EXPORT ArctixCTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

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
