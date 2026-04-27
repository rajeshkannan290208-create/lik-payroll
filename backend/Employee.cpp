#include "Employee.h"
#include <iomanip>

Employee::Employee(int id, const std::string& n, const std::string& dept,
                   double salary, double allow, double bonus, const std::string& pic,
                   int att, int leave, const std::string& payMethod,
                   const std::string& bName, const std::string& accNum, const std::string& doj)
    : employeeId(id), name(n), department(dept), basicSalary(salary),
      allowance(allow), bonusPercentage(bonus), picture(pic),
      attendance(att), leaveDays(leave), paymentMethod(payMethod),
      bankName(bName), accountNumber(accNum), dateOfJoin(doj) {}

void Employee::displayInfo() const {
    std::cout << "===============================================\n";
    std::cout << std::left << std::setw(20) << "Employee ID:" << employeeId << "\n";
    std::cout << std::left << std::setw(20) << "Name:" << name << "\n";
    std::cout << std::left << std::setw(20) << "Department:" << department << "\n";
    std::cout << std::left << std::setw(20) << "Basic Salary:" << "$" << basicSalary << "\n";
    std::cout << std::left << std::setw(20) << "Allowance:" << "$" << allowance << "\n";
    std::cout << std::left << std::setw(20) << "Bonus %:" << bonusPercentage << "%\n";
    std::cout << std::left << std::setw(20) << "Attendance:" << attendance << " days\n";
    std::cout << std::left << std::setw(20) << "Leave Days:" << leaveDays << " days\n";
    std::cout << std::left << std::setw(20) << "Payment Method:" << paymentMethod << "\n";
    std::cout << "===============================================\n";
}
