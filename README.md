# PAYROLL MANAGEMENT SYSTEM

A comprehensive payroll application with a pure C++ HTTP backend and a modern web frontend.

## Features

- **Employee Management**: Add, remove, and view employees
- **Salary Calculations**: Automatically calculate gross salary, deductions, and net salary
- **Payroll Slips**: Generate detailed payroll slips for individual employees
- **Monthly Reports**: Generate comprehensive monthly payroll reports
- **Statistics**: Display payroll statistics including totals and averages
- **File Operations**: Save and load employee data from JSON files
- **Tax & Deductions**: Automatic tax calculations, provident fund, and health insurance deductions
- **Web API**: C++ server exposes JSON routes for the browser UI
- **Static Frontend Hosting**: C++ server serves the `public` folder directly
- **Lock Screen Login**: Register an app account, then unlock the payroll dashboard with username and password

## Salary Calculations

### Gross Salary
```
Gross Salary = Basic Salary + Allowance + Bonus
```

### Deductions
- **Tax**: 12% of Gross Salary
- **Provident Fund**: 8% of Basic Salary
- **Health Insurance**: Fixed $100

### Net Salary
```
Net Salary = Gross Salary - Total Deductions
```

## File Structure

**C++ Backend:**
- ✅ ApiServer.cpp / .h
- ✅ Employee.cpp / .h
- ✅ Payroll.cpp / .h
- ✅ web_server.cpp
- ✅ Build files (Makefile, build.bat)
- `public/` - Browser UI files (HTML, CSS, JS)

## Compilation

### C++ Web Backend
```bash
build.bat
```

or:

```bash
make
```

This creates `payroll_api_server.exe` on Windows.

### Manual Compilation
```bash
g++ -std=c++11 -Wall -Wextra -c Employee.cpp -o Employee.o
g++ -std=c++11 -Wall -Wextra -c Payroll.cpp -o Payroll.o
g++ -std=c++11 -Wall -Wextra -c ApiServer.cpp -o ApiServer.o
g++ -std=c++11 -Wall -Wextra -c web_server.cpp -o web_server.o
g++ Employee.o Payroll.o ApiServer.o web_server.o -o payroll_api_server.exe -lws2_32
```

## Running the Application

### Web UI with C++ Backend
```bash
build.bat run
```

Then open:

```text
http://localhost:3000
```

The C++ backend serves the frontend and these API routes. Payroll data routes require a login bearer token from `/api/auth/login`.

```text
GET    /api/health
POST   /api/auth/register
POST   /api/auth/login
GET    /api/auth/me
POST   /api/auth/logout
GET    /api/employees
POST   /api/employees
GET    /api/employees/:id
PUT    /api/employees/:id
DELETE /api/employees/:id
GET    /api/payroll
GET    /api/payroll/:id
GET    /api/report/monthly
GET    /api/statistics
POST   /api/export/file
POST   /api/import/file
```

## Sample Data

The system comes with 4 pre-loaded employees:
- John Smith (Engineering)
- Sarah Johnson (Marketing)
- Mike Williams (HR)
- Emily Brown (Finance)

## Requirements

- C++11 or higher
- GCC or Clang compiler
- Standard C++ libraries (iostream, string, vector, fstream, algorithm, iomanip)

## Future Enhancements

- Database integration (MySQL/PostgreSQL)
- Email notification for payslips
- Advanced filtering and search
- Leave management system
- Overtime calculation
- Department-wise reports
- Role-based access control
