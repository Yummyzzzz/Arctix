cmake_minimum_required(VERSION 3.20)

project(ArctixC VERSION 0.3.0 LANGUAGES CXX C)

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

# ---------------------------------------------------------------------------
# ArctixC - ONE standalone DLL (ArctixC.dll + ArctixC.lib).
# Every subsystem (foundation, memory, buffer, virtual memory, thread,
# thread pool, C ABIs) lives in this single binary. There are no
# ArctixC.Memory / ArctixC.Thread DLLs: a consumer ships exactly one DLL
# plus this repo's include/ headers. All three export macros resolve to
# dllexport when building this target (all three *_BUILD defines are set).
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
)
add_library(ArctixC::ArctixC ALIAS ArctixC)

target_compile_definitions(ArctixC PRIVATE
    ARCTIXC_CORE_BUILD
    ARCTIXC_MEMORY_BUILD
    ARCTIXC_THREAD_BUILD
)

target_include_directories(ArctixC
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(ArctixC PROPERTIES
    OUTPUT_NAME "ArctixC"
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
