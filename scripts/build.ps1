# scripts/build.ps1
$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Build = Join-Path $Root "build"

New-Item -ItemType Directory -Force -Path $Build | Out-Null

# Configure (VS 2022 x64)
cmake -S $Root -B $Build -G "Visual Studio 18 2026" -A x64

# Build (Debug)
cmake --build $Build --config Debug
