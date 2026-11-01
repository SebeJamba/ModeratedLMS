#include "User.hpp"
#include "AssessmentService.hpp"
#include "DeadlineRepository.hpp"
#include "Security.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
struct InputClosed {};
template <typename T> T inputValue() {
    T value{};
    if (std::cin >> value)
        return value;
    if (std::cin.eof())
        throw InputClosed{};
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    throw std::invalid_argument("Enter a valid number");
}
std::string inputLine() {
    std::string line;
    if (!std::getline(std::cin >> std::ws, line))
        throw InputClosed{};
    return line;
}
void show(const std::vector<SubmissionView> &records) {
    for (const auto &row : records)
        std::cout << "Submission: " << row.id << " | Assignment: " << row.title
                  << " | Time: " << row.timestamp << " | Status: " << row.status
                  << " | Final: " << row.mark << " | Feedback: " << row.feedback << "\n";
}
} // namespace

// Base User
User::User(int id, std::string uname, std::string hash, int deptID)
    : userID(id), username(uname), passwordHash(hash), departmentID(deptID) {}

int User::getUserID() const { return userID; }
std::string User::getUsername() const { return username; }

// Student
Student::Student(int id, std::string uname, std::string hash, int deptID)
    : User(id, uname, hash, deptID) {}
std::string Student::getRoleName() const { return "STUDENT"; }

void Student::displayMenu(Database &db) {
    AssessmentService assessments(db);
    for (;;) {
        std::cout << "\n=== Student Portal (" << username
                  << ") ===\n1. View Available Assignments\n2. Submit Assignment\n3. View My "
                     "Grades & Submissions\n4. Logout\nEnter choice: ";
        try {
            int choice = inputValue<int>();
            if (choice == 4)
                return;
            if (choice == 1) {
                sqlite3_stmt *raw = nullptr;
                if (sqlite3_prepare_v2(
                        db.getHandle(),
                        "SELECT a.assignmentID,a.title,a.maxMark FROM Assignments a JOIN "
                        "Enrollments e ON e.courseID=a.courseID WHERE e.studentID=?",
                        -1, &raw, nullptr) != SQLITE_OK)
                    throw std::runtime_error("Cannot list assignments");
                std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(
                    raw, sqlite3_finalize);
                sqlite3_bind_int(raw, 1, userID);
                int rc;
                while ((rc = sqlite3_step(raw)) == SQLITE_ROW) {
                    std::cout << "ID: " << sqlite3_column_int(raw, 0)
                              << " | Title: " << sqlite3_column_text(raw, 1)
                              << " | Effective deadline: "
                              << DeadlineRepository(db).effectiveDeadline(
                                     sqlite3_column_int(raw, 0), userID)
                              << " | Max Mark: " << sqlite3_column_double(raw, 2) << "\n";
                }
                if (rc != SQLITE_DONE)
                    throw std::runtime_error("Cannot read assignment list");
            } else if (choice == 2) {
                std::cout << "Enter Assignment ID to submit: ";
                int assignment = inputValue<int>();
                std::cout << "Enter submission file path or work reference: ";
                auto path = inputLine();
                assessments.submit(userID, assignment, path);
                std::cout << "[SUCCESS] Assignment submitted successfully.\n";
            } else if (choice == 3)
                show(assessments.studentSubmissions(userID));
            else
                std::cout << "[ERROR] Unknown menu option.\n";
        } catch (const InputClosed &) {
            return;
        } catch (const std::exception &e) {
            std::cout << "[ERROR] " << e.what() << "\n";
        }
    }
}

// Lecturer
Lecturer::Lecturer(int id, std::string uname, std::string hash, int deptID)
    : User(id, uname, hash, deptID) {}
std::string Lecturer::getRoleName() const { return "LECTURER"; }

