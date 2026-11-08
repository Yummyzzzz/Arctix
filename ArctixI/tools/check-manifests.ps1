# Validates the manifests embedded in the ArctixI executables.
# Fails the build if a manifest is missing or is not well-formed XML
# (an unquoted attribute once made Windows refuse to start arctixi.exe
# with a "side-by-side configuration is incorrect" error).
# Usage: check-manifests.ps1 <exe> [<exe> ...]

$ErrorActionPreference = 'Stop'

$reader = @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class ManifestReader {
  [DllImport("kernel32.dll", CharSet=CharSet.Unicode)]
  public static extern IntPtr LoadLibraryEx(string p, IntPtr h, uint f);
  [DllImport("kernel32.dll")]
  public static extern IntPtr FindResource(IntPtr h, IntPtr id, IntPtr type);
  [DllImport("kernel32.dll")]
  public static extern IntPtr LoadResource(IntPtr h, IntPtr r);
  [DllImport("kernel32.dll")]
  public static extern IntPtr LockResource(IntPtr r);
  [DllImport("kernel32.dll")]
  public static extern uint SizeofResource(IntPtr h, IntPtr r);
  [DllImport("kernel32.dll")]
  public static extern bool FreeLibrary(IntPtr h);
}
'@
Add-Type -TypeDefinition $reader

$failed = $false
foreach ($exe in $args) {
  $full = (Resolve-Path $exe).Path
  $h = [ManifestReader]::LoadLibraryEx($full, [IntPtr]::Zero, 0x2)
  try {
    $r = [ManifestReader]::FindResource($h, [IntPtr]1, [IntPtr]24)
    if ($r -eq [IntPtr]::Zero) { throw 'no RT_MANIFEST resource embedded' }
    $lr = [ManifestReader]::LoadResource($h, $r)
    $p = [ManifestReader]::LockResource($lr)
    $n = [ManifestReader]::SizeofResource($h, $r)
    $bytes = New-Object byte[] $n
    [Runtime.InteropServices.Marshal]::Copy($p, $bytes, 0, $n)
    $text = [Text.Encoding]::UTF8.GetString($bytes)
    [void]([xml]$text)  # throws on malformed XML
    $level = ([xml]$text).assembly.trustInfo.security.requestedPrivileges.requestedExecutionLevel.level
    Write-Host "OK: $exe (executionLevel=$level)"
  } catch {
    Write-Host "FAIL: $exe : $_" -ForegroundColor Red
    $failed = $true
  } finally {
    [ManifestReader]::FreeLibrary($h) | Out-Null
  }
}
if ($failed) { exit 1 }
