#pragma once

// Shared constants for the three ArctixI programs:
//   arctixi.exe   (setup wizard, requires admin)
//   arctix.exe    (installed command-line helper)
//   uninstall.exe (installed remover, requires admin)
//
// Single place for versions, registry locations, URLs and file lists so the
// wizard, the CLI and the uninstaller can never disagree with each other.

#define ARCTIXI_VERSION "0.1.0"
#define ARCTIXC_VERSION "0.1.1"

#define ARCTIX_COMPANY "Whitepoint Software Development"
#define ARCTIX_DEVELOPER "Yummyzzzz"

#define ARCTIX_URL_REPO "https://github.com/Yummyzzzz/Arctix"
#define ARCTIX_URL_ISSUES "https://github.com/Yummyzzzz/Arctix/issues/"
#define ARCTIX_URL_PROFILE "https://github.com/Yummyzzzz"

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

// Compiled ArctixC payload. The wizard first looks for these in a
// "binaries" folder next to arctixi.exe; anything missing there is fetched
// from the download URL below (empty URL = not public yet -> clean error).
struct DependencyDownload {
    const char* file;
    const char* url; // TODO: picks up once the binaries are hosted publicly.
};

static const DependencyDownload kDependencies[] = {
    {"arctixc.dll", ""},
    {"arctixc.lib", ""},
    {"arctixc.exp", ""},
    {"ArctixC_ExtendedTests.exe", ""},
    {"ArctixC_MemoryTests.exe", ""},
    {"ArctixC_ThreadTests.exe", ""},
    {"Example_Extended.exe", ""},
    {"Example_MemoryAndThread.exe", ""},
    {"Example_MemoryOnly.exe", ""},
    {"Example_ThreadOnly.exe", ""},
};
