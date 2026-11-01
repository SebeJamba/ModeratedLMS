# Phase 5 Verification and Validation

Siyabonga Jamba | Student number 202200995 | CS4433 | 1 November 2026

## Purpose and current decision

Testing evaluates the supplied Moderated LMS against the Phase 2 SRS and corrected Phase 3 design. The implementation has missing Must requirements. Acceptance and school deployment are on hold. Passing tests of existing behaviour do not override failed requirements. The v1.0 and v1.1 names identify local assessment snapshots, not approved operational releases.

The assignment allocates 3 marks to strategy, 5 to automated tests and 4 to system and acceptance testing. This section supplies the strategy; the package includes executable tests, raw results, measured coverage and a defect log. Results must be refreshed after subsequent fixes.

## Baselines and traceability

Inputs are the Phase 1 feasibility and risk documents, Phase 2 SRS and use case diagram, corrected Phase 3 document/schema/seven diagrams, and ModeratedLMS.zip. The corrected Phase 3 archive in the diagrams folder is treated as the latest supplied design, not evidence of supervisor approval. The supplied GitHub repository was also inspected at commit fe7864d154c1ebfd19d33ae5da340fc2932811ad. Its department seeding, dynamic registration department list and include-path fix are preserved. Original files are unchanged. The evidence manifest records input checksums.

The SRS numbers functional requirements 1 to 12 and non-functional requirements 1 to 6 separately. FR01 to FR12 and NFR01 to NFR06 are unambiguous aliases for those existing numbers. UC01 to UC15 retain the supplied use case diagram identifiers. UC16 is the new class-extension use case. A linked test does not imply that a requirement passes.

Phase 2 contains a use case diagram but lacks the three fully dressed specifications required by the brief. The derived specifications below are testing supplements to review and add to Phase 2. They are not presented as previously approved specifications.

## V Model strategy

| Level | Artefact checked | Entry criteria | Exit criteria |
| --- | --- | --- | --- |
| Unit | Detailed design, factory contract, security behaviour, deadline validation | Code compiles; inputs and expected values reviewed; ambiguities recorded | All unit cases pass; no critical/high unit defects; relevant line coverage recorded |
| Integration | Component interfaces, schema, repositories and transactions | Unit tests run; isolated databases available; migration known | Database/boundary cases pass; constraints, persistence and rollback verified |
| System | SRS and complete use case flows, including exceptions | Build succeeds; lower-level failures triaged; synthetic four-role data ready | Every Must requirement passes; no critical/high defects remain |
| Acceptance | Stakeholder needs and measurable criteria | System exit criteria met; role guides and candidate available | Supervisor or stakeholder representatives record acceptance; policy decisions approved |

Verification checks engineering artefacts. Acceptance validates whether the system meets school needs. Student-run technical testing does not establish independent stakeholder acceptance or a supervisor phase gate.

## Scope and environment

Test the four-role offline C++17 console application and SQLite storage. Payments, conferencing, automatic grading, plagiarism detection and web/mobile interfaces remain outside scope. The suite uses vendored Catch2 2.13.10 with its licence, plus the original SQLite amalgamation. Builds were exercised with MSVC 19.33 and GCC 16.2 on Windows x64.

Integration fixtures use fresh in-memory databases; persistence uses a disposable disk database. A separate console workflow uses synthetic accounts in an isolated directory. Fixture IDs distinguish administrator 1, assigned lecturer 2, HoD 3, student 4, unassigned lecturer 5 and second student 6. Never run the suite against the only copy of school data.

## Equivalence partitioning and boundary analysis

The brief requires 10 percent of the available maximum mark per late day, not 10 percent of the student's raw mark. Earlier documents do not define partial-day rounding or exact equality for resubmission. Proposed tests treat equality at the submission deadline as on time, positive lateness as starting day 1, and resubmission as strictly before the deadline. Review these interpretations before acceptance. CF04 uses the proposed partial-day rule; source inspection independently confirms the absence of any penalty calculation.

For raw mark 80 and maximum 100, the proposed final mark is max(0, raw - 0.10 * maximum * ceil(lateness_seconds / 86400)) for up to three days, and zero beyond 72 hours. An extension changes the effective deadline used in the calculation. These are expected values, not implemented functionality.

