@echo off
setlocal

set "ROOT=%~dp0"
set "APP=%ROOT%TSRE5.exe"

if not exist "%APP%" (
    set "APP=%ROOT%TSRE5-test-dist\TSRE5.exe"
)

if not exist "%APP%" (
    set "APP=%ROOT%dist\tsre-scomod-v0.2\TSRE5.exe"
)

if not exist "%APP%" (
    echo TSRE5.exe was not found.
    echo.
    echo Keep this shortcut helper in the top-level TSRE folder,
    echo or in the same folder as TSRE5.exe.
    pause
    exit /b 1
)

set "ICON=%ROOT%content\tsre.ico"
if not exist "%ICON%" (
    set "ICON=%APP%"
)

set "TSRE_APP=%APP%"
set "TSRE_ICON=%ICON%"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$target=$env:TSRE_APP; $icon=$env:TSRE_ICON; $shortcutPath=Join-Path ([Environment]::GetFolderPath('Desktop')) 'TSRE GenX.lnk'; $ws=New-Object -ComObject WScript.Shell; $s=$ws.CreateShortcut($shortcutPath); $s.TargetPath=$target; $s.WorkingDirectory=Split-Path -Parent $target; $s.IconLocation=$icon; $s.Description='Launch TSRE GenX route editor'; $s.Save(); Write-Host 'Created desktop shortcut:' $shortcutPath"

echo.
pause
