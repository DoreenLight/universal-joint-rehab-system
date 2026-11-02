param(
    [string]$QtRoot = 'C:\Qt\6.8.2\mingw_64',
    [string]$CompilerRoot = 'C:\Qt\Tools\mingw1310_64\bin',
    [string]$CMake = 'C:\Qt\Tools\CMake_64\bin\cmake.exe',
    [string]$Ninja = 'C:\Qt\Tools\Ninja\ninja.exe'
)
$ErrorActionPreference = 'Stop'
function Check-Exit([string]$Step) {
    if ($LASTEXITCODE -ne 0) { throw "$Step failed ($LASTEXITCODE)" }
}
Push-Location $PSScriptRoot
$savedPath = $env:PATH
$savedPlatform = $env:QT_QPA_PLATFORM
try {
    $env:PATH = "$QtRoot\bin;$CompilerRoot;$env:PATH"
    $env:QT_QPA_PLATFORM = 'offscreen'
    python prepare.py
    Check-Exit 'Scope check and fixture generation'
    & "$CompilerRoot\gcc.exe" -std=c99 -O2 -ffp-contract=off -Wall -Wextra -Werror -pedantic test_reference.c -lm -o build/test_reference.exe
    Check-Exit 'Production reference C compilation'
    & .\build\test_reference.exe | Tee-Object -FilePath results.txt
    Check-Exit 'Reference tests'
    # Host syntax checks only: these do not generate or link ARM firmware.
    $firmwareRoot = (Resolve-Path ../..).Path
    $syntaxArgs = @('-std=c99', '-fsyntax-only', '-DUSE_HAL_DRIVER', '-DSTM32F407xx')
    foreach ($inc in @('Core/Inc', 'app', 'bsp', 'Drivers/STM32F4xx_HAL_Driver/Inc',
                       'Drivers/STM32F4xx_HAL_Driver/Inc/Legacy',
                       'Drivers/CMSIS/Device/ST/STM32F4xx/Include', 'Drivers/CMSIS/Include')) {
        $syntaxArgs += "-I$firmwareRoot/$inc"
    }
    & "$CompilerRoot\gcc.exe" @syntaxArgs "$firmwareRoot/Core/Src/main.c" 2> firmware-syntax-current.txt
    Check-Exit 'Current main.c host syntax check (not ARM build)'
    & "$CompilerRoot\gcc.exe" @syntaxArgs -x c main.before.bin 2> firmware-syntax-original.txt
    Check-Exit 'Legacy main.c host syntax check (not ARM build)'
    & $CMake -S qt -B build/qt -G Ninja "-DCMAKE_MAKE_PROGRAM=$Ninja" "-DCMAKE_CXX_COMPILER=$CompilerRoot/g++.exe" "-DCMAKE_PREFIX_PATH=$QtRoot" -DCMAKE_BUILD_TYPE=Release
    Check-Exit 'Qt test configuration'
    & $CMake --build build/qt -j 4
    Check-Exit 'Qt test build'
    & .\build\qt\reference_telemetry_test.exe 2> qt-stderr.txt | Tee-Object -FilePath qt-results.txt
    Check-Exit 'Offline telemetry integration test'
    $hostSource = (Resolve-Path ../../../../03-HostApp/01_SerialConsole).Path
    & $CMake -S $hostSource -B build/host -G Ninja "-DCMAKE_MAKE_PROGRAM=$Ninja" "-DCMAKE_CXX_COMPILER=$CompilerRoot/g++.exe" "-DCMAKE_PREFIX_PATH=$QtRoot" -DCMAKE_BUILD_TYPE=Release | Tee-Object -FilePath host-build.txt
    Check-Exit 'Production Qt application configuration'
    & $CMake --build build/host -j 4 | Tee-Object -FilePath host-build.txt -Append
    Check-Exit 'Production Qt application build (not launched)'
} finally {
    $env:PATH = $savedPath
    $env:QT_QPA_PLATFORM = $savedPlatform
    Pop-Location
}
