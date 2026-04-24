# Login Troubleshooting Guide

## Quick Fix - Session Expired Issue

If you're seeing "Session expired. Please login again." even after successfully logging in, follow these steps:

### Step 1: Make Sure the API Server is Running
The C++ backend server MUST be running on port 3000 for the system to work properly.

**To start the server:**
1. Open Command Prompt/PowerShell in the project folder
2. Run: `build.bat run`
3. Wait for the output: `Server running on http://localhost:3000`
4. Keep this window open while using the app

### Step 2: Clear Browser Cache & LocalStorage
1. Open the Payroll app in your browser
2. Press `F12` to open Developer Tools
3. Go to **Application** tab > **Storage** > **Local Storage**
4. Find and click on `http://localhost:5500` (or your current domain)
5. Clear all data
6. Refresh the page (F5)

### Step 3: Test Login
Try logging in with these credentials (portal-specific):

**Admin Portal (`/admin/`)**

| Username | Password |
|----------|----------|
| rajesh | rajesh123456 |

**Employee Portal (`/user/`)**

| Username | Password |
|----------|----------|
| rajeshk | rajeshk123456 |
| rajeshka | rajeshka123456 |

### Step 4: Check Browser Console for Errors
1. Open Developer Tools (F12)
2. Go to **Console** tab
3. Look for any error messages
4. Check if you see "PayrollAPI loaded successfully"
5. Look for any API errors when you try to login

### Common Issues & Solutions

**Issue: "Failed to connect to server" or API not responding**
- Solution: Make sure `build.bat run` is executed and the server is running

**Issue: "Invalid username or password"**  
- Solution: Check the username and password are correct (case-sensitive)
- Make sure you're logging into the correct portal (Admin vs Employee)

**Issue: Login works but then shows "Session expired" immediately**
- Solution:
  1. Make sure the C++ API server is running (not just in demo/offline mode)
  2. Clear browser localStorage (see Step 2)
  3. Try logging in again

**Issue: "Cannot connect to port 3000"**
- Solution:
  1. Make sure no other application is using port 3000
  2. Try running: `netstat -an | find ":3000"` to check if port is in use
  3. Rebuild the server: `build.bat clean` then `build.bat run`

### How to Add New Users

If you want to add new login credentials, edit `users.json`:

```json
[
  {"id":1,"u":"username","p":"<password_digest>","role":"admin","employeeId":-1},
  {"id":2,"u":"employeeuser","p":"<password_digest>","role":"user","employeeId":101}
]
```

The password_digest is calculated as: **username + password**

Example: For username="test" and password="pass123", the digest is "testpass123"

### Manual Server Build (if needed)

```batch
build.bat clean       # Clean old build files
build.bat web         # Build the server
build.bat run         # Build and run the server
```

### Debug Information
Check the server console output for:
- Connection attempts from the browser
- Login attempts (successful or failed)
- Session creation/validation messages

If you see errors in the server output, the issue is likely there.

---

**Need more help?** Check the README.md for architecture details.
