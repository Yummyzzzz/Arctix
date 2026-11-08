# ArctixI — Arctix Installer

**Component:** ArctixI (Arctix Installer: `ArctixI.exe` setup wizard, `arctix.exe` helper, `uninstall.exe` remover)
**Whitepoint:** Whitepoint Software Development (independent, not a registered company) — **Author / Owner:** Yummyzzzz (https://github.com/Yummyzzzz)
**Status:** Implemented. Pure Win32 API, no frameworks, no installer toolkit.

## What it is

A standalone `.exe` setup wizard. It installs Arctix on a developer machine:

- `arctix.exe`, `uninstall.exe`, `LICENSE.md` → install root (default `C:\Program Files\Arctix`)
- `arctixc.dll`, `arctixc.lib`, `arctixc.exp`, test programs and examples → `dependencies\`
- an empty `temp\` scratch folder for future use
- registry keys (`HKLM\SOFTWARE\Arctix` + an Add/Remove Programs entry)
- optionally, the install folder on the **machine PATH**, so `arctix <command>`
  works in any PowerShell / console window (this is the checkbox on the options page)

## Wizard pages

1. **Welcome** — what will be installed.
2. **License** — shows the full `LICENSE.md` shipped next to the setup exe.
   The **I accept** checkbox is required; without `LICENSE.md` beside
   `ArctixI.exe` the install cannot continue.
3. **Install location** — any folder works (local drive, Desktop, `C:\`, …).
   Default is `C:\Program Files`. If the chosen folder is not itself named
   `Arctix`, a subfolder named `Arctix` is appended automatically.
4. **Options** — the PATH checkbox (on by default) plus a summary of the layout.
5. **Install** — creates the folders, copies the files (or downloads them, see
   below), writes the registry, extends PATH. Requires admin rights, which the
   setup requests via its manifest.
6. **Finish** — summary with an *Open install folder* button.

## Where the payload comes from

At install time each dependency file is taken from a `binaries\` folder next
to `ArctixI.exe`; anything missing there is fetched from the download table in
`src/common.hpp`. Those URLs are currently **empty placeholders** — the
binaries are not hosted publicly yet. A missing file with no URL aborts the
install with a clear message instead of leaving a half-installed state. Once
the links are public, fill them into `kDependencies` and rebuild.

The in-repo source of the payload is `ArctixC/binaries/` (committed to git on
purpose — unlike `build/`, which stays ignored). See its README for the file list.

## The `arctix` command

After installing with the PATH checkbox on, any new console understands:

| Command     | Effect |
| ----------- | ------ |
| `version`   | installer + core versions, install folder |
| `website`   | prints the repository URL and opens it |
| `contact`   | repository, developer (Yummyzzzz) and team (Whitepoint Software Development) |
| `bug`       | how/where to report bugs, opens the issues page |
| `licenses`  | prints the full Arctix License 1.0 from the install folder |
| `uninstall` | asks for confirmation, then launches `uninstall.exe` |
| `delete`    | empties the `temp\` scratch folder (keeps the folder) |
| `help`      | this list (`helpe`, `-h`, `--help` work too; `delet` = `delete`) |

With no arguments `arctix` prints the help. `version` and `help` also work
before installation (the tool falls back to its own folder).

## Uninstall

Run `arctix uninstall`, use Add/Remove Programs, or run `uninstall.exe` from
the install folder. It removes the PATH entry, both registry keys and the
whole install folder (everything inside it), then deletes itself via a
`%TEMP%` staging copy — there is nothing left behind to clean manually.

## Build

```powershell
powershell -ExecutionPolicy Bypass -File ArctixI\build.ps1
```

Builds all three exes (Release), runs the CLI smoke tests, and assembles
`ArctixI/dist/` — the shippable setup folder (`ArctixI.exe`, `arctix.exe`,
`uninstall.exe`, `LICENSE.md`, `binaries/`). `dist/` is git-ignored; ship the
whole folder as-is.

## Sources

- `src/installer.cpp` — the wizard (`ArctixI.exe`)
- `src/arctix_cli.cpp` — the helper (`arctix.exe`)
- `src/uninstall.cpp` — the remover (`uninstall.exe`)
- `src/common.hpp` — versions, registry keys, URLs, payload/download lists
- `resources/arctixi.rc.in` — VERSIONINFO (CompanyName Whitepoint Software Development, admin manifests
  for setup + uninstaller via CMake)
