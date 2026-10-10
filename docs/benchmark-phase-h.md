# Phase H benchmark: CLI linear training

## Question and baseline

Measure end-to-end wall time for the CLI to load a deterministic CSV, train linear regression, and serialize its model. This is a within-build repeatability sample; there is no prior-version or alternate-implementation baseline.

## Environment

- OS: Windows 11 10.0.26200, AMD64; processor reported by Python as Intel64 Family 6 Model 151 Stepping 5, GenuineIntel
- Compiler: GCC 15.1.0 (MSYS2); C11 with `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`, compiler default optimization level
- Python: 3.12.3; CLI version 0.1.0

## Input and method

- 5,000 rows, one feature, target `y = 2x + 1`; 93,754 CSV bytes
- Deterministic feature formula: `x = ((row_index mod 1000) - 500) / 500`; no random seed is used
- Dataset SHA-256: `b65a88ea122d472997ea3671406b341a43cbd18c7be8d94b40709f02547a3e74`
- 200 training iterations, learning rate 0.05; one warm-up and five timed repetitions
- `time.perf_counter`; measured subprocess startup, CSV loading, training, and model save
- Correctness check predicts 1.499428193272436 for `x=0.25`; expected 1.5 ± 0.02

## Raw results

| Repetition | Seconds |
|---:|---:|
| 1 | 0.026197 |
| 2 | 0.026443 |
| 3 | 0.026888 |
| 4 | 0.026149 |
| 5 | 0.026005 |

Median: **0.026197 s**. Minimum: **0.026005 s**.

## Limits

This measures one local machine and one small synthetic, one-feature dataset. Process startup and local filesystem work are included, so these values do not isolate model-kernel performance and should not be generalized to other machines or datasets. Reproduce with `make benchmark` on POSIX, or `python scripts/benchmark_cli.py --exe build/cml-lab.exe --compiler <compiler-path>` on Windows.
