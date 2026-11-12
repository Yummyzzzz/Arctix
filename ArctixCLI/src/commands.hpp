#pragma once

// ArctixCLI - built-in command table. All arctix commands live here.

#include "cli.hpp"

#include <cstddef>

namespace arctixcli
{

// Finds a command by name (case-insensitive). Returns nullptr when unknown.
const Command* FindCommand(const std::string& name);

// Number of entries in the table (for help).
std::size_t CommandCount();

// Command by index in [0, CommandCount()).
const Command& CommandAt(std::size_t index);

// Runs a parsed line's command; prints an error for unknown names.
// Returns the command exit code (0 = ok).
int RunCommand(const std::string& name, const std::vector<std::string>& argv);

// Prints every command grouped under colored category headings,
// most useful categories first. Used by `help` and `--help`.
void PrintHelpOverview();

} // namespace arctixcli
