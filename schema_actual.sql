-- Current schema for a new database. Existing files are upgraded additively by Database.cpp and DeadlineRepository.cpp.
PRAGMA foreign_keys=ON;
CREATE TABLE Assignments (  assignmentID INTEGER PRIMARY KEY AUTOINCREMENT,  title TEXT NOT NULL,  deadline TEXT NOT NULL,  maxMark REAL NOT NULL, courseID TEXT REFERENCES Courses(courseID));
CREATE TABLE AuditLog (  logID INTEGER PRIMARY KEY AUTOINCREMENT,  actorID INTEGER,  timestamp TEXT,  oldValue TEXT,  newValue TEXT,  reason TEXT, submissionID INTEGER REFERENCES Submissions(submissionID) ON DELETE RESTRICT,  FOREIGN KEY(actorID) REFERENCES Users(userID));
CREATE TABLE ClassExtensions(assignmentID INTEGER PRIMARY KEY REFERENCES Assignments(assignmentID),extendedDeadline TEXT NOT NULL,actorID INTEGER NOT NULL REFERENCES Users(userID),changedAt TEXT NOT NULL);
CREATE TABLE Courses(courseID TEXT PRIMARY KEY,courseName TEXT NOT NULL,departmentID INTEGER NOT NULL REFERENCES Departments(departmentID), lecturerID INTEGER REFERENCES Users(userID));
CREATE TABLE Departments (  departmentID INTEGER PRIMARY KEY AUTOINCREMENT,  departmentName TEXT NOT NULL);
CREATE TABLE Enrollments(studentID INTEGER NOT NULL REFERENCES Users(userID),courseID TEXT NOT NULL REFERENCES Courses(courseID),PRIMARY KEY(studentID,courseID));
CREATE TABLE Extensions(extensionID INTEGER PRIMARY KEY AUTOINCREMENT,assignmentID INTEGER NOT NULL REFERENCES Assignments(assignmentID),studentID INTEGER NOT NULL REFERENCES Users(userID),extendedDeadline TEXT NOT NULL,UNIQUE(assignmentID,studentID));
CREATE TABLE SchemaMigrations(version INTEGER PRIMARY KEY, appliedAt TEXT NOT NULL);
CREATE TABLE Submissions (  submissionID INTEGER PRIMARY KEY AUTOINCREMENT,  assignmentID INTEGER,  studentID INTEGER,  filePath TEXT,  submissionTime TEXT,  status TEXT,  rawMark REAL,  finalMark REAL, feedback TEXT, isActive INTEGER NOT NULL DEFAULT 1 CHECK(isActive IN (0,1)),  FOREIGN KEY(assignmentID) REFERENCES Assignments(assignmentID),  FOREIGN KEY(studentID) REFERENCES Users(userID));
CREATE TABLE Users (  userID INTEGER PRIMARY KEY AUTOINCREMENT,  username TEXT UNIQUE NOT NULL,  passwordHash TEXT NOT NULL,  role TEXT CHECK(role IN ('ADMIN','LECTURER','HOD','STUDENT')) NOT NULL,  departmentID INTEGER,  FOREIGN KEY(departmentID) REFERENCES Departments(departmentID));
CREATE UNIQUE INDEX one_active_submission ON Submissions(assignmentID,studentID) WHERE isActive=1;
CREATE TRIGGER immutable_audit_delete BEFORE DELETE ON AuditLog BEGIN SELECT RAISE(ABORT,'Audit records are permanent'); END;
CREATE TRIGGER immutable_audit_update BEFORE UPDATE ON AuditLog BEGIN SELECT RAISE(ABORT,'Audit records are permanent'); END;
