# {fmt} (vendored)

{fmt} 12.2.0, vendored from https://github.com/fmtlib/fmt (tag `12.2.0`).

## Local patches

Drop each patch on an update to a release that already contains it.

* `include/fmt/base.h` carries upstream
  [#4822](https://github.com/fmtlib/fmt/pull/4822) (commit `b54093c658ae`),
  which stops MSVC warning C4702 (unreachable code) in `print`, `println` and
  `code_point_length` under C++20. It was merged after 12.2.1.
