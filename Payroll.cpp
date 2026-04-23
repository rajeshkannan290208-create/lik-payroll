#include "Payroll.h"

double Payroll::calculateGrossSalary(const Employee& emp) const {
    double bonus = emp.getBasicSalary() * (emp.getBonusPercentage() / 100.0);
    return emp.getBasicSalary() + emp.getAllowance() + bonus;
}

double Payroll::calculateTax(double grossSalary) const {
    return grossSalary * TAX_RATE;
}

double Payroll::calculateProvidentFund(const Employee& emp) const {
    return emp.getBasicSalary() * PROVIDENT_FUND_RATE;
}

double Payroll::calculateTotalDeductions(double grossSalary, const Employee& emp) const {
    double tax = calculateTax(grossSalary);
    double pf = calculateProvidentFund(emp);
    return tax + pf + HEALTH_INSURANCE;
}

double Payroll::calculateNetSalary(const Employee& emp) const {
    double grossSalary = calculateGrossSalary(emp);
    double deductions = calculateTotalDeductions(grossSalary, emp);
    return grossSalary - deductions;
}

void Payroll::displayPayrollSlip(const Employee& emp) const {
    double grossSalary = calculateGrossSalary(emp);
    double tax = calculateTax(grossSalary);
    double pf = calculateProvidentFund(emp);
    double netSalary = calculateNetSalary(emp);

    std::cout << "\n========= PAYROLL SLIP =========\n";
    std::cout << "Employee ID: " << emp.getEmployeeId() << "\n";
    std::cout << "Name: " << emp.getName() << "\n";
    std::cout << "Department: " << emp.getDepartment() << "\n";
    std::cout << "================================\n";
    std::cout << std::left << std::setw(30) << "Basic Salary:" << "$" << std::fixed << std::setprecision(2) << emp.getBasicSalary() << "\n";
    std::cout << std::left << std::setw(30) << "Allowance:" << "$" << emp.getAllowance() << "\n";
    double bonus = emp.getBasicSalary() * (emp.getBonusPercentage() / 100.0);
    std::cout << std::left << std::setw(30) << "Bonus (" << emp.getBonusPercentage() << "%):" << "$" << bonus << "\n";
    std::cout << "--------------------------------\n";
    std::cout << std::left << std::setw(30) << "Gross Salary:" << "$" << grossSalary << "\n";
    std::cout << "\nDEDUCTIONS:\n";
    std::cout << "--------------------------------\n";
    std::cout << std::left << std::setw(30) << "Tax (12%):" << "$" << tax << "\n";
    std::cout << std::left << std::setw(30) << "Provident Fund (8%):" << "$" << pf << "\n";
    std::cout << std::left << std::setw(30) << "Health Insurance:" << "$" << HEALTH_INSURANCE << "\n";
    std::cout << "--------------------------------\n";
    std::cout << std::left << std::setw(30) << "Total Deductions:" << "$" << (tax + pf + HEALTH_INSURANCE) << "\n";
    std::cout << "================================\n";
    std::cout << std::left << std::setw(30) << "Net Salary:" << "$" << netSalary << "\n";
    std::cout << "================================\n\n";
}
