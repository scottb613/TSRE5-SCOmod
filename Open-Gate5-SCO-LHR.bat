@echo off
setlocal

set "TSRE_RUNTIME=%~dp0TSREvcTST"
set "TSRE_EXE=%TSRE_RUNTIME%\TSRE5.exe"
set "TSRE_ROOT=M:\ORTSmini_F\SCO_CLEAN\Train Simulator"
set "TSRE_ROUTE=SCO_LHR"

if not exist "%TSRE_EXE%" (
    echo TSRE GenX v0.9 was not found:
    echo "%TSRE_EXE%"
    pause
    exit /b 1
)

if not exist "%TSRE_ROOT%\routes\%TSRE_ROUTE%\%TSRE_ROUTE%.trk" (
    echo Gate 5 route was not found:
    echo "%TSRE_ROOT%\routes\%TSRE_ROUTE%"
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

start "TSRE GenX v0.9 Gate 5 - SCO_LHR" "%TSRE_EXE%" --routeedit --root "%TSRE_ROOT%" --route "%TSRE_ROUTE%"

popd
endlocal
