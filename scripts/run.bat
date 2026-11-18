@echo off
REM BLUEGUARD - Run script (Windows): builds the C engine if needed, installs deps, starts the server.
cd /d "%~dp0.."
call scripts\build.bat
if not exist "node_modules" (
    echo Installing Node.js dependencies...
    call npm install
)
echo Starting BLUEGUARD on http://localhost:3000 ...
call npm start
