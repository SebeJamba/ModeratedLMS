#include "Database.hpp"
#include "DeadlineRepository.hpp"
#include "Security.hpp"
#include "User.hpp"
#include <iostream>
#include <memory>

int main() {
    try {
        std::cout << "==================================================\n";
        std::cout << "   MODERATED LMS - PROFESSIONAL MULTI-FILE ARCH   \n";
        std::cout << "==================================================\n";

        Database db("lms.db");
        db.initializeSchema();
        DeadlineRepository(db).initialize();

        // Seed default Department and Admin if empty
        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db.getHandle(), "SELECT COUNT(*) FROM Users;", -1, &stmt, nullptr);
        sqlite3_step(stmt);
        int userCount = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        if (userCount == 0) {

            std::string salt = generateSalt();
            std::string pwdHash = hashPassword("Admin123!", salt);
            sqlite3_stmt *insertStmt;
            sqlite3_prepare_v2(db.getHandle(),
                               "INSERT INTO Users (username, passwordHash, role, departmentID) "
                               "VALUES ('admin', ?, 'ADMIN', 1);",
                               -1, &insertStmt, nullptr);
            sqlite3_bind_text(insertStmt, 1, pwdHash.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(insertStmt);
            sqlite3_finalize(insertStmt);
            std::cout
                << "[INIT] Seeded default department and administrator ('admin' / 'Admin123!').\n";
        }

        // Main Login Loop
        bool running = true;
        while (running) {
            std::string username, password;
            std::cout << "\n--------------------------------------------------\n";
            std::cout << "SYSTEM LOGIN (Type 'exit' as username to quit)\n";
            std::cout << "--------------------------------------------------\n";
            std::cout << "Username: ";
            if (!(std::cin >> username))
                break;
            if (username == "exit")
                break;

            std::cout << "Password: ";
            if (!(std::cin >> password))
                break;

            sqlite3_prepare_v2(db.getHandle(),
                               "SELECT userID, username, passwordHash, role, departmentID FROM "
                               "Users WHERE username = ?;",
                               -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                std::string uname = (const char *)sqlite3_column_text(stmt, 1);
                std::string storedHash = (const char *)sqlite3_column_text(stmt, 2);
                std::string role = (const char *)sqlite3_column_text(stmt, 3);
                int deptID = sqlite3_column_int(stmt, 4);
                sqlite3_finalize(stmt);

                if (verifyPassword(password, storedHash)) {
                    if (storedHash.rfind("pbkdf2$", 0) != 0) {
                        auto upgraded = hashPassword(password, generateSalt());
                        sqlite3_stmt *upgrade = nullptr;
                        if (sqlite3_prepare_v2(db.getHandle(),
                                               "UPDATE Users SET passwordHash=? WHERE userID=?", -1,
                                               &upgrade, nullptr) != SQLITE_OK)
                            throw std::runtime_error("Cannot upgrade password");
                        sqlite3_bind_text(upgrade, 1, upgraded.c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_int(upgrade, 2, id);
                        int rc = sqlite3_step(upgrade);
                        sqlite3_finalize(upgrade);
                        if (rc != SQLITE_DONE)
                            throw std::runtime_error("Cannot save secure password");
                        storedHash = upgraded;
                    }
                    std::cout << "\n[SUCCESS] Welcome back, " << uname << "!\n";
                    std::unique_ptr<User> currentUser =
                        UserFactory::createUser(id, uname, storedHash, role, deptID);
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
    } catch (const std::exception &e) {
        std::cerr << "[CRITICAL EXCEPTION] " << e.what() << "\n";
        return 1;
    }
    return 0;
}