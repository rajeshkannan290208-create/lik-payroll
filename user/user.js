let currentUser = null;
let currentEmployee = null;
let myAttendance = [];
let myPayroll = null;

function localDateISO() {
    const now = new Date();
    const yyyy = now.getFullYear();
    const mm = String(now.getMonth() + 1).padStart(2, '0');
    const dd = String(now.getDate()).padStart(2, '0');
    return `${yyyy}-${mm}-${dd}`;
}

function formatCurrency(value) {
    return `$${Number(value || 0).toFixed(2)}`;
}

function setText(id, value) {
    const element = document.getElementById(id);
    if (element) {
        element.textContent = value;
    }
}

document.addEventListener('DOMContentLoaded', () => {
    PayrollAPI.sessionStorageKey = 'payroll-user-session';

    setupEventListeners();
    setupAuthEventListeners();
    checkServerStatus();
    lockApp();
    renderToday();
});

function setupEventListeners() {
    document.querySelectorAll('.nav-btn').forEach(btn => {
        btn.addEventListener('click', () => switchPage(btn.getAttribute('data-page')));
    });

    const markTodayBtn = document.getElementById('markTodayBtn');
    if (markTodayBtn) markTodayBtn.addEventListener('click', markAttendanceToday);

    const markAttendanceBtn = document.getElementById('markAttendanceBtn');
    if (markAttendanceBtn) markAttendanceBtn.addEventListener('click', markAttendanceToday);

    const openPayslipBtn = document.getElementById('openPayslipBtn');
    if (openPayslipBtn) openPayslipBtn.addEventListener('click', () => switchPage('payslip'));

    const downloadQuickPayslipBtn = document.getElementById('downloadQuickPayslipBtn');
    if (downloadQuickPayslipBtn) downloadQuickPayslipBtn.addEventListener('click', downloadMyPayslipPDF);

    const downloadPayslipBtn = document.getElementById('downloadPayslipBtn');
    if (downloadPayslipBtn) downloadPayslipBtn.addEventListener('click', downloadMyPayslipPDF);

    const refreshPayslipBtn = document.getElementById('refreshPayslipBtn');
    if (refreshPayslipBtn) refreshPayslipBtn.addEventListener('click', () => loadMyPayslip(true));

    const logoutBtn = document.getElementById('logoutBtn');
    if (logoutBtn) logoutBtn.addEventListener('click', logout);
}

function switchPage(pageName) {
    document.querySelectorAll('.page').forEach(page => page.classList.remove('active'));
    document.querySelectorAll('.nav-btn').forEach(btn => btn.classList.remove('active'));

    const page = document.getElementById(pageName);
    const nav = document.querySelector(`[data-page="${pageName}"]`);
    if (page) page.classList.add('active');
    if (nav) nav.classList.add('active');

    if (pageName === 'attendance') {
        loadAttendance();
    }
    if (pageName === 'payslip') {
        loadMyPayslip();
    }
}

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

            const response = await PayrollAPI.userLogin({ username, password });
            PayrollAPI.setSession(response.data);
            unlockApp(response.data.user);
            await loadHome();
            showNotification('Welcome to your employee portal', 'success');
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
    currentEmployee = null;
    myAttendance = [];
    myPayroll = null;
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

function renderToday() {
    setText('todayLine', `Today: ${localDateISO()}`);
}

async function checkServerStatus() {
    const dot = document.getElementById('statusDot');
    const text = document.getElementById('statusText');
    try {
        await PayrollAPI.healthCheck();
        if (dot) dot.className = 'status-dot';
        if (text) text.textContent = 'Connected';
    } catch (error) {
        if (dot) dot.className = 'status-dot disconnected';
        if (text) text.textContent = 'Server Offline';
    }
}

async function loadHome() {
    await Promise.allSettled([loadProfile(), loadAttendance(), loadMyPayslip()]);
    switchPage('home');
}

async function loadProfile() {
    try {
        const response = await PayrollAPI.getEmployeeMe();
        currentEmployee = response.data;
        renderProfile(currentEmployee);
    } catch (error) {
        currentEmployee = null;
        const fallbackMessage = currentUser?.employeeId
            ? `Employee ID ${currentUser.employeeId} is not linked to a profile yet. Ask admin to add that employee.`
            : 'Failed to load profile';
        showNotification(error.message || fallbackMessage, 'error');
    }
}

