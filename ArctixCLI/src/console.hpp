#pragma once

// ArctixCLI - tiny cross-platform console color helper.
// Windows uses SetConsoleTextAttribute, everything else uses ANSI escapes.
// Colors auto-disable when stdout is not a console, when NO_COLOR is set,
// when TERM=dumb, or when the user runs `color off`.

#include <iostream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace arctixcli
{
namespace console
{

enum class Color
{
    Default,
    Red,
    Green,
    Yellow,
    Cyan,
    White,
    Gray
};

inline bool& EnabledFlag()
{
    static bool enabled = true;
    return enabled;
}

inline void SetEnabled(bool on) { EnabledFlag() = on; }
inline bool IsEnabled() { return EnabledFlag(); }

// Call once at startup: disables colors for pipes, NO_COLOR, dumb terms.
inline void AutoDetect()
{
    bool isConsole = true;
#ifdef _WIN32
    DWORD mode = 0;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    isConsole = (out != INVALID_HANDLE_VALUE) && (GetConsoleMode(out, &mode) != 0);
#else
    isConsole = (isatty(STDOUT_FILENO) != 0);
#endif
    if (!isConsole)
    {
        SetEnabled(false);
        return;
    }
#ifdef _WIN32
    char noColor[2] = {};
    if (GetEnvironmentVariableA("NO_COLOR", noColor, sizeof(noColor)) != 0)
    {
        SetEnabled(false);
        return;
    }
#else
    if (std::getenv("NO_COLOR") != nullptr)
    {
        SetEnabled(false);
        return;
    }
    const char* term = std::getenv("TERM");
    if (term != nullptr && std::string(term) == "dumb")
    {
        SetEnabled(false);
    }
#endif
}

inline void Set(Color color)
{
    if (!IsEnabled())
    {
        return;
    }
#ifdef _WIN32
    WORD attr = 7; // light gray (cmd default)
    switch (color)
    {
    case Color::Default: attr = 7; break;
    case Color::Red: attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
    case Color::Green: attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
    case Color::Yellow: attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
    case Color::Cyan: attr = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
    case Color::White: attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
    case Color::Gray: attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    }
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != INVALID_HANDLE_VALUE)
    {
        SetConsoleTextAttribute(out, attr);
    }
#else
    const char* code = "0";
    switch (color)
    {
    case Color::Default: code = "0"; break;
    case Color::Red: code = "91"; break;
    case Color::Green: code = "92"; break;
    case Color::Yellow: code = "93"; break;
    case Color::Cyan: code = "96"; break;
    case Color::White: code = "97"; break;
    case Color::Gray: code = "90"; break;
    }
    std::cout << "\x1b[" << code << "m";
#endif
}

inline void Reset() { Set(Color::Default); }

// RAII tint: colors everything printed inside the scope.
class Scope
{
public:
    explicit Scope(Color color) { Set(color); }
    ~Scope() { Reset(); }

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};

} // namespace console
} // namespace arctixcli
