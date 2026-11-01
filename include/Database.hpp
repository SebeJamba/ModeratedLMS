#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "sqlite3.h"
#include <string>

class Database {
  private:
    sqlite3 *db = nullptr;

  public:
    Database(const std::string &dbName);
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;
    sqlite3 *getHandle() const;
    void initializeSchema();
};

#endif // DATABASE_HPP