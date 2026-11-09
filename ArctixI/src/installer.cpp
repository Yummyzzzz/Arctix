// arctixi.exe - Arctix setup wizard (pure Win32 API, no frameworks).
//
// Wizard pages: Welcome -> License -> Directory -> Options -> Install -> Finish.
// Installs:  arctix.exe, uninstall.exe, LICENSE.md        (to the root)
//            arctixc.dll/.lib/.exp + test/example exes    (to dependencies/)
//            temp/                                         (empty scratch dir)
// Registers HKLM\SOFTWARE\Arctix + Add/Remove Programs entry, and - when the
// checkbox is set - appends the install folder to the machine PATH so that
// `arctix <command>` works in any PowerShell/console window.
//
// The payload comes from a "binaries" folder next to arctixi.exe; anything
// missing there is downloaded via the URL table in common.hpp (URLs are
// empty until the binaries are hosted publicly - missing entries abort the
// install with a clear message instead of a half-installed state).

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <urlmon.h>
#include <commctrl.h>

#include <cstring>
#include <string>
#include <vector>

#include "common.hpp"

#pragma comment(lib, "comctl32.lib")

namespace {

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

std::wstring Widen(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), n);
    return out;
}

std::wstring ExeDir() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p(buf, n);
    size_t slash = p.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? L"." : p.substr(0, slash);
}

bool DirExists(const std::wstring& p) {
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

bool FileExists(const std::wstring& p) {
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

bool MakeDirs(const std::wstring& path) {
    if (DirExists(path)) return true;
    size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos && slash > 2) {
        if (!MakeDirs(path.substr(0, slash))) return false;
    }
    return CreateDirectoryW(path.c_str(), nullptr) || DirExists(path);
}

// LICENSE.md embedded by CMake as RCDATA (see resources/arctixi.rc.in).
#define IDR_LICENSE_TEXT 101

bool GetEmbeddedLicense(std::string& out) {
    HRSRC res = FindResourceW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDR_LICENSE_TEXT),
                              RT_RCDATA);
    if (!res) return false;
    DWORD size = SizeofResource(GetModuleHandleW(nullptr), res);
    HGLOBAL mem = LoadResource(GetModuleHandleW(nullptr), res);
    if (!mem || size == 0) return false;
    const char* p = (const char*)LockResource(mem);
    if (!p) return false;
    out.assign(p, size);
    return true;
}

bool ReadFileBytes(const std::wstring& path, std::string& out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD size = GetFileSize(h, nullptr);
    out.resize(size);
    DWORD read = 0;
    BOOL ok = ReadFile(h, out.data(), size, &read, nullptr);
    CloseHandle(h);
    return ok && read == size;
}

bool WriteFileBytes(const std::wstring& path, const std::string& data) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD done = 0;
    BOOL ok = WriteFile(h, data.data(), (DWORD)data.size(), &done, nullptr);
    CloseHandle(h);
    return ok && done == data.size();
}

// ---------------------------------------------------------------------------
// Wizard state
// ---------------------------------------------------------------------------

enum Page { P_WELCOME, P_LICENSE, P_DIR, P_OPTIONS, P_INSTALL, P_FINISH, P_COUNT };

struct State {
    Page page = P_WELCOME;
    std::wstring dir = L"C:\\Program Files\\Arctix";
    bool licenseOk = false;   // license text available (sidecar or embedded)
    bool accepted = false;
    bool addPath = true;
    bool installing = false;
    bool installed = false;
    std::wstring licenseText;
};

State g_s;
HINSTANCE g_hInst;
HWND g_main;
HFONT g_font, g_titleFont;

// Control ids
enum {
    ID_TITLE = 100, ID_BODY, ID_LICENSE_EDIT, ID_ACCEPT, ID_DIR_LABEL,
    ID_DIR_EDIT, ID_BROWSE, ID_PATH_CHECK, ID_OPTIONS_INFO,
    ID_PROGRESS, ID_LOG, ID_STATUS, ID_FINISH_TEXT, ID_OPEN_FOLDER,
    ID_BACK, ID_NEXT, ID_CANCEL
};

HWND Ctl(int id) { return GetDlgItem(g_main, id); }
void Show(int id, bool v) { ShowWindow(Ctl(id), v ? SW_SHOW : SW_HIDE); }

