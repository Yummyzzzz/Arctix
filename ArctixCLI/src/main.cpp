// ArctixCLI - entry point. No arguments starts the interactive shell;
// arguments run one command non-interactively (for scripts and CI).

#include "cli.hpp"
#include "commands.hpp"
#include "console.hpp"
#include "shell.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace
{

void PrintOneShotHelp()
{
    std::cout << "Usage:\n";
    std::cout << "  ArctixCLI                 Start the interactive shell.\n";
    std::cout << "  ArctixCLI <command> ...   Run one arctix command and exit.\n";
    std::cout << "  ArctixCLI --help          Show this help.\n";
    std::cout << "  ArctixCLI --version       Show versions.\n";
    std::cout << "  ArctixCLI --bench ...     Shortcut for 'bench ...'.\n";
    std::cout << "  ArctixCLI --check         Shortcut for 'check'.\n";
    std::cout << "\n";
    arctixcli::PrintHelpOverview();
}

} // namespace

int main(int argc, char** argv)
{
    arctixcli::console::AutoDetect();
    if (argc < 2)
    {
        return arctixcli::Repl();
    }

    std::string first = argv[1];
    if (first == "--help" || first == "-h" || first == "/?")
    {
        PrintOneShotHelp();
        return 0;
    }
    if (first == "--version" || first == "-v")
    {
        return arctixcli::RunCommand("version", {"version"});
    }
    if (first == "--bench")
    {
        std::vector<std::string> args = {"bench"};
        for (int i = 2; i < argc; ++i)
        {
            args.emplace_back(argv[i]);
        }
        return arctixcli::RunCommand("bench", args);
    }
    if (first == "--check")
    {
        return arctixcli::RunCommand("check", {"check"});
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
    {
        args.emplace_back(argv[i]);
    }
    return arctixcli::RunCommand(args[0], args);
}
