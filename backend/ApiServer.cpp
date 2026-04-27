#include "ApiServer.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
#ifdef _WIN32
typedef SOCKET SocketType;
const SocketType INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
typedef int SocketType;
const SocketType INVALID_SOCKET_VALUE = -1;
#endif

void closeSocket(SocketType socketHandle) {
#ifdef _WIN32
    closesocket(socketHandle);
#else
    close(socketHandle);
#endif
}

int socketErrorValue() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

void setClientSocketTimeouts(SocketType socketHandle, int timeoutMs) {
#ifdef _WIN32
    setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
    setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
#else
    timeval timeoutValue;
    timeoutValue.tv_sec = timeoutMs / 1000;
    timeoutValue.tv_usec = (timeoutMs % 1000) * 1000;
    setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, &timeoutValue, sizeof(timeoutValue));
    setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO, &timeoutValue, sizeof(timeoutValue));
#endif
}

bool sendAll(SocketType socketHandle, const std::string& response) {
    std::size_t totalSent = 0;

    while (totalSent < response.size()) {
        std::size_t remaining = response.size() - totalSent;
        int chunkSize = static_cast<int>(std::min<std::size_t>(remaining, 8192));
        int sent = send(socketHandle, response.c_str() + totalSent, chunkSize, 0);

        if (sent <= 0) {
            return false;
        }

        totalSent += static_cast<std::size_t>(sent);
    }

    return true;
}

Employee makeSeedEmployee(int employeeId) {
    switch (employeeId) {
        case 101:
            return Employee(101, "John Smith", "Engineering", 5000, 500, 10);
        case 102:
            return Employee(102, "Sarah Johnson", "Marketing", 4500, 450, 8);
        case 103:
            return Employee(103, "Mike Williams", "HR", 4000, 400, 7);
        case 104:
            return Employee(104, "Emily Brown", "Finance", 5500, 550, 12);
        default:
            return Employee();
    }
}

bool isSeedEmployeeId(int employeeId) {
    return employeeId >= 101 && employeeId <= 104;
}
}

ApiServer::ApiServer(int serverPort) : port(serverPort), nextUserId(1) {
    std::srand(static_cast<unsigned int>(std::time(NULL)));
    
    if (!loadEmployeesFromJsonFile("employees.json")) {
        seedEmployees();
        saveEmployeesToJsonFile("employees.json");
    }
    
    loadUsersFromFile();

    bool repairedEmployees = false;
    for (std::size_t i = 0; i < users.size(); ++i) {
        if (users[i].role != "user" || users[i].employeeId <= 0) continue;
        if (findEmployeeIndex(users[i].employeeId) != -1) continue;
        if (!isSeedEmployeeId(users[i].employeeId)) continue;

        employees.push_back(makeSeedEmployee(users[i].employeeId));
        repairedEmployees = true;
    }

    if (repairedEmployees) {
        std::sort(employees.begin(), employees.end(), [](const Employee& a, const Employee& b) {
            return a.getEmployeeId() < b.getEmployeeId();
        });
        saveEmployeesToJsonFile("employees.json");
    }

    loadAttendanceFromFile();
}

void ApiServer::seedEmployees() {
    employees.clear();
    employees.push_back(Employee(101, "John Smith", "Engineering", 5000, 500, 10));
    employees.push_back(Employee(102, "Sarah Johnson", "Marketing", 4500, 450, 8));
    employees.push_back(Employee(103, "Mike Williams", "HR", 4000, 400, 7));
    employees.push_back(Employee(104, "Emily Brown", "Finance", 5500, 550, 12));
}

bool ApiServer::start() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock.\n";
        return false;
    }
#endif

    SocketType serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET_VALUE) {
        std::cerr << "Failed to create socket. Error: " << socketErrorValue() << "\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    int reuse = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(static_cast<unsigned short>(port));

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind to port " << port << ". Error: " << socketErrorValue() << "\n";
        closeSocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    if (listen(serverSocket, 16) < 0) {
        std::cerr << "Failed to listen on port " << port << ". Error: " << socketErrorValue() << "\n";
        closeSocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    std::cout << "\n========================================\n";
    std::cout << "Payroll Management System C++ API Server\n";
    std::cout << "========================================\n";
    std::cout << "Server running on http://localhost:" << port << "\n";
    std::cout << "Press Ctrl+C to stop the server.\n";
    std::cout << "========================================\n\n";

    while (true) {
        sockaddr_in clientAddress;
#ifdef _WIN32
        int clientLength = sizeof(clientAddress);
#else
        socklen_t clientLength = sizeof(clientAddress);
#endif
        SocketType clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);

        if (clientSocket == INVALID_SOCKET_VALUE) {
            continue;
        }

        setClientSocketTimeouts(clientSocket, 250);
        handleClient(static_cast<std::intptr_t>(clientSocket));
    }

    closeSocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return true;
}

void ApiServer::handleClient(std::intptr_t clientSocketValue) {
    SocketType clientSocket = static_cast<SocketType>(clientSocketValue);
    std::string rawRequest;
    char buffer[4096];
    int contentLength = 0;
    std::size_t headerEnd = std::string::npos;
    bool extendedTimeout = false;

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            closeSocket(clientSocket);
            return;
        }

        rawRequest.append(buffer, bytesReceived);
        if (!extendedTimeout) {
            setClientSocketTimeouts(clientSocket, 1000);
            extendedTimeout = true;
        }
        headerEnd = rawRequest.find("\r\n\r\n");

        if (headerEnd != std::string::npos) {
            HttpRequest partialRequest = parseRequest(rawRequest.substr(0, headerEnd + 4));
            std::map<std::string, std::string>::const_iterator lengthIt = partialRequest.headers.find("content-length");
            if (lengthIt != partialRequest.headers.end()) {
                contentLength = std::atoi(lengthIt->second.c_str());
            }

            std::size_t bodyStart = headerEnd + 4;
            if (rawRequest.size() >= bodyStart + static_cast<std::size_t>(contentLength)) {
                break;
            }
        }
    }

    HttpRequest request = parseRequest(rawRequest);
    std::string response = handleRequest(request);
    sendAll(clientSocket, response);
    closeSocket(clientSocket);
}

