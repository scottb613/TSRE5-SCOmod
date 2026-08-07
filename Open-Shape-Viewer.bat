@echo off
setlocal

set "TSRE_RUNTIME=%~dp0TSREvcTST"
set "TSRE_EXE=%TSRE_RUNTIME%\TSRE5.exe"

if not exist "%TSRE_EXE%" (
    echo TSRE GenX v0.9 was not found:
    echo "%TSRE_EXE%"
    echo.
    echo Build and copy TSRE5.exe into TSREvcTST before launching Shape Viewer.
    pause
    exit /b 1
)

pushd "%TSRE_RUNTIME%" >nul
if errorlevel 1 (
    echo Could not open the controlled runtime folder:
    echo "%TSRE_RUNTIME%"
    pause
    exit /b 1
)

if "%~1"=="" (
    start "TSRE GenX v0.9 Shape Viewer" "%TSRE_EXE%" --shapeview
) else (
    start "TSRE GenX v0.9 Shape Viewer" "%TSRE_EXE%" --shapeview --file "%~f1"
)

popd
endlocal
