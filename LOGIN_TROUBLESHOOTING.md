# LOGIN TROUBLESHOOTING

## 1. Start The Backend

From the project root run:

```batch
START_SERVER.bat
```

Keep that window open while using the app.

## 2. Clear Browser Storage

1. Open the app in the browser
2. Press `F12`
3. Go to Application > Storage > Local Storage
4. Clear the stored payroll data
5. Refresh and log in again

## 3. Check The Correct Portal

Admin portal:

- `http://localhost:3000/admin/`

Employee portal:

- `http://localhost:3000/user/`

## 4. Common Problems

### Failed to connect to server

- Make sure `START_SERVER.bat` is running
- Check port `3000` is free

### Invalid username or password

- Use the correct portal
- Check the username/password exactly
- User records are stored in `backend\users.json`

### Session expired immediately

- Clear browser local storage
- Log in again
- Make sure the backend is still running

### Device Guard policy block

If you see:

```text
was blocked by your organization's Device Guard policy
```

then Windows is blocking `backend\payroll_api_server.exe`. The project structure is already fixed, but online mode cannot run until that executable is allowed by local admin/support.

## 5. Rebuild The Backend

From the project root:

```batch
build.bat clean
build.bat web
build.bat run
```

## 6. Demo Mode

If the backend cannot run, you can still open:

```text
frontend\index.html
```

and use the frontend in offline/demo mode.
