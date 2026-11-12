$ErrorActionPreference = "Stop"

# ArctixCLI build script -> produces ArctixCLI.exe.
#
# Configures + builds ArctixCLI (Release, cmd-style interactive shell
# with built-in arctix commands only). The SDK (arctixc.dll) is pulled
# in from ../ArctixC sources and copied next to the exe automatically.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File ArctixCLI\build.ps1

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
    } else {
        Write-Host "WARNING: vcvars64.bat not found at $vcvars" -ForegroundColor Yellow
    }
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        Write-Host "WARNING: cl.exe still not on PATH" -ForegroundColor Yellow
    }
}
Ensure-Msvc

if (-not (Test-Path (Join-Path $BuildDir "build.ninja"))) {
    Write-Host "Configuring ArctixCLI (CMake + Ninja, Release) ..."
    & $CmakeBin -S $Root -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} else {
    Write-Host "Build directory already configured."
}

Write-Host "Building ArctixCLI ..."
& $CmakeBin --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exe = Get-ChildItem -Path $BuildDir -Recurse -Filter "ArctixCLI.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $exe) {
    Write-Host "Build finished but ArctixCLI.exe was not found in $BuildDir" -ForegroundColor Red
    exit 1
}
Write-Host ""
Write-Host ("OK: " + $exe.FullName) -ForegroundColor Green
Write-Host "Ship ArctixCLI.exe with arctixc.dll next to it."
