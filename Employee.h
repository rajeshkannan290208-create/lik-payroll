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

public:
    // Constructor
    Employee(int id = 0, const std::string& n = "", const std::string& dept = "", 
             double salary = 0, double allow = 0, double bonus = 0, const std::string& pic = "");

    // Getters
    int getEmployeeId() const { return employeeId; }
    std::string getName() const { return name; }
    std::string getDepartment() const { return department; }
    double getBasicSalary() const { return basicSalary; }
    double getAllowance() const { return allowance; }
    double getBonusPercentage() const { return bonusPercentage; }
    std::string getPicture() const { return picture; }

    // Setters
    void setEmployeeId(int id) { employeeId = id; }
    void setName(const std::string& n) { name = n; }
    void setDepartment(const std::string& dept) { department = dept; }
    void setBasicSalary(double salary) { basicSalary = salary; }
    void setAllowance(double allow) { allowance = allow; }
    void setBonusPercentage(double bonus) { bonusPercentage = bonus; }
    void setPicture(const std::string& pic) { picture = pic; }

    // Display employee info
    void displayInfo() const;
};

#endif
