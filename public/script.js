// Global variables
let currentEditingEmployee = null;
let allEmployees = [];
let currentUser = null;

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    initializeApp();
});

// Initialize application
async function initializeApp() {
    if (typeof PayrollAPI === 'undefined') {
        setTimeout(initializeApp, 100);
        return;
    }
    
    setupEventListeners();
    setupAuthEventListeners();
    await checkServerStatus();

    const health = await PayrollAPI.healthCheck();
    const userCount = health.userCount || 0;

    const savedSession = PayrollAPI.getSession();
    if (!savedSession) {
        lockApp(userCount === 0 ? 'register' : 'login');
        return;
    }

    try {
        const response = await PayrollAPI.getCurrentUser();
        currentUser = response.data;
        unlockApp(currentUser);
        loadEmployees();
        loadDashboard();
    } catch (error) {
        PayrollAPI.clearSession();
        lockApp('login');
    }
}

// Setup lock screen listeners
function setupAuthEventListeners() {
    const loginForm = document.getElementById('loginForm');
    const registerForm = document.getElementById('registerForm');

    if (loginForm) {
        loginForm.addEventListener('submit', async (event) => {
            event.preventDefault();
            const username = document.getElementById('loginUsername').value;
            const password = document.getElementById('loginPassword').value;
            try {
                const response = await PayrollAPI.login({ username, password });
                PayrollAPI.setSession(response.data);
                unlockApp(response.data.user);
                loadEmployees();
                loadDashboard();
            } catch (error) {
                showNotification(error.message, 'error');
            }
        });
    }

    if (registerForm) {
        registerForm.addEventListener('submit', async (event) => {
            event.preventDefault();
            const username = document.getElementById('registerUsername').value;
            const password = document.getElementById('registerPassword').value;
            try {
                await PayrollAPI.register({ username, password });
                showNotification('Registered successfully! Please login.', 'success');
                switchAuthMode('login');
            } catch (error) {
                showNotification(error.message, 'error');
            }
        });
    }
}

function switchAuthMode(mode) {
    const isRegistering = mode === 'register';
    document.getElementById('loginForm').classList.toggle('hidden', isRegistering);
    document.getElementById('registerForm').classList.toggle('hidden', !isRegistering);
    document.getElementById('loginTab').classList.toggle('active', !isRegistering);
    document.getElementById('registerTab').classList.toggle('active', isRegistering);
}

function lockApp(mode = 'login') {
    currentUser = null;
    document.body.classList.add('is-locked');
    document.getElementById('lockScreen').classList.remove('hidden');
    switchAuthMode(mode);
}

function unlockApp(user) {
    currentUser = user;
    document.body.classList.remove('is-locked');
    document.getElementById('lockScreen').classList.add('hidden');
}

// Check server status
async function checkServerStatus() {
    const dot = document.getElementById('statusDot');
    const text = document.getElementById('statusText');
    try {
        const response = await PayrollAPI.healthCheck();
        dot.className = 'status-dot';
        text.textContent = 'Connected';
    } catch (error) {
        dot.className = 'status-dot disconnected';
        text.textContent = 'Demo Mode';
    }
}

// Event listeners
function setupEventListeners() {
    document.querySelectorAll('.nav-btn').forEach(btn => {
        btn.addEventListener('click', () => switchPage(btn.getAttribute('data-page')));
    });
}

function switchPage(pageName) {
    document.querySelectorAll('.page').forEach(page => page.classList.remove('active'));
    document.querySelectorAll('.nav-btn').forEach(btn => btn.classList.remove('active'));
    document.getElementById(pageName).classList.add('active');
    document.querySelector(`[data-page="${pageName}"]`).classList.add('active');
    if (pageName === 'payroll') loadAllPayrolls();
}

// Load employees
async function loadEmployees() {
    try {
        const response = await PayrollAPI.getEmployees();
        allEmployees = response.data;
        displayEmployees(allEmployees);
    } catch (error) {
        console.error('Failed to load employees:', error);
    }
}

function displayEmployees(employees) {
    const tbody = document.getElementById('employeesTable');
    if (!tbody) return;
    tbody.innerHTML = employees.map(emp => `
        <tr>
            <td><img src="${emp.picture || 'https://via.placeholder.com/40'}" style="width: 40px; height: 40px; border-radius: 50%; object-fit: cover;"></td>
            <td>${emp.id}</td>
            <td>${emp.name}</td>
            <td>${emp.department}</td>
            <td>$${emp.basicSalary.toFixed(2)}</td>
            <td>
                <button class="btn btn-info" onclick="editEmployee(${emp.id})">Edit</button>
                <button class="btn btn-danger" onclick="deleteEmployee(${emp.id})">Delete</button>
            </td>
        </tr>
    `).join('');
}

