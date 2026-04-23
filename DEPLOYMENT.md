# Complete Setup & Deployment Guide

## Architecture Overview

The Payroll Management System has **two separate components** that work together:

### 1. **C++ Backend** (Console/Standalone Application)
- Employee management
- Payroll calculations
- File I/O operations
- Can run independently as a console app

### 2. **Node.js/Express API Server + Web Frontend**
- REST API that mirrors C++ backend logic
- Web-based user interface
- Real-time dashboard and reports
- RESTful endpoints for all operations

## Current Setup Status

✅ **Completed:**
- C++ Backend: Full implementation (main.cpp, Employee.cpp, Payroll.cpp, PayrollSystem.cpp)
- Node.js API Server: Complete REST API (server.js)
- Web Frontend: Interactive UI with HTML, CSS, and JavaScript

❌ **Currently Separate:**
- C++ backend and Node.js API have separate data stores
- C++ uses console/file I/O
- Node.js uses in-memory storage

---

## How to Run the System

### **Option 1: Run Web Frontend Only** (Recommended for Web Usage)

#### Prerequisites:
- Node.js v14+ installed
- npm package manager

#### Steps:

1. **Navigate to project directory:**
```bash
cd "c:\game\New folder (2)"
```

2. **Install dependencies:**
```bash
npm install
```

3. **Start the server:**
```bash
npm start
```

4. **Open in browser:**
```
http://localhost:3000
```

**Output:**
```
========================================
Payroll Management System API Server
========================================
Server running on http://localhost:3000
Frontend: http://localhost:3000
API Base: http://localhost:3000/api
========================================
```

### **Option 2: Run C++ Backend Only** (Standalone Console App)

#### Prerequisites:
- GCC or MinGW compiler
- Windows (for .bat) or Linux/Mac (for Makefile)

#### On Windows:

1. **Navigate to project directory:**
```cmd
cd "c:\game\New folder (2)"
```

2. **Build using batch script:**
```bash
build.bat
```

3. **Run the application:**
```bash
payroll_system.exe
```

#### On Linux/Mac:

```bash
cd "path/to/project"
make
./payroll_system
```

---

## Running Both Together (Advanced Setup)

To connect C++ backend with Node.js frontend, you have two options:

### **Option A: Use Node.js API as Bridge** (Currently Implemented)

```
Web Frontend (HTML/CSS/JS)
        ↓ (HTTP)
  Node.js API Server (server.js)
        ↓ (In-Memory Storage)
    Data Storage
```

**Advantage:** Works out of the box, no additional configuration
**Run:**
```bash
npm install
npm start
# Then open http://localhost:3000
```

### **Option B: Call C++ from Node.js** (Advanced)

To make Node.js call the compiled C++ executable:

1. Compile C++ to executable
2. Modify server.js to call C++ exe via child_process
3. C++ would need to be refactored to accept JSON input and output JSON

**This requires:**
- Modifying main.cpp to read/write JSON
- Creating IPC (Inter-Process Communication) between Node.js and C++
- Testing data serialization

---

## Project Structure

```
c:\game\New folder (2)\
│
├── Backend (C++)
│   ├── main.cpp                 # Main console application
│   ├── Employee.h/cpp           # Employee class
│   ├── Payroll.h/cpp            # Payroll calculations
│   ├── PayrollSystem.h/cpp       # System management
│   ├── Config.h                 # Configuration constants
│   ├── Makefile                 # Linux/Mac build
│   └── build.bat                # Windows build
│
├── Frontend (Web)
│   ├── server.js                # Node.js Express API server
│   ├── package.json             # Dependencies
│   │
│   └── public/
│       ├── index.html           # Web interface
│       ├── styles.css           # Styling
│       ├── api.js               # API client
│       └── script.js            # Frontend logic
│
└── Documentation
    ├── README.md                # Main documentation
    ├── DEPLOYMENT.md            # This file
    └── API_DOCUMENTATION.md     # API endpoints (below)
```

---

## API Endpoints Reference

All endpoints return JSON responses with `{ success: boolean, data: object, message: string }`

### **Employee Management**

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/employees` | Get all employees |
| GET | `/api/employees/:id` | Get single employee |
| POST | `/api/employees` | Create new employee |
| PUT | `/api/employees/:id` | Update employee |
| DELETE | `/api/employees/:id` | Delete employee |

### **Payroll**

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/payroll/:id` | Get payroll for employee |
| GET | `/api/payroll` | Get all payroll |
| GET | `/api/report/monthly` | Generate monthly report |
| GET | `/api/statistics` | Get statistics |

