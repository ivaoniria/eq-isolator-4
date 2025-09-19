@echo off
setlocal enabledelayedexpansion

echo ==================================================
echo  EQIsolator4 - Static VST3 Build (no VC++ needed)
echo ==================================================

REM 1) Check CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake not found in PATH. Install from https://cmake.org/download/
    goto :error
)
echo [OK] CMake found

REM 2) Set JUCE path (adjust if needed)
set JUCE_PATH=C:/audio-plugins-dev/tools/JUCE
if not exist "%JUCE_PATH%" (
    echo [ERROR] JUCE not found at %JUCE_PATH%
    goto :error
)
echo [OK] JUCE at %JUCE_PATH%

REM 3) Find a VS generator (2022 preferred, then 2019)
set "VS_GENERATOR="
set "VS_PATH="

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

if defined VS_PATH (
    if exist "%VS_PATH%\Common7\Tools\VsDevCmd.bat" (
        REM Determine generator based on VS installation path
        echo %VS_PATH% | find /i "\2022\" >nul && set "VS_GENERATOR=Visual Studio 17 2022"
        if not defined VS_GENERATOR (
            echo %VS_PATH% | find /i "\2019\" >nul && set "VS_GENERATOR=Visual Studio 16 2019"
        )
        if not defined VS_GENERATOR set "VS_GENERATOR=Visual Studio 17 2022"
        echo [OK] Visual Studio at %VS_PATH%
    )
)

if not defined VS_GENERATOR (
    REM Fallback to registry
    reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v "17.0" >nul 2>&1 && set "VS_GENERATOR=Visual Studio 17 2022"
    if not defined VS_GENERATOR reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v "16.0" >nul 2>&1 && set "VS_GENERATOR=Visual Studio 16 2019"
)

if not defined VS_GENERATOR (
    echo [ERROR] Visual Studio 2019/2022 not found. Install Desktop development with C++.
    goto :error
)

echo [OK] Using generator: %VS_GENERATOR%

REM 4) Init VS environment (x64)
if defined VS_PATH if exist "%VS_PATH%\Common7\Tools\VsDevCmd.bat" (
    echo [INFO] Initializing VS Dev Cmd (x64)...
    call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
)

REM 5) Prepare dirs
set BUILD_DIR=static_build
set DIST_DIR=dist
if exist %BUILD_DIR% rmdir /S /Q %BUILD_DIR%
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %DIST_DIR% mkdir %DIST_DIR%

REM 6) Configure CMake with static MSVC runtime
REM    MultiThreaded for Release, MultiThreadedDebug for Debug
cmake -B %BUILD_DIR% -S . -G "%VS_GENERATOR%" -DJUCE_PATH=%JUCE_PATH% ^
      -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configure failed
    goto :error
)

echo [INFO] Building Release...
cmake --build %BUILD_DIR% --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed
    goto :error
)

echo [OK] Build finished

REM 7) Copy resulting .vst3 bundle to dist
set OUT_VST3=%BUILD_DIR%\Release\VST3\EQIsolator4.vst3
if not exist "%OUT_VST3%" (
    REM Try non-multi-config path as fallback
    set OUT_VST3=%BUILD_DIR%\VST3\EQIsolator4.vst3
)

if not exist "%OUT_VST3%" (
    echo [ERROR] Could not find output VST3 bundle.
    echo        Looked for: %BUILD_DIR%\Release\VST3\EQIsolator4.vst3
    echo                  or: %BUILD_DIR%\VST3\EQIsolator4.vst3
    goto :error
)

echo [INFO] Copying VST3 to %DIST_DIR% ...
xcopy /Y /E /I "%OUT_VST3%" "%DIST_DIR%\EQIsolator4.vst3" >nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to copy VST3 to dist
    goto :error
)

echo.
echo ==================================================
echo  DONE

echo  VST3 ready to share:
for %%F in ("%CD%\%DIST_DIR%\EQIsolator4.vst3") do echo      %%~fF

echo  Tell users to place the folder in:

echo      C:\\Program Files\\Common Files\\VST3

echo  Then rescan plugins in their DAW.
echo ==================================================

goto :end

:error
echo.
echo Build failed. See messages above.
exit /b 1

:end
endlocal
exit /b 0