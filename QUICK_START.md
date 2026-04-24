# QUICK START - Payroll Management System

## Option 1: ONLINE MODE (Recommended)
Best for full features and data persistence

```batch
build.bat run
```
Then open: `http://localhost:3000`

Choose a portal:
- **Admin:** `/admin/`
- **Employee:** `/user/`

**Admin Login:**
- Username: **rajesh** | Password: **rajesh123456**

**Employee Login (Sample):**
- Username: **rajeshk** | Password: **rajeshk123456** (Employee 101)
- Username: **rajeshka** | Password: **rajeshka123456** (Employee 102)

## Option 2: OFFLINE/DEMO MODE  
No server needed, works even if C++ server is down

1. Open `public/index.html` in browser (or use a local server)
2. Open **Admin** or **Employee** portal
3. Login uses browser storage (demo users)
4. Changes saved locally only (don't persist if browser cleared)

---

## Fix for "Session Expired" Error

### ✅ What We Fixed:
- Session now stored in browser automatically
- No more server verification needed on page load
- Server restart doesn't break your session
- Hybrid online/offline support

### ✅ To Use:
1. **First time?** Login normally (either online or offline)
2. **Refreshed page?** You stay logged in! ✓
3. **Server crashed?** You stay logged in in offline mode! ✓

### ❌ When You Lose Session:
- Clear browser cache/cookies manually
- 1 hour of inactivity (token expires)
- Switch between different browsers

---

## Still Getting Errors?

### Check if server is running:
```
netstat -an | find ":3000"
```
If no output → Server not running → Run `build.bat run`

### Clear all browser data:
1. F12 → Application → Storage → Local Storage
2. Delete everything
3. Refresh (F5) and login again

### Check browser console (F12 → Console):
- ✓ Should show "✓ Session restored, user: rajesh"
- ❌ Shows "OFFLINE mode" → Server not running

---

## Architecture Overview

```
Browser (Client)
├── Stores: token, expiresAt, user info in localStorage
├── Offline Mode: Full app works with demo data
└── Online Mode: Syncs with C++ backend

C++ Server (port 3000)
├── Validates tokens
├── Manages employees
├── Calculates payroll
└── Sessions in-memory only (doesn't persist across restarts)
```

---

**Still having issues?** Check `LOGIN_TROUBLESHOOTING.md` for advanced debugging.
