# Architecture & Connection Map

## 🗺️ System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        FRONTEND LAYER                                   │
│                     (User Interface)                                    │
│                                                                         │
│  ┌────────────────────────────────────────────────────────────────┐   │
│  │  Web Browser (HTML + CSS + JavaScript)                        │   │
│  │                                                                │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │   │
│  │  │  Dashboard   │  │  Employees   │  │   Payroll    │         │   │
│  │  │   Page       │  │    Page      │  │    Page      │         │   │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘         │   │
│  │         │                  │                  │                 │   │
│  │         └──────────────────┼──────────────────┘                 │   │
│  │                            │                                    │   │
│  │                  script.js (Business Logic)                     │   │
│  │                  api.js (API Client)                           │   │
│  │                            │                                    │   │
│  └────────────────────────────┼────────────────────────────────────┘   │
│                               │                                         │
└───────────────────────────────┼─────────────────────────────────────────┘
                                │ HTTP/REST API Calls
                                │ JSON Data Exchange
                                ↓
┌─────────────────────────────────────────────────────────────────────────┐
│                      API LAYER (Node.js/Express)                        │
│                        (server.js)                                      │
│                                                                         │
│  Port: 3000                                                             │
│                                                                         │
│  ┌────────────────────────────────────────────────────────────────┐   │
│  │  REST API Endpoints                                           │   │
│  │                                                                │   │
│  │  POST   /api/employees          (Add Employee)               │   │
│  │  GET    /api/employees          (List All)                   │   │
│  │  GET    /api/employees/:id      (Get Single)                 │   │
│  │  PUT    /api/employees/:id      (Update)                     │   │
│  │  DELETE /api/employees/:id      (Delete)                     │   │
│  │                                                                │   │
│  │  GET    /api/payroll            (All Payroll)                │   │
│  │  GET    /api/payroll/:id        (Single Payroll)             │   │
│  │  GET    /api/report/monthly     (Monthly Report)             │   │
│  │  GET    /api/statistics         (Stats)                      │   │
│  │                                                                │   │
│  │  POST   /api/export/file        (Export to JSON)             │   │
│  │  POST   /api/import/file        (Import from JSON)           │   │
│  │  GET    /api/health             (Health Check)               │   │
│  └────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  ┌────────────────────────────────────────────────────────────────┐   │
│  │  Business Logic Layer (JavaScript)                            │   │
│  │                                                                │   │
│  │  ┌──────────────────────────────────────────────────────┐    │   │
│  │  │  calculateGrossSalary()                              │    │   │
│  │  │  calculateTax()                                      │    │   │
│  │  │  calculateDeductions()                               │    │   │
│  │  │  calculateNetSalary()                                │    │   │
│  │  │  getPayrollDetails()                                 │    │   │
│  │  └──────────────────────────────────────────────────────┘    │   │
│  └────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  ┌────────────────────────────────────────────────────────────────┐   │
│  │  Data Storage Layer                                           │   │
│  │                                                                │   │
│  │  ┌─────────────────────────────────────────────────────┐     │   │
│  │  │  In-Memory JSON Data                                │     │   │
│  │  │                                                     │     │   │
│  │  │  [{                                                 │     │   │
│  │  │    id: 101,                                         │     │   │
│  │  │    name: "John Smith",                              │     │   │
│  │  │    department: "Engineering",                       │     │   │
│  │  │    basicSalary: 5000,                               │     │   │
│  │  │    allowance: 500,                                  │     │   │
│  │  │    bonusPercentage: 10                              │     │   │
│  │  │  }, ...]                                            │     │   │
│  │  └─────────────────────────────────────────────────────┘     │   │
│  │                                                                │   │
│  │  Optional: JSON File Export/Import                            │   │
│  │  - employees.json                                             │   │
│  │  - payroll_export.json                                        │   │
│  └────────────────────────────────────────────────────────────────┘   │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│              OPTIONAL: C++ Backend (Standalone)                         │
│                                                                         │
│  Status: ⚠️ NOT CURRENTLY CONNECTED                                     │
│                                                                         │
│  ┌────────────────────────────────────────────────────────────────┐   │
│  │  Console Application                                          │   │
│  │  (main.cpp + Employee.cpp + Payroll.cpp + PayrollSystem.cpp)  │   │
│  │                                                                │   │
│  │  ┌──────────────────────────────────────────────────────┐    │   │
│  │  │  Payroll Calculation Engine (C++)                    │    │   │
│  │  │                                                      │    │   │
│  │  │  calculateGrossSalary()                              │    │   │
│  │  │  calculateTax()                                      │    │   │
│  │  │  calculateDeductions()                               │    │   │
│  │  │  calculateNetSalary()                                │    │   │
│  │  │  generatePayrollSlip()                               │    │   │
│  │  └──────────────────────────────────────────────────────┘    │   │
│  │                                                                │   │
│  │  ┌──────────────────────────────────────────────────────┐    │   │
│  │  │  Employee Management (C++)                          │    │   │
│  │  │                                                      │    │   │
│  │  │  addEmployee()                                       │    │   │
│  │  │  removeEmployee()                                    │    │   │
│  │  │  updateEmployee()                                    │    │   │
│  │  │  findEmployee()                                      │    │   │
│  │  └──────────────────────────────────────────────────────┘    │   │
│  │                                                                │   │
│  │  ┌──────────────────────────────────────────────────────┐    │   │
│  │  │  File I/O (Pipe-Delimited Format)                   │    │   │
│  │  │                                                      │    │   │
│  │  │  saveEmployees()                                     │    │   │
│  │  │  loadEmployees()                                     │    │   │
│  │  │                                                      │    │   │
│  │  │  Format: id|name|dept|salary|allowance|bonus        │    │   │
│  │  │  Example: 101|John|Engineering|5000|500|10          │    │   │
│  │  └──────────────────────────────────────────────────────┘    │   │
│  └────────────────────────────────────────────────────────────────┘   │
│                                                                         │
│  How to Run:                                                            │
│  Windows:  build.bat  →  payroll_system.exe                            │
│  Linux:    make       →  ./payroll_system                              │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 🔄 Data Flow Examples

