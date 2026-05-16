@echo off
rem Build the MWIS solver (Windows / MinGW-w64).
setlocal
cd /d "%~dp0"
if not exist build mkdir build

set FLAGS=-O2 -pipe -std=c++17 -DNDEBUG -static -static-libgcc -static-libstdc++

echo Building solve.exe (this takes a few seconds)...
g++ %FLAGS% solve.cpp -o build\solve.exe || goto :err

echo Done.
dir /b build
goto :eof

:err
echo BUILD FAILED
exit /b 1
endlocal
