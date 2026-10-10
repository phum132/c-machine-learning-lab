# Phase I handoff: Portfolio documentation

## Summary

Updated the portfolio documentation to describe the implemented architecture, concrete setup/test commands, CLI workflows, known limitations, and local sanitizer evidence. Added small deterministic CSV fixtures so a new user can run linear train/save/load/predict from the repository.

## Files Changed

- `README.md` — setup, build/test, feature/API summary, runnable CLI example, diagnostic build notes, and known limits.
- `docs/architecture.md` — current module map, data flow, ownership, input/numeric policy, and verification boundary.
- `examples/linear-train.csv`, `examples/linear-features.csv` — deterministic `y = 2x + 1` fixture and prediction rows.
- `docs/phase-h-handoff.md` — records the successful local Clang ASan/UBSan run and keeps hosted CI verification open.
- `docs/implementation-roadmap.md` — updates Phase H and records Phase I as in progress.

## Interface Changes

None. Documentation and example data only.

## Verification

- `build.bat test` — exit 0; unit tests, CLI subprocess tests, and infrastructure validation passed.
- Source-only copy under `%TEMP%` with `build/` excluded: `cmd /c build.bat test` — exit 0; all tests passed without pre-existing build artifacts.
- The README train/predict commands also passed from that source-only copy; outputs were approximately `-2.00003`, `1.99997`, and `6.99997`.
- Clang ASan/UBSan invocation recorded in `docs/phase-h-handoff.md` — exit 0; full test suite passed.
- README train command — exit 0; trained on five rows and saved the model.
- README predict command — exit 0; predictions for `-1.5`, `0.5`, and `3` were approximately `-2.00003`, `1.99997`, and `6.99997`.
- `build.bat test` with normal GCC after sanitizer run — exit 0; restored the regular local build output.

## Issues

- No `.git` directory is present in the workspace, so tracked/untracked status and clean-checkout verification cannot be established here.
- GitHub Actions is configured, but a hosted passing run has not been observed.
- A source-only snapshot excluding `build/` passed `build.bat test` and the README example. This verifies the setup does not need existing build outputs, but it is not a Git clean checkout.
- GCC continues to emit existing `-Wfloat-conversion` diagnostics around its `isfinite` macro expansion.

## Risks

The docs correctly distinguish source-snapshot evidence from a Git clean checkout and local evidence from hosted CI status. Do not describe Phase I as complete until a Git clean-checkout setup and a real hosted CI pass have been verified.

## Next Steps

1. Verify repository state and clean-checkout setup in a Git-backed checkout.
2. Observe the configured GitHub Actions workflow pass on Ubuntu 24.04.
3. Revisit Phase I acceptance and the release checklist after those results exist.
