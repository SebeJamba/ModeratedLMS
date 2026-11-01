Historical baseline report. For the corrected branch, see 06_Fixed_Requirements.md and assessment-evidence/fixes.

# Phase 7 Deployment and User Guides

## Deployment scope and status

The UML deployment diagram shows a Windows x64 school workstation, Windows execution environment, console executable and local SQLite database, plus an external backup device. Git/CI are development infrastructure, not runtime dependencies. This supports one offline computer. Concurrent local-network use has not been tested or deployed.

Review executables and source are supplied. The gate decision is HOLD because moderation/release, penalties, feedback, password security and robustness remain incomplete. These instructions support a synthetic demonstration and a subsequent approved release; no school installation is claimed.

## Technician installation guide

1. Use a Windows x64 workstation and a dedicated folder such as C:\SchoolLMS. Verify package checksums. Use synthetic accounts/results for this review. The supplied MSVC review executable needs no external compiler-runtime DLL; BCrypt is part of Windows.
2. Put the selected version's ModeratedLMS.exe in its own folder. Keep v1.0 and v1.1 separate. Keep a source backup. The database is lms.db in the working directory, so launch from the same application folder each time.
3. Start the program. First run creates the database, Computer Science/Physics/Mathematics departments and administrator. The inherited main routine also inserts another Computer Science row; use department ID 1 in the demonstration. Login is admin / Admin123!. The program has no password-change facility; that is a release blocker, not an operational password policy.
4. For v1.1, Administrator option 1 creates lecturers, HoD and students in department 1. Option 2 shows their IDs. Option 4 creates a course and allocates its lecturer. Option 5 enrols each student.
5. Old v1.0 assignments lack course links. Administrator option 6 maps each existing assignment to its reviewed course. Until mapped, it is omitted from enrolled-student lists and extension authorisation fails. Do not guess the mapping.
6. As assigned lecturer, create an assignment with course code, a one-word title supported by the legacy input routine, YYYY-MM-DD HH:MM:SS deadline and a positive maximum mark. Option 4 grants class extension; option 5 grants an individual extension. Enter a later timestamp.
7. As enrolled student, option 1 must show the effective deadline. Check a longer individual override if applicable. Retain actual smoke-test output.
8. Exit before backing up lms.db. Also copy referenced work files: the program stores paths and does not ingest those files. Keep dated backups/checksums and test restoration in a separate folder. An operational school restore drill has not yet been executed.

For source builds, install Visual Studio C++ desktop tools, Windows SDK and CMake. In a developer terminal run cmake -S . -B build -A x64, then cmake --build build --config Debug. Run ctest --test-dir build -C Debug --output-on-failure before a release. Ten requirement checks currently fail; repair the defects rather than remove tests.

## Administrator manual

Option 1 registers username, password, valid role and existing department ID. Option 2 lists account IDs/roles/departments. In v1.1 option 4 creates a course and lecturer allocation; option 5 enrols a student; option 6 links an old assignment to a course. Option 3 logs out.

Complete account update/deactivation and department management required by FR02 are absent. Some validation is limited. These are outstanding features, not available procedures.

## Lecturer manual

Option 1 creates assignments. v1.1 asks for an allocated course code and full timestamp. Option 2 records a numeric mark. In v1.1 option 4 extends a class deadline and option 5 extends an individual's deadline. Option 3 logs out.

The new class date must be later than the current class/base date. Longer individual dates remain. Later enrollees inherit the class override. Invalid dates, unauthorised requests and transaction failures produce errors. Grading remains incomplete: no written feedback, mark-range enforcement or late penalty, and the legacy status is GRADED rather than MARKED.

## Head of Department manual

Option 1 lists submissions, option 2 records a moderated mark and one-word reason, option 3 lists audit rows and option 4 logs out. v1.1 fixes temporary-string lifetime in audit insertion. The list is not restricted to the HoD's department and no separate release action exists. The current menu cannot perform full UC12/UC13 and must not authorise real publication.

Required future procedure: review only department submissions in MARKED, approve/adjust with a reason, verify audit/MODERATED state, explicitly release and check student visibility. This describes required future functionality rather than an existing menu option.

## Student manual

Option 1 lists assignments. v1.1 lists enrolled courses' assignments and effective deadlines. Option 2 records an assignment ID and file-path token; the program does not transfer file contents. Option 3 shows submissions/marks; option 4 logs out.

Submission/resubmission rules are incomplete, time is hard-coded and marks appear before release. A second submission creates another row. These failures are recorded in tests; displayed grades must not be treated as authorised school results.

## v1.0 review release notes

Contains the supplied four-role console program, account creation, assignment creation, file-path submission, numeric grading and basic moderation/audit. Preparation adds the SQLite include-path correction, CMake test target, Catch2 tests and CI. Eleven test cases pass; ten requirement checks fail. This is a local baseline, not evidence of a prior school release.

Limitations include premature marks, no separate release, no penalties/feedback, incomplete audit/schema/admin workflows, unsafe password hashing, fixed time, poor error handling and absent layered-domain design. Gate 3 is not passed.

## v1.1 review release notes

Adds CR1, its repository, prerequisite course/enrolment setup, mapping of old assignments, individual/class extensions, atomic bulk change, longer-individual precedence, later-enrolment inheritance, student deadline display, migration tracking and the audit binding fix. Tests, schema and design addendum accompany the change.

All ten baseline conformance failures remain. Deadline storage/display does not connect the missing penalty strategy or repair submission timestamps. Acceptance, hosted release publication and school deployment remain pending.
