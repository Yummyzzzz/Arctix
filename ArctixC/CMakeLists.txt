cmake_minimum_required(VERSION 3.20)

# Single source of truth for the version: the generated
# arctixc/core/version.hpp macros AND the arctixc.dll VERSIONINFO
# resource (ProductVersion etc.) are both configured from this.
project(ArctixC VERSION 0.1.1 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
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

# Generated Windows VERSIONINFO resource: embeds CompanyName,
# FileDescription, FileVersion, InternalName, OriginalFilename,
# ProductName (arctixc.dll), ProductVersion and LegalCopyright
# into arctixc.dll. Version numbers track project VERSION above.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/arctixc.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc.rc
    @ONLY
)

if(WIN32)
    set(ARCTIXC_VERSION_RESOURCE ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixc.rc)
endif()

# ---------------------------------------------------------------------------
# ArctixC - ONE standalone DLL (arctixc.dll + arctixc.lib, all lowercase).
# Every subsystem (foundation, memory, buffer, virtual memory, thread,
# thread pool, C ABIs) lives in this single binary. There are no
# ArctixC.Memory / ArctixC.Thread DLLs: a consumer ships exactly one DLL
# plus this repo's include/ headers. All three export macros resolve to
# dllexport when building this target (all three *_BUILD defines are set).
# The CMake target stays ArctixC::ArctixC; only the file name is lowercase.
# ---------------------------------------------------------------------------
add_library(ArctixC SHARED
    src/core/version.cpp
    src/core/status.cpp
    src/memory/allocator.cpp
    src/memory/memory_pool.cpp
    src/memory/memory_manager.cpp
    src/memory/statistics_manager.cpp
    src/memory/windows_memory.cpp
    src/memory/buffer.cpp
    src/memory/virtual_memory.cpp
    src/memory/c_api_memory.cpp
    src/thread/thread.cpp
    src/thread/thread_pool.cpp
    src/thread/c_api_thread.cpp
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
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

# ---------------------------------------------------------------------------
# Tests (all link the single ArctixC target)
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

    # Verifies both C ABI headers compile from pure C (not just C++).
    add_library(ArctixC_CApiCheck STATIC tests/c_api_check.c)
    target_link_libraries(ArctixC_CApiCheck PRIVATE ArctixC::ArctixC)
    add_test(NAME CApiCheckBuild COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ArctixC_CApiCheck)
endif()

# ---------------------------------------------------------------------------
# Examples (each links only ArctixC::ArctixC - see DOC.html)
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
    TARGETS ArctixC
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
