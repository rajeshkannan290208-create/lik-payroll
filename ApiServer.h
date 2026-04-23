#ifndef API_SERVER_H
#define API_SERVER_H

#include "Employee.h"
#include "Payroll.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

class ApiServer {
private:
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string query;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    struct User {
        int id;
        std::string username;
        std::string passwordHash;
    };

    struct Session {
        std::string token;
        int userId;
        double expiresAt;
    };

    std::vector<Employee> employees;
    std::vector<User> users;
    std::vector<Session> sessions;
    Payroll payroll;
    int port;
    int nextUserId;

    void seedEmployees();
    void handleClient(std::intptr_t clientSocket);
    HttpRequest parseRequest(const std::string& rawRequest) const;
    std::string handleRequest(const HttpRequest& request);
    std::string handleApiRequest(const HttpRequest& request);
    std::string handleAuthRequest(const HttpRequest& request);
    std::string handleStaticRequest(const HttpRequest& request) const;

    std::string buildResponse(int statusCode, const std::string& contentType, const std::string& body) const;
    std::string buildJsonResponse(int statusCode, const std::string& body) const;
    std::string buildNoContentResponse() const;

    std::string employeesJson() const;
    std::string employeeJson(const Employee& employee) const;
    std::string userJson(const User& user) const;
    std::string payrollJson(const Employee& employee) const;
    std::string allPayrollJson() const;
    std::string monthlyReportJson() const;
    std::string statisticsJson() const;

    int findEmployeeIndex(int employeeId) const;
    int findUserIndexByUsername(const std::string& username) const;
    int findUserIndexById(int userId) const;
    int findSessionIndex(const std::string& token) const;
    int nextEmployeeId() const;
    bool isAuthenticated(const HttpRequest& request) const;
    std::string makeSessionToken() const;

    bool saveEmployeesToJsonFile(const std::string& filename) const;
    bool loadEmployeesFromJsonFile(const std::string& filename);
    bool saveUsersToFile() const;
    bool loadUsersFromFile();

    static double currentTimeMillis();
    static std::string jsonEscape(const std::string& value);
    static std::string numberJson(double value);
    static std::string getMimeType(const std::string& path);
    static std::string sanitizeFilename(const std::string& filename);
    static std::string normalizeUsername(const std::string& username);
    static bool isValidUsername(const std::string& username);
    static bool isValidPassword(const std::string& password);
    static std::string passwordDigest(const std::string& username, const std::string& password);
    static std::string bearerToken(const HttpRequest& request);
    static bool extractJsonString(const std::string& json, const std::string& key, std::string& value);
    static bool extractJsonNumber(const std::string& json, const std::string& key, double& value);
    static std::string trim(const std::string& value);
    static std::string toLower(std::string value);
    static bool startsWith(const std::string& value, const std::string& prefix);

public:
    explicit ApiServer(int serverPort = 3000);
    bool start();
};

#endif
