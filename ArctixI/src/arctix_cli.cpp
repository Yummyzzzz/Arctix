// arctix.exe - the installed `arctix <command>` helper (console, no admin).
//
// Finds the install folder via HKLM\SOFTWARE\Arctix\InstallDir and falls back
// to its own directory, so `version`/`help` also work before installation.
//
// Commands:
//   version    print installer + core versions and the install folder
//   website    print the repository URL (and open it)
//   contact    how to reach the project / developer / team
//   bug        how and where to report bugs (and open the issues page)
//   licenses   print the full Arctix License 1.0 (install folder, or download
//              it from the repo when the local copy is missing)
//   uninstall  confirm, then launch uninstall.exe (elevates itself)
//   delete     empty the temp\ scratch folder (safe cleanup, keeps the folder)
//   help       this text  (aliases: helpe, -h, --help, -?)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <urlmon.h>

#include <cstdio>
#include <string>
#include <vector>

#include "common.hpp"

namespace {

std::wstring Widen(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), n);
    return out;
}

void Print(const std::wstring& s) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD done = 0, mode = 0;
    if (GetConsoleMode(h, &mode)) {
        // Real console: WriteConsoleW keeps non-ASCII text intact.
        WriteConsoleW(h, s.c_str(), (DWORD)s.size(), &done, nullptr);
        return;
    }
    // Redirected (pipe/file): WriteConsoleW would fail, emit UTF-8 bytes.
    int n = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0,
                                nullptr, nullptr);
    std::string u((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), u.data(), n, nullptr,
                        nullptr);
    WriteFile(h, u.data(), (DWORD)u.size(), &done, nullptr);
}

void PrintLine(const std::wstring& s = L"") { Print(s + L"\r\n"); }

std::wstring ExeDir() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p(buf, n);
    size_t slash = p.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? L"." : p.substr(0, slash);
}

std::wstring InstallDir() {
    HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, Widen(ARCTIX_REG_KEY).c_str(), 0,
                      KEY_READ, &k) == ERROR_SUCCESS) {
        wchar_t buf[MAX_PATH];
        DWORD size = sizeof(buf), type = 0;
        if (RegQueryValueExW(k, Widen(ARCTIX_REG_VALUE_DIR).c_str(), nullptr, &type,
                             (BYTE*)buf, &size) == ERROR_SUCCESS &&
            size >= 2) {
            RegCloseKey(k);
            return buf;
        }
        RegCloseKey(k);
    }
    return ExeDir();
}

int CmdVersion(const std::wstring& dir) {
    PrintLine(L"Arctix installer v" + Widen(ARCTIXI_VERSION) +
              L"  (ArctixC core v" + Widen(ARCTIXC_VERSION) + L")");
    PrintLine(L"Install folder: " + dir);
    return 0;
}

int CmdHelp() {
    PrintLine(L"Usage: arctix <command>");
    PrintLine();
    PrintLine(L"  version    show installer/core versions and the install folder");
    PrintLine(L"  website    show the Arctix repository URL (opens it too)");
    PrintLine(L"  contact    show how to reach the project and the developer team");
    PrintLine(L"  bug        show how and where to report bugs / issues");
    PrintLine(L"  licenses   print the full Arctix License 1.0 (downloads it if missing)");
    PrintLine(L"  uninstall  remove Arctix (folder, PATH entry, registry)");
    PrintLine(L"  delete     empty the temp\\ scratch folder");
    PrintLine(L"  help       show this text");
    return 0;
}

int CmdWebsite() {
    PrintLine(Widen(ARCTIX_URL_REPO));
    ShellExecuteW(nullptr, L"open", Widen(ARCTIX_URL_REPO).c_str(), nullptr, nullptr,
                  SW_SHOW);
    return 0;
}

int CmdContact() {
    PrintLine(L"Arctix");
    PrintLine(L"  Repository : " + Widen(ARCTIX_URL_REPO));
    PrintLine(L"  Developer  : " + Widen(ARCTIX_DEVELOPER) + L" (" +
              Widen(ARCTIX_URL_PROFILE) + L")");
    PrintLine(L"  Team       : " + Widen(ARCTIX_COMPANY) +
              L" (independent developer team; not a registered company or legal entity)");
    return 0;
}

int CmdBug() {
    PrintLine(L"Found a bug or a security issue?");
    PrintLine(L"  1. Check whether it is already reported: " + Widen(ARCTIX_URL_ISSUES));
    PrintLine(L"  2. Otherwise open a new issue there with steps to reproduce,");
    PrintLine(L"     your Windows version, and the output of `arctix version`.");
    PrintLine(L"  3. Never exploit a vulnerability - report it (see the license).");
    ShellExecuteW(nullptr, L"open", Widen(ARCTIX_URL_ISSUES).c_str(), nullptr, nullptr,
                  SW_SHOW);
    return 0;
}

