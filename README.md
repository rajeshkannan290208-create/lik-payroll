<<<<<<< HEAD
# PAYROLL MANAGEMENT SYSTEM

A payroll application with a pure C++ HTTP backend and a browser frontend.

## Project Structure

- `frontend/` - HTML, CSS, JS, assets, admin portal, employee portal
- `backend/` - C++ source files, JSON data, build scripts, logs, executable
- `build.bat` - root wrapper for `backend\build.bat`
- `START_SERVER.bat` - root wrapper for `backend\START_SERVER.bat`

## Features

- Employee management
- Payroll calculations
- Monthly reports
- Admin and employee login portals
- Employee-only payslip access
- Attendance tracking
- Static frontend hosting from the C++ server

## Run The App

### Online Mode

From the project root:

```batch
START_SERVER.bat
```

Then open:

```text
http://localhost:3000
```

### Offline / Demo Mode

Open:

```text
frontend\index.html
```

This uses browser storage and does not require the backend to be running.

## Build Commands

From the project root:

```batch
build.bat web
build.bat run
build.bat clean
```

The compiled executable is created in:

```text
backend\payroll_api_server.exe
```

## Backend API

The backend serves the frontend and these API routes:

```text
GET    /api/health
POST   /api/auth/register
POST   /api/auth/admin/login
POST   /api/auth/user/login
POST   /api/auth/login
GET    /api/auth/me
POST   /api/auth/logout
GET    /api/employees            (admin)
POST   /api/employees            (admin)
GET    /api/employees/:id        (admin)
PUT    /api/employees/:id        (admin)
DELETE /api/employees/:id        (admin)
GET    /api/employees/me         (employee)
GET    /api/admin/users          (admin)
POST   /api/admin/users          (admin)
POST   /api/attendance/mark      (employee)
GET    /api/attendance/me        (employee)
GET    /api/attendance           (admin)
GET    /api/payroll              (admin)
GET    /api/payroll/:id          (admin)
GET    /api/payroll/me           (employee)
GET    /api/report/monthly
GET    /api/statistics
```

## Important Note About Device Guard

If Windows shows a message like:

```text
was blocked by your organization's Device Guard policy
```

the folder structure is not the problem. That means Windows policy is blocking unsigned executables on this machine. The frontend can still run in demo mode, but the backend EXE must be allowed by local admin/support before online features will work.
=======
# lik-payroll
>>>>>>> ee0a33304e68598d155a6111931239f7a305d0bc
