# Older-version export golden references

These references are independent of the downgrade exporter. Do not regenerate an
expected file by exporting `current/`; that would teach the tests to accept bugs.

## Provenance

The DRC references are independently authored text; there is no native DRC-file
serializer.

## Coverage

- `rules`: quotes, parentheses and unsupported-looking tokens in real full-line
  comments; literal hashes in strings; a 10-only constraint; an 11-only constraint.

Invalid inline DRC comments and unbalanced input must also fail closed.

## Comparison contract

DRC references are compared byte-for-byte and parsed.
