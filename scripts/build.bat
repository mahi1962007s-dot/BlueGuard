@echo off
REM ============================================================
REM BLUEGUARD - Build script (Windows)
REM Compiles the C DSA core (c\*.c) into c\beachdsa.exe
REM ============================================================
echo Building Beach Health Monitoring System - C DSA core...
echo.
cd /d "%~dp0..\c"

where gcc >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Using gcc ...
    gcc -std=c99 -Wall -Wextra -O2 -o beachdsa.exe main.c zone.c health_score.c array_ops.c search.c sorting.c linked_list.c stack.c queue.c ny_dataset.c advanced.c
    if %ERRORLEVEL% NEQ 0 ( echo Build FAILED. See errors above. & exit /b 1 )
    echo Build succeeded: c\beachdsa.exe
    exit /b 0
)

where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Using MSVC cl ...
    cl /nologo /O2 /Fe:beachdsa.exe main.c zone.c health_score.c array_ops.c search.c sorting.c linked_list.c stack.c queue.c ny_dataset.c advanced.c
    exit /b %ERRORLEVEL%
)

echo No C compiler found. Install MinGW-w64 (gcc) or use "Developer Command Prompt for VS".
exit /b 1