void Log(const std::wstring& line) {
    HWND log = Ctl(ID_LOG);
    SendMessageW(log, LB_ADDSTRING, 0, (LPARAM)line.c_str());
    SendMessageW(log, LB_SETTOPINDEX, (WPARAM)SendMessageW(log, LB_GETCOUNT, 0, 0), 0);
}

void Progress(int pct) {
    SendMessageW(Ctl(ID_PROGRESS), PBM_SETPOS, (WPARAM)pct, 0);
    UpdateWindow(g_main);
}

// ---------------------------------------------------------------------------
// Registry / PATH
// ---------------------------------------------------------------------------

bool WriteRegistry(const std::wstring& dir) {
    HKEY k;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, Widen(ARCTIX_REG_KEY).c_str(), 0, nullptr,
                        0, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS)
        return false;
    std::wstring ver = Widen(ARCTIXI_VERSION), core = Widen(ARCTIXC_VERSION);
    LONG r = RegSetValueExW(k, Widen(ARCTIX_REG_VALUE_DIR).c_str(), 0, REG_SZ,
                            (const BYTE*)dir.c_str(), (DWORD)((dir.size() + 1) * 2));
    r |= RegSetValueExW(k, Widen(ARCTIX_REG_VALUE_VERSION).c_str(), 0, REG_SZ,
                        (const BYTE*)ver.c_str(), (DWORD)((ver.size() + 1) * 2));
    r |= RegSetValueExW(k, Widen(ARCTIX_REG_VALUE_CORE).c_str(), 0, REG_SZ,
                        (const BYTE*)core.c_str(), (DWORD)((core.size() + 1) * 2));
    RegCloseKey(k);
    if (r != ERROR_SUCCESS) return false;

    // Add/Remove Programs entry.
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, Widen(ARCTIX_REG_UNINSTALL_KEY).c_str(), 0,
                        nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) != ERROR_SUCCESS)
        return false;
    std::wstring name = L"Arctix", pub = Widen(ARCTIX_COMPANY),
                 dispVer = Widen(ARCTIXI_VERSION),
                 uninst = L"\"" + dir + L"\\uninstall.exe\"",
                 about = Widen(ARCTIX_URL_REPO);
    r = RegSetValueExW(k, L"DisplayName", 0, REG_SZ, (const BYTE*)name.c_str(), 16);
    r |= RegSetValueExW(k, L"Publisher", 0, REG_SZ, (const BYTE*)pub.c_str(),
                        (DWORD)((pub.size() + 1) * 2));
    r |= RegSetValueExW(k, L"DisplayVersion", 0, REG_SZ, (const BYTE*)dispVer.c_str(),
                        (DWORD)((dispVer.size() + 1) * 2));
    r |= RegSetValueExW(k, L"InstallLocation", 0, REG_SZ, (const BYTE*)dir.c_str(),
                        (DWORD)((dir.size() + 1) * 2));
    r |= RegSetValueExW(k, L"UninstallString", 0, REG_SZ, (const BYTE*)uninst.c_str(),
                        (DWORD)((uninst.size() + 1) * 2));
    r |= RegSetValueExW(k, L"URLInfoAbout", 0, REG_SZ, (const BYTE*)about.c_str(),
                        (DWORD)((about.size() + 1) * 2));
    RegCloseKey(k);
    return r == ERROR_SUCCESS;
}

bool ReadMachinePath(std::wstring& out) {
    HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                     L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                     0, KEY_READ, &k) != ERROR_SUCCESS)
        return false;
    DWORD type = 0, size = 0;
    RegQueryValueExW(k, L"Path", nullptr, &type, nullptr, &size);
    out.resize(size / 2 + 1);
    LONG r = RegQueryValueExW(k, L"Path", nullptr, &type, (BYTE*)out.data(), &size);
    RegCloseKey(k);
    if (r != ERROR_SUCCESS) return false;
    out.resize(wcsnlen(out.c_str(), out.size()));
    return true;
}

bool AddToMachinePath(const std::wstring& dir) {
    std::wstring path;
    if (!ReadMachinePath(path)) return false;
    std::wstring needle = L";" + dir + L";";
    std::wstring hay = L";" + path + L";";
    for (auto& c : hay) c = towlower(c);
    for (auto& c : needle) c = towlower(c);
    if (hay.find(needle) != std::wstring::npos) return true; // already there
    if (!path.empty() && path.back() != L';') path += L';';
    path += dir;
    HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                      0, KEY_SET_VALUE, &k) != ERROR_SUCCESS)
        return false;
    LONG r = RegSetValueExW(k, L"Path", 0, REG_EXPAND_SZ, (const BYTE*)path.c_str(),
                            (DWORD)((path.size() + 1) * 2));
    RegCloseKey(k);
    if (r != ERROR_SUCCESS) return false;
    DWORD_PTR res = 0;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment",
                        SMTO_ABORTIFHUNG, 5000, &res);
    return true;
}

