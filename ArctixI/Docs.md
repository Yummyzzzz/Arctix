# ArctixI — Arctix Installer

**Component:** ArctixI (Arctix Installer: `arctixi.exe` setup wizard, `arctix.exe` helper, `uninstall.exe` remover)
**Developer:** Whitepoint Software Development (independent team, not a registered company or legal entity) — **Author / Owner:** Yummyzzzz (https://github.com/Yummyzzzz)
**Status:** Implemented. Pure Win32 API, no frameworks, no installer toolkit.

## What it is

A standalone `.exe` setup wizard. It installs Arctix on a developer machine:

- `arctix.exe`, `uninstall.exe`, `LICENSE.md` → install root (default `C:\Program Files\Arctix`)
- `arctixc.dll` + `arctixc_c.dll`, `arctixc.lib` + `arctixc_c.lib`, `.exp` files, test programs and examples → `dependencies\`
- an empty `temp\` scratch folder for future use
- registry keys (`HKLM\SOFTWARE\Arctix` + an Add/Remove Programs entry)
- optionally, the install folder on the **machine PATH**, so `arctix <command>`
  works in any PowerShell / console window (this is the checkbox on the options page)

## Wizard pages

1. **Welcome** — what will be installed.
2. **License** — shows the full `LICENSE.md`, pulled live from the repo
   (`ARCTIX_URL_LICENSE` in `src/common.hpp`) when needed. Priority: a
   `LICENSE.md` shipped next to the setup exe first, then the copy embedded
   in the exe itself (RCDATA, so a lone `arctixi.exe` still works), then a
   fresh download from the repository (needs network). The **I accept**
   checkbox is required to continue.
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
to `arctixi.exe`; anything missing there is downloaded from the raw-file
URLs in `kDependencies` (`src/common.hpp`, base `ARCTIX_URL_BINARIES` pointing
at `ArctixC/binaries/` on the main branch). Note: the download only works
once the repo is public — the installer downloads anonymously, and a private
repo answers 404. Until then, ship the `binaries/` folder next to the setup
(a lone `arctixi.exe` still installs, but the payload must come from somewhere).
A missing file with no URL aborts the install with a clear message instead of
leaving a half-installed state.

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
| `licenses`  | prints the full Arctix License 1.0 (`license` works too; downloads it from the repo if the install-folder copy is missing) |
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
`ArctixI/dist/` — the shippable setup folder (`arctixi.exe`, `arctix.exe`,
`uninstall.exe`, `LICENSE.md`, `binaries/`). `dist/` is git-ignored; ship the
whole folder as-is.

## Sources

- `src/installer.cpp` — the wizard (`arctixi.exe`)
- `src/arctix_cli.cpp` — the helper (`arctix.exe`)
- `src/uninstall.cpp` — the remover (`uninstall.exe`)
- `src/common.hpp` — versions, registry keys, URLs, payload/download lists
- `resources/arctixi.rc.in` — VERSIONINFO (CompanyName Whitepoint Software Development) plus the
  embedded `LICENSE.md` (RCDATA, setup exe only; admin manifests for setup
  + uninstaller come from linker flags in `CMakeLists.txt`)