function showAddEmployeeForm() {
    currentEditingEmployee = null;
    document.getElementById('formTitle').textContent = 'Add New Employee';
    document.getElementById('employeeForm').reset();
    document.getElementById('imagePreviewContainer').classList.add('hidden');
    document.getElementById('employeeFormContainer').classList.remove('hidden');
}

function hideEmployeeForm() {
    document.getElementById('employeeFormContainer').classList.add('hidden');
}

async function saveEmployee() {
    const name = document.getElementById('empName').value.trim();
    const department = document.getElementById('empDept').value.trim();
    const salaryVal = document.getElementById('empSalary').value;
    const basicSalary = parseFloat(salaryVal);
    const allowance = parseFloat(document.getElementById('empAllowance').value) || 0;
    const bonusPercentage = parseFloat(document.getElementById('empBonus').value) || 0;

    if (!name || !department || isNaN(basicSalary) || basicSalary <= 0) {
        showNotification('Please fill in Name, Department, and a valid Salary', 'error');
        return;
    }

    let pictureData = null;
    const imgEl = document.getElementById('imagePreview');
    if (imgEl && imgEl.src && imgEl.src.startsWith('data:')) {
        pictureData = imgEl.src;
    }

    const data = { name, department, basicSalary, allowance, bonusPercentage, picture: pictureData };

    try {
        if (currentEditingEmployee) {
            await PayrollAPI.updateEmployee(currentEditingEmployee.id, data);
        } else {
            await PayrollAPI.createEmployee(data);
        }
        hideEmployeeForm();
        await loadEmployees();
        await loadDashboard();
        showNotification('Employee saved successfully! ✨', 'success');
    } catch (error) {
        showNotification('Error saving: ' + (error.message || 'Unknown error'), 'error');
    }
}

async function editEmployee(id) {
    const emp = allEmployees.find(e => e.id === id);
    if (!emp) return;
    currentEditingEmployee = emp;
    document.getElementById('formTitle').textContent = 'Edit Employee';
    document.getElementById('empName').value = emp.name;
    document.getElementById('empDept').value = emp.department;
    document.getElementById('empSalary').value = emp.basicSalary;
    document.getElementById('empAllowance').value = emp.allowance;
    document.getElementById('empBonus').value = emp.bonusPercentage;
    if (emp.picture) {
        document.getElementById('imagePreview').src = emp.picture;
        document.getElementById('imagePreviewContainer').classList.remove('hidden');
    }
    document.getElementById('employeeFormContainer').classList.remove('hidden');
}

async function deleteEmployee(id) {
    if (!confirm('Are you sure?')) return;
    try {
        await PayrollAPI.deleteEmployee(id);
        loadEmployees();
        loadDashboard();
        showNotification('Deleted', 'success');
    } catch (error) {
        showNotification('Error deleting', 'error');
    }
}

function previewImage(input) {
    if (input.files && input.files[0]) {
        const reader = new FileReader();
        reader.onload = (e) => {
            document.getElementById('imagePreview').src = e.target.result;
            document.getElementById('imagePreviewContainer').classList.remove('hidden');
        };
        reader.readAsDataURL(input.files[0]);
    }
}

async function loadDashboard() {
    try {
        const response = await PayrollAPI.getStatistics();
        const data = response.data;

        // Calculate highest salary with employee name
        let highest = 0;
        let highestEmpName = '—';
        if (allEmployees && allEmployees.length > 0) {
            allEmployees.forEach(e => {
                const bonus = e.basicSalary * ((e.bonusPercentage || 0) / 100);
                const gross = e.basicSalary + (e.allowance || 0) + bonus;
                if (gross > highest) {
                    highest = gross;
                    highestEmpName = e.name;
                }
            });
        }

        const el = (id) => document.getElementById(id);
        if(el('totalEmployees')) el('totalEmployees').textContent = allEmployees.length;
        if(el('totalPayroll')) el('totalPayroll').textContent = '$' + (data.totalPayroll || 0).toFixed(2);
        if(el('averageSalary')) el('averageSalary').textContent = '$' + (data.averageNetSalary || 0).toFixed(2);
        if(el('highestSalary')) el('highestSalary').textContent = '$' + highest.toFixed(2);
        if(el('highestName')) el('highestName').textContent = highestEmpName;
    } catch (error) {
        console.error(error);
    }
}

