#pragma once

// ArctixCLI - cmd-style interactive shell: prompt, tokenizer, dispatch.

#include <string>
#include <vector>

namespace arctixcli
{

// Splits a line into tokens. Double quotes group spaces into one token
// ("a b" -> [a b]); "" is an empty token. A trailing lone quote is
// treated literally so the shell never hangs asking for more input.
std::vector<std::string> Tokenize(const std::string& line);

// Runs one input line (empty lines are no-ops). Returns the exit code of
// the command that ran, or 1 for unknown commands.
int RunLine(const std::string& line);

// cmd-style banner printed once at startup.
void PrintBanner();

// Reads lines on stdin and runs them until exit/quit/EOF.
// Returns the exit code for main().
int Repl();

// Every non-empty line the shell runs, oldest first.
// `history` prints this; `run` does not pollute it.
void ShellRecordHistory(const std::string& line);
const std::vector<std::string>& ShellHistory();

} // namespace arctixcli
