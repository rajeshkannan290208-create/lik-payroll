#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <string>
#include <iostream>

class Employee {
private:
    int employeeId;
    std::string name;
    std::string department;
    double basicSalary;
    double allowance;
    double bonusPercentage;
    std::string picture;

    int attendance;
    int leaveDays;
    std::string paymentMethod;
    std::string bankName;
    std::string accountNumber;
    std::string dateOfJoin;

public:
    // Constructor
    Employee(int id = 0, const std::string& n = "", const std::string& dept = "", 
             double salary = 0, double allow = 0, double bonus = 0, const std::string& pic = "",
             int att = 30, int leave = 0, const std::string& payMethod = "Bank Transfer",
             const std::string& bName = "", const std::string& accNum = "", const std::string& doj = "");

    // Getters
    int getEmployeeId() const { return employeeId; }
    std::string getName() const { return name; }
    std::string getDepartment() const { return department; }
    double getBasicSalary() const { return basicSalary; }
    double getAllowance() const { return allowance; }
    double getBonusPercentage() const { return bonusPercentage; }
    std::string getPicture() const { return picture; }
    int getAttendance() const { return attendance; }
    int getLeaveDays() const { return leaveDays; }
    std::string getPaymentMethod() const { return paymentMethod; }
    std::string getBankName() const { return bankName; }
    std::string getAccountNumber() const { return accountNumber; }
    std::string getDateOfJoin() const { return dateOfJoin; }

    // Setters
    void setEmployeeId(int id) { employeeId = id; }
    void setName(const std::string& n) { name = n; }
    void setDepartment(const std::string& dept) { department = dept; }
    void setBasicSalary(double salary) { basicSalary = salary; }
    void setAllowance(double allow) { allowance = allow; }
    void setBonusPercentage(double bonus) { bonusPercentage = bonus; }
    void setPicture(const std::string& pic) { picture = pic; }
    void setAttendance(int att) { attendance = att; }
    void setLeaveDays(int leave) { leaveDays = leave; }
    void setPaymentMethod(const std::string& payMethod) { paymentMethod = payMethod; }
    void setBankName(const std::string& bName) { bankName = bName; }
    void setAccountNumber(const std::string& accNum) { accountNumber = accNum; }
    void setDateOfJoin(const std::string& doj) { dateOfJoin = doj; }

    // Display employee info
    void displayInfo() const;
};

#endif