ApiServer::HttpRequest ApiServer::parseRequest(const std::string& rawRequest) const {
    HttpRequest request;
    std::size_t headerEnd = rawRequest.find("\r\n\r\n");
    std::string headersPart = headerEnd == std::string::npos ? rawRequest : rawRequest.substr(0, headerEnd);
    request.body = headerEnd == std::string::npos ? "" : rawRequest.substr(headerEnd + 4);

    std::istringstream stream(headersPart);
    std::string line;
    if (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }

        std::istringstream requestLine(line);
        std::string target;
        requestLine >> request.method >> target;

        std::size_t queryPosition = target.find('?');
        if (queryPosition == std::string::npos) {
            request.path = target.empty() ? "/" : target;
        } else {
            request.path = target.substr(0, queryPosition);
            request.query = target.substr(queryPosition + 1);
        }
    }

    while (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }

        std::size_t colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }

        std::string key = toLower(trim(line.substr(0, colon)));
        std::string value = trim(line.substr(colon + 1));
        request.headers[key] = value;
    }

    return request;
}

std::string ApiServer::handleRequest(const HttpRequest& request) {
    if (request.method == "OPTIONS") {
        return buildNoContentResponse();
    }

    if (startsWith(request.path, "/api")) {
        return handleApiRequest(request);
    }

    return handleStaticRequest(request);
}

