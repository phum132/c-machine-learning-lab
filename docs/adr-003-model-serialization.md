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

Hexadecimal floating-point values are emitted with C `%a` and parsed with `strtod` in the C numeric locale. The representation stores values as text and does not serialize C structs, pointers, or native byte order. The loader accepts ASCII whitespace between tokens, requires exactly the declared number of values, and rejects trailing tokens.

The file size is limited to 16 MiB and feature count to 250,000. Invalid framing, type, version, dimensions, or extra/truncated tokens return `ML_STATUS_PARSE_ERROR`; non-finite/unrepresentable parameters return `ML_STATUS_NUMERIC_ERROR`; file errors return `ML_STATUS_IO_ERROR`. Loads return an owned opaque model, and output is null on failure. Save truncates the destination directly and is not atomic.

The model APIs expose validated parameter setters. They require an exact feature-count match, validate all values before mutation, and leave a model unchanged on failure.

## Consequences

The format is human-inspectable and round-trips finite `double` values on conforming C implementations. Locale must be C when reading or writing. Version changes require an explicit format version and loader policy. Atomic replacement and checksums are future work.
