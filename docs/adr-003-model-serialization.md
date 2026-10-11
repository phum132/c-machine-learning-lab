# ADR-003: Model serialization

## Decision

Phase F stores linear and logistic model parameters in a versioned ASCII text format. Version 1 consists of whitespace-separated tokens:

```text
CMLMODEL 1
linear|logistic
<feature count>
<bias as hexadecimal floating point>
<one hexadecimal floating point weight per feature>
```

Phase J adds version 2 for models trained with input standardization. It repeats the version 1 fields with version `2`, followed by the scaling method and one mean/scale pair per feature:

```text
CMLMODEL 2
linear|logistic
<feature count>
<bias>
<weights>
standard
<mean> <scale>   (one pair per feature)
```

Hexadecimal floating-point values are emitted with C `%a` and parsed with `strtod` in the C numeric locale. The representation stores values as text and does not serialize C structs, pointers, or native byte order. The loader accepts ASCII whitespace between tokens, requires exactly the declared number of values, and rejects trailing tokens. Scales must be finite and positive.

The file size is limited to 32 MiB (raised from 16 MiB so a version 2 file at the 250,000-feature cap still fits) and feature count to 250,000. Invalid framing, type, version, scaling method, dimensions, or extra/truncated tokens return `ML_STATUS_PARSE_ERROR`; non-finite/unrepresentable parameters and non-positive scales return `ML_STATUS_NUMERIC_ERROR`; file errors return `ML_STATUS_IO_ERROR`. Loads return an owned opaque model, and output is null on failure.

`ml_model_load_*_scaled` accept both versions and return the scaling separately (empty for version 1). The original `ml_model_load_*` functions keep their signature and accept version 1 only; a version 2 file returns `ML_STATUS_INVALID_ARGUMENT`, because silently dropping the scaling would produce wrong predictions.

Saves create a new temporary file `<path>.<random>-<attempt>.tmp` with C11 exclusive mode (`"wbx"`), close it, then replace `<path>` with `rename` (POSIX) or `MoveFileExA` with `MOVEFILE_REPLACE_EXISTING` (Windows). A failed save removes the temporary file and leaves an existing destination unchanged. The data is not fsynced, so the replacement is atomic with respect to other readers but not durable across power loss. Exclusive creation refuses any existing entry at the temporary name, including a planted symlink, and the random suffix lets concurrent or crashed saves coexist; the last completed replacement wins.

The model APIs expose validated parameter setters. They require an exact feature-count match, validate all values before mutation, and leave a model unchanged on failure.

## Consequences

The format is human-inspectable and round-trips finite `double` values on conforming C implementations. Locale must be C when reading or writing. Version changes require an explicit format version and loader policy. Checksums and fsync-based durability are future work.