async function loadAllPayrolls() {
    try {
        const response = await PayrollAPI.getAllPayroll();
        const payrolls = response.data;
        document.getElementById('payrollList').innerHTML = payrolls.map(p => `
            <div class="stat-card" style="cursor: pointer; margin-bottom: 1rem" onclick="viewPayrollDetails(${p.employee.id})">
                <h3>${p.employee.name}</h3>
                <p>${p.employee.department}</p>
                <p class="stat-value" style="font-size: 1.5rem">$${p.netSalary.toFixed(2)}</p>
            </div>
        `).join('');
    } catch (error) {
        console.error(error);
    }
}

async function viewPayrollDetails(id) {
    try {
        const response = await PayrollAPI.request(`/payroll/${id}`);
        const p = response.data;
        
        const esi = p.deductions.healthInsurance || 0;
        
        document.getElementById('payrollDetails').innerHTML = `
            <div class="payslip-wrapper">
                <div class="payslip-header">
                    <h2>${p.employee.name}</h2>
                    <p class="text-muted">${p.employee.department} | ID: EMP-${p.employee.id}</p>
                </div>
                
                <div class="payslip-grid">
                    <div class="payslip-section earnings">
                        <h3>💰 Earnings (Additions)</h3>
                        <div class="payslip-row">
                            <span>Basic pay</span>
                            <span>$${p.salary.basic.toFixed(2)}</span>
                        </div>
                        <div class="payslip-row">
                            <span>Allowances (HRA, DA, travel, etc.)</span>
                            <span>$${(p.salary.allowance || 0).toFixed(2)}</span>
                        </div>
                        <div class="payslip-row">
                            <span>Bonuses / incentives</span>
                            <span>$${(p.salary.bonus || 0).toFixed(2)}</span>
                        </div>
                        <div class="payslip-row">
                            <span>Overtime pay</span>
                            <span>$0.00</span>
                        </div>
                        <div class="payslip-total">
                            <strong>Total Earnings</strong>
                            <strong>$${p.salary.gross.toFixed(2)}</strong>
                        </div>
                    </div>

                    <div class="payslip-section deductions">
                        <h3>📉 Deductions</h3>
                        <div class="payslip-row">
                            <span>Tax (Income Tax / TDS)</span>
                            <span>$${p.deductions.tax.toFixed(2)}</span>
                        </div>
                        <div class="payslip-row">
                            <span>PF (Provident Fund)</span>
                            <span>$${p.deductions.providentFund.toFixed(2)}</span>
                        </div>
                        <div class="payslip-row">
                            <span>ESI (if applicable)</span>
                            <span>$${esi.toFixed(2)}</span>
                        </div>
                        <div class="payslip-row">
                            <span>Loan or advance deductions</span>
                            <span>$0.00</span>
                        </div>
                        <div class="payslip-total">
                            <strong>Total Deductions</strong>
                            <strong>$${p.deductions.total.toFixed(2)}</strong>
                        </div>
                    </div>
                </div>

                <div class="payslip-net">
                    <h3>🧾 Net Salary</h3>
                    <p class="net-formula">👉 Final amount employee receives = Earnings – Deductions</p>
                    <div class="net-amount">$${p.netSalary.toFixed(2)}</div>
                </div>

                <div class="payslip-others">
                    <h3>📊 Other important parts</h3>
                    <div class="others-grid">
                        <div class="other-item"><span class="icon">📄</span> Payslip generation</div>
                        <div class="other-item"><span class="icon">📅</span> Attendance tracking</div>
                        <div class="other-item"><span class="icon">🌴</span> Leave management</div>
                        <div class="other-item"><span class="icon">🧮</span> Salary calculation system</div>
                        <div class="other-item"><span class="icon">💳</span> Payment method (Bank Transfer)</div>
                    </div>
                </div>

                <div class="payslip-actions">
                    <button class="btn btn-primary" onclick="window.print()">Print / Download</button>
                    <button class="btn btn-secondary" onclick="closePayrollModal()">Close</button>
                </div>
            </div>
        `;
        document.getElementById('payrollModal').classList.remove('hidden');
    } catch (error) {
        showNotification('Error loading payroll details', 'error');
    }
}

function closePayrollModal() {
    document.getElementById('payrollModal').classList.add('hidden');
}

function showNotification(message, type = 'info') {
    const note = document.getElementById('notification');
    note.textContent = message;
    note.className = 'notification ' + type;
    note.classList.remove('hidden');
    setTimeout(() => note.classList.add('hidden'), 3000);
}
