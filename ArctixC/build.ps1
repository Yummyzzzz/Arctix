param([switch]$NoTests, [ValidateSet(17,20,23)][int]$CppStandard = 17)

$ErrorActionPreference = "Stop"

# ArctixC build script.
# Configures + builds BOTH standalone SDK binaries (Release) into the
# same folder:
#   arctixc.dll + arctixc.lib     (C++ API: arctix::Core/Memory/Buffer/Thread)
#   arctixc_c.dll + arctixc_c.lib (C API: Arctix_*/ArctixC_* from c_api/*.h)
# Both DLLs share the same sources, headers and namespace; only the
# binaries (and the C wrappers) differ. C++ consumers link arctixc.lib,
# C consumers link arctixc_c.lib.
# plus tests (each suite runs against both DLLs) and examples.
# Both DLLs land next to the test/example exes so they run without
# extra PATH setup.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File ArctixC\build.ps1
#   powershell -ExecutionPolicy Bypass -File ArctixC\build.ps1 -NoTests
#   powershell -ExecutionPolicy Bypass -File ArctixC\build.ps1 -CppStandard 20

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $Root "build"
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

$configureArgs = @("-S", $Root, "-B", $BuildDir, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DARCTIXC_CXX_STANDARD=$CppStandard")
if ($NoTests) {
    $configureArgs += "-DARCTIXC_BUILD_TESTS=OFF"
    $configureArgs += "-DARCTIXC_BUILD_EXAMPLES=OFF"
}

if (-not (Test-Path (Join-Path $BuildDir "build.ninja"))) {
    Write-Host "Configuring ArctixC (CMake + Ninja, Release, C++$CppStandard) ..."
    & $CmakeBin @configureArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} else {
    Write-Host "Build directory already configured."
    Write-Host "NOTE: to change C++ standard, delete ArctixC/build and re-run with -CppStandard <17|20|23>."
}

Write-Host "Building ArctixC (C++$CppStandard) ..."
& $CmakeBin --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Build outputs:" -ForegroundColor Green
Get-ChildItem -Path $BuildDir -Recurse -Include "arctixc.dll", "arctixc.lib", "arctixc_c.dll", "arctixc_c.lib" -ErrorAction SilentlyContinue |
    ForEach-Object { Write-Host ("  " + $_.FullName) }

foreach ($dll in @("arctixc.dll", "arctixc_c.dll")) {
    $found = Get-ChildItem -Path $BuildDir -Recurse -Filter $dll -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $found) {
        Write-Host ("ERROR: expected output " + $dll + " was not produced") -ForegroundColor Red
        exit 1
    }
}

# Fail the build if the old split DLLs are ever produced again.
$stale = Get-ChildItem -Path $BuildDir -Recurse -Include "*Memory*.dll", "*Thread*.dll" -ErrorAction SilentlyContinue | Select-Object -First 1
if ($stale) {
    Write-Host ("ERROR: stale split DLL still produced: " + $stale.FullName) -ForegroundColor Red
    exit 1
}

if (-not $NoTests) {
    Write-Host ""
    Write-Host "Running tests (ctest) ..."
    & $CmakeBin --build $BuildDir --target test 2>$null
    Push-Location $BuildDir
    try {
        & ctest --output-on-failure
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    } finally {
        Pop-Location
    }
}

# ---------------------------------------------------------------------------
# sdk/ staging (repo root): everything a consumer needs, split by flavor.
# Copies only - nothing inside ArctixC/ is moved or deleted.
#   sdk/cpp/      C++ flavor: arctixc.hpp (+ arctixc_all.hpp), .lib, .dll, static .lib
#   sdk/c/        C flavor:   arctixc_c.h, .lib, .dll
#   sdk/tests/    all test runners + both DLLs (so they run standalone)
#   sdk/examples/ all example exes + both DLLs
# ---------------------------------------------------------------------------
$SdkDir = Join-Path (Split-Path -Parent $Root) "sdk"
foreach ($d in @("cpp", "c", "tests", "examples")) {
    $p = Join-Path $SdkDir $d
    if (-not (Test-Path $p)) { New-Item -ItemType Directory -Path $p | Out-Null }
}
function Copy-Artifact($name, $destSub) {
    $found = Get-ChildItem -Path $BuildDir -Recurse -Filter $name -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { Copy-Item $found.FullName (Join-Path $SdkDir $destSub) -Force }
    else { Write-Host ("WARNING: sdk staging skipped, not found: " + $name) -ForegroundColor Yellow }
}
# C++ flavor
Copy-Artifact "arctixc.hpp" "cpp"
Copy-Artifact "arctixc_all.hpp" "cpp"
Copy-Artifact "arctixc.lib" "cpp"
Copy-Artifact "arctixc.dll" "cpp"
Copy-Artifact "arctixc_static.lib" "cpp"
# C flavor
Copy-Artifact "arctixc_c.h" "c"
Copy-Artifact "arctixc_c.lib" "c"
Copy-Artifact "arctixc_c.dll" "c"
# DLLs next to the runners so they execute standalone.
Copy-Artifact "arctixc.dll" "tests"
Copy-Artifact "arctixc_c.dll" "tests"
Copy-Artifact "arctixc.dll" "examples"
Copy-Artifact "arctixc_c.dll" "examples"
Get-ChildItem -Path $BuildDir -Recurse -Include "ArctixC_*.exe", "Example_*.exe" -ErrorAction SilentlyContinue |
    ForEach-Object {
        if ($_.Name -like "Example_*") { Copy-Item $_.FullName (Join-Path $SdkDir "examples") -Force }
        else { Copy-Item $_.FullName (Join-Path $SdkDir "tests") -Force }
    }
Write-Host ("sdk/ staged at " + $SdkDir) -ForegroundColor Green

Write-Host ""
Write-Host "OK: ArctixC build complete (C++$CppStandard)." -ForegroundColor Green
Write-Host "Ship arctixc.dll + arctixc.lib (C++) and arctixc_c.dll + arctixc_c.lib (C) + include/arctixc/."
