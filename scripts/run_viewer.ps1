# scripts/run_viewer.ps1
$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Build = Join-Path $Root "build"

$Exe = Join-Path $Build "apps\viewer\Debug\viewer.exe"
if (-not (Test-Path $Exe)) {
  $Exe = Join-Path $Build "apps\viewer\Release\viewer.exe"
}

if (-not (Test-Path $Exe)) {
  throw "viewer.exe not found. Build first with scripts\build.ps1."
}

Set-Location $Root
& $Exe "assets\test_scenes\week2test.json"


