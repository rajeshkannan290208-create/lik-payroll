// Global variables
let currentEditingEmployee = null;
let allEmployees = [];
let allUsers = [];
let allAttendance = [];
let attendanceSummary = {};
let currentUser = null;

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    if (typeof PayrollAPI !== 'undefined') {
        PayrollAPI.sessionStorageKey = 'payroll-admin-session';
    }
    initializeApp();
    initializePeriodSelectors();
});

// Initialize application
async function initializeApp() {
    if (currentUser) return;

    if (typeof PayrollAPI === 'undefined') {
        setTimeout(initializeApp, 100);
        return;
    }

    setupEventListeners();
    setupAuthEventListeners();
    setupAdminEventListeners();
    await checkServerStatus();
    lockApp();
}

// Setup lock screen listeners
function setupAuthEventListeners() {
    const loginBtn = document.getElementById('loginBtn');
    const loginForm = document.getElementById('loginForm');
    if (!loginBtn || !loginForm) return;

    const handleLogin = async (event) => {
        if (event) event.preventDefault();
        const username = document.getElementById('loginUsername')?.value?.trim();
        const password = document.getElementById('loginPassword')?.value;

        if (!username || !password) {
            setAuthMessage('Please enter both username and password', 'error');
            showNotification('Please enter both username and password', 'error');
            return;
        }

        try {
            loginBtn.disabled = true;
            loginBtn.textContent = 'Checking...';
            setAuthMessage('');

            const response = await PayrollAPI.adminLogin({ username, password });
            PayrollAPI.setSession(response.data);
            unlockApp(response.data.user);
            await loadEmployees();
            await loadDashboard();
            showNotification('Welcome back!', 'success');
            setAuthMessage('');
        } catch (error) {
            setAuthMessage(error.message || 'Login failed', 'error');
            showNotification(error.message || 'Login failed', 'error');
        } finally {
            loginBtn.disabled = false;
            loginBtn.textContent = 'Login';
        }
    };

    loginBtn.addEventListener('click', handleLogin);
    loginForm.addEventListener('submit', handleLogin);
}

function lockApp() {
    currentUser = null;
    document.body.classList.add('is-locked');
    document.getElementById('lockScreen').classList.remove('hidden');
}

function unlockApp(user) {
    currentUser = user;
    document.body.classList.remove('is-locked');
    document.getElementById('lockScreen').classList.add('hidden');
    setAuthMessage('');
}

window.lockApp = lockApp;
window.unlockApp = unlockApp;

// Check server status
async function checkServerStatus() {
    const dot = document.getElementById('statusDot');
    const text = document.getElementById('statusText');
    try {
        await PayrollAPI.healthCheck();
        dot.className = 'status-dot';
        text.textContent = 'Connected';
    } catch (error) {
        dot.className = 'status-dot disconnected';
        text.textContent = 'Server Offline';
    }
}

// Event listeners
function setupEventListeners() {
    document.querySelectorAll('.nav-btn').forEach(btn => {
        btn.addEventListener('click', () => switchPage(btn.getAttribute('data-page')));
    });

    const logoutBtn = document.getElementById('adminLogoutBtn');
    if (logoutBtn) {
        logoutBtn.addEventListener('click', logoutAdmin);
    }
}

async function logoutAdmin() {
    try {
        await PayrollAPI.logout();
    } catch (error) {
        console.warn('Logout request failed:', error);
    }

    PayrollAPI.clearSession();
    lockApp();
    showNotification('Logged out', 'info');
}

function setAuthMessage(message, type = 'error') {
    const authMessage = document.getElementById('authMessage');
    if (!authMessage) return;

    if (!message) {
        authMessage.textContent = '';
        authMessage.classList.add('hidden');
        return;
    }

    authMessage.textContent = message;
    authMessage.className = `auth-message ${type}`;
    authMessage.classList.remove('hidden');
}

function setupAdminEventListeners() {
    const form = document.getElementById('createUserForm');
    if (!form) return;

    form.addEventListener('submit', async (event) => {
        event.preventDefault();

        if (PayrollAPI.offlineMode) {
            showNotification('User management is unavailable while the server is offline', 'error');
            return;
        }

        const employeeId = Number(document.getElementById('userEmployeeId')?.value || 0);
        const username = document.getElementById('newUsername')?.value?.trim();
        const password = document.getElementById('newPassword')?.value;

        if (!employeeId || !username || !password) {
            showNotification('Please fill Employee, Username and Password', 'error');
            return;
        }

        try {
            const response = await PayrollAPI.createAdminUser({ username, password, employeeId });
            document.getElementById('newUsername').value = '';
            document.getElementById('newPassword').value = '';
            await loadUsersPage();
            showNotification(`Employee login created. User ID ${response.data.id} linked to employee ${employeeId}`, 'success');
        } catch (error) {
            showNotification(error.message || 'Failed to create login', 'error');
        }
    });
}

