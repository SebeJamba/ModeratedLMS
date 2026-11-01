#pragma once
#include "Database.hpp"
#include <string>

// CR1 persistence boundary. Deadlines use sortable local ISO timestamps.
class DeadlineRepository {
    Database& database;
public:
    explicit DeadlineRepository(Database& db) : database(db) {}
    void initialize();
    void extendClass(int lecturerID, int assignmentID, const std::string& deadline);
    void extendIndividual(int lecturerID, int assignmentID, int studentID, const std::string& deadline);
    std::string effectiveDeadline(int assignmentID, int studentID);
};
