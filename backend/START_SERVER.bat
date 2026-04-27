@echo off
REM Start the Payroll API Server
cd /d "%~dp0"
echo ========================================
echo Starting Payroll API Server...
echo ========================================
echo.
echo The server will run on http://localhost:3000
echo Server must be running to use online features!
echo.

if not exist payroll_api_server.exe (
    echo Building server first...
    call build.bat web
    if errorlevel 1 (
        echo Build failed!
        pause
        exit /b 1
    )
)

echo Starting server...
payroll_api_server.exe
if errorlevel 1 (
    echo.
    echo Server could not start.
    echo If Windows showed a Device Guard or organization policy message, this machine is blocking unsigned EXE files.
    echo The project paths are fixed, but the EXE must be allowed by local policy before the backend can run.
)
pause
