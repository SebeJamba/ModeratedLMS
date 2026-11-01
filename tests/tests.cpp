#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "Database.hpp"
#include "Security.hpp"
#include "User.hpp"
#ifdef LMS_CR1
#include "DeadlineRepository.hpp"
#endif
#include <sstream>
#include <fstream>
#include <cstdio>
#include <chrono>

struct Fixture {
    Database db{":memory:"};
    Fixture() {
        db.initializeSchema();
#ifdef LMS_CR1
        DeadlineRepository(db).initialize();
#endif
        sql("INSERT OR REPLACE INTO Departments VALUES(1,'Computing');");
        sql("INSERT INTO Users VALUES(1,'admin','test','ADMIN',1),(2,'lecturer','test','LECTURER',1),"
            "(3,'hod','test','HOD',1),(4,'student','test','STUDENT',1),(5,'other','test','LECTURER',1),"
            "(6,'student2','test','STUDENT',1);");
        sql("INSERT INTO Assignments(assignmentID,title,deadline,maxMark) VALUES(1,'Essay','2026-11-01 12:00:00',100);");
#ifdef LMS_CR1
        sql("INSERT INTO Courses VALUES('CS4433','Software engineering',1,2);"
            "INSERT INTO Enrollments VALUES(4,'CS4433'),(6,'CS4433');"
            "UPDATE Assignments SET courseID='CS4433';");
#endif
    }
    void sql(const std::string& value) {
        char* error=nullptr;
        int status=sqlite3_exec(db.getHandle(),value.c_str(),nullptr,nullptr,&error);
        std::string message=error?error:""; sqlite3_free(error);
        if(status!=SQLITE_OK) throw std::runtime_error(message);
    }
    std::string scalar(const std::string& sql) {
        sqlite3_stmt* statement=nullptr;
        if(sqlite3_prepare_v2(db.getHandle(),sql.c_str(),-1,&statement,nullptr)!=SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db.getHandle()));
        std::string value;
        if(sqlite3_step(statement)==SQLITE_ROW && sqlite3_column_type(statement,0)!=SQLITE_NULL)
            value=reinterpret_cast<const char*>(sqlite3_column_text(statement,0));
        sqlite3_finalize(statement);return value;
    }
    std::string menu(const std::string& role,const std::string& commands,int id) {
        std::istringstream input(commands);std::ostringstream output;
        auto oldInput=std::cin.rdbuf(input.rdbuf());auto oldOutput=std::cout.rdbuf(output.rdbuf());
        try {
            UserFactory::createUser(id,"test","test",role,1)->displayMenu(db);
        } catch(...) {
            std::cin.rdbuf(oldInput);std::cout.rdbuf(oldOutput);std::cin.clear();throw;
        }
        std::cin.rdbuf(oldInput);std::cout.rdbuf(oldOutput);std::cin.clear();return output.str();
    }
    void submission() {sql("INSERT INTO Submissions(submissionID,assignmentID,studentID,filePath,submissionTime,status) "
        "VALUES(1,1,4,'essay.txt','2026-11-01 12:00:01','LATE');");}
};
TEST_CASE("UT01 factory creates all four polymorphic roles","[unit]") {
    for(auto role:{"ADMIN","LECTURER","HOD","STUDENT"}) {
        auto user=UserFactory::createUser(7,"name","hash",role,1);
        CHECK(user->getRoleName()==role);CHECK(user->getUserID()==7);CHECK(user->getUsername()=="name");
    }
}
TEST_CASE("UT02 unknown role is rejected","[unit]") {
    CHECK_THROWS_AS(UserFactory::createUser(1,"x","y","SUPERUSER",1),std::invalid_argument);
}
TEST_CASE("UT03 password round trip and rejection","[unit]") {
    auto stored=hashPassword("Secret123!",generateSalt());
    CHECK(verifyPassword("Secret123!",stored));CHECK_FALSE(verifyPassword("wrong",stored));
    CHECK_FALSE(verifyPassword("Secret123!","malformed"));
}
TEST_CASE("IT01 schema creation is repeatable and foreign keys are enabled","[integration]") {
    Fixture f;CHECK_NOTHROW(f.db.initializeSchema());CHECK(f.scalar("PRAGMA foreign_keys;")=="1");
}
TEST_CASE("IT02 username and role constraints reject invalid records","[integration]") {
    Fixture f;CHECK_THROWS(f.sql("INSERT INTO Users VALUES(9,'student','x','STUDENT',1);"));
    CHECK_THROWS(f.sql("INSERT INTO Users VALUES(9,'unique','x','ROOT',1);"));
}
TEST_CASE("IT03 invalid foreign key rejected","[integration]") {
    Fixture f;CHECK_THROWS(f.sql("INSERT INTO Submissions(assignmentID,studentID) VALUES(999,4);"));
}
TEST_CASE("IT04 data survives database reopening","[integration]") {
    const char* name="persistence-test.db";std::remove(name);
    {Database db(name);db.initializeSchema();REQUIRE(sqlite3_exec(db.getHandle(),"UPDATE Departments SET departmentName='Science' WHERE departmentID=1;",nullptr,nullptr,nullptr)==SQLITE_OK);}
    {Database db(name);sqlite3_stmt* p=nullptr;REQUIRE(sqlite3_prepare_v2(db.getHandle(),"SELECT departmentName FROM Departments WHERE departmentID=1;",-1,&p,nullptr)==SQLITE_OK);
    REQUIRE(sqlite3_step(p)==SQLITE_ROW);CHECK(std::string(reinterpret_cast<const char*>(sqlite3_column_text(p,0)))=="Science");sqlite3_finalize(p);}
    std::remove(name);
}
TEST_CASE("ST01 student submits existing assignment","[system]") {
    Fixture f;auto output=f.menu("STUDENT","2\n1\nessay.txt\n4\n",4);
    CHECK(output.find("submitted successfully")!=std::string::npos);
    CHECK(f.scalar("SELECT COUNT(*) FROM Submissions WHERE studentID=4;")=="1");
}
TEST_CASE("ST02 lecturer records mark","[system]") {
    Fixture f;f.submission();f.menu("LECTURER","2\n1\n80\n3\n",2);
    CHECK(f.scalar("SELECT rawMark FROM Submissions WHERE submissionID=1;")=="80.0");
}
TEST_CASE("ST03 administrator creates a valid user","[system]") {
    Fixture f;f.menu("ADMIN","1\nnewstudent\nSecret123!\nSTUDENT\n1\n3\n",1);
    CHECK(f.scalar("SELECT role FROM Users WHERE username='newstudent';")=="STUDENT");
    CHECK(f.scalar("SELECT passwordHash FROM Users WHERE username='newstudent';")!="Secret123!");
}
TEST_CASE("ST04 all role menus can log out","[system]") {
    Fixture f;CHECK_FALSE(f.menu("ADMIN","2\n3\n",1).empty());
    CHECK_FALSE(f.menu("LECTURER","3\n",2).empty());
    CHECK_FALSE(f.menu("HOD","1\n3\n4\n",3).empty());
    CHECK_FALSE(f.menu("STUDENT","1\n3\n4\n",4).empty());
}
TEST_CASE("CF01 FR10 marks stay hidden before release","[known-defect][system]") {
    Fixture f;f.submission();f.menu("LECTURER","2\n1\n80\n3\n",2);
    auto output=f.menu("STUDENT","3\n4\n",4);CHECK(output.find("80.000000")==std::string::npos);
}
TEST_CASE("CF02 FR11 lecturer mark change creates audit record","[known-defect][system]") {
    Fixture f;f.submission();f.menu("LECTURER","2\n1\n80\n3\n",2);
    CHECK(f.scalar("SELECT COUNT(*) FROM AuditLog;")=="1");
}
TEST_CASE("CF03 FR08 marks above maximum are rejected","[known-defect][system]") {
    Fixture f;f.submission();f.menu("LECTURER","2\n1\n101\n3\n",2);
    CHECK(f.scalar("SELECT rawMark IS NULL FROM Submissions WHERE submissionID=1;")=="1");
}
TEST_CASE("CF04 FR07 one partial late day deducts ten available marks","[known-defect][system]") {
    Fixture f;f.submission();f.menu("LECTURER","2\n1\n80\n3\n",2);
    CHECK(f.scalar("SELECT finalMark FROM Submissions WHERE submissionID=1;")=="70.0");
}
TEST_CASE("CF05 FR05 submission records current date","[known-defect][system]") {
    Fixture f;f.menu("STUDENT","2\n1\nessay.txt\n4\n",4);
    CHECK(f.scalar("SELECT date(submissionTime)=date('now','localtime') FROM Submissions;")=="1");
}
TEST_CASE("CF06 FR06 resubmission updates active attempt","[known-defect][system]") {
    Fixture f;f.menu("STUDENT","2\n1\nfirst.txt\n2\n1\nsecond.txt\n4\n",4);
    CHECK(f.scalar("SELECT COUNT(*) FROM Submissions WHERE assignmentID=1 AND studentID=4;")=="1");
}
TEST_CASE("CF07 FR08 written feedback column exists","[known-defect][integration]") {
    Fixture f;CHECK(f.scalar("SELECT COUNT(*) FROM pragma_table_info('Submissions') WHERE name='feedback';")=="1");
}
TEST_CASE("CF08 FR11 audit records identify the changed submission","[known-defect][integration]") {
    Fixture f;CHECK(f.scalar("SELECT COUNT(*) FROM pragma_table_info('AuditLog') WHERE name='submissionID';")=="1");
}
TEST_CASE("CF09 NFR02 stored hash identifies a recognised password KDF","[known-defect][unit]") {
    auto hash=hashPassword("Secret123!",generateSalt());
    CHECK((hash.find("$argon2")==0 || hash.find("pbkdf2$")==0 || hash.find("$2b$")==0));
}
TEST_CASE("CF10 FR08 marking uses the designed MARKED state","[known-defect][system]") {
    Fixture f;f.submission();f.menu("LECTURER","2\n1\n80\n3\n",2);
    CHECK(f.scalar("SELECT status FROM Submissions WHERE submissionID=1;")=="MARKED");
}
#ifdef LMS_CR1
TEST_CASE("QA01 moderation copies temporary mark strings safely","[quality][integration]") {
    Fixture f;f.submission();f.sql("UPDATE Submissions SET finalMark=80 WHERE submissionID=1;");
    f.menu("HOD","2\n1\n75\nreview\n4\n",3);
    CHECK(f.scalar("SELECT oldValue FROM AuditLog LIMIT 1;")=="80.000000");
    CHECK(f.scalar("SELECT newValue FROM AuditLog LIMIT 1;")=="75.000000");
}
TEST_CASE("MT01 FR12 class extension affects every enrolled student","[maintenance][integration]") {
    Fixture f;DeadlineRepository r(f.db);r.extendClass(2,1,"2026-11-03 12:00:00");
    CHECK(r.effectiveDeadline(1,4)=="2026-11-03 12:00:00");CHECK(r.effectiveDeadline(1,6)=="2026-11-03 12:00:00");
    CHECK(f.scalar("SELECT COUNT(*) FROM Extensions;")=="2");
}
TEST_CASE("MT02 longer individual extension is preserved","[maintenance][integration]") {
    Fixture f;DeadlineRepository r(f.db);r.extendIndividual(2,1,4,"2026-11-05 12:00:00");
    r.extendClass(2,1,"2026-11-03 12:00:00");CHECK(r.effectiveDeadline(1,4)=="2026-11-05 12:00:00");
    CHECK(r.effectiveDeadline(1,6)=="2026-11-03 12:00:00");
}
TEST_CASE("MT03 new enrolment inherits durable class deadline","[maintenance][integration]") {
    Fixture f;DeadlineRepository r(f.db);r.extendClass(2,1,"2026-11-03 12:00:00");
    f.sql("INSERT INTO Users VALUES(7,'later','x','STUDENT',1);INSERT INTO Enrollments VALUES(7,'CS4433');");
    CHECK(r.effectiveDeadline(1,7)=="2026-11-03 12:00:00");
}
TEST_CASE("MT04 unauthorised class change rejected without mutation","[maintenance][integration]") {
    Fixture f;DeadlineRepository r(f.db);
    for(int actor:{1,3,4,5}) CHECK_THROWS(r.extendClass(actor,1,"2026-11-03 12:00:00"));
    CHECK(f.scalar("SELECT COUNT(*) FROM Extensions;")=="0");CHECK(f.scalar("SELECT COUNT(*) FROM ClassExtensions;")=="0");
}
TEST_CASE("MT05 equal earlier and invalid dates rejected","[maintenance][unit]") {
    Fixture f;DeadlineRepository r(f.db);
    for(auto date:{"2026-11-01 12:00:00","2026-10-31 12:00:00","2026-02-30 12:00:00","invalid","2026-13-01 12:00:00","2026-11-03 24:00:00","2026-11-03 12:60:00","2026-11-03 12:00:60","2027-02-29 12:00:00","1969-11-03 12:00:00","2026-00-03 12:00:00","2026-11-00 12:00:00"})
        CHECK_THROWS(r.extendClass(2,1,date));
    CHECK_NOTHROW(r.extendClass(2,1,"2028-02-29 12:00:00"));
}
TEST_CASE("MT06 class update rolls back on a database error","[maintenance][integration]") {
    Fixture f;DeadlineRepository r(f.db);
    f.sql("CREATE TRIGGER reject_extension BEFORE INSERT ON Extensions WHEN NEW.studentID=6 BEGIN SELECT RAISE(ABORT,'test failure'); END;");
    CHECK_THROWS(r.extendClass(2,1,"2026-11-03 12:00:00"));
    CHECK(f.scalar("SELECT COUNT(*) FROM ClassExtensions;")=="0");CHECK(f.scalar("SELECT COUNT(*) FROM Extensions;")=="0");
}
TEST_CASE("MT07 student list shows effective deadline","[maintenance][system]") {
    Fixture f;DeadlineRepository(f.db).extendClass(2,1,"2026-11-03 12:00:00");
    CHECK(f.menu("STUDENT","1\n4\n",4).find("Effective deadline: 2026-11-03 12:00:00")!=std::string::npos);
}
TEST_CASE("MT08 lecturer menu performs class extension","[maintenance][system]") {
    Fixture f;f.menu("LECTURER","4\n1\n2026-11-03 12:00:00\n3\n",2);
    CHECK(DeadlineRepository(f.db).effectiveDeadline(1,4)=="2026-11-03 12:00:00");
}
TEST_CASE("MT09 migration is repeatable and leaves submission data intact","[maintenance][integration]") {
    Fixture f;f.submission();CHECK_NOTHROW(DeadlineRepository(f.db).initialize());
    CHECK(f.scalar("SELECT COUNT(*) FROM Submissions;")=="1");
    CHECK(f.scalar("SELECT COUNT(*) FROM SchemaMigrations;")=="1");
}
TEST_CASE("MT10 unknown assignment and unenrolled individual rejected","[maintenance][integration]") {
    Fixture f;DeadlineRepository r(f.db);CHECK_THROWS(r.extendClass(2,999,"2026-11-03 12:00:00"));
    CHECK_THROWS(r.effectiveDeadline(999,4));CHECK_THROWS(r.extendIndividual(2,1,5,"2026-11-03 12:00:00"));
}
TEST_CASE("MT11 base and individual deadline boundaries","[maintenance][integration]") {
    Fixture f;DeadlineRepository r(f.db);CHECK(r.effectiveDeadline(1,4)=="2026-11-01 12:00:00");
    CHECK_THROWS(r.extendIndividual(2,1,4,"2026-11-01 12:00:00"));
    CHECK_NOTHROW(r.extendIndividual(2,1,4,"2026-11-01 12:00:01"));
    CHECK(r.effectiveDeadline(1,4)=="2026-11-01 12:00:01");
}
#endif