// ---------------------------------------------------------------------------
// Payload staging: local "binaries" folder first, download table as fallback
// ---------------------------------------------------------------------------

bool StageFile(const std::wstring& setupDir, const std::wstring& depsDir,
               const char* file, std::wstring& error) {
    std::wstring name = Widen(file);
    std::wstring local = setupDir + L"\\binaries\\" + name;
    std::wstring dest = depsDir + L"\\" + name;
    if (FileExists(local)) {
        if (!CopyFileW(local.c_str(), dest.c_str(), FALSE)) {
            error = L"Could not copy " + name;
            return false;
        }
        Log(L"  copied " + name);
        return true;
    }
    // Fall back to the download table.
    const char* url = "";
    for (const auto& d : kDependencies) {
        if (strcmp(d.file, file) == 0) { url = d.url; break; }
    }
    if (url[0] == '\0') {
        error = L"Missing " + name +
                L" (not in binaries/ and no download link is public yet).";
        return false;
    }
    Log(std::wstring(L"  downloading ") + name + L" ...");
    if (URLDownloadToFileW(nullptr, Widen(url).c_str(), dest.c_str(), 0, nullptr) != S_OK ||
        !FileExists(dest)) {
        error = L"Download failed for " + name;
        return false;
    }
    Log(L"  downloaded " + name);
    return true;
}

// ---------------------------------------------------------------------------
// The install run (synchronous; files are small, UI updates between steps)
// ---------------------------------------------------------------------------

bool RunInstall() {
    const std::wstring setupDir = ExeDir();
    const std::wstring dir = g_s.dir;
    const std::wstring depsDir = dir + L"\\dependencies";
    const std::wstring tempDir = dir + L"\\temp";

    Log(L"Installing Arctix " + Widen(ARCTIXI_VERSION) + L" to " + dir);
    Progress(5);

    if (!MakeDirs(depsDir) || !MakeDirs(tempDir)) {
        Log(L"ERROR: could not create the install folders (admin rights?).");
        return false;
    }
    Log(L"Created folders.");
    Progress(15);

    // Ship files live next to the setup exe.
    int step = 0;
    for (const char* f : kShipFiles) {
        std::wstring src = setupDir + L"\\" + Widen(f);
        std::wstring dst = dir + L"\\" + Widen(f);
        if (!FileExists(src)) {
            // A lone arctixi.exe still installs: LICENSE.md is embedded.
            std::string embedded;
            if (strcmp(f, "LICENSE.md") == 0 && GetEmbeddedLicense(embedded) &&
                WriteFileBytes(dst, embedded)) {
                Log(L"Installed LICENSE.md (embedded copy).");
                Progress(15 + (++step) * 5);
                continue;
            }
            Log(L"ERROR: " + Widen(f) + L" is missing next to arctixi.exe.");
            return false;
        }
        if (!CopyFileW(src.c_str(), dst.c_str(), FALSE)) {
            Log(L"ERROR: could not copy " + Widen(f) + L".");
            return false;
        }
        Log(L"Installed " + Widen(f) + L".");
        Progress(15 + (++step) * 5);
    }

    // Compiled ArctixC payload.
    std::wstring error;
    int done = 0;
    const int total = (int)(sizeof(kDependencies) / sizeof(kDependencies[0]));
    for (const auto& d : kDependencies) {
        if (!StageFile(setupDir, depsDir, d.file, error)) {
            Log(L"ERROR: " + error);
            return false;
        }
        Progress(30 + (++done) * 50 / total);
    }

    if (!WriteRegistry(dir)) {
        Log(L"ERROR: could not write the registry (admin rights?).");
        return false;
    }
    Log(L"Registered Arctix (HKLM\\SOFTWARE\\Arctix + Add/Remove Programs).");
    Progress(90);

    if (g_s.addPath) {
        if (!AddToMachinePath(dir)) {
            Log(L"ERROR: could not extend the machine PATH.");
            return false;
        }
        Log(L"Added to the machine PATH - `arctix` works in any console.");
    } else {
        Log(L"PATH untouched (checkbox off) - run arctix.exe from the folder.");
    }
    Progress(100);
    Log(L"Done.");
    return true;
}

