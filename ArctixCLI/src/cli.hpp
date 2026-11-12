#pragma once

// ArctixCLI - shared types for the shell and its built-in commands.
// Every command runs inside this process and talks to arctixc.dll;
// nothing shells out to PowerShell or relies on PATH.

#include <string>
#include <vector>

namespace arctixcli
{

// CLI version (tracks ArctixCLI/CMakeLists.txt project VERSION).
inline const char* CliVersionString() { return "0.1.0"; }

// A built-in command: name, one-line summary, usage line, entry point.
// argv[0] is the command name itself, like in main().
struct Command
{
    const char* name;
    const char* summary;
    const char* usage;
    const char* category; // e.g. "Benchmarks" - help groups by this.
    int (*run)(const std::vector<std::string>& argv);
};

} // namespace arctixcli