### Example 1: User Adds Employee

```
┌─────────────────────────────────────────────────────────────────────┐
│ STEP 1: User Interaction (Browser)                                 │
│                                                                     │
│ User fills form:                                                    │
│ - Name: "John Smith"                                                │
│ - Department: "Engineering"                                         │
│ - Basic Salary: $5000                                               │
│ - Allowance: $500                                                   │
│ - Bonus: 10%                                                        │
│                                                                     │
│ Clicks "Save" button                                                │
└────────────────┬──────────────────────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────────────────────────────┐
│ STEP 2: Frontend Processing (script.js)                             │
│                                                                     │
│ saveEmployee() function:                                            │
│ - Validate input                                                    │
│ - Create data object                                                │
│ - Call PayrollAPI.createEmployee(data)                              │
└────────────────┬──────────────────────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────────────────────────────┐
│ STEP 3: API Request (api.js)                                        │
│                                                                     │
│ POST http://localhost:3000/api/employees                            │
│ Content-Type: application/json                                      │
│                                                                     │
│ {                                                                   │
│   "name": "John Smith",                                             │
│   "department": "Engineering",                                      │
│   "basicSalary": 5000,                                              │
│   "allowance": 500,                                                 │
│   "bonusPercentage": 10                                             │
│ }                                                                   │
└────────────────┬──────────────────────────────────────────────────┘
                 │ HTTP
                 ↓
┌─────────────────────────────────────────────────────────────────────┐
│ STEP 4: Backend Processing (server.js)                              │
│                                                                     │
│ Route: POST /api/employees                                          │
│ Handler receives request                                            │
│ - Validate data                                                     │
│ - Generate employee ID                                              │
│ - Add to employees array                                            │
│ - Return success response                                           │
└────────────────┬──────────────────────────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────────────────────────────┐
│ STEP 5: API Response                                                │
│                                                                     │
│ HTTP 201 Created                                                    │
│ {                                                                   │
│   "success": true,                                                  │
│   "data": {                                                         │
│     "id": 105,                                                      │
│     "name": "John Smith",                                           │
│     "department": "Engineering",                                    │
│     "basicSalary": 5000,                                            │
│     "allowance": 500,                                               │
│     "bonusPercentage": 10                                           │
│   },                                                                │
│   "message": "Employee added successfully"                          │
│ }                                                                   │
└────────────────┬──────────────────────────────────────────────────┘
                 │ JSON
                 ↓
┌─────────────────────────────────────────────────────────────────────┐
│ STEP 6: Frontend Update                                             │
│                                                                     │
│ - Receive response in script.js                                     │
│ - Show success notification                                         │
│ - Reload employees list                                             │
│ - Clear form                                                        │
│ - Update dashboard stats                                            │
│ - Employee visible in table                                         │
└─────────────────────────────────────────────────────────────────────┘
```

