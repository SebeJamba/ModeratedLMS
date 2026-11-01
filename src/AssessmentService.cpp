#include "AssessmentService.hpp"
#include "DeadlineRepository.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace {
class Query {
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement{nullptr, sqlite3_finalize};

  public:
    Query(sqlite3 *db, const char *sql) {
        sqlite3_stmt *raw = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
            sqlite3_finalize(raw);
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        statement.reset(raw);
    }
    void bind(int i, int v) {
        if (sqlite3_bind_int(statement.get(), i, v) != SQLITE_OK)
            throw std::runtime_error("Cannot bind ID");
    }
    void bind(int i, double v) {
        if (sqlite3_bind_double(statement.get(), i, v) != SQLITE_OK)
            throw std::runtime_error("Cannot bind mark");
    }
    void bind(int i, const std::string &v) {
        if (sqlite3_bind_text(statement.get(), i, v.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
            throw std::runtime_error("Cannot bind text");
    }
    bool next() {
        int rc = sqlite3_step(statement.get());
        if (rc == SQLITE_ROW)
            return true;
        if (rc == SQLITE_DONE)
            return false;
        throw std::runtime_error(sqlite3_errmsg(sqlite3_db_handle(statement.get())));
    }
    void run() {
        if (next())
            throw std::runtime_error("Unexpected query result");
    }
    std::string text(int i) const {
        auto p = sqlite3_column_text(statement.get(), i);
        return p ? reinterpret_cast<const char *>(p) : "";
    }
    int integer(int i) const { return sqlite3_column_int(statement.get(), i); }
    double number(int i) const { return sqlite3_column_double(statement.get(), i); }
    bool null(int i) const { return sqlite3_column_type(statement.get(), i) == SQLITE_NULL; }
};
class Transaction {
    sqlite3 *db;
    bool committed = false;

  public:
    explicit Transaction(sqlite3 *value) : db(value) { Query(db, "BEGIN IMMEDIATE").run(); }
    void commit() {
        Query(db, "COMMIT").run();
        committed = true;
    }
    ~Transaction() {
        if (!committed)
            sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    }
};
void requireText(const std::string &value) {
    if (value.find_first_not_of(" \t\r\n") == std::string::npos)
        throw std::invalid_argument("Text must not be blank");
}
void requireRole(sqlite3 *db, int actor, const std::string &role) {
    Query q(db, "SELECT 1 FROM Users WHERE userID=? AND role=?");
    q.bind(1, actor);
    q.bind(2, role);
    if (!q.next())
        throw std::runtime_error("Access denied for this role");
}
struct Assessment {
    int assignment, student, lecturer, department;
    double maximum;
    std::string submitted, status, old;
};
Assessment load(sqlite3 *db, int id) {
    Query q(db, "SELECT "
                "s.assignmentID,s.studentID,c.lecturerID,c.departmentID,a.maxMark,s.submissionTime,"
                "s.status,s.finalMark "
                "FROM Submissions s JOIN Assignments a ON a.assignmentID=s.assignmentID JOIN "
                "Courses c ON c.courseID=a.courseID "
                "WHERE s.submissionID=? AND s.isActive=1");
    q.bind(1, id);
    if (!q.next())
        throw std::runtime_error("Active course-linked submission not found");
    return {q.integer(0), q.integer(1), q.integer(2), q.integer(3),
            q.number(4),  q.text(5),    q.text(6),    q.null(7) ? "" : std::to_string(q.number(7))};
}
void checkHod(sqlite3 *db, int actor, int department) {
    Query q(db, "SELECT 1 FROM Users WHERE userID=? AND role='HOD' AND departmentID=?");
    q.bind(1, actor);
    q.bind(2, department);
    if (!q.next())
        throw std::runtime_error("Only this department's HoD may perform that action");
}
void audit(sqlite3 *db, int actor, int submission, const std::string &time, const std::string &old,
           double value, const std::string &reason) {
    Query q(db, "INSERT INTO AuditLog(actorID,submissionID,timestamp,oldValue,newValue,reason) "
                "VALUES(?,?,?,?,?,?)");
    q.bind(1, actor);
    q.bind(2, submission);
    q.bind(3, time);
    q.bind(4, old);
    q.bind(5, std::to_string(value));
    q.bind(6, reason);
    q.run();
}
} // namespace
std::string currentTimestamp() {
    auto now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    if (localtime_s(&local, &now) != 0)
        throw std::runtime_error("Cannot read current time");
#else
    if (!localtime_r(&now, &local))
        throw std::runtime_error("Cannot read current time");
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return out.str();
}
long long timestampSeconds(const std::string &value) {
    std::tm parsed{};
    std::istringstream input(value);
    input >> std::get_time(&parsed, "%Y-%m-%d %H:%M:%S");
    if (input.fail() || value.size() != 19)
        throw std::invalid_argument("Use YYYY-MM-DD HH:MM:SS");
#ifdef _WIN32
    auto seconds = _mkgmtime64(&parsed);
#else
    auto seconds = timegm(&parsed);
#endif
    std::ostringstream normalized;
    normalized << std::put_time(&parsed, "%Y-%m-%d %H:%M:%S");
    if (seconds < 0 || normalized.str() != value)
        throw std::invalid_argument("Invalid calendar date");
    return static_cast<long long>(seconds);
}
double StandardLatePenalty::finalMark(double raw, double maximum, long long seconds) const {
    if (!std::isfinite(raw) || !std::isfinite(maximum) || maximum <= 0 || raw < 0 || raw > maximum)
        throw std::invalid_argument("Mark must be between zero and the assignment maximum");
    if (seconds <= 0)
        return raw;
    if (seconds > 3 * 86400)
        return 0;
    const long long days = (seconds + 86399) / 86400;
    return std::max(0.0, raw - 0.10 * maximum * static_cast<double>(days));
}
int AssessmentService::submit(int student, int assignment, const std::string &path) {
    requireText(path);
    auto db = database.getHandle();
    Transaction tx(db);
    requireRole(db, student, "STUDENT");
    Query allowed(db, "SELECT 1 FROM Assignments a JOIN Enrollments e ON e.courseID=a.courseID "
                      "WHERE a.assignmentID=? AND e.studentID=?");
    allowed.bind(1, assignment);
    allowed.bind(2, student);
    if (!allowed.next())
        throw std::runtime_error("Student is not enrolled in this assignment's course");
    const auto now = clock();
    const auto cutoff = DeadlineRepository(database).effectiveDeadline(assignment, student);
    const auto late = timestampSeconds(now) - timestampSeconds(cutoff);
    Query active(db, "SELECT submissionID,status FROM Submissions WHERE assignmentID=? AND "
                     "studentID=? AND isActive=1");
    active.bind(1, assignment);
    active.bind(2, student);
    int id = 0;
    if (active.next()) {
        id = active.integer(0);
        if (late >= 0 || active.text(1) != "SUBMITTED")
            throw std::runtime_error(
                "Resubmission is only allowed before the effective deadline on unmarked work");
        Query update(db, "UPDATE Submissions SET filePath=?,submissionTime=?,status='SUBMITTED' "
                         "WHERE submissionID=?");
        update.bind(1, path);
        update.bind(2, now);
        update.bind(3, id);
        update.run();
    } else {
        Query insert(
            db, "INSERT INTO Submissions(assignmentID,studentID,filePath,submissionTime,status) "
                "VALUES(?,?,?,?,?)");
        insert.bind(1, assignment);
        insert.bind(2, student);
        insert.bind(3, path);
        insert.bind(4, now);
        insert.bind(5, std::string(late > 0 ? "LATE" : "SUBMITTED"));
        insert.run();
        id = static_cast<int>(sqlite3_last_insert_rowid(db));
    }
    tx.commit();
    return id;
}
void AssessmentService::mark(int lecturer, int submission, double raw, const std::string &feedback,
                             const std::string &reason) {
    requireText(feedback);
    requireText(reason);
    auto db = database.getHandle();
    Transaction tx(db);
    requireRole(db, lecturer, "LECTURER");
    auto a = load(db, submission);
    if (a.lecturer != lecturer)
        throw std::runtime_error("Only the assigned lecturer may mark this work");
    const auto cutoff = DeadlineRepository(database).effectiveDeadline(a.assignment, a.student);
    const auto final =
        penalty.finalMark(raw, a.maximum, timestampSeconds(a.submitted) - timestampSeconds(cutoff));
    Query q(db, "UPDATE Submissions SET rawMark=?,finalMark=?,feedback=?,status='MARKED' WHERE "
                "submissionID=?");
    q.bind(1, raw);
    q.bind(2, final);
    q.bind(3, feedback);
    q.bind(4, submission);
    q.run();
    audit(db, lecturer, submission, clock(), a.old, final, reason);
    tx.commit();
}
void AssessmentService::moderate(int hod, int submission, double final, const std::string &reason) {
    requireText(reason);
    auto db = database.getHandle();
    Transaction tx(db);
    auto a = load(db, submission);
    checkHod(db, hod, a.department);
    if (a.status != "MARKED")
        throw std::runtime_error("Only MARKED submissions can be moderated");
    penalty.finalMark(final, a.maximum, 0);
    Query q(db, "UPDATE Submissions SET finalMark=?,status='MODERATED' WHERE submissionID=?");
    q.bind(1, final);
    q.bind(2, submission);
    q.run();
    audit(db, hod, submission, clock(), a.old, final, reason);
    tx.commit();
}
void AssessmentService::release(int hod, int submission) {
    auto db = database.getHandle();
    Transaction tx(db);
    auto a = load(db, submission);
    checkHod(db, hod, a.department);
    if (a.status != "MODERATED")
        throw std::runtime_error("Only MODERATED submissions can be released");
    Query q(db, "UPDATE Submissions SET status='RELEASED' WHERE submissionID=?");
    q.bind(1, submission);
    q.run();
    audit(db, hod, submission, clock(), a.old, std::stod(a.old), "Release approved marks");
    tx.commit();
}
std::vector<SubmissionView> AssessmentService::studentSubmissions(int student) {
    auto db = database.getHandle();
    requireRole(db, student, "STUDENT");
    Query q(db,
            "SELECT s.submissionID,a.title,s.submissionTime,s.status,CASE WHEN s.status='RELEASED' "
            "THEN s.finalMark END,"
            "CASE WHEN s.status='RELEASED' THEN s.feedback END FROM Submissions s JOIN Assignments "
            "a ON a.assignmentID=s.assignmentID WHERE s.studentID=? AND s.isActive=1");
    q.bind(1, student);
    std::vector<SubmissionView> result;
    while (q.next())
        result.push_back({q.integer(0), q.text(1), q.text(2), q.text(3),
                          q.null(4) ? "Pending" : std::to_string(q.number(4)), q.text(5)});
    return result;
}
std::vector<SubmissionView> AssessmentService::departmentSubmissions(int hod) {
    auto db = database.getHandle();
    requireRole(db, hod, "HOD");
    Query q(db, "SELECT s.submissionID,a.title,s.submissionTime,s.status,s.finalMark,s.feedback "
                "FROM Submissions s JOIN Assignments a ON a.assignmentID=s.assignmentID "
                "JOIN Courses c ON c.courseID=a.courseID JOIN Users u ON "
                "u.departmentID=c.departmentID WHERE u.userID=? AND s.isActive=1");
    q.bind(1, hod);
    std::vector<SubmissionView> result;
    while (q.next())
        result.push_back({q.integer(0), q.text(1), q.text(2), q.text(3),
                          q.null(4) ? "Pending" : std::to_string(q.number(4)), q.text(5)});
    return result;
}
std::vector<std::string> AssessmentService::departmentAudit(int hod) {
    auto db = database.getHandle();
    requireRole(db, hod, "HOD");
    Query q(db,
            "SELECT l.logID,l.actorID,l.timestamp,l.oldValue,l.newValue,l.reason,l.submissionID "
            "FROM AuditLog l JOIN Submissions s ON s.submissionID=l.submissionID "
            "JOIN Assignments a ON a.assignmentID=s.assignmentID JOIN Courses c ON "
            "c.courseID=a.courseID JOIN Users u ON u.departmentID=c.departmentID WHERE u.userID=? "
            "ORDER BY l.logID");
    q.bind(1, hod);
    std::vector<std::string> rows;
    while (q.next())
        rows.push_back("LogID: " + q.text(0) + " | Actor: " + q.text(1) + " | Time: " + q.text(2) +
                       " | Old: " + q.text(3) + " | New: " + q.text(4) + " | Reason: " + q.text(5) +
                       " | Submission: " + q.text(6));
    return rows;
}
