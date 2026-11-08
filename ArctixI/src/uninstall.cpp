// uninstall.exe - Arctix remover (pure Win32 API, requires admin).
//
// Removes, in order: the machine PATH entry, the registry keys
// (HKLM\SOFTWARE\Arctix + the Add/Remove Programs entry) and the whole
// install folder. Because a running exe cannot delete its own folder, the
// remover copies itself to %TEMP%, relaunches from there with --cleanup and
// exits; the temp copy does the deletion and then deletes itself.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

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

std::wstring ExePath() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::wstring(buf, n);
}

std::wstring ReadInstallDir() {
    HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, Widen(ARCTIX_REG_KEY).c_str(), 0,
                      KEY_READ, &k) == ERROR_SUCCESS) {
        wchar_t buf[MAX_PATH];
        DWORD size = sizeof(buf);
        if (RegQueryValueExW(k, Widen(ARCTIX_REG_VALUE_DIR).c_str(), nullptr, nullptr,
                             (BYTE*)buf, &size) == ERROR_SUCCESS &&
            size >= 2) {
            RegCloseKey(k);
            return buf;
        }
        RegCloseKey(k);
    }
    std::wstring me = ExePath();
    size_t slash = me.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? me : me.substr(0, slash);
}

bool SamePath(const std::wstring& a, const std::wstring& b) {
    auto norm = [](std::wstring s) {
        while (!s.empty() && s.back() == L'\\') s.pop_back();
        for (auto& c : s) c = towlower(c);
        return s;
    };
    return norm(a) == norm(b);
}

bool RemoveFromMachinePath(const std::wstring& dir) {
    const wchar_t* envKey =
        L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
    HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, envKey, 0, KEY_READ | KEY_SET_VALUE, &k) !=
        ERROR_SUCCESS)
        return false;
    DWORD type = 0, size = 0;
    RegQueryValueExW(k, L"Path", nullptr, &type, nullptr, &size);
    std::wstring path(size / 2 + 1, L'\0');
    LONG r = RegQueryValueExW(k, L"Path", nullptr, &type, (BYTE*)path.data(), &size);
    if (r != ERROR_SUCCESS) {
        RegCloseKey(k);
        return false;
    }
    path.resize(wcsnlen(path.c_str(), path.size()));

    std::vector<std::wstring> keep;
    size_t start = 0;
    for (size_t i = 0; i <= path.size(); ++i) {
        if (i == path.size() || path[i] == L';') {
            std::wstring part = path.substr(start, i - start);
            if (!part.empty() && !SamePath(part, dir)) keep.push_back(part);
            start = i + 1;
        }
    }
    std::wstring out;
    for (size_t i = 0; i < keep.size(); ++i) {
        if (i) out += L';';
        out += keep[i];
    }
    r = RegSetValueExW(k, L"Path", 0, REG_EXPAND_SZ, (const BYTE*)out.c_str(),
                       (DWORD)((out.size() + 1) * 2));
    RegCloseKey(k);
    if (r != ERROR_SUCCESS) return false;
    DWORD_PTR res = 0;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment",
                        SMTO_ABORTIFHUNG, 5000, &res);
    return true;
}

void RemoveRegKeys() {
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, Widen(ARCTIX_REG_UNINSTALL_KEY).c_str());
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, Widen(ARCTIX_REG_KEY).c_str());
}

bool RemoveTree(const std::wstring& dir) {
    bool ok = true;
    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            std::wstring name = fd.cFileName;
            if (name == L"." || name == L"..") continue;
            std::wstring full = dir + L"\\" + name;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!RemoveTree(full)) ok = false;
            } else {
                SetFileAttributesW(full.c_str(), FILE_ATTRIBUTE_NORMAL);
                if (!DeleteFileW(full.c_str())) ok = false;
            }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    if (!RemoveDirectoryW(dir.c_str())) ok = false;
    return ok;
}

void SelfDelete() {
    // Classic delayed self-delete: cmd waits ~1s, deletes us, exits.
    wchar_t cmd[1024];
    _snwprintf_s(cmd, _TRUNCATE,
                 L"cmd.exe /c ping -n 2 127.0.0.1 >nul & del \"%s\"",
                 ExePath().c_str());
    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if (CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                       nullptr, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

int Cleanup(const std::wstring& dir) {
    // Wait for the original uninstall.exe in the install folder to exit.
    for (int i = 0; i < 100; ++i) {
        HANDLE h = CreateFileW((dir + L"\\uninstall.exe").c_str(), GENERIC_READ,
                               0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) break; // gone (or never there)
        CloseHandle(h);
        Sleep(100);
    }
    RemoveFromMachinePath(dir);
    RemoveRegKeys();
    bool ok = RemoveTree(dir);
    MessageBoxW(nullptr,
                ok ? L"Arctix was removed completely."
                   : L"Arctix was unregistered, but some files could not be deleted.\n"
                     L"Close any program using the install folder and delete it manually.",
                L"Arctix Uninstall", MB_OK | (ok ? MB_ICONINFORMATION : MB_ICONWARNING));
    SelfDelete(); // deletes the %TEMP% copy
    return ok ? 0 : 1;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR cmdLine, int) {
    std::wstring args = cmdLine ? cmdLine : L"";
    if (args.find(L"--cleanup") == 0) {
        // Temp copy: --cleanup "<install dir>"
        size_t q1 = args.find(L'"'), q2 = args.find_last_of(L'"');
        std::wstring dir = (q1 != std::wstring::npos && q2 > q1)
                               ? args.substr(q1 + 1, q2 - q1 - 1)
                               : ReadInstallDir();
        return Cleanup(dir);
    }

    std::wstring dir = ReadInstallDir();
    std::wstring msg = L"Remove Arctix completely?\n\nFolder:\n" + dir +
                       L"\n\nThis deletes every file inside it, the PATH entry "
                        L"and the registry keys.";
    if (MessageBoxW(nullptr, msg.c_str(), L"Arctix Uninstall",
                    MB_YESNO | MB_ICONQUESTION) != IDYES)
        return 0;

    // Stage the temp copy and relaunch - the original cannot delete itself.
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    std::wstring copy = std::wstring(tmp) + L"arctix_uninstall_" +
                        std::to_wstring(GetCurrentProcessId()) + L".exe";
    if (!CopyFileW(ExePath().c_str(), copy.c_str(), FALSE)) {
        MessageBoxW(nullptr, L"Could not stage the uninstaller.", L"Arctix Uninstall",
                    MB_OK | MB_ICONERROR);
        return 1;
    }
    std::wstring params = L"--cleanup \"" + dir + L"\"";
    ShellExecuteW(nullptr, L"open", copy.c_str(), params.c_str(), nullptr, SW_SHOW);
    return 0;
}
