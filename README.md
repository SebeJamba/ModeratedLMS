# Moderated LMS

The current phases-5-to-8 branch fixes the ten baseline conformance failures. Baseline v1.0 and v1.1 tags are preserved as historical snapshots; they have not been moved.

## Build and test on Windows

Install Visual Studio C++ desktop tools, Windows SDK and CMake. From this repository folder:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\ModeratedLMS.exe
```

Both test groups should now pass. The historical [known-defect] tag identifies the ten original conformance checks and does not mean they are still failing. All checks remain enabled. Raw JUnit results are written in the build directory.

## Corrected assessment workflow

Create courses, assign lecturers and enrol students using the administrator menu. Existing unmapped assignments must be linked to their course before assessment operations.

Students can submit and replace an unmarked attempt strictly before its effective deadline. Initial late work is accepted and labelled LATE. Effective deadline is the latest of the base, class and individual dates. Use full YYYY-MM-DD HH:MM:SS timestamps.

Lecturer option 6 lists submission IDs. Option 2 records the raw mark, written feedback and a mandatory reason. Marks must be finite and within 0..maximum. Each started 24-hour late period deducts 10 percent of the available maximum, up to 72 hours; later work receives zero. Marks cannot fall below zero. Saved status is MARKED.

The department HoD uses option 2 to moderate a MARKED submission, giving a reason. Option 5 separately releases a MODERATED submission. Students receive neither marks nor feedback before RELEASED. Lecturer re-marking resets the status to MARKED and hides results until moderation/release are repeated.

All mark writes and their submission-linked audit entries commit or roll back together. Audit entries cannot be edited or deleted by the application database API. HoDs can view or change only their own department's assessment records.

## Existing databases and passwords

Back up lms.db and referenced work before upgrading. Schema changes add feedback, audit submission links and an active-attempt flag. Duplicate historical submissions are retained; the most recent ID is active. Legacy GRADED states become MARKED. Old audit rows without submission IDs remain unchanged rather than receiving invented provenance.

New passwords use Windows BCrypt PBKDF2-HMAC-SHA256 with 600000 iterations, a 16-byte random salt and a 32-byte derived key. Random-generation failure aborts safely. A successful login with an old hash upgrades it immediately; accounts that have not logged in retain old hashes and need migration or reset before operational deployment. Initial demo credentials remain admin / Admin123!; use synthetic data. There is still no complete account-management/password-reset screen.

## Evidence and limits

assessment-evidence/fixes contains the new results. assessment-documents/06_Fixed_Requirements.md supersedes the earlier failure counts for the current branch. Earlier reports and v1.0/v1.1 results remain historical evidence. Remaining scope includes full administrator update/deactivation, assignment descriptions, complete original repository-interface separation, target-machine acceptance and review of deadline policy. Passing this test suite is not a claim that every assignment criterion is finished.

Catch2 and SQLite are vendored. The application runs offline. Only Windows is currently supported by the BCrypt password implementation.
