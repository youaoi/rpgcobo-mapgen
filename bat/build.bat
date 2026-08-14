@echo off
setlocal
cd %~dp0\..\src

set SCRIPT_DIR=.
if "%SCRIPT_DIR:~-1%"=="\" set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%
set BUILD_DIR=..\build

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

cmake -S "%SCRIPT_DIR%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 exit /b 1

echo Build complete.

copy "%BUILD_DIR%\Release\*.exe" "%~dp0\.."
