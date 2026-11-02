param([switch]$CheckOnly, [string]$OutputPath)
$ErrorActionPreference = 'Stop'
$packageRoot = Split-Path -Parent $PSScriptRoot
$appDir = Join-Path $packageRoot 'DesktopHost\build\Desktop_Qt_6_8_2_MinGW_64_bit_Release'
$app = Join-Path $appDir 'Uart_controller.exe'
if (-not (Test-Path -LiteralPath $app)) { throw 'Extract the complete package before creating its desktop shortcut.' }
if ($CheckOnly) { 'Shortcut target exists.'; exit 0 }
if (-not $OutputPath) {
    $desktopFolder = [Environment]::GetFolderPath('Desktop')
    $OutputPath = Join-Path $desktopFolder '腕关节康复设备-上位机.lnk'
}
if (Test-Path -LiteralPath $OutputPath) {
    $oldShortcut = [IO.Path]::ChangeExtension($OutputPath, '.previous-' + [DateTime]::Now.ToString('yyyyMMdd-HHmmssfff') + '.lnk')
    Copy-Item -LiteralPath $OutputPath -Destination $oldShortcut
}
$wsh = New-Object -ComObject WScript.Shell
$link = $wsh.CreateShortcut($OutputPath)
$link.TargetPath = $app
$link.WorkingDirectory = $appDir
$link.IconLocation = $app + ',0'
$link.Description = '腕关节康复上位机：Motor1/Motor2独立单程调速'
$link.Save()
$check = $wsh.CreateShortcut($OutputPath)
if ($check.TargetPath -ne $app) { throw 'Shortcut verification failed.' }
'Created: ' + $OutputPath
