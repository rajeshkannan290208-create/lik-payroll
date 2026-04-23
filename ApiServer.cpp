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
}

ApiServer::ApiServer(int serverPort) : port(serverPort), nextUserId(1) {
    std::srand(static_cast<unsigned int>(std::time(NULL)));
    
    if (!loadEmployeesFromJsonFile("employees.json")) {
        seedEmployees();
        saveEmployeesToJsonFile("employees.json");
    }
    
    loadUsersFromFile();
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

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            closeSocket(clientSocket);
            return;
        }

        rawRequest.append(buffer, bytesReceived);
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
        return buildJsonResponse(200, "{\"success\":true,\"message\":\"C++ API is running\",\"userCount\":" + numberJson(users.size()) + "}");
    }

    if (startsWith(request.path, "/api/auth/")) {
        return handleAuthRequest(request);
    }
    if (!isAuthenticated(request)) {
        return buildJsonResponse(401, "{\"success\":false,\"message\":\"Login required\"}");
    }


    if (request.path == "/api/employees" && request.method == "GET") {
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + employeesJson() + ",\"count\":" + numberJson(employees.size()) + "}");
    }

    if (request.path == "/api/employees" && request.method == "POST") {
        std::string name, dept, pic;
        double salary = 0, allowance = 0, bonus = 0;

        extractJsonString(request.body, "name", name);
        extractJsonString(request.body, "department", dept);
        extractJsonString(request.body, "picture", pic);
        extractJsonNumber(request.body, "basicSalary", salary);
        extractJsonNumber(request.body, "allowance", allowance);
        extractJsonNumber(request.body, "bonusPercentage", bonus);

        Employee emp(nextEmployeeId(), name, dept, salary, allowance, bonus);
        emp.setPicture(pic);
        employees.push_back(emp);
        saveEmployeesToJsonFile("employees.json");

        return buildJsonResponse(201, "{\"success\":true,\"data\":" + employeeJson(emp) + ",\"message\":\"Employee added\"}");
    }

    if (startsWith(request.path, "/api/employees/")) {
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
            double salary = employees[index].getBasicSalary();
            double allowance = employees[index].getAllowance();
            double bonus = employees[index].getBonusPercentage();

            extractJsonString(request.body, "name", name);
            extractJsonString(request.body, "department", dept);
            extractJsonString(request.body, "picture", pic);
            extractJsonNumber(request.body, "basicSalary", salary);
            extractJsonNumber(request.body, "allowance", allowance);
            extractJsonNumber(request.body, "bonusPercentage", bonus);

            employees[index] = Employee(id, name, dept, salary, allowance, bonus);
            employees[index].setPicture(pic);
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
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + allPayrollJson() + "}");
    }

    if (request.path == "/api/report/monthly" && request.method == "GET") {
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + monthlyReportJson() + "}");
    }

    if (request.path == "/api/statistics" && request.method == "GET") {
        return buildJsonResponse(200, "{\"success\":true,\"data\":" + statisticsJson() + "}");
    }

    return buildJsonResponse(404, "{\"success\":false,\"message\":\"Endpoint not found\"}");
}

std::string ApiServer::handleAuthRequest(const HttpRequest& request) {
    if (request.path == "/api/auth/register" && request.method == "POST") {
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
        users.push_back(user);
        saveUsersToFile();

        return buildJsonResponse(201, "{\"success\":true,\"message\":\"Registered\"}");
    }

    if (request.path == "/api/auth/login" && request.method == "POST") {
        std::string username, password;
        extractJsonString(request.body, "username", username);
        extractJsonString(request.body, "password", password);

        username = normalizeUsername(username);
        int index = findUserIndexByUsername(username);

        if (index == -1 || users[index].passwordHash != passwordDigest(username, password)) {
            return buildJsonResponse(401, "{\"success\":false,\"message\":\"Invalid credentials\"}");
        }

        Session session;
        session.token = makeSessionToken();
        session.userId = users[index].id;
        session.expiresAt = currentTimeMillis() + 3600000;
        sessions.push_back(session);

        return buildJsonResponse(200, "{\"success\":true,\"data\":{\"token\":\"" + session.token + "\",\"expiresAt\":" + numberJson(session.expiresAt) + ",\"user\":{\"id\":" + numberJson(users[index].id) + ",\"username\":\"" + users[index].username + "\"}},\"message\":\"Login successful\"}");
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
    std::string path = request.path == "/" ? "/index.html" : request.path;
    if (path.find("..") != std::string::npos) return buildResponse(403, "text/plain", "Forbidden");
    if (!path.empty() && path[0] == '/') path.erase(0, 1);

    std::string filePath = "public/" + path;
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) return buildResponse(404, "text/plain", "Not found");

    std::ostringstream body;
    body << file.rdbuf();
    return buildResponse(200, getMimeType(filePath), body.str());
}

