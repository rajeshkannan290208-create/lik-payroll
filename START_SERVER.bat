@echo off
REM Start the Payroll API Server
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
pause
