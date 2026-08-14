@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd %~dp0\..\src

set SCRIPT_DIR=.
if "%SCRIPT_DIR:~-1%"=="\" set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

set EXE=%SCRIPT_DIR%\build\Release\mapimggen.exe
if not exist "%EXE%" (
    echo ERROR: executable not found: %EXE%
    echo Build first: src\build.bat
    exit /b 1
)

set WIDTH=512
set DEPTH=512
set SCALE=2.0
set SEA_LEVEL=0.48
set RIDGE_STRENGTH=0.55

for /f %%I in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set SESSION=%%I
set OUT_DIR=%SCRIPT_DIR%\..\out\%SESSION%

if not exist "%OUT_DIR%" (
    mkdir "%OUT_DIR%"
)

set /a BASE_SEED=%RANDOM% * 32768 + %RANDOM%

echo Session: %SESSION%
echo Output : %OUT_DIR%

set FAIL=0
for /L %%I in (1,1,30) do (
    set /a SEED=!BASE_SEED! + %%I * 7919
    "%EXE%" -type world -w %WIDTH% -d %DEPTH% -seed !SEED! -p scale=%SCALE% -p seaLevel=%SEA_LEVEL% -p ridgeStrength=%RIDGE_STRENGTH% -dir "%OUT_DIR%" -out %%I --png >nul
    if errorlevel 1 (
        set /a FAIL+=1
        echo [%%I/30] FAILED  seed=!SEED!
    ) else (
        echo [%%I/30] OK      seed=!SEED! -^> %%I.png
    )
)

if %FAIL% gtr 0 (
    echo Completed with %FAIL% failure(s).
    exit /b 2
)

echo Completed successfully.
echo Files: %OUT_DIR%\1.png to %OUT_DIR%\30.png
exit /b 0
