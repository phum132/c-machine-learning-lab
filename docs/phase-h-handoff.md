# Phase H handoff: Quality checks

## Scope delivered

- Added POSIX Make targets for GCC static analysis (`test-analyzer`), AddressSanitizer/UndefinedBehaviorSanitizer (`test-sanitize`), and a repeatable CLI benchmark (`benchmark`). CI now runs analyzer and sanitizer targets after the main test suite.
- Added opt-in `CFLAGS_OVERRIDE` and `LDFLAGS_OVERRIDE` to `build.bat` for local diagnostic builds.
- Fixed a test cleanup path found by GCC analyzer: failed `fclose` no longer leaves a stale `FILE *` that cleanup can close again.
- Added explicit target-array allocation overflow checks in linear train/validation split.
- Fixed Windows Make rebuild behavior by delegating `make` and `make test` to `build.bat`; verified a forced rebuild.
- Added a deterministic benchmark harness and recorded measured results in [benchmark-phase-h](benchmark-phase-h.md).

## Verification

- `build.bat test`: passed with normal warning flags, including all unit/CLI/infrastructure checks.
- `mingw32-make -B test`: passed on Windows after the Make delegation change.
- PowerShell `$env:CFLAGS_OVERRIDE='... -fanalyzer'; .\build.bat test`: passed; zero `-Wanalyzer-*` diagnostics after the fixes. GCC still reports `-Wfloat-conversion` where its `isfinite` macro expands on `double` arguments, and those warnings remain visible.
- `python scripts/benchmark_cli.py ...`: passed its numerical correctness check; raw measurements and configuration are recorded in the benchmark report.
- MinGW GCC's local sanitizer link failed because that toolchain has no Windows `libasan`/`libubsan` runtime. Installed `mingw-w64-clang-x86_64-clang` (which includes `compiler-rt`) through MSYS2; the Clang ASan/UBSan build and full suite then passed with:

  ```powershell
  $env:CC_OVERRIDE = 'C:\msys64\clang64\bin\clang.exe'
  $env:CFLAGS_OVERRIDE = '-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -fsanitize=address,undefined -fno-omit-frame-pointer'
  $env:LDFLAGS_OVERRIDE = '-fsanitize=address,undefined'
  .\build.bat test
  ```

  CI also runs sanitizer targets on Ubuntu 24.04; a hosted GitHub Actions run was not available from this workspace.

## Remaining limits

- Sanitizer execution is confirmed locally with MSYS2 Clang; confirmation on the configured hosted Ubuntu runner remains open.
- Existing `isfinite` macro conversion diagnostics remain unsuppressed.
- Benchmark numbers are a local end-to-end sample, not a cross-version performance claim.
