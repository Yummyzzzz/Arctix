// ArctixCLI - interactive shell: prompt loop, tokenizer, dispatch.
// Looks and feels like cmd (banner, prompt, cls, echo) but only arctix
// commands exist here; unknown input is rejected, never shelled out.

#include "shell.hpp"

#include "cli.hpp"
#include "commands.hpp"
#include "console.hpp"

#include <cstdlib>
#include <iostream>

namespace arctixcli
{

namespace
{

std::vector<std::string>& HistoryStore()
{
    static std::vector<std::string> history;
    return history;
}

} // namespace

void ShellRecordHistory(const std::string& line)
{
    HistoryStore().push_back(line);
}

const std::vector<std::string>& ShellHistory()
{
    return HistoryStore();
}

std::vector<std::string> Tokenize(const std::string& line)
{
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    bool hasToken = false;

    for (std::size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];
        if (inQuotes)
        {
            if (c == '"')
            {
                inQuotes = false;
            }
            else
            {
                current += c;
                hasToken = true;
            }
        }
        else if (c == '"')
        {
            inQuotes = true;
            hasToken = true;
        }
        else if (c == ' ' || c == '\t' || c == '\r')
        {
            if (hasToken)
            {
                tokens.push_back(current);
                current.clear();
                hasToken = false;
            }
        }
        else
        {
            current += c;
            hasToken = true;
        }
    }
    if (hasToken)
    {
        tokens.push_back(current);
    }
    return tokens;
}

int RunLine(const std::string& line)
{
    std::vector<std::string> argv = Tokenize(line);
    if (argv.empty())
    {
        return 0;
    }
    return RunCommand(argv[0], argv);
}

void PrintBanner()
{
    {
        console::Scope cyan(console::Color::Cyan);
        std::cout << "ArctixCLI [Version " << CliVersionString() << "]";
    }
    std::cout << "\n(c) Whitepoint Software Development. Type \"help\" for arctix commands.\n";
}

int Repl()
{
    console::AutoDetect();
    PrintBanner();
    std::string line;
    int lastCode = 0;
    for (;;)
    {
        {
            console::Scope cyan(console::Color::Cyan);
            std::cout << "arctix> " << std::flush;
        }
        if (!std::getline(std::cin, line))
        {
            // EOF (Ctrl+Z + Enter): leave like cmd does.
            std::cout << "\n";
            break;
        }
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        std::vector<std::string> argv = Tokenize(line);
        if (argv.empty())
        {
            continue;
        }
        ShellRecordHistory(line);
        const Command* cmd = FindCommand(argv[0]);
        if (cmd == nullptr)
        {
            console::Scope red(console::Color::Red);
            std::cout << "'" << argv[0] << "' is not recognized as an arctix command. Type \"help\".\n";
            lastCode = 1;
            continue;
        }
        lastCode = cmd->run(argv);
        if (cmd->name == std::string("exit") || cmd->name == std::string("quit"))
        {
            break;
        }
    }
    return lastCode;
}

} // namespace arctixcli
