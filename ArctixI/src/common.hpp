#pragma once

// Shared constants for the three ArctixI programs:
//   arctixi.exe   (setup wizard, requires admin)
//   arctix.exe    (installed command-line helper)
//   uninstall.exe (installed remover, requires admin)
//
// Single place for versions, registry locations, URLs and file lists so the
// wizard, the CLI and the uninstaller can never disagree with each other.

#define ARCTIXI_VERSION "0.1.0"
// Bundled ArctixC library version. Must match project(ArctixC VERSION ...)
// in ArctixC/CMakeLists.txt - update both together on every SDK bump.
#define ARCTIXC_VERSION "0.2.0"

#define ARCTIX_COMPANY "Whitepoint Software Development"
#define ARCTIX_DEVELOPER "Yummyzzzz"

#define ARCTIX_URL_REPO "https://github.com/Yummyzzzz/Arctix"
#define ARCTIX_URL_ISSUES "https://github.com/Yummyzzzz/Arctix/issues/"
#define ARCTIX_URL_PROFILE "https://github.com/Yummyzzzz"

// Raw LICENSE.md on main (the /blob/ page is HTML - this is the raw file
// the installer and the `arctix license` command actually download).
#define ARCTIX_URL_LICENSE "https://github.com/Yummyzzzz/Arctix/raw/refs/heads/main/LICENSE.md"

#define ARCTIX_DEFAULT_INSTALL_DIR "C:\\Program Files\\Arctix"

// Registry (HKLM - the setup wizard runs elevated).
#define ARCTIX_REG_KEY "SOFTWARE\\Arctix"
#define ARCTIX_REG_VALUE_DIR "InstallDir"
#define ARCTIX_REG_VALUE_VERSION "Version"
#define ARCTIX_REG_VALUE_CORE "CoreVersion"
#define ARCTIX_REG_UNINSTALL_KEY \
    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Arctix"

// Files shipped NEXT TO arctixi.exe (copied to the install root).
// These are hard requirements: the install fails without them.
static const char* const kShipFiles[] = {
    "arctix.exe",
    "uninstall.exe",
    "LICENSE.md",
};

// Direct-download base for the compiled ArctixC payload (raw files on main).
#define ARCTIX_URL_BINARIES "https://github.com/Yummyzzzz/Arctix/raw/refs/heads/main/ArctixC/binaries/"

// Compiled ArctixC payload. The wizard first looks for these in a
// "binaries" folder next to arctixi.exe; anything missing there is fetched
// from the download URL below (empty URL = not public yet -> clean error).
struct DependencyDownload {
    const char* file;
    const char* url;
};

static const DependencyDownload kDependencies[] = {
    {"arctixc.dll", ARCTIX_URL_BINARIES "arctixc.dll"},
    {"arctixc.lib", ARCTIX_URL_BINARIES "arctixc.lib"},
    {"arctixc.exp", ARCTIX_URL_BINARIES "arctixc.exp"},
    {"arctixc_c.dll", ARCTIX_URL_BINARIES "arctixc_c.dll"},
    {"arctixc_c.lib", ARCTIX_URL_BINARIES "arctixc_c.lib"},
    {"arctixc_c.exp", ARCTIX_URL_BINARIES "arctixc_c.exp"},
    {"ArctixC_ExtendedTests.exe", ARCTIX_URL_BINARIES "ArctixC_ExtendedTests.exe"},
    {"ArctixC_MemoryTests.exe", ARCTIX_URL_BINARIES "ArctixC_MemoryTests.exe"},
    {"ArctixC_ThreadTests.exe", ARCTIX_URL_BINARIES "ArctixC_ThreadTests.exe"},
    {"ArctixC_C_ExtendedTests.exe", ARCTIX_URL_BINARIES "ArctixC_C_ExtendedTests.exe"},
    {"ArctixC_C_MemoryTests.exe", ARCTIX_URL_BINARIES "ArctixC_C_MemoryTests.exe"},
    {"ArctixC_C_ThreadTests.exe", ARCTIX_URL_BINARIES "ArctixC_C_ThreadTests.exe"},
    {"ArctixC_CApiTests.exe", ARCTIX_URL_BINARIES "ArctixC_CApiTests.exe"},
    {"Example_Extended.exe", ARCTIX_URL_BINARIES "Example_Extended.exe"},
    {"Example_MemoryAndThread.exe", ARCTIX_URL_BINARIES "Example_MemoryAndThread.exe"},
    {"Example_MemoryOnly.exe", ARCTIX_URL_BINARIES "Example_MemoryOnly.exe"},
    {"Example_ThreadOnly.exe", ARCTIX_URL_BINARIES "Example_ThreadOnly.exe"},
    {"Example_CApiDemo.exe", ARCTIX_URL_BINARIES "Example_CApiDemo.exe"},
};
