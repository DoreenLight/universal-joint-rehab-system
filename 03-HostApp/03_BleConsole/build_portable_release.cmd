@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"
set "BUILD_DIR=%PROJECT_DIR%\build\PortableRelease"
set "PACKAGE_DIR=%PROJECT_DIR%\build\BleConsole_Portable"
set "CMAKE=C:\Qt\Tools\CMake_64\bin\cmake.exe"
set "NINJA=C:\Qt\Tools\Ninja\ninja.exe"
set "QT_DIR=C:\Qt\6.11.1\msvc2022_64"
set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

call "%VCVARS%" >nul
if errorlevel 1 exit /b %errorlevel%

"%CMAKE%" -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -G Ninja ^
  "-DCMAKE_MAKE_PROGRAM=%NINJA%" ^
  -DCMAKE_BUILD_TYPE=Release ^
  "-DCMAKE_PREFIX_PATH=%QT_DIR%"
if errorlevel 1 exit /b %errorlevel%

"%CMAKE%" --build "%BUILD_DIR%" --parallel
if errorlevel 1 exit /b %errorlevel%

if not exist "%PACKAGE_DIR%" mkdir "%PACKAGE_DIR%"
copy /y "%BUILD_DIR%\BleConsole.exe" "%PACKAGE_DIR%\BleConsole.exe" >nul
if errorlevel 1 exit /b %errorlevel%

"%QT_DIR%\bin\windeployqt.exe" --release --compiler-runtime --dir "%PACKAGE_DIR%" "%PACKAGE_DIR%\BleConsole.exe"
if errorlevel 1 exit /b %errorlevel%

echo.
echo Portable application:
echo "%PACKAGE_DIR%\BleConsole.exe"
endlocal