std::string ApiServer::handleApiRequest(const HttpRequest& request) {
    if (request.path == "/api/health" && request.method == "GET") {
        std::size_t adminCount = 0;
        std::size_t userCount = 0;
        for (std::size_t i = 0; i < users.size(); ++i) {
            if (users[i].role == "admin") adminCount++;
            if (users[i].role == "user") userCount++;
        }

        return buildJsonResponse(200,
                                 "{\"success\":true,\"message\":\"C++ API is running\","
                                 "\"userCount\":" + numberJson(users.size()) + ","
                                 "\"adminCount\":" + numberJson(adminCount) + ","
                                 "\"employeeUserCount\":" + numberJson(userCount) +
                                 "}");
    }

    if (startsWith(request.path, "/api/auth/")) {
        return handleAuthRequest(request);
    }

    int authUserIndex = authenticatedUserIndex(request);
    if (authUserIndex == -1) {
        return buildJsonResponse(401, "{\"success\":false,\"message\":\"Login required\"}");
    }

    const User& authUser = users[authUserIndex];
    const bool isAdmin = authUser.role == "admin";
    const bool isEmployeeUser = authUser.role == "user";

    if (request.path == "/api/employees/me" && request.method == "GET") {
        if (!isEmployeeUser) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"User access required\"}");
        }
        if (authUser.employeeId <= 0) {
            return buildJsonResponse(400, "{\"success\":false,\"message\":\"User is not linked to an employee\"}");
        }
        int index = findEmployeeIndex(authUser.employeeId);
        if (index == -1) {
            return buildJsonResponse(404, "{\"success\":false,\"message\":\"Employee not found\"}");
        }
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + employeeJson(employees[index]) + "}");
    }

    if (request.path == "/api/payroll/me" && request.method == "GET") {
        if (!isEmployeeUser) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"User access required\"}");
        }
        if (authUser.employeeId <= 0) {
            return buildJsonResponse(400, "{\"success\":false,\"message\":\"User is not linked to an employee\"}");
        }
        int index = findEmployeeIndex(authUser.employeeId);
        if (index == -1) {
            return buildJsonResponse(404, "{\"success\":false,\"message\":\"Employee not found\"}");
        }
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + payrollJson(employees[index]) + "}");
    }

    if (request.path == "/api/attendance/mark" && request.method == "POST") {
        if (!isEmployeeUser) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"User access required\"}");
        }
        if (authUser.employeeId <= 0) {
            return buildJsonResponse(400, "{\"success\":false,\"message\":\"User is not linked to an employee\"}");
        }

        int empIndex = findEmployeeIndex(authUser.employeeId);
        if (empIndex == -1) {
            return buildJsonResponse(404, "{\"success\":false,\"message\":\"Employee not found\"}");
        }

        std::string today = currentDateISO();
        for (std::size_t i = 0; i < attendanceRecords.size(); ++i) {
            if (attendanceRecords[i].employeeId == authUser.employeeId &&
                attendanceRecords[i].date == today) {
                return buildJsonResponse(409, "{\"success\":false,\"message\":\"Attendance already marked for today\"}");
            }
        }

        AttendanceRecord record;
        record.employeeId = authUser.employeeId;
        record.date = today;
        record.timestamp = currentTimeMillis();
        attendanceRecords.push_back(record);
        saveAttendanceToFile();

        std::string monthPrefix = currentMonthISO();
        int monthCount = 0;
        for (std::size_t i = 0; i < attendanceRecords.size(); ++i) {
            const AttendanceRecord& r = attendanceRecords[i];
            if (r.employeeId != authUser.employeeId) continue;
            if (r.date.size() >= 7 && r.date.substr(0, 7) == monthPrefix) monthCount++;
        }
        employees[empIndex].setAttendance(monthCount);
        saveEmployeesToJsonFile("employees.json");

        std::ostringstream data;
        data << "{";
        data << "\"employeeId\":" << authUser.employeeId << ",";
        data << "\"date\":\"" << today << "\",";
        data << "\"attendanceThisMonth\":" << monthCount;
        data << "}";

        return buildJsonResponse(201, "{\"success\":true,\"data\":" + data.str() + ",\"message\":\"Attendance marked\"}");
    }

    if (request.path == "/api/attendance/me" && request.method == "GET") {
        if (!isEmployeeUser) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"User access required\"}");
        }
        if (authUser.employeeId <= 0) {
            return buildJsonResponse(400, "{\"success\":false,\"message\":\"User is not linked to an employee\"}");
        }

        std::vector<AttendanceRecord> filtered;
        for (std::size_t i = 0; i < attendanceRecords.size(); ++i) {
            if (attendanceRecords[i].employeeId == authUser.employeeId) {
                filtered.push_back(attendanceRecords[i]);
            }
        }
        std::sort(filtered.begin(), filtered.end(), [](const AttendanceRecord& a, const AttendanceRecord& b) {
            if (a.date != b.date) return a.date > b.date;
            return a.timestamp > b.timestamp;
        });

        std::ostringstream json;
        json << "[";
        for (std::size_t i = 0; i < filtered.size(); ++i) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"employeeId\":" << filtered[i].employeeId << ",";
            json << "\"date\":\"" << jsonEscape(filtered[i].date) << "\",";
            json << "\"timestamp\":" << numberJson(filtered[i].timestamp);
            json << "}";
        }
        json << "]";

        return buildJsonResponse(200, "{\"success\":true,\"data\":" + json.str() + ",\"count\":" + numberJson(filtered.size()) + "}");
    }

    if (request.path == "/api/attendance" && request.method == "GET") {
        if (!isAdmin) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin access required\"}");
        }

        std::vector<AttendanceRecord> records = attendanceRecords;
        std::sort(records.begin(), records.end(), [](const AttendanceRecord& a, const AttendanceRecord& b) {
            if (a.date != b.date) return a.date > b.date;
            return a.timestamp > b.timestamp;
        });

        std::ostringstream json;
        json << "[";
        for (std::size_t i = 0; i < records.size(); ++i) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"employeeId\":" << records[i].employeeId << ",";
            json << "\"date\":\"" << jsonEscape(records[i].date) << "\",";
            json << "\"timestamp\":" << numberJson(records[i].timestamp);
            json << "}";
        }
        json << "]";

        return buildJsonResponse(200, "{\"success\":true,\"data\":" + json.str() + ",\"count\":" + numberJson(records.size()) + "}");
    }

    if (request.path == "/api/admin/users" && request.method == "GET") {
        if (!isAdmin) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin access required\"}");
        }

        std::ostringstream json;
        json << "[";
        std::size_t count = 0;
        for (std::size_t i = 0; i < users.size(); ++i) {
            if (users[i].role != "user") continue;
            if (count > 0) json << ",";
            json << userJson(users[i]);
            count++;
        }
        json << "]";

        return buildJsonResponse(200, "{\"success\":true,\"data\":" + json.str() + ",\"count\":" + numberJson(count) + "}");
    }

    if (request.path == "/api/admin/users" && request.method == "POST") {
        if (!isAdmin) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin access required\"}");
        }

        std::string username, password;
        double employeeIdNumber = 0;
        extractJsonString(request.body, "username", username);
        extractJsonString(request.body, "password", password);
        extractJsonNumber(request.body, "employeeId", employeeIdNumber);

        username = normalizeUsername(username);
        int employeeId = static_cast<int>(employeeIdNumber);
        if (username.empty() || password.empty() || employeeId <= 0) {
            return buildJsonResponse(400, "{\"success\":false,\"message\":\"username, password and employeeId are required\"}");
        }

        if (findUserIndexByUsername(username) != -1) {
            return buildJsonResponse(409, "{\"success\":false,\"message\":\"Username already exists\"}");
        }

        int empIndex = findEmployeeIndex(employeeId);
        if (empIndex == -1) {
            return buildJsonResponse(404, "{\"success\":false,\"message\":\"Employee not found\"}");
        }

        for (std::size_t i = 0; i < users.size(); ++i) {
            if (users[i].role == "user" && users[i].employeeId == employeeId) {
                return buildJsonResponse(409, "{\"success\":false,\"message\":\"Employee already has a login\"}");
            }
        }

        User user;
        user.id = nextUserId++;
        user.username = username;
        user.passwordHash = passwordDigest(username, password);
        user.role = "user";
        user.employeeId = employeeId;
        users.push_back(user);
        saveUsersToFile();

        return buildJsonResponse(201, "{\"success\":true,\"data\":" + userJson(user) + ",\"message\":\"User created\"}");
    }

    if (request.path == "/api/employees" && request.method == "GET") {
        if (!isAdmin) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin access required\"}");
        }
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + employeesJson() + ",\"count\":" + numberJson(employees.size()) + "}");
    }

    if (request.path == "/api/employees" && request.method == "POST") {
        if (!isAdmin) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin access required\"}");
        }
        std::string name, dept, pic, payMethod, bName, accNum, doj;
        double salary = 0, allowance = 0, bonus = 0, attendance = 30, leaveDays = 0;

        extractJsonString(request.body, "name", name);
        extractJsonString(request.body, "department", dept);
        extractJsonString(request.body, "picture", pic);
        extractJsonString(request.body, "paymentMethod", payMethod);
        if (payMethod.empty()) payMethod = "Bank Transfer";
        extractJsonString(request.body, "bankName", bName);
        extractJsonString(request.body, "accountNumber", accNum);
        extractJsonString(request.body, "dateOfJoin", doj);
        
        extractJsonNumber(request.body, "basicSalary", salary);
        extractJsonNumber(request.body, "allowance", allowance);
        extractJsonNumber(request.body, "bonusPercentage", bonus);
        extractJsonNumber(request.body, "attendance", attendance);
        extractJsonNumber(request.body, "leaveDays", leaveDays);

        Employee emp(nextEmployeeId(), name, dept, salary, allowance, bonus, pic, 
                     static_cast<int>(attendance), static_cast<int>(leaveDays), payMethod, bName, accNum, doj);
        employees.push_back(emp);
        saveEmployeesToJsonFile("employees.json");

        return buildJsonResponse(201, "{\"success\":true,\"data\":" + employeeJson(emp) + ",\"message\":\"Employee added\"}");
    }

    if (startsWith(request.path, "/api/employees/")) {
        if (!isAdmin) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin access required\"}");
        }
        int id = std::atoi(request.path.substr(15).c_str());
        int index = findEmployeeIndex(id);

        if (index == -1) {
            return buildJsonResponse(404, "{\"success\":false,\"message\":\"Not found\"}");
        }

        if (request.method == "GET") {
            return buildJsonResponse(200, "{\"success\":true,\"data\":" + employeeJson(employees[index]) + "}");
        }

        if (request.method == "PUT") {
            std::string name = employees[index].getName();
            std::string dept = employees[index].getDepartment();
            std::string pic = employees[index].getPicture();
            std::string payMethod = employees[index].getPaymentMethod();
            std::string bName = employees[index].getBankName();
            std::string accNum = employees[index].getAccountNumber();
            std::string doj = employees[index].getDateOfJoin();
            
            double salary = employees[index].getBasicSalary();
            double allowance = employees[index].getAllowance();
            double bonus = employees[index].getBonusPercentage();
            double attendance = employees[index].getAttendance();
            double leaveDays = employees[index].getLeaveDays();

            extractJsonString(request.body, "name", name);
            extractJsonString(request.body, "department", dept);
            extractJsonString(request.body, "picture", pic);
            extractJsonString(request.body, "paymentMethod", payMethod);
            extractJsonString(request.body, "bankName", bName);
            extractJsonString(request.body, "accountNumber", accNum);
            extractJsonString(request.body, "dateOfJoin", doj);
            
            extractJsonNumber(request.body, "basicSalary", salary);
            extractJsonNumber(request.body, "allowance", allowance);
            extractJsonNumber(request.body, "bonusPercentage", bonus);
            extractJsonNumber(request.body, "attendance", attendance);
            extractJsonNumber(request.body, "leaveDays", leaveDays);

            employees[index] = Employee(id, name, dept, salary, allowance, bonus, pic,
                                       static_cast<int>(attendance), static_cast<int>(leaveDays),
                                       payMethod, bName, accNum, doj);
            saveEmployeesToJsonFile("employees.json");

            return buildJsonResponse(200, "{\"success\":true,\"data\":" + employeeJson(employees[index]) + "}");
        }

        if (request.method == "DELETE") {
            employees.erase(employees.begin() + index);
            saveEmployeesToJsonFile("employees.json");
            return buildJsonResponse(200, "{\"success\":true,\"message\":\"Deleted\"}");
        }
    }

    if (request.path == "/api/payroll" && request.method == "GET") {
        if (!isAdmin) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin access required\"}");
        }
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + allPayrollJson() + "}");
    }

    if (startsWith(request.path, "/api/payroll/")) {
        if (!isAdmin) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin access required\"}");
        }
        int id = std::atoi(request.path.substr(13).c_str());
        int index = findEmployeeIndex(id);
        if (index == -1) return buildJsonResponse(404, "{\"success\":false,\"message\":\"Not found\"}");
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + payrollJson(employees[index]) + "}");
    }

    if (request.path == "/api/report/monthly" && request.method == "GET") {
        if (!isAdmin) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin access required\"}");
        }
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + monthlyReportJson() + "}");
    }

    if (request.path == "/api/statistics" && request.method == "GET") {
        if (!isAdmin) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin access required\"}");
        }
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + statisticsJson() + "}");
    }

    return buildJsonResponse(404, "{\"success\":false,\"message\":\"Endpoint not found\"}");
}

