@echo off
SETLOCAL

SET "PSScriptRoot=%~dp0"
SET "PSScriptRoot=%PSScriptRoot:~0,-1%"

SET "OriginalDir=%CD%"

SET "BuildDir=%PSScriptRoot%\build"
SET "TargetPath=%BuildDir%\nc.sln"
SET "ShortcutPath=%PSScriptRoot%\nc.sln.lnk"

IF NOT EXIST "%BuildDir%" (
    echo Creating build directory...
    mkdir "%BuildDir%"
)

PUSHD "%BuildDir%"
IF ERRORLEVEL 1 (
    echo FAILED: Could not change directory to %BuildDir%
    GOTO :Cleanup
)

cmake .. -G "Visual Studio 17 2022"

IF ERRORLEVEL 1 (
    echo.
    echo --- FAILURE: CMake failed to generate project files. ---
    GOTO :Cleanup
)
echo CMake successful.

powershell.exe -ExecutionPolicy Bypass -Command "$WshShell = New-Object -ComObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%ShortcutPath%'); $Shortcut.TargetPath = '%TargetPath%'; $Shortcut.WorkingDirectory = '%PSScriptRoot%'; $Shortcut.Save()"

IF ERRORLEVEL 1 (
    echo.
    echo --- WARNING: Shortcut creation failed. ---
    GOTO :Cleanup
)

echo.
echo --- SUCCESS ---


:Cleanup
POPD

ENDLOCAL