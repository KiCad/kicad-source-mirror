# gzip-hpp

Vendored from https://github.com/mapbox/gzip-hpp (BSD-2-Clause, see `LICENSE.BSD2`).  Only
`decompress.hpp` is carried; the compression and utility headers are unused.

`decompress.hpp` has diverged from upstream.  Record any further local change here.

| Commit | Change |
| --- | --- |
| a38c2aad1f | Vendored for compressed STEP and VRML support |
| 9115f6031f | Suppress an MSVC warning about a GCC pragma |
| f7065a2643, d0e504f26e | Add and name the include guard |
| f2d92a9e09 | Add `decompression_error` and `limit_error` so callers can tell a size refusal from a corrupt stream; reject inputs larger than `unsigned int`; stop dereferencing a null `inflate_s.msg` |
