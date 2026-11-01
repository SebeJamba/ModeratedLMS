#include <iostream>
#include <memory>
#include "Database.hpp"
#include "Security.hpp"
#include "User.hpp"
#include "DeadlineRepository.hpp"

int main() {
    try {
        std::cout << "==================================================\n";
        std::cout << "   MODERATED LMS - PROFESSIONAL MULTI-FILE ARCH   \n";
        std::cout << "==================================================\n";
        
        Database db("lms.db");
        db.initializeSchema();
        DeadlineRepository(db).initialize();

        // Seed default Department and Admin if empty
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db.getHandle(), "SELECT COUNT(*) FROM Users;", -1, &stmt, nullptr);
        sqlite3_step(stmt);
        int userCount = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        if (userCount == 0) {
            sqlite3_exec(db.getHandle(), "INSERT INTO Departments (departmentName) VALUES ('Computer Science');", nullptr, nullptr, nullptr);
            
            std::string salt = generateSalt();
            std::string pwdHash = hashPassword("Admin123!", salt);
            sqlite3_stmt* insertStmt;
            sqlite3_prepare_v2(db.getHandle(), "INSERT INTO Users (username, passwordHash, role, departmentID) VALUES ('admin', ?, 'ADMIN', 1);", -1, &insertStmt, nullptr);
            sqlite3_bind_text(insertStmt, 1, pwdHash.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(insertStmt);
            sqlite3_finalize(insertStmt);
            std::cout << "[INIT] Seeded default department and administrator ('admin' / 'Admin123!').\n";
        }

        // Main Login Loop
        bool running = true;
        while (running) {
            std::string username, password;
            std::cout << "\n--------------------------------------------------\n";
            std::cout << "SYSTEM LOGIN (Type 'exit' as username to quit)\n";
            std::cout << "--------------------------------------------------\n";
            std::cout << "Username: ";
            std::cin >> username;
            if (username == "exit") break;
            
            std::cout << "Password: ";
            std::cin >> password;

            sqlite3_prepare_v2(db.getHandle(), "SELECT userID, username, passwordHash, role, departmentID FROM Users WHERE username = ?;", -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                std::string uname = (const char*)sqlite3_column_text(stmt, 1);
                std::string storedHash = (const char*)sqlite3_column_text(stmt, 2);
                std::string role = (const char*)sqlite3_column_text(stmt, 3);
                int deptID = sqlite3_column_int(stmt, 4);
                sqlite3_finalize(stmt);

                if (verifyPassword(password, storedHash)) {
                    std::cout << "\n[SUCCESS] Welcome back, " << uname << "!\n";
                    std::unique_ptr<User> currentUser = UserFactory::createUser(id, uname, storedHash, role, deptID);
                    currentUser->displayMenu(db);
                } else {
                    std::cout << "\n[ERROR] Incorrect password.\n";
                }
            } else {
                sqlite3_finalize(stmt);
                std::cout << "\n[ERROR] User not found.\n";
            }
        }

        std::cout << "\nShutting down Moderated LMS. Goodbye!\n";
    } catch (const std::exception& e) {
        std::cerr << "[CRITICAL EXCEPTION] " << e.what() << "\n";
        return 1;
    }
    return 0;
}