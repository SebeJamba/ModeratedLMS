#include "Database.hpp"
#include <stdexcept>

Database::Database(const std::string &dbName) {
    if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
        if (db) {
            sqlite3_close(db);
            db = nullptr;
        }
        throw std::runtime_error("Failed to open SQLite database.");
    }
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

Database::~Database() {
    if (db)
        sqlite3_close(db);
}

sqlite3 *Database::getHandle() const { return db; }

void Database::initializeSchema() {
    const char *sql =
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
        "INSERT OR IGNORE INTO Departments (departmentID, departmentName) VALUES (1, 'Computer "
        "Science');"
        "INSERT OR IGNORE INTO Departments (departmentID, departmentName) VALUES (2, 'Physics');"
        "INSERT OR IGNORE INTO Departments (departmentID, departmentName) VALUES (3, "
        "'Mathematics');";

    char *errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string error = errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error("Schema init failed: " + error);
    }

    // Additive upgrade: retain existing submissions and old audit entries.
    auto execute = [this](const char *statement) {
        char *error = nullptr;
        if (sqlite3_exec(db, statement, nullptr, nullptr, &error) != SQLITE_OK) {
            std::string message = error ? error : "Database migration failed";
            sqlite3_free(error);
            throw std::runtime_error(message);
        }
    };
    auto hasColumn = [this](const char *statement, const std::string &name) {
        sqlite3_stmt *raw = nullptr;
        if (sqlite3_prepare_v2(db, statement, -1, &raw, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db));
        bool found = false;
        int rc;
        while ((rc = sqlite3_step(raw)) == SQLITE_ROW) {
            if (name == reinterpret_cast<const char *>(sqlite3_column_text(raw, 1)))
                found = true;
        }
        sqlite3_finalize(raw);
        if (rc != SQLITE_DONE)
            throw std::runtime_error("Cannot inspect schema");
        return found;
    };
    execute("BEGIN IMMEDIATE");
    try {
        if (!hasColumn("PRAGMA table_info(Submissions)", "feedback"))
            execute("ALTER TABLE Submissions ADD COLUMN feedback TEXT");
        if (!hasColumn("PRAGMA table_info(AuditLog)", "submissionID"))
            execute("ALTER TABLE AuditLog ADD COLUMN submissionID INTEGER REFERENCES "
                    "Submissions(submissionID) ON DELETE RESTRICT");
        if (!hasColumn("PRAGMA table_info(Submissions)", "isActive")) {
            execute("ALTER TABLE Submissions ADD COLUMN isActive INTEGER NOT NULL DEFAULT 1 "
                    "CHECK(isActive IN (0,1))");
            execute("UPDATE Submissions SET isActive=0 WHERE submissionID NOT IN (SELECT "
                    "MAX(submissionID) FROM Submissions GROUP BY assignmentID,studentID)");
        }
        execute("CREATE UNIQUE INDEX IF NOT EXISTS one_active_submission ON "
                "Submissions(assignmentID,studentID) WHERE isActive=1");
        execute("UPDATE Submissions SET status='MARKED' WHERE status='GRADED'");
        execute("CREATE TRIGGER IF NOT EXISTS immutable_audit_update BEFORE UPDATE ON AuditLog "
                "BEGIN SELECT RAISE(ABORT,'Audit records are permanent'); END");
        execute("CREATE TRIGGER IF NOT EXISTS immutable_audit_delete BEFORE DELETE ON AuditLog "
                "BEGIN SELECT RAISE(ABORT,'Audit records are permanent'); END");
        execute("COMMIT");
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        throw;
    }
}