function switchPage(pageName) {
    document.querySelectorAll('.page').forEach(page => page.classList.remove('active'));
    document.querySelectorAll('.nav-btn').forEach(btn => btn.classList.remove('active'));
    document.getElementById(pageName).classList.add('active');
    document.querySelector(`[data-page="${pageName}"]`).classList.add('active');
    if (pageName === 'payroll') loadAllPayrolls();
    if (pageName === 'users') loadUsersPage();
    if (pageName === 'attendance') loadAttendancePage();
}

// Load employees
async function loadEmployees() {
    try {
        const response = await PayrollAPI.getEmployees();
        allEmployees = response.data;
        if (!PayrollAPI.offlineMode) {
            await loadAttendanceSummary();
        } else {
            attendanceSummary = {};
            allAttendance = [];
        }
        displayEmployees(allEmployees);
        populateUserEmployeeSelect();
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
            <td>${(attendanceSummary[emp.id]?.monthCount ?? emp.attendance ?? '-')}</td>
            <td>${attendanceSummary[emp.id]?.lastDate || '-'}</td>
            <td>
                <button class="btn btn-info" onclick="editEmployee(${emp.id})">Edit</button>
                <button class="btn btn-danger" onclick="deleteEmployee(${emp.id})">Delete</button>
            </td>
        </tr>
    `).join('');
}

async function loadAttendanceSummary() {
    try {
        const response = await PayrollAPI.getAttendanceAll();
        allAttendance = response.data || [];

        const monthPrefix = new Date().toISOString().slice(0, 7); // YYYY-MM
        const summary = {};

        for (const record of allAttendance) {
            const employeeId = Number(record.employeeId);
            if (!employeeId) continue;

            if (!summary[employeeId]) {
                summary[employeeId] = { monthCount: 0, lastDate: null };
            }

            const date = String(record.date || '');
            if (date) {
                if (!summary[employeeId].lastDate || date > summary[employeeId].lastDate) {
                    summary[employeeId].lastDate = date;
                }
                if (date.startsWith(monthPrefix)) {
                    summary[employeeId].monthCount++;
                }
            }
        }

        attendanceSummary = summary;
    } catch (error) {
        console.error('Failed to load attendance:', error);
        attendanceSummary = {};
        allAttendance = [];
    }
}

function populateUserEmployeeSelect() {
    const select = document.getElementById('userEmployeeId');
    if (!select) return;

    const assignedEmployeeIds = new Set(
        (allUsers || [])
            .filter(u => u && u.role === 'user' && Number(u.employeeId) > 0)
            .map(u => Number(u.employeeId))
    );

    const options = (allEmployees || [])
        .filter(e => e && !assignedEmployeeIds.has(Number(e.id)))
        .map(e => `<option value="${e.id}">${e.id} - ${e.name}</option>`)
        .join('');

    select.innerHTML = options || '<option value="">No available employees</option>';
}

async function loadUsersPage() {
    if (PayrollAPI.offlineMode) {
        showNotification('User management is unavailable while the server is offline', 'error');
        return;
    }

    try {
        if (!allEmployees.length) {
            await loadEmployees();
        }

        const response = await PayrollAPI.getAdminUsers();
        allUsers = response.data || [];
        displayUsers(allUsers);
        populateUserEmployeeSelect();
    } catch (error) {
        console.error('Failed to load users:', error);
        showNotification('Failed to load users', 'error');
    }
}

function displayUsers(users) {
    const tbody = document.getElementById('usersTable');
    if (!tbody) return;

    const employeeMap = new Map((allEmployees || []).map(e => [Number(e.id), e]));
    const employeeUsers = (users || []).filter(u => u && u.role === 'user');

    if (employeeUsers.length === 0) {
        tbody.innerHTML = '<tr><td colspan="4" style="color:var(--text-dim)">No employee logins yet.</td></tr>';
        return;
    }

    tbody.innerHTML = employeeUsers.map(u => {
        const emp = employeeMap.get(Number(u.employeeId));
        const employeeId = u.employeeId ? String(u.employeeId) : '-';
        const employeeName = emp ? emp.name : 'Not linked';
        return `
            <tr>
                <td>${u.id}</td>
                <td>${u.username}</td>
                <td>${employeeId}</td>
                <td>${employeeName}</td>
            </tr>
        `;
    }).join('');
}

async function loadAttendancePage() {
    if (PayrollAPI.offlineMode) {
        showNotification('Attendance is unavailable while the server is offline', 'error');
        return;
    }

    if (!allEmployees.length) {
        await loadEmployees();
    }
    await loadAttendanceSummary();
    displayAttendanceAdmin(allAttendance);
}

function displayAttendanceAdmin(records) {
    const tbody = document.getElementById('attendanceTableAdmin');
    if (!tbody) return;

    const employeeMap = new Map((allEmployees || []).map(e => [Number(e.id), e]));
    const list = records || [];

    if (list.length === 0) {
        tbody.innerHTML = '<tr><td colspan="3" style="color:var(--text-dim)">No attendance marked yet.</td></tr>';
        return;
    }

    tbody.innerHTML = list.map(r => {
        const empId = Number(r.employeeId);
        const emp = employeeMap.get(empId);
        const name = emp ? emp.name : 'â€”';
        return `
            <tr>
                <td>${r.date || 'â€”'}</td>
                <td>${empId || 'â€”'}</td>
                <td>${name}</td>
            </tr>
        `;
    }).join('');
}

function showAddEmployeeForm() {
    currentEditingEmployee = null;
    document.getElementById('formTitle').textContent = 'Add New Employee';
    document.getElementById('employeeForm').reset();
    document.getElementById('empId').value = '';
    document.getElementById('empIdGroup').classList.add('hidden');
    document.getElementById('imagePreviewContainer').classList.add('hidden');
    document.getElementById('employeeFormContainer').classList.remove('hidden');
    toggleBankDetails();
}

function toggleBankDetails() {
    const method = document.getElementById('empPaymentMethod').value;
    const bankDetails = document.getElementById('bankDetails');
    if (method === 'Bank Transfer') {
        bankDetails.classList.remove('hidden');
    } else {
        bankDetails.classList.add('hidden');
    }
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

    const attendance = parseInt(document.getElementById('empAttendance').value) || 0;
    const leaveDays = parseInt(document.getElementById('empLeave').value) || 0;
    const paymentMethod = document.getElementById('empPaymentMethod').value;
    const bankName = document.getElementById('empBankName').value.trim();
    const accountNumber = document.getElementById('empAccountNumber').value.trim();
    const dateOfJoin = document.getElementById('empDOJ').value;

    let pictureData = null;
    const imgEl = document.getElementById('imagePreview');
    if (imgEl && imgEl.src && imgEl.src.startsWith('data:')) {
        pictureData = imgEl.src;
    }

    const data = { 
        name, department, basicSalary, allowance, bonusPercentage, 
        picture: pictureData,
        attendance, leaveDays, paymentMethod, bankName, accountNumber,
        dateOfJoin
    };

    try {
        if (currentEditingEmployee) {
            await PayrollAPI.updateEmployee(currentEditingEmployee.id, data);
        } else {
            await PayrollAPI.createEmployee(data);
        }
        hideEmployeeForm();
        await loadEmployees();
        await loadDashboard();
        showNotification('Employee saved successfully! âœ¨', 'success');
    } catch (error) {
        showNotification('Error saving: ' + (error.message || 'Unknown error'), 'error');
    }
}

async function editEmployee(id) {
    const emp = allEmployees.find(e => e.id === id);
    if (!emp) return;
    currentEditingEmployee = emp;
    document.getElementById('formTitle').textContent = 'Edit Employee';
    document.getElementById('empId').value = emp.id;
    document.getElementById('empIdGroup').classList.remove('hidden');
    document.getElementById('empName').value = emp.name;
    document.getElementById('empDept').value = emp.department;
    document.getElementById('empSalary').value = emp.basicSalary;
    document.getElementById('empAllowance').value = emp.allowance;
    document.getElementById('empBonus').value = emp.bonusPercentage;
    document.getElementById('empAttendance').value = emp.attendance || 30;
    document.getElementById('empLeave').value = emp.leaveDays || 0;
    document.getElementById('empPaymentMethod').value = emp.paymentMethod || 'Bank Transfer';
    document.getElementById('empBankName').value = emp.bankName || '';
    document.getElementById('empAccountNumber').value = emp.accountNumber || '';
    document.getElementById('empDOJ').value = emp.dateOfJoin || '';
    
    toggleBankDetails();

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

        // Calculate highest salary with employee name using attendance-based logic
        let highest = 0;
        let highestEmpName = 'â€”';
        if (allEmployees && allEmployees.length > 0) {
            allEmployees.forEach(e => {
                const attendance = e.attendance || 30;
                const earnedBasic = (e.basicSalary / 30) * attendance;
                const bonus = earnedBasic * ((e.bonusPercentage || 0) / 100);
                const gross = earnedBasic + (e.allowance || 0) + bonus;
                
                if (gross > highest) {
                    highest = gross;
                    highestEmpName = e.name;
                }
            });
        }

        const el = (id) => document.getElementById(id);
        if(el('totalEmployees')) {
            el('totalEmployees').textContent = allEmployees.length;
            adjustFontSize(el('totalEmployees'), 2.4, 8);
        }
        if(el('totalPayroll')) {
            el('totalPayroll').textContent = '$' + (data.totalPayroll || 0).toFixed(2);
            adjustFontSize(el('totalPayroll'), 2.4, 10);
        }
        if(el('averageSalary')) {
            el('averageSalary').textContent = '$' + (data.averageNetSalary || 0).toFixed(2);
            adjustFontSize(el('averageSalary'), 2.4, 10);
        }
        if(el('highestSalary')) {
            el('highestSalary').textContent = '$' + highest.toFixed(2);
            adjustFontSize(el('highestSalary'), 2.4, 10);
        }
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
                <p class="stat-value" style="font-size: 1.5rem" id="pay-net-${p.employee.id}">$${p.netSalary.toFixed(2)}</p>
            </div>
        `).join('');

        // Apply dynamic font adjustment to cards
        payrolls.forEach(p => {
            adjustFontSize(document.getElementById(`pay-net-${p.employee.id}`), 1.5, 10);
        });
    } catch (error) {
        console.error(error);
    }
}

