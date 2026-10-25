#include "User.hpp"
#include "Security.hpp"
#include <iostream>

// Base User
User::User(int id, std::string uname, std::string hash, int deptID)
    : userID(id), username(uname), passwordHash(hash), departmentID(deptID) {}

int User::getUserID() const { return userID; }
std::string User::getUsername() const { return username; }

// Student
Student::Student(int id, std::string uname, std::string hash, int deptID) : User(id, uname, hash, deptID) {}
std::string Student::getRoleName() const { return "STUDENT"; }

void Student::displayMenu(Database& db) {
    int choice = 0;
    do {
        std::cout << "\n=== Student Portal (" << username << ") ===\n";
        std::cout << "1. View Available Assignments\n";
        std::cout << "2. Submit Assignment\n";
        std::cout << "3. View My Grades & Submissions\n";
        std::cout << "4. Logout\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;

        if (choice == 1) {
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.getHandle(), "SELECT assignmentID, title, deadline, maxMark FROM Assignments;", -1, &stmt, nullptr);
            std::cout << "\n--- Assignments ---\n";
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::cout << "ID: " << sqlite3_column_int(stmt, 0) 
                          << " | Title: " << sqlite3_column_text(stmt, 1) 
                          << " | Deadline: " << sqlite3_column_text(stmt, 2) 
                          << " | Max Mark: " << sqlite3_column_double(stmt, 3) << "\n";
            }
            sqlite3_finalize(stmt);
        } 
        else if (choice == 2) {
            int assignID;
            std::string filePath;
            std::cout << "Enter Assignment ID to submit: ";
            std::cin >> assignID;
            std::cout << "Enter submission file path: ";
            std::cin >> filePath;

            std::string timestamp = "2026-10-25 20:00:00"; 
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.getHandle(), 
                "INSERT INTO Submissions (assignmentID, studentID, filePath, submissionTime, status) VALUES (?, ?, ?, ?, 'SUBMITTED');", 
                -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, assignID);
            sqlite3_bind_int(stmt, 2, userID);
            sqlite3_bind_text(stmt, 3, filePath.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, timestamp.c_str(), -1, SQLITE_STATIC);
            
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                std::cout << "[SUCCESS] Assignment submitted successfully.\n";
            } else {
                std::cout << "[ERROR] Submission failed.\n";
            }
            sqlite3_finalize(stmt);
        }
        else if (choice == 3) {
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.getHandle(), 
                "SELECT s.assignmentID, a.title, s.submissionTime, s.status, s.rawMark, s.finalMark "
                "FROM Submissions s JOIN Assignments a ON s.assignmentID = a.assignmentID WHERE s.studentID = ?;", 
                -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, userID);
            std::cout << "\n--- My Submissions & Grades ---\n";
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::cout << "Assignment: " << sqlite3_column_text(stmt, 1)
                          << " | Time: " << sqlite3_column_text(stmt, 2)
                          << " | Status: " << sqlite3_column_text(stmt, 3)
                          << " | Raw: " << (sqlite3_column_type(stmt, 4) == SQLITE_NULL ? "Ungraded" : std::to_string(sqlite3_column_double(stmt, 4)))
                          << " | Final: " << (sqlite3_column_type(stmt, 5) == SQLITE_NULL ? "Pending" : std::to_string(sqlite3_column_double(stmt, 5))) << "\n";
            }
            sqlite3_finalize(stmt);
        }
    } while (choice != 4);
}

// Lecturer
Lecturer::Lecturer(int id, std::string uname, std::string hash, int deptID) : User(id, uname, hash, deptID) {}
std::string Lecturer::getRoleName() const { return "LECTURER"; }