std::string ApiServer::handleAuthRequest(const HttpRequest& request) {
    if (request.path == "/api/auth/register" && request.method == "POST") {
        bool adminExists = false;
        for (std::size_t i = 0; i < users.size(); ++i) {
            if (users[i].role == "admin") {
                adminExists = true;
                break;
            }
        }
        if (adminExists) {
            return buildJsonResponse(403, "{\"success\":false,\"message\":\"Admin already exists. Ask admin to create employee logins.\"}");
        }

        std::string username, password;
        extractJsonString(request.body, "username", username);
        extractJsonString(request.body, "password", password);

        username = normalizeUsername(username);
        if (findUserIndexByUsername(username) != -1) {
            return buildJsonResponse(409, "{\"success\":false,\"message\":\"Already registered\"}");
        }

        User user;
        user.id = nextUserId++;
        user.username = username;
        user.passwordHash = passwordDigest(username, password);
        user.role = "admin";
        user.employeeId = -1;
        users.push_back(user);
        saveUsersToFile();

        return buildJsonResponse(201, "{\"success\":true,\"message\":\"Registered\"}");
    }

    if ((request.path == "/api/auth/login" ||
         request.path == "/api/auth/admin/login" ||
         request.path == "/api/auth/user/login") && request.method == "POST") {
        std::string username, password;
        extractJsonString(request.body, "username", username);
        extractJsonString(request.body, "password", password);

        username = normalizeUsername(username);
        int index = findUserIndexByUsername(username);
        std::string expectedPasswordHash = passwordDigest(username, password);
        bool passwordMatches = index != -1 &&
                               (users[index].passwordHash == expectedPasswordHash ||
                                users[index].passwordHash == password);

        if (!passwordMatches) {
            return buildJsonResponse(401, "{\"success\":false,\"message\":\"Invalid credentials\"}");
        }

        if (users[index].passwordHash != expectedPasswordHash) {
            users[index].passwordHash = expectedPasswordHash;
            saveUsersToFile();
        }

        if (request.path == "/api/auth/admin/login" && users[index].role != "admin") {
            return buildJsonResponse(401, "{\"success\":false,\"message\":\"Admin credentials required\"}");
        }
        if (request.path == "/api/auth/user/login" && users[index].role != "user") {
            return buildJsonResponse(401, "{\"success\":false,\"message\":\"User credentials required\"}");
        }
        if (request.path == "/api/auth/user/login" && users[index].employeeId <= 0) {
            return buildJsonResponse(401, "{\"success\":false,\"message\":\"User account is not linked to an employee\"}");
        }

        Session session;
        session.token = makeSessionToken();
        session.userId = users[index].id;
        session.expiresAt = currentTimeMillis() + 3600000;
        sessions.push_back(session);

        return buildJsonResponse(200,
                                 "{\"success\":true,\"data\":{"
                                 "\"token\":\"" + session.token + "\","
                                 "\"expiresAt\":" + numberJson(session.expiresAt) + ","
                                 "\"user\":" + userJson(users[index]) +
                                 "},\"message\":\"Login successful\"}");
    }

    if (request.path == "/api/auth/me" && request.method == "GET") {
        std::string token = bearerToken(request);
        int sIdx = findSessionIndex(token);
        if (sIdx == -1 || sessions[sIdx].expiresAt <= currentTimeMillis()) {
            return buildJsonResponse(401, "{\"success\":false,\"message\":\"Expired\"}");
        }

        int uIdx = findUserIndexById(sessions[sIdx].userId);
        if (uIdx == -1) return buildJsonResponse(401, "{\"success\":false,\"message\":\"User not found\"}");
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + userJson(users[uIdx]) + "}");
    }

    if (request.path == "/api/auth/logout" && request.method == "POST") {
        std::string token = bearerToken(request);
        int sIdx = findSessionIndex(token);
        if (sIdx != -1) sessions.erase(sessions.begin() + sIdx);
        return buildJsonResponse(200, "{\"success\":true,\"message\":\"Logged out\"}");
    }

    return buildJsonResponse(404, "{\"success\":false,\"message\":\"Not found\"}");
}

