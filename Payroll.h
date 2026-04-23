#ifndef PAYROLL_H
#define PAYROLL_H

#include "Employee.h"
#include <iomanip>

class Payroll {
private:
    const double TAX_RATE = 0.12;          // 12% tax
    const double HEALTH_INSURANCE = 100;   // Fixed deduction
    const double PROVIDENT_FUND_RATE = 0.08; // 8% of basic salary

public:
    // Calculate gross salary
    double calculateGrossSalary(const Employee& emp) const;

    // Calculate deductions
    double calculateTax(double grossSalary) const;
    double calculateProvidentFund(const Employee& emp) const;
    double calculateTotalDeductions(double grossSalary, const Employee& emp) const;

    // Calculate net salary
    double calculateNetSalary(const Employee& emp) const;

    // Display payroll details
    void displayPayrollSlip(const Employee& emp) const;
};

#endif