void Lecturer::displayMenu(Database& db) {
    int choice = 0;
    do {
        std::cout << "\n=== Lecturer Portal (" << username << ") ===\n";
        std::cout << "1. Create New Assignment\n";
        std::cout << "2. Grade Student Submissions\n";
        std::cout << "3. Logout\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;

        if (choice == 1) {
            std::string title, deadline;
            double maxMark;
            std::cout << "Enter Assignment Title: ";
            std::cin >> title;
            std::cout << "Enter Deadline (YYYY-MM-DD): ";
            std::cin >> deadline;
            std::cout << "Enter Maximum Mark: ";
            std::cin >> maxMark;

            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.getHandle(), "INSERT INTO Assignments (title, deadline, maxMark) VALUES (?, ?, ?);", -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, deadline.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 3, maxMark);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            std::cout << "[SUCCESS] Assignment created.\n";
        }
        else if (choice == 2) {
            int subID;
            double mark;
            std::cout << "Enter Submission ID to grade: ";
            std::cin >> subID;
            std::cout << "Enter Raw Mark: ";
            std::cin >> mark;

            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.getHandle(), "UPDATE Submissions SET rawMark = ?, finalMark = ?, status = 'GRADED' WHERE submissionID = ?;", -1, &stmt, nullptr);
            sqlite3_bind_double(stmt, 1, mark);
            sqlite3_bind_double(stmt, 2, mark);
            sqlite3_bind_int(stmt, 3, subID);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            std::cout << "[SUCCESS] Grade recorded.\n";
        }
    } while (choice != 3);
}

// HoD
HeadOfDepartment::HeadOfDepartment(int id, std::string uname, std::string hash, int deptID) : User(id, uname, hash, deptID) {}
std::string HeadOfDepartment::getRoleName() const { return "HOD"; }

void HeadOfDepartment::displayMenu(Database& db) {
    int choice = 0;
    do {
        std::cout << "\n=== HoD Portal (" << username << ") ===\n";
        std::cout << "1. View All Submissions & Grades\n";
        std::cout << "2. Moderate Grade (with Audit Log)\n";
        std::cout << "3. View Audit Logs\n";
        std::cout << "4. Logout\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;

        if (choice == 1) {
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.getHandle(), "SELECT submissionID, assignmentID, studentID, status, rawMark, finalMark FROM Submissions;", -1, &stmt, nullptr);
            std::cout << "\n--- Department Submissions ---\n";
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::cout << "SubID: " << sqlite3_column_int(stmt, 0)
                          << " | AssignID: " << sqlite3_column_int(stmt, 1)
                          << " | StudentID: " << sqlite3_column_int(stmt, 2)
                          << " | Status: " << sqlite3_column_text(stmt, 3)
                          << " | Final: " << (sqlite3_column_type(stmt, 5) == SQLITE_NULL ? "N/A" : std::to_string(sqlite3_column_double(stmt, 5))) << "\n";
            }
            sqlite3_finalize(stmt);
        }
        else if (choice == 2) {
            int subID;
            double newMark;
            std::string reason;
            std::cout << "Enter Submission ID to moderate: ";
            std::cin >> subID;
            std::cout << "Enter New Moderated Mark: ";
            std::cin >> newMark;
            std::cout << "Enter Reason for Moderation: ";
            std::cin >> reason;

            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.getHandle(), "SELECT finalMark FROM Submissions WHERE submissionID = ?;", -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, subID);
            double oldMark = 0.0;
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                oldMark = sqlite3_column_double(stmt, 0);
            }
            sqlite3_finalize(stmt);

            sqlite3_prepare_v2(db.getHandle(), "UPDATE Submissions SET finalMark = ?, status = 'MODERATED' WHERE submissionID = ?;", -1, &stmt, nullptr);
            sqlite3_bind_double(stmt, 1, newMark);
            sqlite3_bind_int(stmt, 2, subID);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            sqlite3_prepare_v2(db.getHandle(), "INSERT INTO AuditLog (actorID, timestamp, oldValue, newValue, reason) VALUES (?, '2026-10-25 21:00:00', ?, ?, ?);", -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, userID);
            sqlite3_bind_text(stmt, 2, std::to_string(oldMark).c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, std::to_string(newMark).c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, reason.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            std::cout << "[SUCCESS] Grade moderated and logged to Audit Trail.\n";
        }
        else if (choice == 3) {
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.getHandle(), "SELECT logID, actorID, timestamp, oldValue, newValue, reason FROM AuditLog;", -1, &stmt, nullptr);
            std::cout << "\n--- Audit Trail Logs ---\n";
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::cout << "LogID: " << sqlite3_column_int(stmt, 0)
                          << " | ActorID: " << sqlite3_column_int(stmt, 1)
                          << " | Time: " << sqlite3_column_text(stmt, 2)
                          << " | Old: " << sqlite3_column_text(stmt, 3)
                          << " | New: " << sqlite3_column_text(stmt, 4)
                          << " | Reason: " << sqlite3_column_text(stmt, 5) << "\n";
            }
            sqlite3_finalize(stmt);
        }
    } while (choice != 4);
}

