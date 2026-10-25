#include "Database.hpp"
#include <stdexcept>

Database::Database(const std::string& dbName) {
    if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open SQLite database.");
    }
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

Database::~Database() {
    if (db) sqlite3_close(db);
}

sqlite3* Database::getHandle() const {
    return db;
}

void Database::initializeSchema() {
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS Departments ("
        "  departmentID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  departmentName TEXT NOT NULL);"
        "CREATE TABLE IF NOT EXISTS Users ("
        "  userID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT UNIQUE NOT NULL,"
        "  passwordHash TEXT NOT NULL,"
        "  role TEXT CHECK(role IN ('ADMIN','LECTURER','HOD','STUDENT')) NOT NULL,"
        "  departmentID INTEGER,"
        "  FOREIGN KEY(departmentID) REFERENCES Departments(departmentID));"
        "CREATE TABLE IF NOT EXISTS Assignments ("
        "  assignmentID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  title TEXT NOT NULL,"
        "  deadline TEXT NOT NULL,"
        "  maxMark REAL NOT NULL);"
        "CREATE TABLE IF NOT EXISTS Submissions ("
        "  submissionID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  assignmentID INTEGER,"
        "  studentID INTEGER,"
        "  filePath TEXT,"
        "  submissionTime TEXT,"
        "  status TEXT,"
        "  rawMark REAL,"
        "  finalMark REAL,"
        "  FOREIGN KEY(assignmentID) REFERENCES Assignments(assignmentID),"
        "  FOREIGN KEY(studentID) REFERENCES Users(userID));"
        "CREATE TABLE IF NOT EXISTS AuditLog ("
        "  logID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  actorID INTEGER,"
        "  timestamp TEXT,"
        "  oldValue TEXT,"
        "  newValue TEXT,"
        "  reason TEXT,"
        "  FOREIGN KEY(actorID) REFERENCES Users(userID));"
        // --- Seed Default Departments ---
        "INSERT OR IGNORE INTO Departments (departmentID, departmentName) VALUES (1, 'Computer Science');"
        "INSERT OR IGNORE INTO Departments (departmentID, departmentName) VALUES (2, 'Physics');"
        "INSERT OR IGNORE INTO Departments (departmentID, departmentName) VALUES (3, 'Mathematics');";
    
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string error = errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error("Schema init failed: " + error);
    }
}
