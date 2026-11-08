cmake_minimum_required(VERSION 3.20)

# ArctixI - Arctix Installer (setup wizard GUI, pure Win32 API).
# Builds three standalone executables, none of which link ArctixC:
#   ArctixI.exe   - the setup wizard (requires admin: Program Files + HKLM)
#   arctix.exe    - the installed `arctix <command>` helper (no admin)
#   uninstall.exe - the installed remover (requires admin)
#
# The wizard ships arctix.exe + uninstall.exe + LICENSE.md next to itself
# plus the compiled ArctixC payload from ArctixC/binaries/ (or, once the
# links in src/common.hpp are filled in, downloads them on demand).

project(ArctixI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(UNICODE _UNICODE)
endif()

# Aliases for the .rc.in template (project() only defines PROJECT_VERSION_*).
set(ARCTIXI_VERSION ${PROJECT_VERSION})
set(ARCTIXI_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(ARCTIXI_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(ARCTIXI_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# One VERSIONINFO template, configured per program (name/description differ).
function(arctixi_version_resource out_var internal_name file_name description)
    set(ARCTIXI_INTERNAL_NAME ${internal_name})
    set(ARCTIXI_ORIGINAL_FILENAME ${file_name})
    set(ARCTIXI_FILE_DESCRIPTION ${description})
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/arctixi.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/${internal_name}.rc
        @ONLY
    )
    set(${out_var} ${CMAKE_CURRENT_BINARY_DIR}/generated/${internal_name}.rc PARENT_SCOPE)
endfunction()

arctixi_version_resource(ARCTIXI_SETUP_RC ArctixI ArctixI.exe
    "Arctix setup wizard (installs ArctixC and the arctix command)")
arctixi_version_resource(ARCTIXI_CLI_RC arctix arctix.exe
    "Arctix command-line helper (arctix <command>)")
arctixi_version_resource(ARCTIXI_UNINSTALL_RC uninstall uninstall.exe
    "Arctix remover")

# ---------------------------------------------------------------------------
# ArctixI.exe - setup wizard (WIN32 GUI, pure Win32 API, no frameworks)
# ---------------------------------------------------------------------------
add_executable(ArctixI WIN32
    src/installer.cpp
    ${ARCTIXI_SETUP_RC}
)
target_include_directories(ArctixI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# SHBrowseForFolder + registry + URLDownloadToFileW.
target_link_libraries(ArctixI PRIVATE shell32 advapi32 urlmon)

# ---------------------------------------------------------------------------
# arctix.exe - installed `arctix <command>` helper (console, no admin)
# ---------------------------------------------------------------------------
add_executable(ArctixCLITool
    src/arctix_cli.cpp
    ${ARCTIXI_CLI_RC}
)
set_target_properties(ArctixCLITool PROPERTIES OUTPUT_NAME "arctix")
target_include_directories(ArctixCLITool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ArctixCLITool PRIVATE shell32)

# ---------------------------------------------------------------------------
# uninstall.exe - installed remover (WIN32 GUI, requires admin for cleanup)
# ---------------------------------------------------------------------------
add_executable(ArctixI_Uninstall WIN32
    src/uninstall.cpp
    ${ARCTIXI_UNINSTALL_RC}
)
set_target_properties(ArctixI_Uninstall PROPERTIES OUTPUT_NAME "uninstall")
target_include_directories(ArctixI_Uninstall PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ArctixI_Uninstall PRIVATE shell32 advapi32)

# Setup + uninstaller touch Program Files, HKLM and the machine PATH, so
# they must elevate. arctix.exe stays asInvoker (runs in any console).
if(MSVC)
    target_link_options(ArctixI PRIVATE "/MANIFESTUAC:level=requireAdministrator")
    target_link_options(ArctixI_Uninstall PRIVATE "/MANIFESTUAC:level=requireAdministrator")
endif()

# ---------------------------------------------------------------------------
# Smoke tests (run the freshly built arctix.exe - no install required)
# ---------------------------------------------------------------------------
enable_testing()
add_test(NAME ArctixCliHelp COMMAND ArctixCLITool help)
add_test(NAME ArctixCliVersion COMMAND ArctixCLITool version)