// Administrator
Administrator::Administrator(int id, std::string uname, std::string hash, int deptID) : User(id, uname, hash, deptID) {}
std::string Administrator::getRoleName() const { return "ADMIN"; }

void Administrator::displayMenu(Database& db) {
    int choice = 0;
    do {
        std::cout << "\n=== Administrator Portal (" << username << ") ===\n";
        std::cout << "1. Register New User\n";
        std::cout << "2. View All Users\n";
        std::cout << "3. Logout\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;

        if (choice == 1) {
            std::string newUname, newPwd, role;
            int deptID;
            std::cout << "Enter username: ";
            std::cin >> newUname;
            std::cout << "Enter password: ";
            std::cin >> newPwd;
            std::cout << "Enter role (STUDENT, LECTURER, HOD, ADMIN): ";
            std::cin >> role;

            // Fetch and display available departments dynamically
            std::cout << "\n--- Available Departments ---\n";
            sqlite3_stmt* deptStmt;
            std::string query = "SELECT departmentID, departmentName FROM Departments;";
            if (sqlite3_prepare_v2(db.getHandle(), query.c_str(), -1, &deptStmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(deptStmt) == SQLITE_ROW) {
                    int deptIdCol = sqlite3_column_int(deptStmt, 0);
                    const unsigned char* deptName = sqlite3_column_text(deptStmt, 1);
                    std::cout << "ID: " << deptIdCol << " | Name: " << (deptName ? reinterpret_cast<const char*>(deptName) : "Unknown") << "\n";
                }
                sqlite3_finalize(deptStmt);
            } else {
                std::cout << "(No departments found or table does not exist yet)\n";
            }
            std::cout << "Enter Department ID: ";
            std::cin >> deptID;

            std::string salt = generateSalt();
            std::string hash = hashPassword(newPwd, salt);

            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.getHandle(), "INSERT INTO Users (username, passwordHash, role, departmentID) VALUES (?, ?, ?, ?);", -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, newUname.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 4, deptID);
            
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                std::cout << "[SUCCESS] User registered successfully.\n";
            } else {
                // Print the exact SQLite error message to diagnose the failure
                std::cout << "[ERROR] Registration failed: " << sqlite3_errmsg(db.getHandle()) << "\n";
            }
            sqlite3_finalize(stmt);
        }
        else if (choice == 2) {
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db.getHandle(), "SELECT userID, username, role, departmentID FROM Users;", -1, &stmt, nullptr);
            std::cout << "\n--- Registered Users ---\n";
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                std::cout << "ID: " << sqlite3_column_int(stmt, 0)
                          << " | Username: " << sqlite3_column_text(stmt, 1)
                          << " | Role: " << sqlite3_column_text(stmt, 2)
                          << " | Dept ID: " << sqlite3_column_int(stmt, 3) << "\n";
            }
            sqlite3_finalize(stmt);
        }
    } while (choice != 3);
}

// Factory
std::unique_ptr<User> UserFactory::createUser(int id, const std::string& uname, const std::string& hash, const std::string& role, int deptID) {
    if (role == "STUDENT") return std::make_unique<Student>(id, uname, hash, deptID);
    if (role == "LECTURER") return std::make_unique<Lecturer>(id, uname, hash, deptID);
    if (role == "HOD") return std::make_unique<HeadOfDepartment>(id, uname, hash, deptID);
    if (role == "ADMIN") return std::make_unique<Administrator>(id, uname, hash, deptID);
    throw std::invalid_argument("Unknown user role.");
}