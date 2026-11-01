Historical baseline report. For the corrected branch, see 06_Fixed_Requirements.md and assessment-evidence/fixes.

# Phase 6 Configuration Management and Quality Assurance

## Configuration policy

Git tracks source, CMake, dependency licences, tests, workflow, schema and documentation. Builds, executables, local databases and coverage counters are excluded. Local tags v1.0/v1.1 identify review snapshots, not accepted school releases. The original supplied files remain unchanged. The v1.0 snapshot adds the missing SQLite include path and a runnable test/build harness.

Use main for integrated work and feature branches reviewed by pull request. The maintenance work is represented by feature/CR1-class-extension and a merge commit. Commit descriptions identify actual changes. Preparation commits extend the actual supplied repository commit fe7864d; they are not backdated or presented as earlier student work. A Git bundle preserves snapshots and tags for offline inspection.

The supplied hosted repository is https://github.com/SebeJamba/ModeratedLMS. Its main branch was inspected at fe7864d154c1ebfd19d33ae5da340fc2932811ad and had zero Actions runs at inspection. The local preparation branch is based on that commit. No remote write was performed. Push the reviewed work and retain the real workflow URL before claiming hosted CI evidence. Local execution logs must not be labelled GitHub Actions runs.

## Continuous integration

The included workflow triggers on push and pull_request with read-only repository permissions. A Windows MSVC job configures, builds and runs CTest. A Windows GCC job downloads a pinned portable compiler, verifies SHA256, builds with coverage, runs all tests and exports gcov JSON. Catch2 and SQLite are vendored. Result artefacts are uploaded even on failure.

Tests have no continue-on-error exemption. The current expected result is failure because ten requirement checks fail. Only report green CI after repairing those failures and observing the real hosted run. CI needs internet to fetch infrastructure; the built program does not need internet.

## Static analysis

clang-tidy analysed all five v1.1 C++ source files with clang-analyzer-* and selected dangling-handle/use-after-move checks, C++17 and project include paths. The saved log lists each analysed file and has no diagnostics for that configuration. This limited result is not proof of security or memory safety. Third-party SQLite/Catch2 were outside application-code review.

Manual review found a concrete lifetime issue: HoD audit insertion passed std::to_string(...).c_str() with SQLITE_STATIC, allowing destruction before SQLite consumes the value. v1.1 uses SQLITE_TRANSIENT for the two values. QA01 verifies the persisted old/new strings. This fix does not repair missing lecturer audit entries or missing submission links.

## Metrics and review

code-metrics.csv contains Lizard non-comment lines and cyclomatic complexity per application function, excluding dependencies/tests. Large displayMenu functions mix presentation, rules and SQL, confirming the Phase 3 architecture gap. Extract services/repositories before claiming the designed layering is implemented.

Initial analysis gave the new date validator complexity 25. Review separated format, range and leap-year validation and added invalid-time/calendar tests. Final measured values appear in the results report. This makes boundary reasoning easier and provides a recorded improvement rather than an invented review history.

| Review item | Finding | Action or outstanding work |
| --- | --- | --- |
| Build | sqlite3.h not reachable by declared includes | ZIP lacked the include path; the hosted baseline already fixed it; retain fix and add test targets |
| Lifetime | Temporary strings bound with SQLITE_STATIC | Use SQLITE_TRANSIENT; QA01 passes |
| Tests | No automated framework supplied | Add Catch2, default CTest groups and honest failure evidence |
| Atomicity | Class change must not partially update enrolments | Transaction/rollback and injected-failure MT06 |
| Authorisation | Only allocated lecturer may extend assignment | Course/role validation and MT04 |
| Complexity | Dense new date validation | Split helpers, broaden boundary tests, remeasure |
| Confidentiality | Student sees marks before release | Critical open CF01; withhold acceptance |
| Security | Custom hash and fixed salt fallback | Critical open CF09; integrate recognised password KDF |
| Architecture | SQL in UI; designed Strategy/interfaces absent | Open design-conformance defect; CR1 repository is only a partial improvement |
| Robustness | Unchecked stream/SQLite failures | Open high defect; add recovery and checked operations |

## Risk register update at current review

| Risk | Observed status | Next mitigation and owner |
| --- | --- | --- |
| R01 SQLite resource/connection failure | CR1 uses RAII and rollback; legacy errors remain | Refactor/check legacy operations; Developer |
| R02 Scope/misalignment | Design/code mismatch materialised | Complete Must requirements first; Project Manager |
| R03 Individual workload | Remains open; no illness is inferred | Small commits, backup, realistic tasks; Developer |
| R04 Gate/design rework | Architecture/schema/lifecycle diverge | Reconcile artefacts and obtain real reviews; Architect |
| R05 Password library | Materialised as non-cryptographic custom hash | Integrate recognised KDF and migration; Security Engineer |
| R06 Hardware/internet failure | Offline smoke test passed only on preparation computer | Target-machine rehearsal and USB backup; Release Engineer |
| R07 Business-layer coverage | Mixed-layer measurement available; domain layer absent | Extract domain layer and measure separately; Test Engineer |

Phase 1 likelihood/impact scores are retained unless reassessed. This update records current observations; it does not fabricate earlier gate reviews.
