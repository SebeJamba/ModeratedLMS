#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include "sqlite3.h"

class Database {
private:
    sqlite3* db;
public:
    Database(const std::string& dbName);
    ~Database();
    sqlite3* getHandle() const;
    void initializeSchema();
};

#endif // DATABASE_HPP