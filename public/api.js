// API Configuration
const API_BASE_URL = window.location.port === '3000'
    ? '/api'
    : 'http://localhost:3000/api';

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
                if (response.status === 401 && endpoint !== '/auth/login') {
                    this.clearSession();
                }
                throw new Error(result.message || 'API Error');
            }

            this.offlineMode = false;
            return result;
        } catch (error) {
            if (this.canUseOfflineFallback(error)) {
                return this.offlineRequest(endpoint, method, data);
            }
            console.error('API Error:', error);
            throw error;
        }
    }

    static canUseOfflineFallback(error) {
        return error instanceof TypeError || error.message === 'Failed to fetch';
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

    static getSession() {
        const savedSession = localStorage.getItem(this.sessionStorageKey);
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
        localStorage.setItem(this.sessionStorageKey, JSON.stringify(authData));
    }

    static clearSession() {
        localStorage.removeItem(this.sessionStorageKey);
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
        const bonus = emp.basicSalary * (emp.bonusPercentage / 100);
        const grossSalary = emp.basicSalary + (emp.allowance || 0) + bonus;
        const tax = grossSalary * this.taxRate;
        const providentFund = emp.basicSalary * this.providentFundRate;
        const totalDeductions = tax + providentFund + this.healthInsurance;

        return {
            employee: { id: emp.id, name: emp.name, department: emp.department, picture: emp.picture },
            salary: { basic: emp.basicSalary, allowance: emp.allowance, bonus: bonus, gross: grossSalary },
            deductions: { tax, providentFund, healthInsurance: this.healthInsurance, total: totalDeductions },
            netSalary: grossSalary - totalDeductions
        };
    }

    static offlineRequest(endpoint, method, data) {
        this.offlineMode = true;
        const employees = this.getOfflineEmployees();

        if (endpoint === '/health') return this.response({ offline: true }, 'Demo mode');

        if (endpoint === '/auth/register' && method === 'POST') {
            const username = this.normalizeUsername(data.username);
            const users = this.getOfflineUsers();
            if (users.some(u => u.username === username)) throw new Error('Username is already registered');
            const newUser = { id: Date.now(), username, password: data.password };
            users.push(newUser);
            this.saveOfflineUsers(users);
            return this.response(newUser, 'Registration complete');
        }

        if (endpoint === '/auth/login' && method === 'POST') {
            const username = this.normalizeUsername(data.username);
            const users = this.getOfflineUsers();
            const user = users.find(u => u.username === username);
            if (!user || user.password !== data.password) throw new Error('Invalid username or password');
            return this.response({
                token: 'demo-token-' + Date.now(),
                expiresAt: Date.now() + 3600000,
                user: { id: user.id, username: user.username }
            }, 'Login successful');
        }

        if (endpoint === '/employees' && method === 'GET') return this.response(employees);
        
        if (endpoint === '/employees' && method === 'POST') {
            const newEmp = { ...data, id: Date.now() };
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
            return this.response({ payrolls: employees.map(e => ({ name: e.name, net: 1000 })) }); // Simplified
        }

        if (endpoint === '/statistics') {
            return this.response({ totalEmployees: employees.length }); // Simplified
        }

        throw new Error('Not supported in demo mode');
    }

    // Helper wrappers
    static async register(d) { return this.request('/auth/register', 'POST', d); }
    static async login(d) { return this.request('/auth/login', 'POST', d); }
    static async getCurrentUser() { return this.request('/auth/me'); }
    static async logout() { return this.request('/auth/logout', 'POST'); }
    static async getEmployees() { return this.request('/employees'); }
    static async getAllPayroll() { return this.request('/payroll'); }
    static async createEmployee(d) { return this.request('/employees', 'POST', d); }
    static async updateEmployee(id, d) { return this.request(`/employees/${id}`, 'PUT', d); }
    static async deleteEmployee(id) { return this.request(`/employees/${id}`, 'DELETE'); }
    static async getMonthlyReport() { return this.request('/report/monthly'); }
    static async getStatistics() { return this.request('/statistics'); }
    static async healthCheck() { return this.request('/health'); }
};

// Static properties
PayrollAPI.offlineMode = false;
PayrollAPI.storageKey = 'payroll-demo-employees';
PayrollAPI.authStorageKey = 'payroll-demo-users';
PayrollAPI.sessionStorageKey = 'payroll-session';
PayrollAPI.taxRate = 0.12;
PayrollAPI.healthInsurance = 100;
PayrollAPI.providentFundRate = 0.08;

window.PayrollAPI = PayrollAPI;
console.log('PayrollAPI loaded successfully');
