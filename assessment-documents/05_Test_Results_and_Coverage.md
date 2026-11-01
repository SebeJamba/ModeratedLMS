# Test Results Coverage and Quality Evidence

## Executed results

| Snapshot and compiler | Test cases | Passed | Failed | Assertions |
| --- | --- | --- | --- | --- |
| v1.0-all-tests.xml | 21 | 11 | 10 | 44 |
| v1.1-all-tests.xml | 33 | 23 | 10 | 86 |
| v1.1-gcc-all-tests.xml | 33 | 23 | 10 | 86 |

The 12 additions in v1.1 are eleven CR1 cases and QA01. All pass on both compilers. The ten baseline conformance failures remain. CTest reports one passing group and one failing group; its 50 percent group figure is not a test-case pass rate. Exact failure assertions are in the JUnit files.

## Measured line coverage

GCC gcov measured 313 of 377 executable lines, or 83.02 percent, across the four application source files linked into lms_core. Profiles were cleared after the final code changes before the final full-suite run. Coverage includes execution by failing tests; it does not imply correctness.

| Source | Covered lines | Executable lines | Coverage percent |
| --- | --- | --- | --- |
| Database.cpp | 14 | 19 | 73.68 |
| DeadlineRepository.cpp | 86 | 92 | 93.48 |
| Security.cpp | 22 | 23 | 95.65 |
| User.cpp | 191 | 243 | 78.6 |

Denominator excludes main.cpp, headers, SQLite, Catch2 and the standard library. The application mixes UI, rules and persistence; no isolated business-logic layer exists. Although the measured application figure exceeds 70 percent, that alone does not establish the SRS business-layer architecture/coverage criterion. Never relabel it as complete domain-layer coverage. Raw compressed gcov JSON and the summarisation script are supplied.

An initial OpenCppCoverage attempt failed because its debugger could not resolve the process handle in this environment. GCC instrumentation supplied the final measurement instead. No zero, estimated or invented coverage figure substitutes for an unavailable measurement.

## Console workflow

The saved v1.1 console transcript records real process execution with synthetic data: initial login, lecturer/student creation, course allocation/enrolment, assignment creation, class extension and student assignment-list display. The visible effective deadline is 2026-11-03 12:00:00. Exit code was zero. This is a developer-run system smoke test, not stakeholder acceptance.

## Metrics

| Function | Non-comment lines | Cyclomatic complexity |
| --- | --- | --- |
| Administrator::displayMenu | 87 | 17 |
| Student::displayMenu | 65 | 10 |
| Lecturer::displayMenu | 61 | 9 |
| HeadOfDepartment::displayMenu | 71 | 9 |
| main | 63 | 7 |
| UserFactory::createUser | 7 | 5 |
| DeadlineRepository::initialize | 21 | 4 |
| DeadlineRepository::extendClass | 23 | 4 |
| generateSalt | 11 | 4 |

Lizard metrics exclude dependencies and test code. Final date validation was refactored after an initial complexity of 25. clang-tidy emitted no diagnostics under the recorded selected checks. The manual audit lifetime finding and its correction remain important despite that tool result.

## Unexecuted and blocked evidence

School acceptance, supervisor gate review, hosted CI, a target-machine installation/restore drill, login timing with 800 stored users, full penalty boundaries and the complete moderation/release lifecycle remain pending or blocked. The traceability matrix records these gaps. Release decision remains HOLD.
