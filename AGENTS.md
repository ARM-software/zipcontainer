# Repository Guidelines

## Project Structure & Module Organization
- `zipc.cpp`/`zipc.h` exposes a small C API for reading/writing uncompressed ZIP containers.
- `zipc_utility.h` adds an additional C++ API with extra utility functions.
- Tests reside in `tests/zipctest.cpp`
- Artifacts are kept in a `build/` directory when following the commands below.
- We do not have and do not want any external dependencies.

## Build, Test, and Development Commands
- Configure: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- Build library and test binary: `cmake --build build`.
- Run tests: `ctest --test-dir build --output-on-failure`.
- Verify created zip files with `zip -T <filename.zip>`
- We target Linux and Android platforms primarily.

## Coding Style & Naming Conventions
- Language: C++20 with `-Wall`; avoid compiler warnings.
- Indentation: Tab indentation and Allman braces (opening brace on a new line).
- Public API uses `zipc_*` and `zipcstream_*` snake_case; keep new functions consistent.
- Keep headers minimal; prefer forward declarations over extra includes to limit compile time.
- No formatter is enforced; keep diffs small and readable.

## Testing Guidelines
- Framework: plain `assert` within `tests/zipctest.cpp` executed via `ctest`.
- Add coverage for both happy paths and error handling (e.g., missing entries, invalid modes).
- When adding APIs, include at least one integration-style test that exercises `zipc_open` → operation → `zipc_close`.

## Security & Configuration
- Library intentionally handles uncompressed ZIP files only. Make sure we fail gracefully on compressed files.
- We do not want to add any thread synchronization inside the library. Document any concurrency guarantees added or removed.
