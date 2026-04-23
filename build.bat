@echo off
REM Payroll Management System - C++ Backend Build Script

echo ========================================
echo Payroll Management System - C++ Backend
echo ========================================
echo.

set CXXFLAGS=-std=c++11 -Wall -Wextra -D_WIN32_WINNT=0x0601
set CFLAGS=-D_WIN32_WINNT=0x0601
if "%1"=="" goto build_web
if "%1"=="web" goto build_web
if "%1"=="run" goto run_web
if "%1"=="console" goto build_console
if "%1"=="run-console" goto run_console
if "%1"=="clean" goto clean
if "%1"=="help" goto help

echo Unknown command: %1
goto help

:build_web
echo Compiling C++ API server...
g++ %CXXFLAGS% -c Employee.cpp -o Employee.o
if errorlevel 1 goto error

g++ %CXXFLAGS% -c Payroll.cpp -o Payroll.o
if errorlevel 1 goto error

g++ %CXXFLAGS% -c ApiServer.cpp -o ApiServer.o
if errorlevel 1 goto error

g++ %CXXFLAGS% -c web_server.cpp -o web_server.o
if errorlevel 1 goto error

echo Linking C++ API server...
g++ Employee.o Payroll.o ApiServer.o web_server.o -o payroll_api_server.exe -lws2_32
if errorlevel 1 goto error

echo Build successful!
echo Run with: payroll_api_server.exe
goto end

:run_web
if not exist payroll_api_server.exe (
    call build.bat web
    if errorlevel 1 goto error
)
echo Running C++ API server...
payroll_api_server.exe
goto end

:build_console
echo Compiling original console app...
g++ -std=c++11 -Wall -Wextra -c Employee.cpp -o Employee.o
if errorlevel 1 goto error

g++ -std=c++11 -Wall -Wextra -c Payroll.cpp -o Payroll.o
if errorlevel 1 goto error

g++ -std=c++11 -Wall -Wextra -c PayrollSystem.cpp -o PayrollSystem.o
if errorlevel 1 goto error

g++ -std=c++11 -Wall -Wextra -c main.cpp -o main.o
if errorlevel 1 goto error

echo Linking console app...
g++ Employee.o Payroll.o PayrollSystem.o main.o -o payroll_system.exe
if errorlevel 1 goto error

echo Console build successful!
echo Run with: payroll_system.exe
goto end

:run_console
if not exist payroll_system.exe (
    call build.bat console
    if errorlevel 1 goto error
)
echo Running console app...
payroll_system.exe
goto end

:clean
echo Cleaning up...
del /q *.o 2>nul
del /q payroll_api_server.exe 2>nul
del /q payroll_system.exe 2>nul
echo Cleanup complete!
goto end

:help
echo Usage:
echo   build.bat             - Build the C++ API web server
echo   build.bat web         - Build the C++ API web server
echo   build.bat run         - Build and run the C++ API web server
echo   build.bat console     - Build the original console app
echo   build.bat run-console - Build and run the original console app
echo   build.bat clean       - Remove build files
echo   build.bat help        - Show this help
goto end

:error
echo Build failed!
exit /b 1

:end
