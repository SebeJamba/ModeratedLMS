# Phase 8 Maintenance and Design Addendum

## Request and classification

CR1 requests whole-class deadline extensions by a lecturer and visibility of extensions in the student's assignment list. This is primarily perfective maintenance: an enhancement to meet a user need, rather than adaptation to a new environment or repair of an already implemented class-extension feature.

Phase 2 already lists class extension as FR12 with Should priority. This change implements that deferred requirement and makes its display obligation explicit. FR04 remains the individual-extension Must requirement. Mention of class overrides in conflict-resolution text does not prove earlier implementation.

The assignment places maintenance after an accepted v1.0 release. The supplied implementation has not passed that gate. Here, v1.0/v1.1 demonstrate local baseline/maintenance snapshots while release approval is on hold. No school release, supervisor approval or historic gate is invented.

## Impact analysis

| Artefact | Impact |
| --- | --- |
| FR12 | Implement whole-class transaction and visible effective deadline |
| FR04 | Preserve individual extensions and longer individual precedence |
| FR03 and FR05 | Supply course-linked assignments and resolve effective deadline for lists |
| FR06 and FR07 | Resubmission/penalty rules must later consume the same deadline; baseline integration remains open |
| FR01 and FR02 | Validate assigned lecturer; provide course allocation/enrolment prerequisites |
| NFR03 and NFR06 | Bind variable extension values; validate calendar; rollback failed writes |
| UC05 and UC10 | Update assignment list and individual extension path |
| UC16 | Add fully described class-extension flow and exceptions |
| Class diagram | Add concrete DeadlineRepository operations and dependencies; retain User hierarchy |
| Component diagram | Add maintenance repository between Lecturer/Student UI and SQLite; full original layering remains incomplete |
| Sequence diagrams | Add authorisation, transaction, class/current-student update, rollback and effective-deadline read |
| Activity diagram | Add class/individual deadline resolution before submission-time decision and in assignment list; original submission integration remains required |
| State machine | States unchanged; effective-deadline guards must be used when original submission rules are implemented |
| ERD and dictionary | Add ClassExtensions/SchemaMigrations and the course/enrolment/extension structures missing from supplied code |
| Schema | schema_actual.sql records executable v1.1 structure; it is not falsely equated with the fuller Phase 3 schema |
| Source | DeadlineRepository.hpp/.cpp, User.cpp, main.cpp, CMakeLists.txt, tests and workflow |
| Tests | MT01 to MT11, QA01 and full regression; CF04/CF05/CF06 remain failed deadline-workflow evidence |
| Guides | Update lecturer/student/admin procedures, migration, release notes, traceability and risks |

## Policy and design rationale

Effective deadline is the latest of base assignment date, retained class date and student's individual date. A class extension must move the class/base deadline later; a longer individual extension remains effective. This resolves an ambiguity in earlier documents and requires stakeholder review before acceptance.

Class membership comes from course enrolment. Only the course's assigned lecturer may extend its assignment. Current enrolled students receive extension rows in one transaction, matching FR12's criterion. ClassExtensions retains the class override for later enrollees. A per-student-only bulk update was rejected because later enrolment would miss it. A NULL studentID extension was rejected because it weakens the original per-student relationship and complicates uniqueness.

DeadlineRepository uses prepared values, RAII statement cleanup, explicit commit/rollback and exceptions. It does not implement the originally designed ISubmissionRepository/ILatePenaltyCalculator interfaces. Legacy presentation code continues direct SQL access. This addendum records that limitation instead of claiming the entire three-tier design has been realised.

## Updated dictionary

| Table or column | Key and constraints | Purpose |
| --- | --- | --- |
| Courses | courseID PK; required department FK; lecturer FK | Course ownership and lecturer allocation |
| Enrollments | studentID/courseID composite PK and FKs | Affected class membership |
| Assignments.courseID | Nullable FK added to original code schema | Preserve old rows; manually map them before CR1 use |
| Extensions | extensionID PK; assignment/student FKs; unique pair | Per-student override; preserve longer existing date |
| ClassExtensions | assignmentID PK/FK; required date, actor FK and changedAt | Durable class override and latest change metadata |
| SchemaMigrations | version PK; appliedAt required | Idempotent initialisation marker version 11 |

ClassExtensions stores one current class override, while Extensions materialises current student overrides. This deliberate overlap supports the requested bulk transaction and future enrolment; maximum-date reads prevent shortening. The database still lacks Phase 3 fields/constraints such as written feedback and submission-specific audit linking. Full schema conformity cannot yet be claimed.

## Migration and rollback

Close the application, back up lms.db and referenced work, and retain the matching v1.0 executable. v1.1 creates missing maintenance tables, conditionally adds Assignments.courseID and records version 11 once. Repeated initialisation is tested without losing submissions. Administrator must create course/enrolment records and link each old assignment before CR1 use. Old date-only deadlines require an agreed time-of-day conversion before relying on timestamp comparisons; the migration does not guess that policy. Deadline inputs represent the same agreed school-local clock; changedAt is stored as SQLite UTC metadata.

This migration targets the database created by the supplied implementation. It is not a general importer for a database independently created from corrected Phase 3 SQL, which uses submissionTimestamp instead of submissionTime and may lack extension uniqueness required by upsert. Inspect and convert such a database separately; do not overwrite it.

On failed upgrade verification, close the program, preserve the failed database, restore the pre-upgrade copy in a separate folder and restart the matching v1.0 executable. An in-place schema downgrade is not supported. Do not discard newer records without a recovery decision. Synthetic initialisation/transaction rollback were tested; a school backup/restore drill remains pending.

## Regression and decision

MT01 to MT11 exercise all enrolments, longer individual dates, future enrolment, role/course denial, equal/earlier/invalid dates, injected transaction failure, student display, lecturer menu, repeated migration and unknown targets. QA01 checks the audit lifetime correction. A separate console test exercises real login, setup, assignment creation and visible extension.

The ten inherited failures remain in full regression. The change is technically demonstrated but is not an approved operational v1.1 release. Integrate genuine timestamps, resubmission guards and late penalties, repair all Must defects, review precedence policy and rerun acceptance before closing the gate.