void Lecturer::displayMenu(Database &db) {
    AssessmentService assessments(db);
    for (;;) {
        std::cout << "\n=== Lecturer Portal (" << username
                  << ") ===\n1. Create New Assignment\n2. Grade Student Submissions\n3. Logout\n4. "
                     "Extend class deadline\n5. Extend individual deadline\n6. List my "
                     "submissions\nEnter choice: ";
        try {
            int choice = inputValue<int>();
            if (choice == 3)
                return;
            if (choice == 4 || choice == 5) {
                std::cout << "Assignment ID: ";
                int assignment = inputValue<int>();
                int student = 0;
                if (choice == 5) {
                    std::cout << "Student ID: ";
                    student = inputValue<int>();
                }
                std::cout << "New deadline (YYYY-MM-DD HH:MM:SS): ";
                auto deadline = inputLine();
                DeadlineRepository repository(db);
                if (choice == 4)
                    repository.extendClass(userID, assignment, deadline);
                else
                    repository.extendIndividual(userID, assignment, student, deadline);
                std::cout << "[SUCCESS] Deadline extended.\n";
            } else if (choice == 2) {
                std::cout << "Submission ID: ";
                int submission = inputValue<int>();
                std::cout << "Raw mark: ";
                double raw = inputValue<double>();
                std::cout << "Written feedback: ";
                auto feedback = inputLine();
                std::cout << "Reason for mark entry or change: ";
                auto reason = inputLine();
                assessments.mark(userID, submission, raw, feedback, reason);
                std::cout << "[SUCCESS] Grade recorded with feedback and audit entry.\n";
            } else if (choice == 1) {
                std::cout << "Course code: ";
                std::string course = inputLine();
                std::cout << "Assignment title: ";
                auto title = inputLine();
                std::cout << "Deadline (YYYY-MM-DD HH:MM:SS): ";
                auto deadline = inputLine();
                timestampSeconds(deadline);
                std::cout << "Maximum mark: ";
                auto maximum = inputValue<double>();
                StandardLatePenalty().finalMark(0, maximum, 0);
                sqlite3_stmt *raw = nullptr;
                if (sqlite3_prepare_v2(
                        db.getHandle(),
                        "INSERT INTO Assignments(title,deadline,maxMark,courseID) SELECT "
                        "?,?,?,courseID FROM Courses WHERE courseID=? AND lecturerID=?",
                        -1, &raw, nullptr) != SQLITE_OK)
                    throw std::runtime_error("Cannot create assignment");
                std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(
                    raw, sqlite3_finalize);
                sqlite3_bind_text(raw, 1, title.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(raw, 2, deadline.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_double(raw, 3, maximum);
                sqlite3_bind_text(raw, 4, course.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(raw, 5, userID);
                if (sqlite3_step(raw) != SQLITE_DONE || sqlite3_changes(db.getHandle()) != 1)
                    throw std::runtime_error(
                        "Only the assigned lecturer can create this assignment");
                std::cout << "[SUCCESS] Assignment created.\n";
            } else if (choice == 6) {
                sqlite3_stmt *raw = nullptr;
                if (sqlite3_prepare_v2(
                        db.getHandle(),
                        "SELECT s.submissionID,a.title,s.studentID,s.status FROM Submissions s "
                        "JOIN Assignments a ON a.assignmentID=s.assignmentID JOIN Courses c ON "
                        "c.courseID=a.courseID WHERE c.lecturerID=? AND s.isActive=1",
                        -1, &raw, nullptr) != SQLITE_OK)
                    throw std::runtime_error("Cannot list submissions");
                std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(
                    raw, sqlite3_finalize);
                sqlite3_bind_int(raw, 1, userID);
                int rc;
                while ((rc = sqlite3_step(raw)) == SQLITE_ROW)
                    std::cout << "Submission: " << sqlite3_column_int(raw, 0) << " | "
                              << sqlite3_column_text(raw, 1)
                              << " | Student: " << sqlite3_column_int(raw, 2) << " | "
                              << sqlite3_column_text(raw, 3) << "\n";
                if (rc != SQLITE_DONE)
                    throw std::runtime_error("Cannot read submissions");
            } else
                std::cout << "[ERROR] Unknown menu option.\n";
        } catch (const InputClosed &) {
            return;
        } catch (const std::exception &e) {
            std::cout << "[ERROR] " << e.what() << "\n";
        }
    }
}

// HoD
HeadOfDepartment::HeadOfDepartment(int id, std::string uname, std::string hash, int deptID)
    : User(id, uname, hash, deptID) {}
std::string HeadOfDepartment::getRoleName() const { return "HOD"; }

void HeadOfDepartment::displayMenu(Database &db) {
    AssessmentService assessments(db);
    for (;;) {
        std::cout << "\n=== HoD Portal (" << username
                  << ") ===\n1. View department submissions\n2. Moderate grade\n3. View department "
                     "audit logs\n4. Logout\n5. Release approved mark\nEnter choice: ";
        try {
            int choice = inputValue<int>();
            if (choice == 4)
                return;
            if (choice == 1)
                show(assessments.departmentSubmissions(userID));
            else if (choice == 2) {
                std::cout << "Submission ID: ";
                int submission = inputValue<int>();
                std::cout << "New moderated mark: ";
                double mark = inputValue<double>();
                std::cout << "Reason for moderation: ";
                auto reason = inputLine();
                assessments.moderate(userID, submission, mark, reason);
                std::cout << "[SUCCESS] Grade moderated and logged; awaiting separate release.\n";
            } else if (choice == 3) {
                for (const auto &row : assessments.departmentAudit(userID))
                    std::cout << row << "\n";
            } else if (choice == 5) {
                std::cout << "Submission ID to release: ";
                int submission = inputValue<int>();
                assessments.release(userID, submission);
                std::cout << "[SUCCESS] Approved mark and feedback released.\n";
            } else
                std::cout << "[ERROR] Unknown menu option.\n";
        } catch (const InputClosed &) {
            return;
        } catch (const std::exception &e) {
            std::cout << "[ERROR] " << e.what() << "\n";
        }
    }
}

// Administrator
Administrator::Administrator(int id, std::string uname, std::string hash, int deptID)
    : User(id, uname, hash, deptID) {}
std::string Administrator::getRoleName() const { return "ADMIN"; }

void Administrator::displayMenu(Database &db) {
    int choice = 0;
    do {
        std::cout << "\n=== Administrator Portal (" << username << ") ===\n";
        std::cout << "1. Register New User\n";
        std::cout << "2. View All Users\n";
        std::cout << "3. Logout\n4. Create course and assign lecturer\n5. Enrol student\n6. Link "
                     "existing assignment to course\n";
        std::cout << "Enter choice: ";
        if (!(std::cin >> choice)) {
            if (std::cin.eof())
                return;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "[ERROR] Enter a menu number.\n";
            continue;
        }

        if (choice == 4 || choice == 5 || choice == 6) {
            sqlite3_stmt *statement = nullptr;
            std::string course, name;
            int lecturer = 0, department = 0, student = 0, assignment = 0;
            std::cout << "Course code: ";
            std::cin >> course;
            if (choice == 4) {
                std::cout << "Course name: ";
                std::getline(std::cin >> std::ws, name);
                std::cout << "Department ID and Lecturer ID: ";
                std::cin >> department >> lecturer;
                sqlite3_prepare_v2(db.getHandle(), "INSERT INTO Courses VALUES(?,?,?,?);", -1,
                                   &statement, nullptr);
                sqlite3_bind_text(statement, 1, course.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(statement, 2, name.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(statement, 3, department);
                sqlite3_bind_int(statement, 4, lecturer);
            } else if (choice == 5) {
                std::cout << "Student ID: ";
                std::cin >> student;
                sqlite3_prepare_v2(db.getHandle(), "INSERT INTO Enrollments VALUES(?,?);", -1,
                                   &statement, nullptr);
                sqlite3_bind_int(statement, 1, student);
                sqlite3_bind_text(statement, 2, course.c_str(), -1, SQLITE_TRANSIENT);
            } else {
                std::cout << "Assignment ID: ";
                std::cin >> assignment;
                sqlite3_prepare_v2(db.getHandle(),
                                   "UPDATE Assignments SET courseID=? WHERE assignmentID=?;", -1,
                                   &statement, nullptr);
                sqlite3_bind_text(statement, 1, course.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(statement, 2, assignment);
            }
            if (statement && sqlite3_step(statement) == SQLITE_DONE &&
                sqlite3_changes(db.getHandle()) > 0)
                std::cout << "[SUCCESS] Course data saved.\n";
            else
                std::cout << "[ERROR] Course operation failed.\n";
            sqlite3_finalize(statement);
        } else if (choice == 1) {
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
            sqlite3_stmt *deptStmt;
            std::string query = "SELECT departmentID, departmentName FROM Departments;";
            if (sqlite3_prepare_v2(db.getHandle(), query.c_str(), -1, &deptStmt, nullptr) ==
                SQLITE_OK) {
                while (sqlite3_step(deptStmt) == SQLITE_ROW) {
                    int deptIdCol = sqlite3_column_int(deptStmt, 0);
                    const unsigned char *deptName = sqlite3_column_text(deptStmt, 1);
                    std::cout << "ID: " << deptIdCol << " | Name: "
                              << (deptName ? reinterpret_cast<const char *>(deptName) : "Unknown")
                              << "\n";
                }
                sqlite3_finalize(deptStmt);
            } else {
                std::cout << "(No departments found or table does not exist yet)\n";
            }
            std::cout << "Enter Department ID: ";
            std::cin >> deptID;

            std::string salt = generateSalt();
            std::string hash = hashPassword(newPwd, salt);

            sqlite3_stmt *stmt;
            sqlite3_prepare_v2(db.getHandle(),
                               "INSERT INTO Users (username, passwordHash, role, departmentID) "
                               "VALUES (?, ?, ?, ?);",
                               -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, newUname.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 4, deptID);

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                std::cout << "[SUCCESS] User registered successfully.\n";
            } else {
                std::cout << "[ERROR] Registration failed: " << sqlite3_errmsg(db.getHandle())
                          << "\n";
            }
            sqlite3_finalize(stmt);
        } else if (choice == 2) {
            sqlite3_stmt *stmt;
            sqlite3_prepare_v2(db.getHandle(),
                               "SELECT userID, username, role, departmentID FROM Users;", -1, &stmt,
                               nullptr);
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
std::unique_ptr<User> UserFactory::createUser(int id, const std::string &uname,
                                              const std::string &hash, const std::string &role,
                                              int deptID) {
    if (role == "STUDENT")
        return std::make_unique<Student>(id, uname, hash, deptID);
    if (role == "LECTURER")
        return std::make_unique<Lecturer>(id, uname, hash, deptID);
    if (role == "HOD")
        return std::make_unique<HeadOfDepartment>(id, uname, hash, deptID);
    if (role == "ADMIN")
        return std::make_unique<Administrator>(id, uname, hash, deptID);
    throw std::invalid_argument("Unknown user role.");
}