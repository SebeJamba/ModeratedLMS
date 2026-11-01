#include "DeadlineRepository.hpp"
#include <stdexcept>
#include <memory>
#include <ctime>
#include <regex>

namespace {
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
Statement prepare(sqlite3* db, const char* sql) {
    sqlite3_stmt* p = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &p, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db));
    return Statement(p, sqlite3_finalize);
}
void text(sqlite3_stmt* p, int index, const std::string& value) {
    if (sqlite3_bind_text(p, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
        throw std::runtime_error("Cannot bind deadline value");
}
void execute(sqlite3* db, const char* sql) {
    char* error = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &error) != SQLITE_OK) {
        std::string message = error ? error : "SQLite operation failed";
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}
void done(sqlite3_stmt* p) {
    if (sqlite3_step(p) != SQLITE_DONE) throw std::runtime_error("Deadline update failed");
}
void requireRange(int value, int lower, int upper) {
    if (value < lower || value > upper) throw std::invalid_argument("Invalid calendar date");
}
bool leapYear(int year) {
    return year%4==0 && (year%100!=0 || year%400==0);
}
void validDate(const std::string& value) {
    static const std::regex shape("[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}");
    if (!std::regex_match(value, shape)) throw std::invalid_argument("Use YYYY-MM-DD HH:MM:SS");
    int year=std::stoi(value.substr(0,4)), month=std::stoi(value.substr(5,2)), day=std::stoi(value.substr(8,2));
    requireRange(year,1970,9999);requireRange(month,1,12);
    int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(leapYear(year)) days[1]=29;
    requireRange(day,1,days[month-1]);requireRange(std::stoi(value.substr(11,2)),0,23);
    requireRange(std::stoi(value.substr(14,2)),0,59);requireRange(std::stoi(value.substr(17,2)),0,59);
}
std::string authorize(sqlite3* db, int lecturer, int assignment) {
    auto p=prepare(db,"SELECT a.deadline FROM Assignments a JOIN Courses c ON c.courseID=a.courseID "
        "JOIN Users u ON u.userID=c.lecturerID WHERE a.assignmentID=? AND u.userID=? AND u.role='LECTURER';");
    sqlite3_bind_int(p.get(),1,assignment); sqlite3_bind_int(p.get(),2,lecturer);
    if (sqlite3_step(p.get())!=SQLITE_ROW) throw std::runtime_error("Only the assigned lecturer can extend this assignment");
    return reinterpret_cast<const char*>(sqlite3_column_text(p.get(),0));
}
}
void DeadlineRepository::initialize() {
    auto db=database.getHandle();
    execute(db,"CREATE TABLE IF NOT EXISTS Courses(courseID TEXT PRIMARY KEY,courseName TEXT NOT NULL,"
        "departmentID INTEGER NOT NULL REFERENCES Departments(departmentID), lecturerID INTEGER REFERENCES Users(userID));"
        "CREATE TABLE IF NOT EXISTS Enrollments(studentID INTEGER NOT NULL REFERENCES Users(userID),"
        "courseID TEXT NOT NULL REFERENCES Courses(courseID),PRIMARY KEY(studentID,courseID));"
        "CREATE TABLE IF NOT EXISTS Extensions(extensionID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "assignmentID INTEGER NOT NULL REFERENCES Assignments(assignmentID),studentID INTEGER NOT NULL REFERENCES Users(userID),"
        "extendedDeadline TEXT NOT NULL,UNIQUE(assignmentID,studentID));"
        "CREATE TABLE IF NOT EXISTS ClassExtensions(assignmentID INTEGER PRIMARY KEY REFERENCES Assignments(assignmentID),"
        "extendedDeadline TEXT NOT NULL,actorID INTEGER NOT NULL REFERENCES Users(userID),changedAt TEXT NOT NULL);"
        "CREATE TABLE IF NOT EXISTS SchemaMigrations(version INTEGER PRIMARY KEY, appliedAt TEXT NOT NULL);");
    bool hasCourse=false;
    {auto p=prepare(db,"PRAGMA table_info(Assignments);");
    while(sqlite3_step(p.get())==SQLITE_ROW) {
        std::string name=reinterpret_cast<const char*>(sqlite3_column_text(p.get(),1));
        if(name=="courseID") hasCourse=true;
    }}
    if(!hasCourse) execute(db,"ALTER TABLE Assignments ADD COLUMN courseID TEXT REFERENCES Courses(courseID);");
    execute(db,"INSERT OR IGNORE INTO SchemaMigrations VALUES(11,datetime('now'));");
}
std::string DeadlineRepository::effectiveDeadline(int assignment, int student) {
    auto p=prepare(database.getHandle(),"SELECT MAX(a.deadline,COALESCE(c.extendedDeadline,a.deadline),"
        "COALESCE(e.extendedDeadline,a.deadline)) FROM Assignments a "
        "LEFT JOIN ClassExtensions c ON c.assignmentID=a.assignmentID "
        "LEFT JOIN Extensions e ON e.assignmentID=a.assignmentID AND e.studentID=? WHERE a.assignmentID=?;");
    sqlite3_bind_int(p.get(),1,student); sqlite3_bind_int(p.get(),2,assignment);
    if(sqlite3_step(p.get())!=SQLITE_ROW) throw std::runtime_error("Assignment does not exist");
    return reinterpret_cast<const char*>(sqlite3_column_text(p.get(),0));
}
void DeadlineRepository::extendIndividual(int lecturer,int assignment,int student,const std::string& deadline) {
    validDate(deadline); auto db=database.getHandle(); authorize(db,lecturer,assignment);
    {auto p=prepare(db,"SELECT 1 FROM Enrollments e JOIN Assignments a ON a.courseID=e.courseID "
        "JOIN Users u ON u.userID=e.studentID WHERE a.assignmentID=? AND e.studentID=? AND u.role='STUDENT';");
    sqlite3_bind_int(p.get(),1,assignment); sqlite3_bind_int(p.get(),2,student);
    if(sqlite3_step(p.get())!=SQLITE_ROW) throw std::runtime_error("Student is not enrolled");}
    if(deadline<=effectiveDeadline(assignment,student)) throw std::invalid_argument("Extension must move the effective deadline later");
    auto p=prepare(db,"INSERT INTO Extensions(assignmentID,studentID,extendedDeadline) VALUES(?,?,?) "
        "ON CONFLICT(assignmentID,studentID) DO UPDATE SET extendedDeadline=excluded.extendedDeadline;");
    sqlite3_bind_int(p.get(),1,assignment); sqlite3_bind_int(p.get(),2,student); text(p.get(),3,deadline); done(p.get());
}
void DeadlineRepository::extendClass(int lecturer,int assignment,const std::string& deadline) {
    validDate(deadline); auto db=database.getHandle();
    execute(db,"BEGIN IMMEDIATE;");
    try {
        std::string base=authorize(db,lecturer,assignment);
        {auto p=prepare(db,"SELECT extendedDeadline FROM ClassExtensions WHERE assignmentID=?;");
        sqlite3_bind_int(p.get(),1,assignment);
        if(sqlite3_step(p.get())==SQLITE_ROW) base=reinterpret_cast<const char*>(sqlite3_column_text(p.get(),0));}
        if(deadline<=base) throw std::invalid_argument("Class extension must move the deadline later");
        {auto p=prepare(db,"INSERT INTO ClassExtensions VALUES(?,?,?,datetime('now')) ON CONFLICT(assignmentID) "
            "DO UPDATE SET extendedDeadline=excluded.extendedDeadline,actorID=excluded.actorID,changedAt=excluded.changedAt;");
        sqlite3_bind_int(p.get(),1,assignment); text(p.get(),2,deadline); sqlite3_bind_int(p.get(),3,lecturer); done(p.get());}
        // Keep a longer individual extension; materialise current enrolments atomically.
        {auto p=prepare(db,"INSERT INTO Extensions(assignmentID,studentID,extendedDeadline) "
            "SELECT a.assignmentID,e.studentID,? FROM Assignments a JOIN Enrollments e ON e.courseID=a.courseID "
            "JOIN Users u ON u.userID=e.studentID WHERE a.assignmentID=? AND u.role='STUDENT' "
            "ON CONFLICT(assignmentID,studentID) DO UPDATE SET extendedDeadline=MAX(Extensions.extendedDeadline,excluded.extendedDeadline);");
        text(p.get(),1,deadline); sqlite3_bind_int(p.get(),2,assignment); done(p.get());}
        execute(db,"COMMIT;");
    } catch(...) {
        sqlite3_exec(db,"ROLLBACK;",nullptr,nullptr,nullptr);
        throw;
    }
}