bool ReadFileBytes(const std::wstring& path, std::string& out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD size = GetFileSize(h, nullptr);
    out.assign(size, '\0');
    DWORD read = 0;
    BOOL ok = ReadFile(h, out.data(), size, &read, nullptr);
    CloseHandle(h);
    return ok && read == size;
}

int CmdLicenses(const std::wstring& dir) {
    std::string bytes;
    // Installed copy first: instant and works offline.
    if (!ReadFileBytes(dir + L"\\LICENSE.md", bytes)) {
        // Otherwise fetch it fresh from the repo (e.g. LICENSE.md was
        // deleted from the install folder). Needs a network connection.
        wchar_t tmp[MAX_PATH];
        std::wstring tmpFile = dir + L"\\Arctix_LICENSE.md";
        if (GetTempPathW(MAX_PATH, tmp) > 0) tmpFile = std::wstring(tmp) + L"Arctix_LICENSE.md";
        if (URLDownloadToFileW(nullptr, Widen(ARCTIX_URL_LICENSE).c_str(),
                               tmpFile.c_str(), 0, nullptr) != S_OK ||
            !ReadFileBytes(tmpFile, bytes)) {
            PrintLine(L"Could not show the license: LICENSE.md was not found in " + dir);
            PrintLine(L"and the download from the repository failed (no network?).");
            return 1;
        }
        DeleteFileW(tmpFile.c_str());
    }
    Print(Widen(bytes));
    if (bytes.empty() || bytes.back() != '\n') PrintLine();
    return 0;
}

int CmdUninstall(const std::wstring& dir) {
    Print(L"Remove Arctix completely (folder, PATH entry, registry)? [y/N] ");
    wchar_t answer[16] = {};
    DWORD read = 0;
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    ReadConsoleW(in, answer, 15, &read, nullptr);
    if (read == 0 || (answer[0] != L'y' && answer[0] != L'Y')) {
        PrintLine(L"Cancelled.");
        return 0;
    }
    std::wstring un = dir + L"\\uninstall.exe";
    if (GetFileAttributesW(un.c_str()) == INVALID_FILE_ATTRIBUTES) {
        PrintLine(L"uninstall.exe was not found in " + dir);
        return 1;
    }
    ShellExecuteW(nullptr, L"open", un.c_str(), nullptr, nullptr, SW_SHOW);
    return 0;
}

int CmdDelete(const std::wstring& dir) {
    std::wstring temp = dir + L"\\temp";
    DWORD a = GetFileAttributesW(temp.c_str());
    if (a == INVALID_FILE_ATTRIBUTES) {
        PrintLine(L"No temp folder yet - nothing to delete.");
        return 0;
    }
    // Empty temp\ but keep the folder itself.
    std::wstring pattern = temp + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    int removed = 0, kept = 0;
    if (h != INVALID_HANDLE_VALUE) {
        do {
            std::wstring name = fd.cFileName;
            if (name == L"." || name == L"..") continue;
            std::wstring full = temp + L"\\" + name;
            bool ok = false;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                wchar_t cmd[32768];
                _snwprintf_s(cmd, _TRUNCATE, L"cmd.exe /c rmdir /s /q \"%s\"", full.c_str());
                STARTUPINFOW si = {sizeof(si)};
                PROCESS_INFORMATION pi = {};
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;
                if (CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                    WaitForSingleObject(pi.hProcess, INFINITE);
                    DWORD code = 1;
                    GetExitCodeProcess(pi.hProcess, &code);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                    ok = (code == 0);
                }
            } else {
                ok = DeleteFileW(full.c_str()) != FALSE;
            }
            ok ? ++removed : ++kept;
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    PrintLine(L"temp\\ emptied: " + std::to_wstring(removed) + L" removed, " +
              std::to_wstring(kept) + L" kept.");
    return kept == 0 ? 0 : 1;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    std::wstring dir = InstallDir();
    if (argc < 2) return CmdHelp();
    std::wstring cmd = argv[1];
    for (auto& c : cmd) c = towlower(c);
    if (!cmd.empty() && cmd[0] == L'-') cmd.erase(0, cmd[0] == L'-' && cmd.size() > 1 && cmd[1] == L'-' ? 2 : 1);

    if (cmd == L"version" || cmd == L"v") return CmdVersion(dir);
    if (cmd == L"website" || cmd == L"web" || cmd == L"repo") return CmdWebsite();
    if (cmd == L"contact") return CmdContact();
    if (cmd == L"bug" || cmd == L"bugs" || cmd == L"issue" || cmd == L"issues")
        return CmdBug();
    if (cmd == L"licenses" || cmd == L"license") return CmdLicenses(dir);
    if (cmd == L"uninstall" || cmd == L"remove") return CmdUninstall(dir);
    if (cmd == L"delete" || cmd == L"delet" || cmd == L"clean") return CmdDelete(dir);
    if (cmd == L"help" || cmd == L"helpe" || cmd == L"h" || cmd == L"?") return CmdHelp();

    PrintLine(L"Unknown command: " + std::wstring(argv[1]));
    PrintLine();
    return CmdHelp();
}
