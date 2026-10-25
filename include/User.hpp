#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <memory>
#include "Database.hpp"

class User {
protected:
    int userID;
    std::string username;
    std::string passwordHash;
    int departmentID;
public:
    User(int id, std::string uname, std::string hash, int deptID);
    virtual ~User() = default;
    virtual std::string getRoleName() const = 0;
    virtual void displayMenu(Database& db) = 0;
    int getUserID() const;
    std::string getUsername() const;
};

class Student : public User {
public:
    Student(int id, std::string uname, std::string hash, int deptID);
    std::string getRoleName() const override;
    void displayMenu(Database& db) override;
};

class Lecturer : public User {
public:
    Lecturer(int id, std::string uname, std::string hash, int deptID);
    std::string getRoleName() const override;
    void displayMenu(Database& db) override;
};

class HeadOfDepartment : public User {
public:
    HeadOfDepartment(int id, std::string uname, std::string hash, int deptID);
    std::string getRoleName() const override;
    void displayMenu(Database& db) override;
};

class Administrator : public User {
public:
    Administrator(int id, std::string uname, std::string hash, int deptID);
    std::string getRoleName() const override;
    void displayMenu(Database& db) override;
};

class UserFactory {
public:
    static std::unique_ptr<User> createUser(int id, const std::string& uname, const std::string& hash, const std::string& role, int deptID);
};

#endif // USER_HPP