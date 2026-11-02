@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0create_desktop_shortcut.ps1"
if errorlevel 1 (
  echo Desktop shortcut creation failed.
  pause
  exit /b 1
)
echo Desktop shortcut created successfully.
pause