function renderProfile(employee) {
    const photo = document.getElementById('profilePhoto');
    if (photo) photo.src = employee.picture || 'https://via.placeholder.com/90';

    setText('profileName', employee.name || '-');
    setText('profileDept', employee.department || '-');
    setText('profileIdLine', `Employee ID: ${employee.id ?? '-'}`);
    setText('profileUserId', currentUser?.id ?? '-');
    setText('profileUsername', currentUser?.username ?? '-');
    setText('profileDoj', employee.dateOfJoin || '-');
    setText('profileAttendance', `${employee.attendance ?? 0} days this month`);
    setText('profileLeave', String(employee.leaveDays ?? 0));
    setText('profilePayment', employee.paymentMethod || '-');
}

async function loadAttendance() {
    try {
        const response = await PayrollAPI.getAttendanceMe();
        myAttendance = response.data || [];
    } catch (error) {
        myAttendance = [];
    }

    renderAttendanceTable(myAttendance);
    renderTodayStatus(myAttendance);
}

function renderAttendanceTable(records) {
    const tbody = document.getElementById('attendanceTable');
    if (!tbody) return;

    if (!records || records.length === 0) {
        tbody.innerHTML = '<tr><td colspan="2" style="color:var(--text-dim)">No attendance marked yet.</td></tr>';
        return;
    }

    tbody.innerHTML = records.map(record => `
        <tr>
            <td>${record.date || '-'}</td>
            <td>Present</td>
        </tr>
    `).join('');
}

function renderTodayStatus(records) {
    const today = localDateISO();
    const marked = (records || []).some(record => record && record.date === today);

    setText('todayStatus', marked ? 'Attendance marked for today.' : 'Attendance not marked yet.');

    const markTodayBtn = document.getElementById('markTodayBtn');
    if (markTodayBtn) markTodayBtn.disabled = marked;

    const markAttendanceBtn = document.getElementById('markAttendanceBtn');
    if (markAttendanceBtn) markAttendanceBtn.disabled = marked;
}

async function markAttendanceToday() {
    const markTodayBtn = document.getElementById('markTodayBtn');
    const markAttendanceBtn = document.getElementById('markAttendanceBtn');

    try {
        if (markTodayBtn) markTodayBtn.disabled = true;
        if (markAttendanceBtn) markAttendanceBtn.disabled = true;

        await PayrollAPI.markAttendance();
        showNotification('Attendance marked successfully', 'success');
        await Promise.allSettled([loadProfile(), loadAttendance(), loadMyPayslip()]);
    } catch (error) {
        showNotification(error.message || 'Failed to mark attendance', 'error');
        renderTodayStatus(myAttendance);
    }
}

async function loadMyPayslip(showError = false) {
    try {
        const response = await PayrollAPI.getPayrollMe();
        myPayroll = response.data;
        renderPayrollSummary(myPayroll);
        renderPayslipPage(myPayroll);
        togglePayslipButtons(true);
    } catch (error) {
        myPayroll = null;
        renderPayrollSummary(null, error.message);
        renderPayslipPage(null, error.message);
        togglePayslipButtons(false);
        if (showError) {
            showNotification(error.message || 'Unable to load your payslip', 'error');
        }
    }
}

function renderPayrollSummary(payroll, errorMessage = '') {
    setText('homeNetSalary', payroll ? formatCurrency(payroll.netSalary) : '$0.00');

    if (payroll) {
        setText('homePayrollNote', `Payslip fetched securely for employee ID ${payroll.employee.id}.`);
        setText('payrollSecurityLine', `Only the login linked to employee ID ${payroll.employee.id} can open this payslip.`);
        return;
    }

    setText('homePayrollNote', errorMessage || 'Your payslip will appear here once admin creates your employee payroll.');
    setText('payrollSecurityLine', 'Payslip access is locked to your linked employee ID.');
}