### **File Operations**

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/export/file` | Export employees to JSON |
| POST | `/api/import/file` | Import employees from JSON |
| GET | `/api/health` | Health check |

### **Example API Call**

```javascript
// Add employee
fetch('http://localhost:3000/api/employees', {
    method: 'POST',
    headers: {
        'Content-Type': 'application/json'
    },
    body: JSON.stringify({
        name: 'John Doe',
        department: 'Engineering',
        basicSalary: 5000,
        allowance: 500,
        bonusPercentage: 10
    })
})
.then(res => res.json())
.then(data => console.log(data));
```

---

## Development

### **Run in Development Mode with Auto-Reload**

```bash
npm run dev
```

(Requires nodemon: `npm install -g nodemon`)

### **Modifying the C++ Backend**

If you want to make changes to C++ code:

1. **Edit the .cpp/.h files**
2. **Recompile:**
   - Windows: `build.bat`
   - Linux/Mac: `make rebuild`
3. **Run:** `payroll_system.exe` or `./payroll_system`

### **Modifying the Web Frontend**

1. **Edit files in `public/` directory**
2. **Changes auto-reflect when you refresh browser** (no rebuild needed)
3. **Edit `server.js` if changing API logic**
4. **Restart Node.js server if you modify server.js**

---

## Data Storage

### **C++ Backend**
- **Persistent:** Saves to `.txt` files
- **Format:** Pipe-separated values (`id|name|dept|salary|...`)
- **Example:** `employees.txt`

### **Node.js API**
- **Temporary:** In-memory array
- **Format:** JSON objects
- **Export:** Can save to JSON files

### **Converting Between Formats**

C++ format to JSON:
```javascript
// employees.txt content
101|John Smith|Engineering|5000|500|10

// Converts to JSON
{
    "id": 101,
    "name": "John Smith",
    "department": "Engineering",
    "basicSalary": 5000,
    "allowance": 500,
    "bonusPercentage": 10
}
```

---

## Troubleshooting

### **Issue: Port 3000 already in use**
```bash
# Kill process using port 3000
# Windows:
netstat -ano | findstr :3000
taskkill /PID <PID> /F

# Linux/Mac:
lsof -i :3000
kill -9 <PID>
```

### **Issue: "Cannot find module" error**
```bash
# Reinstall dependencies
rm -r node_modules
npm install
```

### **Issue: C++ compilation errors**
```bash
# Ensure GCC/MinGW is installed
g++ --version

# On Windows, download MinGW from:
# https://www.mingw-w64.org/
```

### **Issue: API connection fails**
- Ensure server is running: `npm start`
- Check if port 3000 is accessible
- Verify firewall settings

---

## Performance Considerations

- **C++ Backend:** Optimized for calculations, suitable for large datasets
- **Node.js API:** Good for web requests, handles ~100-1000 employees efficiently
- **Database:** For production, replace in-memory storage with:
  - SQLite (file-based)
  - MySQL/PostgreSQL (server-based)
  - MongoDB (NoSQL)

---

## Production Deployment

### **Deploy Node.js Server**

1. **Use PM2 process manager:**
```bash
npm install -g pm2
pm2 start server.js --name payroll-system
pm2 save
```

2. **Setup reverse proxy (Nginx):**
```nginx
server {
    listen 80;
    server_name example.com;
    
    location / {
        proxy_pass http://localhost:3000;
    }
}
```

3. **Enable HTTPS:**
```bash
# Use Let's Encrypt for SSL certificates
```

### **Deploy C++ Backend**

1. **Cross-compile for target OS**
2. **Use Docker container:**
```dockerfile
FROM gcc:latest
COPY . /app
WORKDIR /app
RUN make
CMD ["./payroll_system"]
```

---

## Summary

| Component | Status | Run Command | Port |
|-----------|--------|-------------|------|
| Web Frontend | ✅ Ready | `npm start` | 3000 |
| API Server | ✅ Ready | `npm start` | 3000 |
| C++ Console App | ✅ Ready | `build.bat` then `payroll_system.exe` | N/A |
| Database | ⚠️ Optional | Configure in server.js | - |

**To start using the system:**
```bash
cd "c:\game\New folder (2)"
npm install
npm start
# Then open http://localhost:3000 in your browser
```

---

## Next Steps

1. ✅ Run the system with `npm start`
2. ✅ Test the web interface
3. ✅ Add/manage employees
4. ✅ View payroll slips and reports
5. ⭐ Optional: Integrate C++ backend using child_process if needed
6. 🔄 Optional: Setup database for persistence

For detailed API documentation, see `API_DOCUMENTATION.md`