async function viewPayrollDetails(id) {
    try {
        const response = await PayrollAPI.getPayroll(id);
        const p = response.data;
        const esi = 100;
        
        document.getElementById('payrollDetails').innerHTML = `
            <div class="payslip-wrapper">
                <div class="payslip-title">MONTHLY PAYSLIP</div>
                <div class="payslip-header">
                    <p><strong>Employee Name:</strong> ${p.employee.name}</p>
                    <p><strong>Employee ID:</strong> ${p.employee.id}</p>
                    <p><strong>Department:</strong> ${p.employee.department}</p>
                    <p><strong>Date of Join:</strong> ${p.employee.dateOfJoin || 'â€”'}</p>
                    <p><strong>Attendance:</strong> ${p.employee.attendance} Days</p>
                    <p><strong>Leaves:</strong> ${p.employee.leaveDays} Days</p>
                </div>
                
                <div class="payslip-section earnings">
                    <h3>EARNINGS</h3>
                    <div class="payslip-row">
                        <span>Basic Salary (Fixed):</span>
                        <span>$${p.salary.basic.toFixed(2)}</span>
                    </div>
                    <div class="payslip-row">
                        <span>Salary Per Day:</span>
                        <span>$${(p.salary.salaryPerDay || 0).toFixed(2)}</span>
                    </div>
                    <div class="payslip-row highlight">
                        <span>Earned Basic (${p.employee.attendance}/30 days):</span>
                        <span>$${p.salary.earnedBasic.toFixed(2)}</span>
                    </div>
                    <div class="payslip-row">
                        <span>Allowance:</span>
                        <span>$${(p.salary.allowance || 0).toFixed(2)}</span>
                    </div>
                    <div class="payslip-row">
                        <span>Bonus:</span>
                        <span>$${(p.salary.bonus || 0).toFixed(2)}</span>
                    </div>
                    <div class="payslip-total">
                        <span>Gross Salary:</span>
                        <span>$${p.salary.gross.toFixed(2)}</span>
                    </div>
                </div>

                <div class="payslip-section deductions">
                    <h3>DEDUCTIONS</h3>
                    <div class="payslip-row">
                        <span>Income Tax (12%):</span>
                        <span>$${p.deductions.tax.toFixed(2)}</span>
                    </div>
                    <div class="payslip-row">
                        <span>Provident Fund (8%):</span>
                        <span>$${p.deductions.providentFund.toFixed(2)}</span>
                    </div>
                    <div class="payslip-row">
                        <span>Health Insurance:</span>
                        <span>$${esi.toFixed(2)}</span>
                    </div>
                    <div class="payslip-total">
                        <span>Total Deductions:</span>
                        <span>$${p.deductions.total.toFixed(2)}</span>
                    </div>
                </div>

                <div class="payslip-net">
                    <h3>Net Salary:</h3>
                    <div class="net-amount">$${p.netSalary.toFixed(2)}</div>
                </div>

                <div class="payslip-footer">
                    <p><strong>Payment Method:</strong> ${p.employee.paymentMethod}</p>
                    ${p.employee.paymentMethod === 'Bank Transfer' ? `
                    <p><strong>Bank:</strong> ${p.employee.bankName || 'â€”'}</p>
                    <p><strong>Account:</strong> ${p.employee.accountNumber || 'â€”'}</p>
                    ` : ''}
                </div>

                <div class="payslip-signature">
                    <div class="signature-line"></div>
                    <p>Authorized Signature</p>
                    <strong>LIK</strong>
                </div>

                <div class="payslip-actions">
                    <button class="btn btn-info" onclick="downloadPayslipPDF();">Download PDF</button>
                    <button class="btn btn-primary" onclick="window.print()">Print</button>
                    <button class="btn btn-secondary" onclick="closePayrollModal()">Close</button>
                </div>
            </div>
        `;
        document.getElementById('payrollModal').classList.remove('hidden');
    } catch (error) {
        showNotification('Error loading payroll details', 'error');
    }
}
async function generateMonthlyReport() {
    try {
        const response = await PayrollAPI.getMonthlyReport();
        const payrolls = response.data.payrolls || [];
        
        if (payrolls.length === 0) {
            document.getElementById('reportContent').innerHTML = '<p class="text-dim">No payroll data available for this month.</p>';
            return;
        }

        let html = `
            <div class="table-container">
                <table class="table">
                    <thead>
                        <tr>
                            <th>Employee Name</th>
                            <th>Net Salary</th>
                        </tr>
                    </thead>
                    <tbody>
                        ${payrolls.map(p => `
                            <tr>
                                <td>${p.name}</td>
                                <td>$${p.net.toFixed(2)}</td>
                            </tr>
                        `).join('')}
                    </tbody>
                </table>
            </div>
        `;
        document.getElementById('reportContent').innerHTML = html;
        showNotification('Report generated', 'success');
    } catch (error) {
        console.error(error);
        showNotification('Error generating report', 'error');
    }
}

