# QUICK START

## Option 1: ONLINE MODE

Run from the project root:

```batch
START_SERVER.bat
```

Then open:

```text
http://localhost:3000
```

Portals:

- Admin: `/admin/`
- Employee: `/user/`

Sample logins:

- Admin: `rajesh` / `rajesh123456`
- Employee 101: `rajeshk` / `rajeshk123456`
- Employee 102: `rajeshka` / `rajeshka123456`

## Option 2: OFFLINE / DEMO MODE

No backend required:

1. Open `frontend\index.html`
2. Open the admin or employee portal
3. Login uses browser demo storage
4. Changes stay local to the browser

## Useful Commands

```batch
build.bat web
build.bat run
build.bat clean
START_SERVER.bat
```

## Notes

- Frontend files now live in `frontend/`
- Backend files, JSON data, and the EXE now live in `backend/`
- If the EXE is blocked by Device Guard, online mode will not start until Windows policy allows `backend\payroll_api_server.exe`