| Partition | Boundary examples | Expected result |
| --- | --- | --- |
| On time | deadline minus 1 second; exactly deadline | SUBMITTED; no penalty |
| First late day | plus 1 second; plus 24 hours | LATE; proposed final mark 70 |
| Second late day | plus 24 hours and 1 second; plus 48 hours | Proposed final mark 60 |
| Third late day | plus 48 hours and 1 second; plus 72 hours | Proposed final mark 50 |
| Beyond window | plus 72 hours and 1 second | Accept work; mark zero |
| Mark range | -0.01, 0, 100, 100.01 for maximum 100 | Reject outside range; accept endpoints |
| Resubmission | deadline minus 1 second, equality, plus 1 second | Replace active attempt only before cutoff under proposed policy |
| Extension | earlier, equal, 1 second later | Reject earlier/equal; accept later |
| Calendar | February 30; non-leap/leap February 29; hour 24; minute/second 60 | Reject invalid dates; accept valid dates |
| Scope | assigned lecturer; other lecturer; student; HoD; administrator | Only assigned lecturer grants extension |
| Overlap | individual date before/equal/after class date | Latest applicable deadline wins |

MT05/MT11 execute extension boundaries. CF03 executes an excessive mark and CF04 exposes the missing penalty. Remaining penalty/resubmission boundary cases are designed but blocked by absent rules. Add injectable time and the designed ILatePenaltyCalculator strategy before executing them deterministically.

## Derived UC06 Submit assignment

Actor: authenticated student enrolled on the course. Preconditions: existing assignment, valid work location and agreed effective-deadline rule. Main flow: select enrolled assignment, provide work, resolve deadline, read system time, validate input/enrolment, save one active attempt as SUBMITTED or LATE and confirm. Postcondition: one active attempt with a genuine timestamp.

Alternative: before the resubmission cutoff, replace content or preserve versions with one clearly identified active attempt. Accept a late first submission and apply the penalty during marking. Exceptions: unknown assignment, unenrolled student, invalid path, database failure and invalid input produce an error without partial writes or misleading success. ST01, CF05 and CF06 cover implemented behaviour and failures. The current program stores a path; it does not transfer or validate file contents.

## Derived UC12 and UC13 Moderate and release

Actor: HoD for the course's department. Preconditions: MARKED submission with mark and feedback. Main flow: load only department submissions, review, approve or adjust with a reason, atomically save the mark/audit record, enter MODERATED, and separately release to RELEASED. Only then may the student see marks and feedback.

Alternative: approval without numeric change records the moderation decision. Exceptions: foreign department, blank reason, out-of-range mark, unknown submission, invalid source state or database failure rejects the action without an incomplete audit trail. Direct release from SUBMITTED, LATE or MARKED must fail. CF01/CF02/CF03/CF08/CF10 expose relevant gaps. The current application lacks a separate release operation, so the complete flow remains blocked.

## Derived UC16 Extend class deadline

Actor: assigned lecturer. Preconditions: allocated course, mapped assignment and YYYY-MM-DD HH:MM:SS timestamp. Main flow: select extension, validate ownership/date, begin transaction, persist class override, materialise current student overrides while preserving longer individual dates, commit, and display success. Student assignment lists resolve the same latest deadline.

Alternative: later enrollees inherit the retained class date. Exceptions: another role, unrelated lecturer, unknown assignment, malformed/non-later date or write error is rejected. A write error rolls back class and individual updates. MT01 to MT11 and the separate console workflow provide evidence.

## Execution and defect handling

Use the source README's CMake/CTest commands. CTest includes implemented_behaviour and requirements_conformance in its default invocation. Both run; conformance failures make the overall result fail. The grouping does not exempt known defects. Raw JUnit and CTest logs are in evidence. This Catch2 version's JUnit tests attribute counts assertions; the results report separately counts testcase elements.

For each defect retain ID, severity, requirement, cause, fix or proposed fix, status and retest. Critical means assessment confidentiality/integrity is compromised; high means a Must workflow is wrong or unavailable; medium denotes a lesser operational limitation. Re-run the failing test and relevant regression tests after a fix. Record rejected or blocked tests explicitly.

## Acceptance scenarios

| ID | Scenario | Current evidence and decision |
| --- | --- | --- |
| AT01 | Administrator manages accounts, departments, courses and enrolment | Account creation and v1.1 course setup exercised; update/deactivate and complete department management absent; not accepted |
| AT02 | Student submits and resubmits before effective deadline | Initial submission works; duplicate attempts and fixed timestamp detected; not accepted |
| AT03 | Lecturer marks with penalty and written feedback | Numeric mark works; penalty/feedback missing; not accepted |
| AT04 | HoD moderates own department and separately releases | Department filtering/release missing; premature visibility; not accepted |
| AT05 | Lecturer extends class and student sees deadline | Eleven CR1 tests and console workflow pass; stakeholder sign-off pending |
| AT06 | IT officer runs offline, restores backup and times login with 800 stored users | Offline smoke workflow exercised; 800-user timing and operational restore drill unexecuted |

Gate 3 exit criteria are not met. Release decision: HOLD. No supervisor meeting, stakeholder signature, school installation or successful hosted CI run is claimed.
