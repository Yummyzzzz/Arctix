$ErrorActionPreference = "Stop"

# ArctixI build script -> produces ArctixI.exe (Arctix Installer).
#
# Status in this task: ArctixI is a DOCUMENTATION PLACEHOLDER. Its real
# implementation is out of scope, so this script builds it only once a real
# CMake target exists. If CMakeLists.txt already defines a project, a normal
# Release build runs; otherwise the script reports the placeholder status
# and exits 0 without failing.

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $Root "build"
$CmakeBin = "cmake"

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
    }
}
Ensure-Msvc

$cmakeLists = Join-Path $Root "CMakeLists.txt"
$hasProject = $false
if (Test-Path $cmakeLists) {
    $hasProject = (Select-String -Path $cmakeLists -Pattern "^\s*project\s*\(" -Quiet) -eq $true
}

if (-not $hasProject) {
    Write-Host "ArctixI is a documentation placeholder in this task - nothing to build yet." -ForegroundColor Yellow
    Write-Host "Real implementation will produce ArctixI.exe (installer packaging the ArctixC SDK)."
    exit 0
}

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

$exe = Get-ChildItem -Path $BuildDir -Recurse -Filter "ArctixI.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $exe) {
    Write-Host "Build finished but ArctixI.exe was not found in $BuildDir" -ForegroundColor Yellow
    exit 1
}
Write-Host ""
Write-Host ("OK: " + $exe.FullName) -ForegroundColor Green