std::string ApiServer::handleStaticRequest(const HttpRequest& request) const {
    std::string path = request.path;
    if (path == "/") {
        path = "/index.html";
    } else if (!path.empty() && path[path.size() - 1] == '/') {
        path += "index.html";
    } else {
        std::size_t lastSlash = path.find_last_of('/');
        std::size_t lastDot = path.find_last_of('.');
        if (lastDot == std::string::npos || (lastSlash != std::string::npos && lastDot < lastSlash)) {
            path += "/index.html";
        }
    }
    if (path.find("..") != std::string::npos) return buildResponse(403, "text/plain", "Forbidden");
    if (!path.empty() && path[0] == '/') path.erase(0, 1);

    std::string filePath = "../frontend/" + path;
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) return buildResponse(404, "text/plain", "Not found");

    std::ostringstream body;
    body << file.rdbuf();
    return buildResponse(200, getMimeType(filePath), body.str());
}

std::string ApiServer::buildResponse(int statusCode, const std::string& contentType, const std::string& body) const {
    std::string reasonPhrase = "OK";
    if (statusCode == 201) reasonPhrase = "Created";
    else if (statusCode == 204) reasonPhrase = "No Content";
    else if (statusCode == 400) reasonPhrase = "Bad Request";
    else if (statusCode == 401) reasonPhrase = "Unauthorized";
    else if (statusCode == 403) reasonPhrase = "Forbidden";
    else if (statusCode == 404) reasonPhrase = "Not Found";
    else if (statusCode == 409) reasonPhrase = "Conflict";

    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << reasonPhrase << "\r\n";
    if (statusCode != 204) {
        response << "Content-Type: " << contentType << "\r\n";
        response << "Content-Length: " << body.size() << "\r\n";
    }
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    if (statusCode != 204) {
        response << body;
    }
    return response.str();
}

std::string ApiServer::buildJsonResponse(int statusCode, const std::string& body) const {
    return buildResponse(statusCode, "application/json; charset=utf-8", body);
}

std::string ApiServer::buildNoContentResponse() const {
    return buildResponse(204, "", "");
}

std::string ApiServer::employeesJson() const {
    std::ostringstream json;
    json << "[";
    for (std::size_t i = 0; i < employees.size(); ++i) {
        if (i > 0) json << ",";
        json << employeeJson(employees[i]);
    }
    json << "]";
    return json.str();
}

