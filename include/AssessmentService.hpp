#pragma once
#include "Database.hpp"
#include <functional>
#include <string>
#include <vector>

std::string currentTimestamp();
long long timestampSeconds(const std::string &timestamp);

class ILatePenaltyCalculator {
  public:
    virtual ~ILatePenaltyCalculator() = default;
    virtual double finalMark(double raw, double maximum, long long lateSeconds) const = 0;
};
class StandardLatePenalty final : public ILatePenaltyCalculator {
  public:
    double finalMark(double raw, double maximum, long long lateSeconds) const override;
};
struct SubmissionView {
    int id;
    std::string title, timestamp, status, mark, feedback;
};

// Assessment operations own their SQL and transactions. Console code cannot bypass
// role/course checks or the release gate through these operations.
class AssessmentService {
    Database &database;
    std::function<std::string()> clock;
    StandardLatePenalty penalty;

  public:
    explicit AssessmentService(Database &db, std::function<std::string()> now = currentTimestamp)
        : database(db), clock(now) {}
    int submit(int student, int assignment, const std::string &path);
    void mark(int lecturer, int submission, double raw, const std::string &feedback,
              const std::string &reason);
    void moderate(int hod, int submission, double final, const std::string &reason);
    void release(int hod, int submission);
    std::vector<SubmissionView> studentSubmissions(int student);
    std::vector<SubmissionView> departmentSubmissions(int hod);
    std::vector<std::string> departmentAudit(int hod);
};