// ---------------------------------------------------------------------------
// Wizard UI
// ---------------------------------------------------------------------------

void SetPage(Page p) {
    g_s.page = p;
    const bool welcome = p == P_WELCOME, license = p == P_LICENSE, dir = p == P_DIR,
               opts = p == P_OPTIONS, inst = p == P_INSTALL, fin = p == P_FINISH;

    static const wchar_t* titles[] = {
        L"Welcome to Arctix", L"License Agreement", L"Install Location",
        L"Install Options", L"Installing Arctix", L"Installation Complete",
    };
    static const wchar_t* bodies[] = {
        L"This wizard installs Arctix on your machine:\n\n"
        L"- the compiled ArctixC SDK (arctixc.dll, .lib, .exp) plus its test programs under dependencies\\\n"
        L"- the arctix command-line helper and uninstall.exe\n"
        L"- an empty temp\\ folder for future scratch files\n\n"
        L"By Whitepoint Software Development - Yummyzzzz.",
        L"Please read the Arctix License 1.0. You must accept it to continue.",
        L"Choose where Arctix will be installed. Any folder works - a local drive, "
        L"the Desktop, C:\\, ... If the folder name is not \"Arctix\", a subfolder "
        L"named Arctix is added automatically.",
        L"",
        L"",
        L"",
    };
    SetWindowTextW(Ctl(ID_TITLE), titles[p]);
    SetWindowTextW(Ctl(ID_BODY), bodies[p]);
    Show(ID_BODY, welcome || license || dir);

    Show(ID_LICENSE_EDIT, license);
    Show(ID_ACCEPT, license);
    Show(ID_DIR_LABEL, dir);
    Show(ID_DIR_EDIT, dir);
    Show(ID_BROWSE, dir);
    Show(ID_PATH_CHECK, opts);
    Show(ID_OPTIONS_INFO, opts);
    Show(ID_PROGRESS, inst);
    Show(ID_LOG, inst);
    Show(ID_STATUS, inst);
    Show(ID_FINISH_TEXT, fin);
    Show(ID_OPEN_FOLDER, fin);

    if (opts) {
        std::wstring info =
            L"This will be installed into\r\n" + g_s.dir +
            L":\r\n\r\n- arctix.exe, uninstall.exe, LICENSE.md\r\n"
            L"- dependencies\\: arctixc.dll, arctixc.lib, arctixc.exp,\r\n"
            L"  test programs and examples\r\n- temp\\: empty scratch folder\r\n\r\n"
            L"Plus registry entries (HKLM\\SOFTWARE\\Arctix and\r\n"
            L"Add/Remove Programs).";
        SetWindowTextW(Ctl(ID_OPTIONS_INFO), info.c_str());
    }
    if (fin) {
        std::wstring t = g_s.installed
            ? (L"Arctix was installed into\r\n" + g_s.dir +
               L"\r\n\r\nOpen a new PowerShell window and type\r\n`arctix version` to verify.")
            : L"The installation did not complete. Go Back to retry or Close to exit.";
        SetWindowTextW(Ctl(ID_FINISH_TEXT), t.c_str());
    }

    Show(ID_BACK, !welcome && !fin && !inst);
    Show(ID_NEXT, !fin && !inst);
    Show(ID_CANCEL, true);
    SetWindowTextW(Ctl(ID_CANCEL), fin ? L"Close" : L"Cancel");
    if (license) {
        EnableWindow(Ctl(ID_NEXT), g_s.accepted && g_s.licenseOk);
        if (!g_s.licenseOk)
            SetWindowTextW(Ctl(ID_BODY),                            L"LICENSE.md was not found next to arctixi.exe - "
                           L"the setup cannot continue without it.");
    }
    if (dir) SetWindowTextW(Ctl(ID_DIR_EDIT), g_s.dir.c_str());
    if (inst && !g_s.installing && !g_s.installed) {
        // First arrival: run the install synchronously.
        g_s.installing = true;
        EnableWindow(Ctl(ID_CANCEL), FALSE);
        SetWindowTextW(Ctl(ID_STATUS), L"Installing...");
        bool ok = RunInstall();
        g_s.installed = ok;
        g_s.installing = false;
        SetWindowTextW(Ctl(ID_STATUS), ok ? L"Installation finished." : L"Installation failed - see log.");
        Show(ID_NEXT, true);
        EnableWindow(Ctl(ID_CANCEL), TRUE);
    }
    if (inst && (g_s.installed || !g_s.installing)) {
        // Re-entering after a finished run: allow Next.
        Show(ID_NEXT, true);
    }
}