std::string ApiServer::employeeJson(const Employee& employee) const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":" << employee.getEmployeeId() << ",";
    json << "\"name\":\"" << jsonEscape(employee.getName()) << "\",";
    json << "\"department\":\"" << jsonEscape(employee.getDepartment()) << "\",";
    json << "\"basicSalary\":" << numberJson(employee.getBasicSalary()) << ",";
    json << "\"allowance\":" << numberJson(employee.getAllowance()) << ",";
    json << "\"bonusPercentage\":" << numberJson(employee.getBonusPercentage()) << ",";
    json << "\"picture\":\"" << jsonEscape(employee.getPicture()) << "\",";
    json << "\"attendance\":" << employee.getAttendance() << ",";
    json << "\"leaveDays\":" << employee.getLeaveDays() << ",";
    json << "\"paymentMethod\":\"" << jsonEscape(employee.getPaymentMethod()) << "\",";
    json << "\"bankName\":\"" << jsonEscape(employee.getBankName()) << "\",";
    json << "\"accountNumber\":\"" << jsonEscape(employee.getAccountNumber()) << "\",";
    json << "\"dateOfJoin\":\"" << jsonEscape(employee.getDateOfJoin()) << "\"";
    json << "}";
    return json.str();
}

std::string ApiServer::userJson(const User& user) const {
    std::ostringstream json;
    json << "{";
    json << "\"id\":" << user.id << ",";
    json << "\"username\":\"" << jsonEscape(user.username) << "\",";
    json << "\"role\":\"" << jsonEscape(user.role) << "\",";
    json << "\"employeeId\":" << numberJson(user.employeeId);
    json << "}";
    return json.str();
}

std::string ApiServer::payrollJson(const Employee& employee) const {
    double earnedBasic = (employee.getBasicSalary() / 30.0) * employee.getAttendance();
    double bonus = earnedBasic * (employee.getBonusPercentage() / 100.0);
    double gross = payroll.calculateGrossSalary(employee);
    double tax = payroll.calculateTax(gross);
    double providentFund = payroll.calculateProvidentFund(employee);
    double totalDeductions = payroll.calculateTotalDeductions(gross, employee);
    double netSalary = payroll.calculateNetSalary(employee);

    std::ostringstream json;
    json << "{";
    json << "\"employee\":{";
    json << "\"id\":" << employee.getEmployeeId() << ",";
    json << "\"name\":\"" << jsonEscape(employee.getName()) << "\",";
    json << "\"department\":\"" << jsonEscape(employee.getDepartment()) << "\",";
    json << "\"picture\":\"" << jsonEscape(employee.getPicture()) << "\",";
    json << "\"attendance\":" << employee.getAttendance() << ",";
    json << "\"leaveDays\":" << employee.getLeaveDays() << ",";
    json << "\"paymentMethod\":\"" << jsonEscape(employee.getPaymentMethod()) << "\",";
    json << "\"bankName\":\"" << jsonEscape(employee.getBankName()) << "\",";
    json << "\"accountNumber\":\"" << jsonEscape(employee.getAccountNumber()) << "\",";
    json << "\"dateOfJoin\":\"" << jsonEscape(employee.getDateOfJoin()) << "\"";
    json << "},";
    json << "\"salary\":{";
    json << "\"basic\":" << numberJson(employee.getBasicSalary()) << ",";
    json << "\"salaryPerDay\":" << numberJson(payroll.calculateSalaryPerDay(employee)) << ",";
    json << "\"earnedBasic\":" << numberJson(earnedBasic) << ",";
    json << "\"allowance\":" << numberJson(employee.getAllowance()) << ",";
    json << "\"bonus\":" << numberJson(bonus) << ",";
    json << "\"gross\":" << numberJson(gross);
    json << "},";
    json << "\"deductions\":{";
    json << "\"tax\":" << numberJson(tax) << ",";
    json << "\"providentFund\":" << numberJson(providentFund) << ",";
    json << "\"healthInsurance\":100,";
    json << "\"total\":" << numberJson(totalDeductions);
    json << "},";
    json << "\"netSalary\":" << numberJson(netSalary);
    json << "}";
    return json.str();
}

std::string ApiServer::allPayrollJson() const {
    std::ostringstream json;
    json << "[";
    for (std::size_t i = 0; i < employees.size(); ++i) {
        if (i > 0) json << ",";
        json << payrollJson(employees[i]);
    }
    json << "]";
    return json.str();
}

std::string ApiServer::monthlyReportJson() const {
    double totalNet = 0;
    std::ostringstream rows;
    rows << "[";
    for (std::size_t i = 0; i < employees.size(); ++i) {
        double net = payroll.calculateNetSalary(employees[i]);
        totalNet += net;
        if (i > 0) rows << ",";
        rows << "{\"name\":\"" << jsonEscape(employees[i].getName()) << "\",\"net\":" << numberJson(net) << "}";
    }
    rows << "]";
    std::ostringstream json;
    json << "{\"payrolls\":" << rows.str() << ",\"summary\":{\"totalNet\":" << numberJson(totalNet) << "}}";
    return json.str();
}

std::string ApiServer::statisticsJson() const {
    if (employees.empty()) {
        return "{\"totalEmployees\":0,\"totalPayroll\":0,\"averageNetSalary\":0,\"highestSalary\":{\"amount\":0,\"employee\":\"-\"}}";
    }

    double totalNet = 0;
    double highestNet = -1;
    std::string highestName = "-";

    for (const auto& emp : employees) {
        double net = payroll.calculateNetSalary(emp);
        totalNet += net;
        if (net > highestNet) {
            highestNet = net;
            highestName = emp.getName();
        }
    }

    std::ostringstream json;
    json << "{";
    json << "\"totalEmployees\":" << employees.size() << ",";
    json << "\"totalPayroll\":" << numberJson(totalNet) << ",";
    json << "\"averageNetSalary\":" << numberJson(totalNet / employees.size()) << ",";
    json << "\"highestSalary\":{";
    json << "\"amount\":" << numberJson(highestNet) << ",";
    json << "\"employee\":\"" << jsonEscape(highestName) << "\"";
    json << "}";
    json << "}";
    return json.str();
}

