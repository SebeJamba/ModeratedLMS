cmake_minimum_required(VERSION 3.15)
project(ModeratedLMS)

set(CMAKE_CXX_STANDARD 17)

# Add your project source files along with sqlite3.c
add_executable(ModeratedLMS
    src/main.cpp
    src/Database.cpp
    src/Security.cpp
    src/User.cpp
    include/sqlite3/sqlite3.c   # <-- Include the SQLite amalgamation source here
)

# Tell the compiler where to find sqlite3.h
target_include_directories(ModeratedLMS PRIVATE
    include
    include/sqlite3             # <-- So Database.hpp can find #include "sqlite3.h" or <sqlite3.h>
)