async function exportEmployees() {
    try {
        const response = await PayrollAPI.getEmployees();
        const data = JSON.stringify(response.data, null, 4);
        const blob = new Blob([data], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'employees_export.json';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        showNotification('Exported successfully', 'success');
    } catch (error) {
        showNotification('Export failed', 'error');
    }
}
function downloadPayslipPDF() {
    const element = document.querySelector('.payslip-wrapper');
    const nameP = Array.from(element.querySelectorAll('.payslip-header p')).find(p => p.textContent.includes('Employee Name:'));
    const empName = nameP ? nameP.textContent.split(':')[1].trim() : 'Payslip';
    const opt = {
        margin:       0.5,
        filename:     `Payslip_${empName.replace(/\s+/g, '_')}.pdf`,
        image:        { type: 'jpeg', quality: 0.98 },
        html2canvas:  { scale: 2, useCORS: true, backgroundColor: '#0b0d14' },
        jsPDF:        { unit: 'in', format: 'letter', orientation: 'portrait' }
    };

    // Show loading notification
    showNotification('Generating PDF...', 'info');
    
    html2pdf().set(opt).from(element).save().then(() => {
        showNotification('PDF Downloaded successfully!', 'success');
    }).catch(err => {
        console.error('PDF Error:', err);
        showNotification('Failed to generate PDF', 'error');
    });
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

/**
 * Adjusts font size of an element based on its text length to prevent overflow
 */
function adjustFontSize(element, baseSizeRem = 2.4, threshold = 10) {
    if (!element) return;
    const text = element.textContent || "";
    const length = text.length;
    if (length > threshold) {
        const ratio = threshold / length;
        const newSize = Math.max(baseSizeRem * ratio, 1.2); // Don't go below 1.2rem
        element.style.fontSize = `${newSize}rem`;
    } else {
        element.style.fontSize = `${baseSizeRem}rem`;
    }
}

function initializePeriodSelectors() {
    const monthSelect = document.getElementById('payrollMonth');
    const yearSelect = document.getElementById('payrollYear');
    if (!monthSelect || !yearSelect) return;

    const months = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];
    monthSelect.innerHTML = months.map((m, i) => `<option value="${i}">${m}</option>`).join('');

    const currentYear = new Date().getFullYear();
    for (let y = currentYear; y >= currentYear - 5; y--) {
        const opt = document.createElement('option');
        opt.value = y;
        opt.textContent = y;
        yearSelect.appendChild(opt);
    }

    const currentMonth = new Date().getMonth();
    monthSelect.value = currentMonth;
}

