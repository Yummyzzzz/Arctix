cmake_minimum_required(VERSION 3.20)

# ArctixCLI - interactive command shell over the ArctixC SDK.
# Produces ArctixCLI.exe: a cmd-style prompt with built-in arctix
# commands only (no PowerShell, no PATH dependencies).
project(ArctixCLI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
endif()

# Reuse the SDK from source, without its tests/examples. The CLI ships
# next to arctixc.dll; the SDK targets below provide headers + import lib.
set(ARCTIXC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ARCTIXC_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../ArctixC
    ${CMAKE_CURRENT_BINARY_DIR}/ArctixC_build
)

# Windows VERSIONINFO for ArctixCLI.exe (tracks this project VERSION).
set(ARCTIXCLI_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(ARCTIXCLI_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(ARCTIXCLI_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(ARCTIXCLI_VERSION_STRING ${PROJECT_VERSION})
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/arctixcli.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixcli.rc
    @ONLY
)
if(WIN32)
    set(ARCTIXCLI_VERSION_RESOURCE ${CMAKE_CURRENT_BINARY_DIR}/generated/arctixcli.rc)
endif()

add_executable(ArctixCLI
    src/main.cpp
    src/shell.cpp
    src/commands.cpp
    ${ARCTIXCLI_VERSION_RESOURCE}
)

target_include_directories(ArctixCLI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# The shell consumes the SDK, it reimplements nothing: allocator,
# pool, buffer, hash and thread diagnostics all call into arctixc.dll.
target_link_libraries(ArctixCLI PRIVATE ArctixC::ArctixC)

# Drop arctixc.dll next to the exe so the shell runs anywhere.
add_custom_command(TARGET ArctixCLI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ArctixC>
        $<TARGET_FILE_DIR:ArctixCLI>
)
