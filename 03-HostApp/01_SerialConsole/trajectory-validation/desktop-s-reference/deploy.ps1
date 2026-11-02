# Update only the tested host application and its two changed source files.
$ErrorActionPreference = 'Stop'
$manifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'baseline-sha256.json') -Raw | ConvertFrom-Json
$tested = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'tested-release.json') -Raw | ConvertFrom-Json
$exeTarget = $manifest.desktop_exe
$exeCandidate = Join-Path $PSScriptRoot 'build/validation/Uart_controller.exe'
$oldExe = Join-Path $PSScriptRoot 'build/rollback/Uart_controller.exe'
$changedSources = @('mainwindow.cpp', 'mainwindow.h')
function Hash([string]$path) { (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
if ((Hash $exeCandidate) -ne $tested.executable_sha256) { throw 'Executable is not the tested build' }
foreach ($name in $changedSources) {
    if ((Hash (Join-Path $PSScriptRoot "build/source/$name")) -ne $tested.source_sha256.$name) { throw "Untested candidate: $name" }
    if ((Hash (Join-Path $PSScriptRoot "build/before/$name")) -ne $manifest.source_sha256.$name) { throw "Rollback source mismatch: $name" }
}
foreach ($entry in $manifest.source_sha256.PSObject.Properties) {
    if ((Hash (Join-Path $manifest.source $entry.Name)) -ne $entry.Value) { throw "Desktop source changed after snapshot: $($entry.Name)" }
}
if ((Hash $exeTarget) -ne $manifest.desktop_exe_sha256) { throw 'Desktop executable changed after snapshot' }
if (@(Get-Process -Name Uart_controller -ErrorAction SilentlyContinue).Count) { throw 'An application instance is running; do not terminate an experiment automatically' }
if ((Hash $oldExe) -ne $manifest.desktop_exe_sha256) { throw 'Executable rollback backup mismatch' }
try {
    foreach ($name in $changedSources) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot "build/source/$name") -Destination (Join-Path $manifest.source $name) -Force
    }
    Copy-Item -LiteralPath $exeCandidate -Destination $exeTarget -Force
    foreach ($name in $changedSources) {
        if ((Hash (Join-Path $manifest.source $name)) -ne $tested.source_sha256.$name) { throw "Deployed source mismatch: $name" }
    }
    if ((Hash $exeTarget) -ne $tested.executable_sha256) { throw 'Executable deployment mismatch' }
} catch {
    foreach ($name in $changedSources) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot "build/before/$name") -Destination (Join-Path $manifest.source $name) -Force
    }
    Copy-Item -LiteralPath $oldExe -Destination $exeTarget -Force
    throw
}
[ordered]@{
    updatedUtc = [DateTime]::UtcNow.ToString('o')
    source = $manifest.source
    sourceSha256 = $tested.source_sha256
    executable = $exeTarget
    executableSha256 = (Hash $exeTarget)
    priorExecutableSha256 = $manifest.desktop_exe_sha256
    shortcutChanged = $false
    flashed = $false
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'deployment-receipt.json') -Encoding utf8
'PASS: adjustable host program and two source files deployed; prior version preserved; no flash operations'
