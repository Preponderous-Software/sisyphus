@echo off
setlocal
rem Plays Sisyphus (the software-upkeep game on the Strata engine) from the
rem latest main branch, building it first. The game saves in
rem %APPDATA%\Sisyphus\default and continues where you left off.
rem Controls: mouse look, W A S D walk (into the boulder from below to push it),
rem Shift hurry, hold E to patch a module, hold R to retire it, Q sit, V his eyes
rem or over his shoulder, M sound, F5 save, F9 load, F1 help, Esc frees the mouse.
rem Extra arguments are passed to the game, e.g.:
rem   play-sisyphus.bat --new                   start over (the old save is kept as the previous one)
rem   play-sisyphus.bat --autopilot --speed 4   watch the autopilot play, four times as fast

set "SIS_DIR=%USERPROFILE%\sisyphus-play"
set "SIS_REPO=https://github.com/Preponderous-Software/sisyphus.git"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "INSTALLED="

where git >nul 2>nul || (echo Git is not installed: https://git-scm.com/download/win & pause & exit /b 1)

where cmake >nul 2>nul || (
    echo Sisyphus needs CMake 3.24 or newer. Installing it with winget ...
    winget install --id Kitware.CMake -e --accept-source-agreements --accept-package-agreements || (echo Install it from https://cmake.org/download/ & pause & exit /b 1)
    set "INSTALLED=1"
)

set "HAS_MSVC="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "HAS_MSVC=1"
)
if not defined HAS_MSVC (
    echo Sisyphus needs the Visual Studio C++ build tools. Installing them with winget, this takes a while ...
    winget install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-source-agreements --accept-package-agreements --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" || (echo Install "Desktop development with C++" from https://visualstudio.microsoft.com/visual-cpp-build-tools/ & pause & exit /b 1)
    set "INSTALLED=1"
)

if defined INSTALLED (
    echo Tools installed. Close this window and run the script again.
    pause
    exit /b 0
)

if not exist "%SIS_DIR%\.git" (
    echo Cloning Sisyphus into %SIS_DIR% ...
    git clone -b main "%SIS_REPO%" "%SIS_DIR%" || (pause & exit /b 1)
) else (
    echo Updating %SIS_DIR% ...
    pushd "%SIS_DIR%"
    git fetch origin || (popd & pause & exit /b 1)
    git checkout -B main origin/main || (popd & pause & exit /b 1)
    popd
)

pushd "%SIS_DIR%"
echo Building (the first time fetches and builds Strata and SDL3, a few minutes) ...
cmake -S . -B build -DSISYPHUS_BUILD_TESTS=OFF -Wno-dev || (popd & pause & exit /b 1)
cmake --build build --config Release --parallel --target sisyphus || (popd & pause & exit /b 1)

build\Release\sisyphus.exe %*
set "RC=%ERRORLEVEL%"
popd
if not "%RC%"=="0" pause
exit /b %RC%