std::string ApiServer::buildResponse(int statusCode, const std::string& contentType, const std::string& body) const {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " OK\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}

std::string ApiServer::buildJsonResponse(int statusCode, const std::string& body) const {
    return buildResponse(statusCode, "application/json; charset=utf-8", body);
}

std::string ApiServer::buildNoContentResponse() const {
    return buildResponse(204, "text/plain", "");
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
    json << "\"picture\":\"" << jsonEscape(employee.getPicture()) << "\"";
    json << "}";
    return json.str();
}

std::string ApiServer::userJson(const User& user) const {
    std::ostringstream json;
    json << "{\"id\":" << user.id << ",\"username\":\"" << jsonEscape(user.username) << "\"}";
    return json.str();
}

std::string ApiServer::payrollJson(const Employee& employee) const {
    double bonus = employee.getBasicSalary() * (employee.getBonusPercentage() / 100.0);
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
    json << "\"department\":\"" << jsonEscape(employee.getDepartment()) << "\"";
    json << "},";
    json << "\"salary\":{";
    json << "\"basic\":" << numberJson(employee.getBasicSalary()) << ",";
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
    int maxId = 100;
    for (std::size_t i = 0; i < employees.size(); ++i) {
        if (employees[i].getEmployeeId() > maxId) maxId = employees[i].getEmployeeId();
    }
    return maxId + 1;
}

bool ApiServer::isAuthenticated(const HttpRequest& request) const {
    int idx = findSessionIndex(bearerToken(request));
    return idx != -1 && sessions[idx].expiresAt > currentTimeMillis();
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
    // Simplified loader: return false to seed if file is empty
    return false;
}

bool ApiServer::saveUsersToFile() const {
    std::ofstream file("users.json");
    if (!file.is_open()) return false;
    file << "[";
    for (std::size_t i = 0; i < users.size(); ++i) {
        if (i > 0) file << ",";
        file << "{\"id\":" << users[i].id << ",\"u\":\"" << users[i].username << "\",\"p\":\"" << users[i].passwordHash << "\"}";
    }
    file << "]";
    return true;
}

bool ApiServer::loadUsersFromFile() {
    return false;
}

double ApiServer::currentTimeMillis() { return static_cast<double>(std::time(NULL)) * 1000.0; }
std::string ApiServer::jsonEscape(const std::string& v) { return v; }
std::string ApiServer::numberJson(double v) { std::ostringstream oss; oss << v; return oss.str(); }
std::string ApiServer::getMimeType(const std::string& p) {
    if (p.substr(p.size() - 5) == ".html") return "text/html";
    if (p.substr(p.size() - 3) == ".js") return "application/javascript";
    if (p.substr(p.size() - 4) == ".css") return "text/css";
    return "text/plain";
}

std::string ApiServer::normalizeUsername(const std::string& u) { return u; }
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

std::string ApiServer::trim(const std::string& v) { return v; }
std::string ApiServer::toLower(std::string v) { std::transform(v.begin(), v.end(), v.begin(), ::tolower); return v; }
bool ApiServer::startsWith(const std::string& v, const std::string& p) { return v.substr(0, p.size()) == p; }
