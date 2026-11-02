param(
    [string]$QtRoot = 'C:\Qt\6.8.2\mingw_64',
    [string]$CompilerRoot = 'C:\Qt\Tools\mingw1310_64\bin',
    [string]$CMake = 'C:\Qt\Tools\CMake_64\bin\cmake.exe',
    [string]$Ninja = 'C:\Qt\Tools\Ninja\ninja.exe'
)
$ErrorActionPreference = 'Stop'
function Check-Exit([string]$step) {
    if ($LASTEXITCODE -ne 0) { throw "$step failed ($LASTEXITCODE)" }
}
Push-Location $PSScriptRoot
$savedPath = $env:PATH
$savedPlatform = $env:QT_QPA_PLATFORM
try {
    $env:PATH = "$QtRoot\bin;$CompilerRoot;$env:PATH"
    $env:QT_QPA_PLATFORM = 'offscreen'
    python prepare.py
    Check-Exit 'Scope and fixtures'
    & $CMake -S . -B build/validation -G Ninja "-DCMAKE_MAKE_PROGRAM=$Ninja" "-DCMAKE_CXX_COMPILER=$CompilerRoot/g++.exe" "-DCMAKE_PREFIX_PATH=$QtRoot" -DCMAKE_BUILD_TYPE=Release | Tee-Object -FilePath build-log.txt
    Check-Exit 'Configuration'
    & $CMake --build build/validation -j 4 | Tee-Object -FilePath build-log.txt -Append
    Check-Exit 'Application and test build'
    & .\build\validation\desktop_reference_test.exe 2> test-stderr.txt | Tee-Object -FilePath test-results.txt
    Check-Exit 'Desktop reference tests'
} finally {
    $env:PATH = $savedPath
    $env:QT_QPA_PLATFORM = $savedPlatform
    Pop-Location
}