int ApiServer::findEmployeeIndex(int id) const {
    for (std::size_t i = 0; i < employees.size(); ++i) {
        if (employees[i].getEmployeeId() == id) return static_cast<int>(i);
    }
    return -1;
}

int ApiServer::findUserIndexByUsername(const std::string& username) const {
    for (std::size_t i = 0; i < users.size(); ++i) {
        if (users[i].username == username) return static_cast<int>(i);
    }
    return -1;
}

int ApiServer::findUserIndexById(int id) const {
    for (std::size_t i = 0; i < users.size(); ++i) {
        if (users[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

int ApiServer::findSessionIndex(const std::string& token) const {
    for (std::size_t i = 0; i < sessions.size(); ++i) {
        if (sessions[i].token == token) return static_cast<int>(i);
    }
    return -1;
}

int ApiServer::nextEmployeeId() const {
    int candidate = 101;
    while (findEmployeeIndex(candidate) != -1) {
        candidate++;
    }
    return candidate;
}

bool ApiServer::isAuthenticated(const HttpRequest& request) const {
    int idx = findSessionIndex(bearerToken(request));
    return idx != -1 && sessions[idx].expiresAt > currentTimeMillis();
}

int ApiServer::authenticatedUserIndex(const HttpRequest& request) const {
    std::string token = bearerToken(request);
    int sIdx = findSessionIndex(token);
    if (sIdx == -1 || sessions[sIdx].expiresAt <= currentTimeMillis()) return -1;
    return findUserIndexById(sessions[sIdx].userId);
}

std::string ApiServer::makeSessionToken() const {
    std::ostringstream token;
    token << "tk-" << std::time(NULL) << "-" << std::rand();
    return token.str();
}

bool ApiServer::saveEmployeesToJsonFile(const std::string& filename) const {
    std::ofstream file(filename.c_str());
    if (!file.is_open()) return false;
    file << employeesJson();
    return true;
}

bool ApiServer::loadEmployeesFromJsonFile(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) return false;
    
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string json = ss.str();
    if (json.empty() || json == "[]") return false;
    
    employees.clear();
    size_t pos = 0;
    while ((pos = json.find("\"id\":", pos)) != std::string::npos) {
        size_t objStart = json.rfind('{', pos);
        if (objStart == std::string::npos) {
            pos += 5;
            continue;
        }
        
        size_t nextId = json.find("\"id\":", pos + 5);
        if (nextId == std::string::npos) nextId = json.length();
        
        std::string obj = json.substr(objStart, nextId - objStart);
        
        double id=0, basic=0, allow=0, bonus=0, att=30, leave=0;
        std::string name, dept, pic, payMethod, bName, accNum, doj;
        
        extractJsonNumber(obj, "id", id);
        extractJsonString(obj, "name", name);
        extractJsonString(obj, "department", dept);
        extractJsonNumber(obj, "basicSalary", basic);
        extractJsonNumber(obj, "allowance", allow);
        extractJsonNumber(obj, "bonusPercentage", bonus);
        extractJsonString(obj, "picture", pic);
        extractJsonNumber(obj, "attendance", att);
        extractJsonNumber(obj, "leaveDays", leave);
        extractJsonString(obj, "paymentMethod", payMethod);
        extractJsonString(obj, "bankName", bName);
        extractJsonString(obj, "accountNumber", accNum);
        extractJsonString(obj, "dateOfJoin", doj);
        
        if (payMethod.empty()) payMethod = "Bank Transfer";
        
        Employee emp(static_cast<int>(id), name, dept, basic, allow, bonus, pic,
                     static_cast<int>(att), static_cast<int>(leave), payMethod, bName, accNum, doj);
        employees.push_back(emp);
        
        pos = nextId;
    }
    
    return !employees.empty();
}

bool ApiServer::saveUsersToFile() const {
    std::ofstream file("users.json");
    if (!file.is_open()) return false;
    file << "[";
    for (std::size_t i = 0; i < users.size(); ++i) {
        if (i > 0) file << ",";
        file << "{";
        file << "\"id\":" << users[i].id << ",";
        file << "\"u\":\"" << users[i].username << "\",";
        file << "\"p\":\"" << users[i].passwordHash << "\",";
        file << "\"role\":\"" << users[i].role << "\",";
        file << "\"employeeId\":" << users[i].employeeId;
        file << "}";
    }
    file << "]";
    return true;
}

bool ApiServer::loadUsersFromFile() {
    std::ifstream file("users.json");
    if (!file.is_open()) return false;
    
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string json = ss.str();
    if (json.empty() || json == "[]") return false;
    
    users.clear();
    size_t pos = 0;
    while ((pos = json.find("\"id\":", pos)) != std::string::npos) {
        size_t objStart = json.rfind('{', pos);
        if (objStart == std::string::npos) {
            pos += 5;
            continue;
        }
        
        size_t nextId = json.find("\"id\":", pos + 5);
        if (nextId == std::string::npos) nextId = json.length();
        
        std::string obj = json.substr(objStart, nextId - objStart);
        
        User user;
        double id = 0;
        extractJsonNumber(obj, "id", id);
        user.id = static_cast<int>(id);
        extractJsonString(obj, "u", user.username);
        extractJsonString(obj, "p", user.passwordHash);

        user.username = normalizeUsername(user.username);

        std::string role;
        if (!extractJsonString(obj, "role", role) || role.empty()) {
            role = "admin"; // legacy users.json entries default to admin
        }
        user.role = role;

        double employeeId = -1;
        if (extractJsonNumber(obj, "employeeId", employeeId)) {
            user.employeeId = static_cast<int>(employeeId);
        } else {
            user.employeeId = -1;
        }

        if (user.role != "user") {
            user.employeeId = -1;
        }
        
        if (user.id >= nextUserId) {
            nextUserId = user.id + 1;
        }
        
        users.push_back(user);
        pos = nextId;
    }
    
    return !users.empty();
}

bool ApiServer::saveAttendanceToFile() const {
    std::ofstream file("attendance.json");
    if (!file.is_open()) return false;

    file << "[";
    for (std::size_t i = 0; i < attendanceRecords.size(); ++i) {
        if (i > 0) file << ",";
        file << "{";
        file << "\"employeeId\":" << attendanceRecords[i].employeeId << ",";
        file << "\"date\":\"" << attendanceRecords[i].date << "\",";
        file << "\"timestamp\":" << numberJson(attendanceRecords[i].timestamp);
        file << "}";
    }
    file << "]";
    return true;
}

bool ApiServer::loadAttendanceFromFile() {
    std::ifstream file("attendance.json");
    if (!file.is_open()) return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string json = ss.str();
    if (json.empty() || json == "[]") return false;

    attendanceRecords.clear();
    size_t pos = 0;
    while ((pos = json.find("\"employeeId\":", pos)) != std::string::npos) {
        size_t objStart = json.rfind('{', pos);
        if (objStart == std::string::npos) {
            pos += 12;
            continue;
        }

        size_t nextObj = json.find("\"employeeId\":", pos + 12);
        if (nextObj == std::string::npos) nextObj = json.length();

        std::string obj = json.substr(objStart, nextObj - objStart);

        double employeeId = 0;
        double timestamp = 0;
        std::string date;
        extractJsonNumber(obj, "employeeId", employeeId);
        extractJsonString(obj, "date", date);
        extractJsonNumber(obj, "timestamp", timestamp);

        AttendanceRecord record;
        record.employeeId = static_cast<int>(employeeId);
        record.date = date;
        record.timestamp = timestamp;
        attendanceRecords.push_back(record);

        pos = nextObj;
    }

    return !attendanceRecords.empty();
}

double ApiServer::currentTimeMillis() { return static_cast<double>(std::time(NULL)) * 1000.0; }
std::string ApiServer::currentDateISO() {
    std::time_t now = std::time(NULL);
    std::tm tmNow = *std::localtime(&now);
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tmNow);
    return std::string(buffer);
}

std::string ApiServer::currentMonthISO() {
    std::time_t now = std::time(NULL);
    std::tm tmNow = *std::localtime(&now);
    char buffer[8];
    std::strftime(buffer, sizeof(buffer), "%Y-%m", &tmNow);
    return std::string(buffer);
}
std::string ApiServer::jsonEscape(const std::string& v) { return v; }
std::string ApiServer::numberJson(double v) { std::ostringstream oss; oss << v; return oss.str(); }
std::string ApiServer::getMimeType(const std::string& p) {
    if (p.size() >= 5 && p.substr(p.size() - 5) == ".html") return "text/html";
    if (p.size() >= 3 && p.substr(p.size() - 3) == ".js") return "application/javascript";
    if (p.size() >= 4 && p.substr(p.size() - 4) == ".css") return "text/css";
    if (p.size() >= 4 && p.substr(p.size() - 4) == ".png") return "image/png";
    if (p.size() >= 4 && p.substr(p.size() - 4) == ".svg") return "image/svg+xml";
    if (p.size() >= 4 && p.substr(p.size() - 4) == ".jpg") return "image/jpeg";
    if (p.size() >= 5 && p.substr(p.size() - 5) == ".jpeg") return "image/jpeg";
    if (p.size() >= 5 && p.substr(p.size() - 5) == ".webp") return "image/webp";
    return "text/plain";
}

std::string ApiServer::normalizeUsername(const std::string& username) { return toLower(trim(username)); }
std::string ApiServer::passwordDigest(const std::string& u, const std::string& p) { return u + p; }
std::string ApiServer::bearerToken(const HttpRequest& r) {
    auto it = r.headers.find("authorization");
    if (it == r.headers.end()) return "";
    if (it->second.substr(0, 7) == "Bearer ") return it->second.substr(7);
    return "";
}

bool ApiServer::extractJsonString(const std::string& j, const std::string& k, std::string& v) {
    size_t pos = j.find("\"" + k + "\"");
    if (pos == std::string::npos) return false;
    size_t start = j.find("\"", pos + k.size() + 2);
    if (start == std::string::npos) return false;
    size_t end = j.find("\"", start + 1);
    if (end == std::string::npos) return false;
    v = j.substr(start + 1, end - start - 1);
    return true;
}

bool ApiServer::extractJsonNumber(const std::string& j, const std::string& k, double& v) {
    size_t pos = j.find("\"" + k + "\"");
    if (pos == std::string::npos) return false;
    size_t start = j.find_first_of("0123456789", pos + k.size() + 2);
    if (start == std::string::npos) return false;
    v = std::atof(j.substr(start).c_str());
    return true;
}

std::string ApiServer::trim(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        start++;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        end--;
    }

    return value.substr(start, end - start);
}
std::string ApiServer::toLower(std::string v) { std::transform(v.begin(), v.end(), v.begin(), ::tolower); return v; }
bool ApiServer::startsWith(const std::string& v, const std::string& p) { return v.substr(0, p.size()) == p; }
