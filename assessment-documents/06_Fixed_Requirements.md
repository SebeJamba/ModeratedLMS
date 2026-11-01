# Corrected requirement checks and regression results

This revision supersedes the ten-failure counts for the current phases-5-to-8 branch. The unchanged v1.0/v1.1 tags and earlier assessment reports retain the original baseline evidence. No tag was moved, and no school acceptance or operational release is asserted.

## Executed result

Both MSVC and GCC pass all 45 test cases: the original 33 cases plus twelve new regression cases. Both CTest groups pass. All ten original conformance checks remain enabled. The historical known-defect test tag is retained for command compatibility; those checks are now passing.

The grading menu now requests written feedback and a mandatory reason, so its scripted test inputs include both fields. The resubmission fixture sets a future deadline to test the intended before-deadline scenario independently of today's clock. The moderation fixture starts in MARKED state to satisfy the newly enforced guard. Their original expected assertions are retained. New tests check the same behaviours independently through the application service.

## Implemented corrections

- SQL-backed student views return neither marks nor feedback until RELEASED.
- HoD moderation changes MARKED to MODERATED. HoD option 5 separately releases approved work. Department ownership and role restrictions apply to listing, moderation, release and audit views.
- Grading validates finite marks in 0..maximum, collects feedback/reason, applies the penalty Strategy and saves MARKED. Regrading hides previously released results until reviewed again.
- Every service mark/moderation/release write commits with its submission-linked audit entry. Injected audit failures roll back the corresponding change. Audit rows cannot be updated or deleted.
- Submissions use the real clock, verify enrolment, resolve class/individual deadlines and maintain one active attempt. Initial late work remains accepted. A replacement is permitted strictly before the effective deadline on unmarked SUBMITTED work.
- Penalty policy: each started late 24-hour period deducts 10 percent of the available maximum, through 72 hours; beyond 72 hours the final mark is zero. Values are clamped at zero. These boundary choices are explicit and still require school policy review.
- Password writes use Windows BCrypt PBKDF2-HMAC-SHA256, 600000 iterations, a random 16-byte salt and a 32-byte key. A known reference computed independently with Python hashlib matches. RNG errors fail closed. Successful legacy logins upgrade their hashes; pending legacy accounts still need migration/reset before deployment.
- Additive schema migration retains old submissions and audit rows. For duplicate old submissions, the greatest submission ID becomes active and older attempts remain retained. Existing GRADED states become MARKED. Old audit provenance is not invented.

## Regression evidence

RG01 covers the complete release gate and regrading. RG02 covers penalty/range boundaries. RG03 controls the clock for resubmission equality/late boundaries. RG04 integrates class and individual deadlines into assessment. RG05 checks roles and departments. RG06 and RG12 inject audit failures. RG07 checks immutable, linked audit records. RG08 checks invalid inputs/EOF. RG09 checks the cryptographic reference and malformed salts. RG10 checks late initial work. RG11 checks old-database migration without deleting history.

The actual console executable was also exercised through account/course setup, extension, student submission, lecturer grading, hidden student results, HoD moderation, separate release and visible feedback. A second login checks automatic upgrading of a legacy administrator hash. The saved transcript uses synthetic accounts only.

## Measured coverage

GCC gcov measured 602 of 757 executable lines across the five application sources in lms_core: 79.52 percent. AssessmentService.cpp coverage is 94.69 percent. The denominator includes assessment service, database, deadlines, security and role menus; excludes main, headers and third-party dependencies. This remains application-layer coverage rather than proof that every original architectural interface is implemented. Profiles were cleared before the final full-suite run. Raw compressed gcov data, both compilers' JUnit results, CTest logs and updated metrics are in assessment-evidence/fixes.

## Remaining assignment work

Full administrator update/deactivation and department management, assignment descriptions, the complete originally designed submission-repository interface, target-machine acceptance/restore and stakeholder policy approval remain outside these ten-check fixes. Seeded demonstration credentials and unmigrated legacy accounts are not suitable for operational use. Legacy date-only deadlines and unmapped assignments require reviewed mapping/full timestamps before assessment. Passing the suite does not establish completion of every assignment mark or school acceptance.

Earlier Word reports describe baseline snapshots. Use these new results and defect retests when updating their current implementation/results sections. Preserve the before/after evidence for the maintenance discussion.

## References

Microsoft. BCryptDeriveKeyPBKDF2: https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptderivekeypbkdf2

OWASP. Password Storage Cheat Sheet, PBKDF2-HMAC-SHA256 settings: https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html
