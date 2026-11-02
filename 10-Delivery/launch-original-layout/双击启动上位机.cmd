@echo off
setlocal
chcp 65001 >nul
set "APPDIR=%~dp0腕部驱动设备\build\Desktop_Qt_6_8_2_MinGW_64_bit_Release"
if not exist "%APPDIR%\Uart_controller.exe" (
  echo Application missing. Extract the entire ZIP before running.
  pause
  exit /b 2
)
if /I "%~1"=="--check-paths" (
  for %%F in (Uart_controller.exe Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6Charts.dll Qt6SerialPort.dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll platforms\qwindows.dll qt.conf) do (
    if not exist "%APPDIR%\%%F" (
      echo Missing file: %%F
      exit /b 3
    )
  )
  echo Portable application paths OK.
  exit /b 0
)
set "QT_PLUGIN_PATH="
set "QT_QPA_PLATFORM_PLUGIN_PATH="
set "QT_QPA_PLATFORM="
pushd "%APPDIR%"
if errorlevel 1 exit /b 4
start "" "%CD%\Uart_controller.exe"
popd
exit /b 0
