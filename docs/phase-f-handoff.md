# Phase F handoff: Model serialization

## Scope delivered

- Added validated parameter restore APIs for opaque linear and logistic models.
- Added version 1 text serialization for both model types with `%a` doubles, a 16 MiB file cap, a 250,000 feature cap, strict token counts, and corrupt/non-finite input handling.
- Added deterministic round-trip, transactional setter, unsupported-version, non-finite, and missing-file tests.

## Verification

Verified on 2026-10-10 with `build.bat test` (GCC 15.1): foundation unit tests, CLI subprocess tests, and infrastructure validation all passed. The build emitted existing `-Wfloat-conversion` diagnostics from GCC's `isfinite` macro expansion for `double` values across numerical modules, including this module; these were not suppressed. Makefile test path was not rerun in this phase.

## Limits and next steps

- Save truncates the destination and is not atomic; a failed write can leave a partial file.
- Version 1 numeric parsing/writing requires the C numeric locale.
- Extend CLI inference in Phase G to expose model save/load and prediction.