### Example 2: User Views Payroll

```
Click "View" on Employee
  ↓
GET /api/payroll/101
  ↓
Backend calculates:
  - Gross = 5000 + 500 + 500 = 6000
  - Tax = 6000 × 0.12 = 720
  - PF = 5000 × 0.08 = 400
  - Insurance = 100
  - Total Deductions = 1220
  - Net = 6000 - 1220 = 4780
  ↓
Return JSON response with all details
  ↓
Frontend displays payroll slip modal
```

---

## 🔗 Connection Status

| Component | Status | Connected To | Communication |
|-----------|--------|--------------|---|
| **Web UI** | ✅ Running | Node.js API | HTTP/REST |
| **Node.js API** | ✅ Running | Web UI | JSON |
| **Payroll Logic (JS)** | ✅ Running | Node.js API | Direct call |
| **Data Storage (Memory)** | ✅ Running | Node.js API | Direct access |
| **C++ Backend** | ⚠️ Standalone | None | Not connected |

---

## 📊 Comparison: Current vs Integrated

### Current Architecture (Active Now ✅)

```
Browser → Node.js API → JS Payroll Logic → Memory Storage
```

**Pros:**
- ✅ Works immediately
- ✅ No additional setup needed
- ✅ Full functionality available
- ✅ Easy to deploy

**Cons:**
- ❌ Duplicate payroll logic (JS vs C++)
- ❌ In-memory storage (lost on restart)
- ❌ C++ backend unused

### Option 1: Child Process Bridge (Medium Integration)

```
Browser → Node.js API → Call C++ Exe → IPC → Return JSON
```

**Pros:**
- ✅ Reuses C++ code
- ✅ Leverages C++ performance
- ✅ Single logic implementation

**Cons:**
- ❌ More complex setup
- ❌ IPC overhead
- ❌ Requires compiled C++ binary

### Option 2: HTTP Server Bridge (Full Integration)

```
Browser → Node.js API → HTTP → C++ HTTP Server → Calculations
```

**Pros:**
- ✅ Best architecture
- ✅ Microservices pattern
- ✅ Scalable
- ✅ Independent services

**Cons:**
- ❌ C++ refactoring needed
- ❌ Complex setup
- ❌ Two servers to manage

### Option 3: Docker Compose (Production)

```
Docker Network:
├── C++ Backend Service (Port 3001)
├── Node.js API Service (Port 3000)
└── Web Frontend (Port 80)
```

**Pros:**
- ✅ Production-ready
- ✅ Easy deployment
- ✅ Reproducible environment

**Cons:**
- ❌ Docker learning curve
- ❌ Performance overhead

---

## 🎯 Recommendation Summary

### For Now (Best Choice)
**Use Current Setup**: Node.js API with JavaScript payroll logic
- Run: `npm install && npm start`
- Open: `http://localhost:3000`
- Status: ✅ Ready to use

### For Later
**If you need C++ integration**, see `CPP_NODEJS_INTEGRATION.md` for:
1. Child Process Bridge (Medium complexity)
2. HTTP Server Bridge (High quality)
3. Docker Microservices (Production)

### Current System Status

```
┌──────────────────────────────────────┐
│  READY FOR IMMEDIATE USE             │
│                                      │
│  Frontend:   ✅ 100% Functional     │
│  API Server: ✅ 100% Functional     │
│  Database:   ⚠️ In-Memory (OK)      │
│  C++ Backend: 🔵 Standalone (OK)    │
│                                      │
│  System Status: 🟢 OPERATIONAL      │
└──────────────────────────────────────┘
```

---

## 📝 Quick Reference

### Start the System
```bash
npm install
npm start
```

### API Base URL
```
http://localhost:3000/api
```

### Browser Access
```
http://localhost:3000
```

### Shutdown
```bash
Press Ctrl+C in terminal
```

---

## ✅ Bottom Line

**The system is FULLY CONNECTED and WORKING:**
- Frontend ↔ API Server ✅ 100% Connected
- API Server ↔ Payroll Logic ✅ 100% Connected
- Payroll Logic ↔ Data Storage ✅ 100% Connected

The C++ backend exists as an optional, standalone application that can be integrated later if needed.

**Start using it now with:**
```bash
npm start
```

Open: http://localhost:3000
