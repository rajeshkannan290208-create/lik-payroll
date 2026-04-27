// API Configuration
const API_BASE_URL = "https://lik-payroll.onrender.com/api";

const DEMO_EMPLOYEES = [
    {
        id: 101,
        name: 'John Smith',
        department: 'Engineering',
        basicSalary: 5000,
        allowance: 500,
        bonusPercentage: 10,
        picture: null
    }
];

// API Client Functions
const PayrollAPI = class {
    // Utility function to make API calls
    static async request(endpoint, method = 'GET', data = null) {
        try {
            const options = {
                method: method,
                headers: {
                    'Content-Type': 'application/json',
                },
            };
            const session = this.getSession();

            if (session?.token) {
                options.headers.Authorization = `Bearer ${session.token}`;
            }

            if (data) {
                options.body = JSON.stringify(data);
            }

            const response = await fetch(`${API_BASE_URL}${endpoint}`, options);
            
            // Handle non-JSON responses
            const contentType = response.headers.get('content-type');
            let result;
            if (contentType && contentType.includes('application/json')) {
                result = await response.json();
            } else {
                const text = await response.text();
                result = { success: response.ok, message: text };
            }

            if (!response.ok) {
                console.warn(`✗ API Error ${response.status}: ${endpoint}`);
                const isLoginEndpoint = endpoint === '/auth/login' || endpoint === '/auth/admin/login' || endpoint === '/auth/user/login';
                if (response.status === 401 && !isLoginEndpoint) {
                    console.error('🔐 Session unauthorized. Re-locking app.');
                    this.clearSession();
                    if (window.lockApp) {
                        window.lockApp('login');
                    }
                }
                throw new Error(result.message || 'API Error');
            }

            this.offlineMode = false;
            return result;
        } catch (error) {
            if (this.canUseOfflineFallback(error, endpoint)) {
                return this.offlineRequest(endpoint, method, data);
            }
            if (this.isNetworkError(error)) {
                this.offlineMode = false;
                throw new Error('Cannot reach the payroll server on http://localhost:3000. Start the server and try again.');
            }
            console.error('API Error:', error);
            throw error;
        }
    }

    static isNetworkError(error) {
        return error instanceof TypeError || error.message === 'Failed to fetch';
    }

    static canUseOfflineFallback(error) {
        return this.offlineFallbackEnabled === true && this.isNetworkError(error);
    }

    static response(data, message = 'OK') {
        return {
            success: true,
            data,
            count: Array.isArray(data) ? data.length : undefined,
            message
        };
    }

    static getOfflineEmployees() {
        const savedEmployees = localStorage.getItem(this.storageKey);
        if (!savedEmployees) {
            this.saveOfflineEmployees(DEMO_EMPLOYEES);
            return [...DEMO_EMPLOYEES];
        }
        try {
            const employees = JSON.parse(savedEmployees);
            return Array.isArray(employees) ? employees : [...DEMO_EMPLOYEES];
        } catch (error) {
            return [...DEMO_EMPLOYEES];
        }
    }

    static saveOfflineEmployees(employees) {
        localStorage.setItem(this.storageKey, JSON.stringify(employees));
    }

    static sessionStore() {
        try {
            return window.sessionStorage;
        } catch (error) {
            return window.localStorage;
        }
    }

    static getSession() {
        const savedSession = this.sessionStore().getItem(this.sessionStorageKey);
        if (!savedSession) return null;
        try {
            const session = JSON.parse(savedSession);
            if (session.expiresAt && Number(session.expiresAt) <= Date.now()) {
                this.clearSession();
                return null;
            }
            return session;
        } catch (error) {
            this.clearSession();
            return null;
        }
    }

    static setSession(authData) {
        this.clearSession();
        this.sessionStore().setItem(this.sessionStorageKey, JSON.stringify(authData));
    }

    static clearSession() {
        const keys = [this.sessionStorageKey, 'payroll-session'];
        [window.sessionStorage, window.localStorage].forEach(store => {
            try {
                keys.forEach(key => store.removeItem(key));
            } catch (error) {
                // Ignore storage access issues.
            }
        });
    }

    static getOfflineUsers() {
        const savedUsers = localStorage.getItem(this.authStorageKey);
        if (!savedUsers) return [];
        try {
            const users = JSON.parse(savedUsers);
            return Array.isArray(users) ? users : [];
        } catch (error) {
            return [];
        }
    }

    static saveOfflineUsers(users) {
        localStorage.setItem(this.authStorageKey, JSON.stringify(users));
    }

    static normalizeUsername(username) {
        return String(username || '').trim().toLowerCase();
    }

    static getPayrollDetails(emp) {
        const attendance = emp.attendance || 30;
        const salaryPerDay = emp.basicSalary / 30;
        const earnedBasic = salaryPerDay * attendance;
        const bonus = earnedBasic * ((emp.bonusPercentage || 0) / 100);
        const grossSalary = earnedBasic + (emp.allowance || 0) + bonus;
        const tax = grossSalary * this.taxRate;
        const providentFund = earnedBasic * this.providentFundRate;
        const totalDeductions = tax + providentFund + this.healthInsurance;

        return {
            employee: { 
                id: emp.id, 
                name: emp.name, 
                department: emp.department, 
                picture: emp.picture,
                attendance: attendance,
                leaveDays: emp.leaveDays || 0,
                paymentMethod: emp.paymentMethod || 'Bank Transfer',
                bankName: emp.bankName || '',
                accountNumber: emp.accountNumber || '',
                dateOfJoin: emp.dateOfJoin || ''
            },
            salary: { 
                basic: emp.basicSalary, 
                salaryPerDay: salaryPerDay,
                earnedBasic: earnedBasic,
                allowance: emp.allowance || 0, 
                bonus: bonus, 
                gross: grossSalary 
            },
            deductions: { tax, providentFund, healthInsurance: this.healthInsurance, total: totalDeductions },
            netSalary: grossSalary - totalDeductions
        };
    }

    static offlineRequest(endpoint, method, data) {
        this.offlineMode = true;
        const employees = this.getOfflineEmployees();

        const attendanceKey = 'payroll-demo-attendance';
        const getAttendance = () => {
            const raw = localStorage.getItem(attendanceKey);
            if (!raw) return [];
            try {
                const parsed = JSON.parse(raw);
                return Array.isArray(parsed) ? parsed : [];
            } catch (e) {
                return [];
            }
        };
        const saveAttendance = (records) => localStorage.setItem(attendanceKey, JSON.stringify(records));
        const localDateISO = () => {
            const now = new Date();
            const yyyy = now.getFullYear();
            const mm = String(now.getMonth() + 1).padStart(2, '0');
            const dd = String(now.getDate()).padStart(2, '0');
            return `${yyyy}-${mm}-${dd}`;
        };
        const localMonthISO = () => localDateISO().slice(0, 7);

        if (endpoint === '/health') {
            const users = this.getOfflineUsers();
            return this.response({ offline: true, userCount: users.length }, 'Demo mode');
        }

        if (endpoint === '/auth/register' && method === 'POST') {
            const username = this.normalizeUsername(data.username);
            const users = this.getOfflineUsers();
            if (users.some(u => u.username === username)) throw new Error('Username is already registered');
            const newUser = { id: Date.now(), username, password: data.password, role: 'admin', employeeId: -1 };
            users.push(newUser);
            this.saveOfflineUsers(users);
            return this.response(newUser, 'Registration complete');
        }

        if ((endpoint === '/auth/login' || endpoint === '/auth/admin/login' || endpoint === '/auth/user/login') && method === 'POST') {
            const username = this.normalizeUsername(data.username);
            const users = this.getOfflineUsers();
            const user = users.find(u => u.username === username);
            if (!user || user.password !== data.password) throw new Error('Invalid username or password');
            if (endpoint === '/auth/admin/login' && user.role !== 'admin') throw new Error('Admin credentials required');
            if (endpoint === '/auth/user/login' && user.role !== 'user') throw new Error('User credentials required');
            return this.response({
                token: 'demo-token-' + Date.now(),
                expiresAt: Date.now() + 3600000,
                user: { id: user.id, username: user.username, role: user.role || 'admin', employeeId: user.employeeId ?? -1 }
            }, 'Login successful');
        }

        if (endpoint === '/employees/me' && method === 'GET') {
            const session = this.getSession();
            if (!session?.user) throw new Error('Login required');
            if (session.user.role !== 'user') throw new Error('User access required');
            const employeeId = Number(session.user.employeeId);
            if (!employeeId) throw new Error('User account is not linked to an employee');
            const emp = employees.find(e => Number(e.id) === employeeId);
            if (!emp) throw new Error('Employee not found');
            return this.response(emp);
        }

        if (endpoint === '/payroll/me' && method === 'GET') {
            const session = this.getSession();
            if (!session?.user) throw new Error('Login required');
            if (session.user.role !== 'user') throw new Error('User access required');
            const employeeId = Number(session.user.employeeId);
            if (!employeeId) throw new Error('User account is not linked to an employee');
            const emp = employees.find(e => Number(e.id) === employeeId);
            if (!emp) throw new Error('Employee not found');
            return this.response(this.getPayrollDetails(emp));
        }

        if (endpoint === '/attendance/mark' && method === 'POST') {
            const session = this.getSession();
            if (!session?.user) throw new Error('Login required');
            if (session.user.role !== 'user') throw new Error('User access required');
            const employeeId = Number(session.user.employeeId);
            if (!employeeId) throw new Error('User account is not linked to an employee');

            const today = localDateISO();
            const records = getAttendance();
            if (records.some(r => Number(r.employeeId) === employeeId && String(r.date) === today)) {
                throw new Error('Attendance already marked for today');
            }

            records.push({ employeeId, date: today, timestamp: Date.now() });
            saveAttendance(records);

            const monthPrefix = localMonthISO();
            const monthCount = records.filter(r => Number(r.employeeId) === employeeId && String(r.date || '').startsWith(monthPrefix)).length;

            const idx = employees.findIndex(e => Number(e.id) === employeeId);
            if (idx !== -1) {
                employees[idx] = { ...employees[idx], attendance: monthCount };
                this.saveOfflineEmployees(employees);
            }

            return this.response({ employeeId, date: today, attendanceThisMonth: monthCount }, 'Attendance marked');
        }

        if (endpoint === '/attendance/me' && method === 'GET') {
            const session = this.getSession();
            if (!session?.user) throw new Error('Login required');
            if (session.user.role !== 'user') throw new Error('User access required');
            const employeeId = Number(session.user.employeeId);
            if (!employeeId) throw new Error('User account is not linked to an employee');

            const records = getAttendance()
                .filter(r => Number(r.employeeId) === employeeId)
                .sort((a, b) => String(b.date || '').localeCompare(String(a.date || '')) || Number(b.timestamp || 0) - Number(a.timestamp || 0));

            return this.response(records);
        }

        if (endpoint === '/attendance' && method === 'GET') {
            return this.response(getAttendance());
        }

        if (endpoint === '/employees' && method === 'GET') return this.response(employees);
        
        if (endpoint === '/employees' && method === 'POST') {
            const maxId = employees.reduce((max, e) => Math.max(max, Number(e.id) || 100), 100);
            const newEmp = { ...data, id: maxId + 1 };
            employees.push(newEmp);
            this.saveOfflineEmployees(employees);
            return this.response(newEmp, 'Employee added');
        }

        const empMatch = endpoint.match(/^\/employees\/(\d+)$/);
        if (empMatch) {
            const id = Number(empMatch[1]);
            const index = employees.findIndex(e => e.id === id);
            if (index === -1) throw new Error('Not found');
            if (method === 'GET') return this.response(employees[index]);
            if (method === 'PUT') {
                employees[index] = { ...employees[index], ...data };
                this.saveOfflineEmployees(employees);
                return this.response(employees[index]);
            }
            if (method === 'DELETE') {
                employees.splice(index, 1);
                this.saveOfflineEmployees(employees);
                return this.response(null, 'Deleted');
            }
        }

        if (endpoint === '/report/monthly') {
            const reportData = employees.map(e => {
                const details = this.getPayrollDetails(e);
                return { name: e.name, net: details.netSalary };
            });
            return this.response({ payrolls: reportData });
        }

        if (endpoint === '/payroll' && method === 'GET') {
            const payrolls = employees.map(e => this.getPayrollDetails(e));
            return this.response(payrolls);
        }

        const payrollMatch = endpoint.match(/^\/payroll\/(\d+)$/);
        if (payrollMatch) {
            const id = Number(payrollMatch[1]);
            const emp = employees.find(e => e.id === id);
            if (!emp) throw new Error('Not found');
            return this.response(this.getPayrollDetails(emp));
        }

        if (endpoint === '/statistics') {
            const totalPayroll = employees.reduce((sum, e) => sum + this.getPayrollDetails(e).netSalary, 0);
            return this.response({ 
                totalEmployees: employees.length,
                totalPayroll: totalPayroll,
                averageNetSalary: employees.length ? totalPayroll / employees.length : 0
            });
        }

        throw new Error('Not supported in demo mode');
    }

    // Helper wrappers
    static async register(d) { return this.request('/auth/register', 'POST', d); }
    static async login(d) { return this.request('/auth/login', 'POST', d); }
    static async adminLogin(d) { return this.request('/auth/admin/login', 'POST', d); }
    static async userLogin(d) { return this.request('/auth/user/login', 'POST', d); }
    static async getCurrentUser() { return this.request('/auth/me'); }
    static async logout() { return this.request('/auth/logout', 'POST'); }
    static async getEmployees() { return this.request('/employees'); }
    static async getEmployeeMe() { return this.request('/employees/me'); }
    static async getAllPayroll() { return this.request('/payroll'); }
    static async getPayrollMe() { return this.request('/payroll/me'); }
    static async createEmployee(d) { return this.request('/employees', 'POST', d); }
    static async updateEmployee(id, d) { return this.request(`/employees/${id}`, 'PUT', d); }
    static async deleteEmployee(id) { return this.request(`/employees/${id}`, 'DELETE'); }
    static async getPayroll(id) { return this.request(`/payroll/${id}`); }
    static async getMonthlyReport() { return this.request('/report/monthly'); }
    static async getStatistics() { return this.request('/statistics'); }
    static async markAttendance() { return this.request('/attendance/mark', 'POST'); }
    static async getAttendanceMe() { return this.request('/attendance/me'); }
    static async getAttendanceAll() { return this.request('/attendance'); }
    static async getAdminUsers() { return this.request('/admin/users'); }
    static async createAdminUser(d) { return this.request('/admin/users', 'POST', d); }
    static async healthCheck() { return this.request('/health'); }
};

// Static properties
PayrollAPI.offlineMode = false;
PayrollAPI.storageKey = 'payroll-demo-employees';
PayrollAPI.authStorageKey = 'payroll-demo-users';
PayrollAPI.sessionStorageKey = 'payroll-session';
PayrollAPI.offlineFallbackEnabled = false;
PayrollAPI.taxRate = 0.12;
PayrollAPI.healthInsurance = 100;
PayrollAPI.providentFundRate = 0.08;

window.PayrollAPI = PayrollAPI;
console.log('PayrollAPI loaded successfully');