int BrowseForFolder(HWND owner, std::wstring& out) {
    BROWSEINFOW bi = {};
    bi.hwndOwner = owner;
    bi.lpszTitle = L"Select the Arctix install folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return 0;
    wchar_t path[MAX_PATH];
    int ok = SHGetPathFromIDListW(pidl, path) ? 1 : 0;
    CoTaskMemFree(pidl);
    if (ok) out = path;
    return ok;
}

void NormalizeDir(std::wstring& dir) {
    while (!dir.empty() && (dir.back() == L'\\' || dir.back() == L'/'))
        dir.pop_back();
    size_t slash = dir.find_last_of(L"\\/");
    std::wstring leaf = (slash == std::wstring::npos) ? dir : dir.substr(slash + 1);
    std::wstring lower = leaf;
    for (auto& c : lower) c = towlower(c);
    if (lower != L"arctix") dir += L"\\Arctix";
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (HIWORD(w) == BN_CLICKED) {
            if (id == ID_CANCEL) {
                if (g_s.page == P_FINISH || !g_s.installing)
                    DestroyWindow(h);
                return 0;
            }
            if (id == ID_BACK && g_s.page > P_WELCOME) {
                SetPage((Page)(g_s.page - 1));
                return 0;
            }
            if (id == ID_NEXT && g_s.page < P_FINISH) {
                if (g_s.page == P_DIR) {
                    wchar_t buf[MAX_PATH];
                    GetWindowTextW(Ctl(ID_DIR_EDIT), buf, MAX_PATH);
                    g_s.dir = buf;
                    NormalizeDir(g_s.dir);
                    if (g_s.dir.empty()) {
                        MessageBoxW(h, L"Please enter an install folder.", L"Arctix Setup",
                                    MB_ICONWARNING | MB_OK);
                        return 0;
                    }
                }
                SetPage((Page)(g_s.page + 1));
                return 0;
            }
            if (id == ID_BROWSE) {
                std::wstring picked;
                if (BrowseForFolder(h, picked)) {
                    NormalizeDir(picked);
                    g_s.dir = picked;
                    SetWindowTextW(Ctl(ID_DIR_EDIT), picked.c_str());
                }
                return 0;
            }
            if (id == ID_OPEN_FOLDER) {
                ShellExecuteW(nullptr, L"open", g_s.dir.c_str(), nullptr, nullptr, SW_SHOW);
                return 0;
            }
        }
        if (HIWORD(w) == BN_CLICKED && LOWORD(w) == ID_ACCEPT) {
            g_s.accepted = Button_GetCheck(Ctl(ID_ACCEPT)) == BST_CHECKED;
            EnableWindow(Ctl(ID_NEXT), g_s.accepted && g_s.licenseOk);
            return 0;
        }
        if (HIWORD(w) == BN_CLICKED && LOWORD(w) == ID_PATH_CHECK) {
            g_s.addPath = Button_GetCheck(Ctl(ID_PATH_CHECK)) == BST_CHECKED;
            return 0;
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

HWND MakeCtl(const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y,
             int w, int h, int id, DWORD ex = 0) {
    HWND c = CreateWindowExW(ex, cls, text, style | WS_CHILD, x, y, w, h, g_main,
                             (HMENU)(INT_PTR)id, g_hInst, nullptr);
    SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    return c;
}

} // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, LPWSTR, int show) {
    g_hInst = inst;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&icc);

    // License text: prefer the LICENSE.md shipped next to the setup exe
    // (always in sync when dist/ is shipped whole), fall back to the copy
    // embedded in the exe itself so a lone arctixi.exe still works.
    std::string licBytes;
    g_s.licenseOk = ReadFileBytes(ExeDir() + L"\\LICENSE.md", licBytes);
    if (!g_s.licenseOk) g_s.licenseOk = GetEmbeddedLicense(licBytes);
    if (g_s.licenseOk) {
        g_s.licenseText = Widen(licBytes);
        // Edit controls want CRLF line endings.
        std::wstring fixed;
        for (size_t i = 0; i < g_s.licenseText.size(); ++i) {
            if (g_s.licenseText[i] == L'\n' && (i == 0 || g_s.licenseText[i - 1] != L'\r'))
                fixed += L'\r';
            fixed += g_s.licenseText[i];
        }
        g_s.licenseText = fixed;
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.lpszClassName = L"ArctixISetup";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    g_main = CreateWindowExW(0, L"ArctixISetup", L"Arctix Setup",
                             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                             CW_USEDEFAULT, CW_USEDEFAULT, 576, 478,
                             nullptr, nullptr, inst, nullptr);

    NONCLIENTMETRICSW ncm = {sizeof(ncm)};
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    g_font = CreateFontIndirectW(&ncm.lfMessageFont);
    LOGFONTW tf = ncm.lfMessageFont;
    tf.lfHeight = -22;
    tf.lfWeight = FW_BOLD;
    g_titleFont = CreateFontIndirectW(&tf);

    // Title + body live at the top on every page.
    MakeCtl(L"STATIC", L"", WS_VISIBLE | SS_LEFT, 20, 14, 520, 32, ID_TITLE);
    SendMessageW(Ctl(ID_TITLE), WM_SETFONT, (WPARAM)g_titleFont, TRUE);
    MakeCtl(L"STATIC", L"", WS_VISIBLE | SS_LEFT, 20, 52, 520, 130, ID_BODY);

    // License page.
    MakeCtl(L"EDIT", L"", WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_MULTILINE |
                          ES_READONLY | ES_AUTOVSCROLL,
            20, 150, 520, 200, ID_LICENSE_EDIT);
    if (g_s.licenseOk) SetWindowTextW(Ctl(ID_LICENSE_EDIT), g_s.licenseText.c_str());
    MakeCtl(L"BUTTON", L"I accept the terms of the Arctix License 1.0",
            WS_VISIBLE | BS_AUTOCHECKBOX, 20, 358, 520, 22, ID_ACCEPT);

    // Directory page.
    MakeCtl(L"STATIC", L"Install folder:", WS_VISIBLE, 20, 200, 520, 20, ID_DIR_LABEL);
    MakeCtl(L"EDIT", L"", WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 20, 224, 400, 24,
            ID_DIR_EDIT);
    MakeCtl(L"BUTTON", L"Browse...", WS_VISIBLE | BS_PUSHBUTTON, 430, 222, 110, 28,
            ID_BROWSE);

    // Options page.
    MakeCtl(L"BUTTON",
            L"Add the `arctix` command to PowerShell (system PATH)",
            WS_VISIBLE | BS_AUTOCHECKBOX, 20, 190, 520, 22, ID_PATH_CHECK);
    Button_SetCheck(Ctl(ID_PATH_CHECK), BST_CHECKED);
    MakeCtl(L"EDIT", L"", WS_VISIBLE | ES_MULTILINE | ES_READONLY, 20, 220, 520, 130,
            ID_OPTIONS_INFO);

    // Install page.
    MakeCtl(L"STATIC", L"", WS_VISIBLE, 20, 120, 520, 20, ID_STATUS);
    HWND prog = MakeCtl(PROGRESS_CLASSW, L"", WS_VISIBLE, 20, 146, 520, 22, ID_PROGRESS);
    SendMessageW(prog, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    MakeCtl(L"LISTBOX", L"", WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
            20, 176, 520, 174, ID_LOG);

    // Finish page.
    MakeCtl(L"EDIT", L"", WS_VISIBLE | ES_MULTILINE | ES_READONLY, 20, 150, 520, 120,
            ID_FINISH_TEXT);
    MakeCtl(L"BUTTON", L"Open install folder", WS_VISIBLE | BS_PUSHBUTTON, 20, 280, 180,
            28, ID_OPEN_FOLDER);

    // Wizard buttons.
    MakeCtl(L"BUTTON", L"< Back", WS_VISIBLE | BS_PUSHBUTTON, 268, 408, 90, 28, ID_BACK);
    MakeCtl(L"BUTTON", L"Next >", WS_VISIBLE | BS_PUSHBUTTON | BS_DEFPUSHBUTTON, 364, 408,
            90, 28, ID_NEXT);
    MakeCtl(L"BUTTON", L"Cancel", WS_VISIBLE | BS_PUSHBUTTON, 460, 408, 80, 28, ID_CANCEL);

    SetPage(P_WELCOME);
    ShowWindow(g_main, show);
    UpdateWindow(g_main);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CoUninitialize();
    return 0;
}
