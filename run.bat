@echo off
rem Run the MWIS solver. Defaults: input.in, 300-second time limit.
rem Usage:  run.bat [INPUT_FILE] [TIME_LIMIT_SECONDS]
setlocal
cd /d "%~dp0"

if not exist build\solve.exe (
    echo Solver not built. Building now...
    call build.bat || exit /b 1
)

set INPUT=%1
set TIMELIMIT=%2
if "%INPUT%"=="" set INPUT=input.in
if "%TIMELIMIT%"=="" set TIMELIMIT=300

if not exist "%INPUT%" (
    echo Input file "%INPUT%" not found.
    echo Usage: run.bat [INPUT_FILE] [TIME_LIMIT_SECONDS]
    echo Defaults: input.in, 300 seconds
    exit /b 1
)

build\solve.exe -i %INPUT% -t %TIMELIMIT%
endlocal
