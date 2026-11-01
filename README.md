# Moderated LMS assessment baseline

This is a review snapshot with known failed Must requirements. See the Phase 5 to 8 report and defect log before using it with real assessment data.

## Build on Windows

Install Visual Studio 2022 C++ desktop tools, a Windows SDK and CMake. In a developer terminal:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\ModeratedLMS.exe
```

CTest deliberately includes the failing requirements-conformance tests. A red result is expected until the listed defects are repaired; the pipeline does not hide them.

Catch2 and SQLite are vendored, so the application and tests do not need the internet to run. Initial credentials are admin / Admin123!. Use synthetic data only. No password-change function exists yet.

For GCC line coverage configure with -DENABLE_COVERAGE=ON, run all tests, run gcov -j on the lms_core source .gcno files, and use tools/coverage_summary.py to summarise the JSON. Report the mixed-layer scope accurately.
