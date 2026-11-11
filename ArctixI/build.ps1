param([switch]$NoTests)

$ErrorActionPreference = "Stop"

# ArctixI build script -> produces the three installer programs (Release):
#   arctixi.exe    the setup wizard (pure Win32 GUI, requires admin)
#   arctix.exe     the installed `arctix <command>` helper
#   uninstall.exe  the installed remover (requires admin)
#
# Plus tests (ctest) and a ready-to-ship dist/ folder:
#   ArctixI/dist/arctixi.exe, arctix.exe, uninstall.exe, LICENSE.md,
#   binaries/ (copied from ArctixC/binaries/).
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File ArctixI\build.ps1
#   powershell -ExecutionPolicy Bypass -File ArctixI\build.ps1 -NoTests

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $Root "build"
$DistDir = Join-Path $Root "dist"
$CmakeBin = "cmake"

# Prefer Visual Studio bundled CMake (has Ninja alongside).
$VsCmake = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$VsNinjaDir = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
if ((Test-Path $VsCmake) -and (Test-Path (Join-Path $VsNinjaDir "ninja.exe"))) {
    $CmakeBin = $VsCmake
    $env:PATH = "$VsNinjaDir;$env:PATH"
}

function Ensure-Msvc {
    if (Get-Command cl.exe -ErrorAction SilentlyContinue) { return }
    $vsWhereExe = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $install = $null
    if (Test-Path $vsWhereExe) {
        $install = & $vsWhereExe -latest -property installationPath 2>$null
        if ($install -is [array]) { $install = $install[0] }
        $install = $install.Trim()
    }
    if (-not $install) { $install = "C:\Program Files\Microsoft Visual Studio\18\Community" }
    $vcvars = Join-Path $install "VC\Auxiliary\Build\vcvars64.bat"
    if (Test-Path $vcvars) {
        Write-Host "Loading MSVC environment from $vcvars ..."
        $tmp = [System.IO.Path]::GetTempFileName()
        cmd /c "`"$vcvars`" >NUL && set" | Set-Content $tmp
        Get-Content $tmp | ForEach-Object {
            if ($_ -match "^(.*?)=(.*)$") {
                Set-Item -Path ("env:" + $matches[1]) -Value $matches[2]
            }
        }
        Remove-Item $tmp -ErrorAction SilentlyContinue
    } else {
        Write-Host "WARNING: vcvars64.bat not found at $vcvars" -ForegroundColor Yellow
    }
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        Write-Host "WARNING: cl.exe still not on PATH" -ForegroundColor Yellow
    }
}
Ensure-Msvc

if (-not (Test-Path (Join-Path $BuildDir "build.ninja"))) {
    Write-Host "Configuring ArctixI (CMake + Ninja, Release) ..."
    & $CmakeBin -S $Root -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} else {
    Write-Host "Build directory already configured."
}

Write-Host "Building ArctixI ..."
& $CmakeBin --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

foreach ($exe in @("arctixi.exe", "arctix.exe", "uninstall.exe")) {
    $found = Get-ChildItem -Path $BuildDir -Recurse -Filter $exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $found) {
        Write-Host ("ERROR: expected output " + $exe + " was not produced") -ForegroundColor Red
        exit 1
    }
    Write-Host ("  " + $found.FullName)
}

if (-not $NoTests) {
    Write-Host ""
    Write-Host "Running tests (ctest) ..."
    Push-Location $BuildDir
    try {
        & ctest --output-on-failure
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    } finally {
        Pop-Location
    }
}

# Validate embedded manifests (malformed XML = Windows refuses to start
# the exe with a side-by-side error). Must run after linking, before dist.
Write-Host ""
Write-Host "Validating embedded manifests..."
$builtExes = foreach ($exe in @("arctixi.exe", "arctix.exe", "uninstall.exe")) {
    (Get-ChildItem -Path $BuildDir -Recurse -Filter $exe -ErrorAction SilentlyContinue | Select-Object -First 1).FullName
}
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root "tools\check-manifests.ps1") @builtExes
if ($LASTEXITCODE -ne 0) { throw "Embedded manifest validation failed." }

# Assemble dist/: everything the setup wizard needs next to it.
Write-Host ""
Write-Host "Assembling dist/ ..."
if (Test-Path $DistDir) { Remove-Item -Recurse -Force $DistDir }
New-Item -ItemType Directory -Path $DistDir | Out-Null
New-Item -ItemType Directory -Path (Join-Path $DistDir "binaries") | Out-Null
foreach ($exe in @("arctixi.exe", "arctix.exe", "uninstall.exe")) {
    $found = Get-ChildItem -Path $BuildDir -Recurse -Filter $exe -ErrorAction SilentlyContinue | Select-Object -First 1
    Copy-Item $found.FullName (Join-Path $DistDir $exe)
}
Copy-Item (Join-Path (Split-Path -Parent $Root) "LICENSE.md") (Join-Path $DistDir "LICENSE.md")
# Only the payload the wizard installs (matches kDependencies in
# ArctixI/src/common.hpp) - no READMEs, no static build byproducts.
$BinSrc = Join-Path (Split-Path -Parent $Root) "ArctixC\binaries"
$BinDst = Join-Path $DistDir "binaries"
$Payload = @("arctixc.dll", "arctixc.lib", "arctixc.exp", "arctixc_c.dll", "arctixc_c.lib", "arctixc_c.exp", "ArctixC_ExtendedTests.exe", "ArctixC_MemoryTests.exe", "ArctixC_ThreadTests.exe", "ArctixC_C_ExtendedTests.exe", "ArctixC_C_MemoryTests.exe", "ArctixC_C_ThreadTests.exe", "ArctixC_CApiTests.exe", "Example_Extended.exe", "Example_MemoryAndThread.exe", "Example_MemoryOnly.exe", "Example_ThreadOnly.exe", "Example_CApiDemo.exe")
foreach ($f in $Payload) {
    $src = Join-Path $BinSrc $f
    if (Test-Path $src) { Copy-Item $src $BinDst -Force }
    else { Write-Host ("WARNING: payload file missing: " + $src) -ForegroundColor Yellow }
}
Write-Host ""
Write-Host "OK: ArctixI build complete." -ForegroundColor Green
Write-Host ("Ship the whole folder: " + $DistDir)
Write-Host "It contains arctixi.exe + arctix.exe + uninstall.exe + LICENSE.md + binaries/."