function buildPayslipMarkup(payroll) {
    const healthInsurance = payroll?.deductions?.healthInsurance ?? 100;
    const bankDetails = payroll.employee.paymentMethod === 'Bank Transfer'
        ? `
            <p><strong>Bank:</strong> ${payroll.employee.bankName || '-'}</p>
            <p><strong>Account:</strong> ${payroll.employee.accountNumber || '-'}</p>
        `
        : '';

    return `
        <div id="employeePayslipPanel" class="payslip-wrapper payslip-inline">
            <div class="payslip-title">Monthly Payslip</div>

            <div class="payslip-header">
                <p><strong>Employee Name:</strong> ${payroll.employee.name}</p>
                <p><strong>Employee ID:</strong> ${payroll.employee.id}</p>
                <p><strong>Department:</strong> ${payroll.employee.department}</p>
                <p><strong>Date of Join:</strong> ${payroll.employee.dateOfJoin || '-'}</p>
                <p><strong>Attendance:</strong> ${payroll.employee.attendance} Days</p>
                <p><strong>Leaves:</strong> ${payroll.employee.leaveDays} Days</p>
            </div>

            <div class="payslip-section earnings">
                <h3>Earnings</h3>
                <div class="payslip-row">
                    <span>Basic Salary (Fixed)</span>
                    <span>${formatCurrency(payroll.salary.basic)}</span>
                </div>
                <div class="payslip-row">
                    <span>Salary Per Day</span>
                    <span>${formatCurrency(payroll.salary.salaryPerDay)}</span>
                </div>
                <div class="payslip-row highlight">
                    <span>Earned Basic (${payroll.employee.attendance}/30 days)</span>
                    <span>${formatCurrency(payroll.salary.earnedBasic)}</span>
                </div>
                <div class="payslip-row">
                    <span>Allowance</span>
                    <span>${formatCurrency(payroll.salary.allowance)}</span>
                </div>
                <div class="payslip-row">
                    <span>Bonus</span>
                    <span>${formatCurrency(payroll.salary.bonus)}</span>
                </div>
                <div class="payslip-total">
                    <span>Gross Salary</span>
                    <span>${formatCurrency(payroll.salary.gross)}</span>
                </div>
            </div>

            <div class="payslip-section deductions">
                <h3>Deductions</h3>
                <div class="payslip-row">
                    <span>Income Tax (12%)</span>
                    <span>${formatCurrency(payroll.deductions.tax)}</span>
                </div>
                <div class="payslip-row">
                    <span>Provident Fund (8%)</span>
                    <span>${formatCurrency(payroll.deductions.providentFund)}</span>
                </div>
                <div class="payslip-row">
                    <span>Health Insurance</span>
                    <span>${formatCurrency(healthInsurance)}</span>
                </div>
                <div class="payslip-total">
                    <span>Total Deductions</span>
                    <span>${formatCurrency(payroll.deductions.total)}</span>
                </div>
            </div>

            <div class="payslip-net">
                <h3>Net Salary</h3>
                <div class="net-amount">${formatCurrency(payroll.netSalary)}</div>
            </div>

            <div class="payslip-footer">
                <p><strong>Payment Method:</strong> ${payroll.employee.paymentMethod}</p>
                ${bankDetails}
            </div>

            <div class="payslip-signature">
                <div class="signature-line"></div>
                <p>Authorized Signature</p>
                <strong>LIK</strong>
            </div>
        </div>
    `;
}

function renderPayslipPage(payroll, errorMessage = '') {
    const container = document.getElementById('employeePayslipContainer');
    if (!container) return;

    if (!payroll) {
        container.innerHTML = `
            <div class="empty-state-card">
                <strong>No payslip available right now.</strong>
                <p style="margin-top:0.7rem">${errorMessage || 'Ask admin to create or update the payroll linked to your employee ID, then refresh this page.'}</p>
            </div>
        `;
        return;
    }

    container.innerHTML = buildPayslipMarkup(payroll);
}

function togglePayslipButtons(enabled) {
    ['downloadQuickPayslipBtn', 'downloadPayslipBtn'].forEach(id => {
        const button = document.getElementById(id);
        if (button) {
            button.disabled = !enabled;
        }
    });
}

function downloadMyPayslipPDF() {
    if (!myPayroll) {
        showNotification('No payslip available yet', 'error');
        return;
    }

    const panel = document.getElementById('employeePayslipPanel');
    if (!panel) {
        showNotification('Open the payslip page first', 'error');
        return;
    }

    if (typeof html2pdf === 'undefined') {
        window.print();
        return;
    }

    const safeName = String(myPayroll.employee.name || 'Employee').replace(/\s+/g, '_');
    const filename = `LIK_Payslip_${safeName}_${myPayroll.employee.id}.pdf`;

    showNotification('Generating payslip PDF...', 'info');

    html2pdf().set({
        margin: 0.45,
        filename,
        image: { type: 'jpeg', quality: 0.98 },
        html2canvas: { scale: 2, useCORS: true, backgroundColor: '#050b16' },
        jsPDF: { unit: 'in', format: 'letter', orientation: 'portrait' }
    }).from(panel).save().then(() => {
        showNotification('Payslip downloaded successfully', 'success');
    }).catch(error => {
        console.error('PDF Error:', error);
        showNotification('Failed to generate payslip PDF', 'error');
    });
}

async function logout() {
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

function showNotification(message, type = 'info') {
    const note = document.getElementById('notification');
    if (!note) return;

    note.textContent = message;
    note.className = 'notification ' + type;
    note.classList.remove('hidden');
    setTimeout(() => note.classList.add('hidden'), 3000);
}
