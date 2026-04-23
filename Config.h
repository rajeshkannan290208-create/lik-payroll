#ifndef CONFIG_H
#define CONFIG_H

// ========== PAYROLL CONFIGURATION ==========

// Tax Configuration
const double TAX_RATE = 0.12;                    // 12% Income Tax

// Deductions
const double HEALTH_INSURANCE_DEDUCTION = 100;  // Fixed monthly deduction
const double PROVIDENT_FUND_RATE = 0.08;        // 8% of basic salary

// Other Settings
const std::string DATA_FILE = "employees.txt";  // Default data file name
const std::string LOG_FILE = "payroll_log.txt"; // Log file name

// Company Information
const std::string COMPANY_NAME = "Your Company Name";
const std::string COMPANY_ADDRESS = "123 Business Street, City, State";
const std::string COMPANY_PHONE = "+1-XXX-XXX-XXXX";

// Bonus Settings
const double MIN_BONUS = 0.0;      // Minimum bonus percentage
const double MAX_BONUS = 50.0;     // Maximum bonus percentage

// Salary Limits
const double MIN_SALARY = 500.0;   // Minimum basic salary
const double MAX_SALARY = 100000.0; // Maximum basic salary

#endif
