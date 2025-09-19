@echo off
setlocal enabledelayedexpansion

echo EQIsolator3 Build Script
echo ========================

REM Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake from https://cmake.org/download/
    echo and make sure it's added to your system PATH
    goto :error
)

echo [✓] CMake found

REM Path to JUCE
set JUCE_PATH=C:/audio-plugins-dev/tools/JUCE

REM Check if JUCE exists
if not exist "%JUCE_PATH%" (
    echo ERROR: JUCE not found at %JUCE_PATH%
    echo Please update JUCE_PATH in this script
    goto :error
)

echo [✓] JUCE found at %JUCE_PATH%

REM Check for Visual Studio installations
set "VS_GENERATOR="
set "VS_PATH="

REM Check for VS 2022 using vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do (
        set "VS_PATH=%%i"
        set "VS_GENERATOR=Visual Studio 17 2022"
        echo [✓] Found Visual Studio 2022 at %%i
        goto :found_vs
    )
)

REM Check for VS 2022 (fallback to registry)
reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v "17.0" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set "VS_GENERATOR=Visual Studio 17 2022"
    for /f "tokens=2*" %%a in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v "17.0"') do set "VS_PATH=%%b"
    echo [✓] Found Visual Studio 2022
    goto :found_vs
)

REM Check for VS 2019
reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v "16.0" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set "VS_GENERATOR=Visual Studio 16 2019"
    for /f "tokens=2*" %%a in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v "16.0"') do set "VS_PATH=%%b"
    echo [✓] Found Visual Studio 2019
    goto :found_vs
)

REM Check for VS 2017
reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v "15.0" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set "VS_GENERATOR=Visual Studio 15 2017 Win64"
    for /f "tokens=2*" %%a in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v "15.0"') do set "VS_PATH=%%b"
    echo [✓] Found Visual Studio 2017
    goto :found_vs
)

REM Check for Build Tools 2022
reg query "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7" /v "17.0" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set "VS_GENERATOR=Visual Studio 17 2022"
    for /f "tokens=2*" %%a in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7" /v "17.0"') do set "VS_PATH=%%b"
    echo [✓] Found Visual Studio Build Tools 2022
    goto :found_vs
)

REM Check for Build Tools 2019
reg query "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7" /v "16.0" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set "VS_GENERATOR=Visual Studio 16 2019"
    for /f "tokens=2*" %%a in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7" /v "16.0"') do set "VS_PATH=%%b"
    echo [✓] Found Visual Studio Build Tools 2019
    goto :found_vs
)

REM Check for Ninja build system as a fallback
where ninja >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [✓] Found Ninja build system, will use it with MSVC
    set "VS_GENERATOR=Ninja"
    goto :found_vs
)

echo ERROR: Could not find Visual Studio or Build Tools installation
echo Please install Visual Studio or Build Tools with C++ support
goto :error

:found_vs
echo [✓] Using generator: %VS_GENERATOR%

REM Initialize Visual Studio environment if needed
if not "%VS_GENERATOR%"=="Ninja" (
    if exist "%VS_PATH%\Common7\Tools\VsDevCmd.bat" (
        echo [✓] Initializing Visual Studio environment...
        call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
    )
)

REM Create build directory if it doesn't exist
if not exist build mkdir build

echo Configuring project with CMake...
cmake -B build -DJUCE_PATH=%JUCE_PATH% -G "%VS_GENERATOR%"

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    goto :error
)

echo Building project...
cmake --build build --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    goto :error
)

echo [✓] Build successful!
echo Plugin is located at: %CD%\build\Debug\VST3\EQIsolator3.vst3
goto :end

:error
echo.
echo Build failed. Please check the errors above.
exit /b 1

:end
echo.
echo Build process completed.
pause
exit /b 0