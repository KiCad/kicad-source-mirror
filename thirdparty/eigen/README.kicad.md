# Eigen (vendored)

Eigen 3.4.0, vendored from https://gitlab.com/libeigen/eigen (tag `3.4.0`).

Only the stable header modules under `Eigen/` are kept; the `unsupported/`
tree is omitted because planegcs, the sole consumer, does not use it.

Eigen is required by `thirdparty/planegcs` (`#include <Eigen/Dense>`,
`<Eigen/QR>`, `<Eigen/Core>`). It is vendored rather than resolved with
`find_package( Eigen3 )` so the build has no external Eigen dependency on any
platform. The macOS and Windows CI toolchains do not ship Eigen.

Eigen is primarily MPL-2.0 licensed with some BSD and LGPL-2.1 third-party
code. See `COPYING.README` and the sibling `COPYING.*` files.

To update, replace the `Eigen/` directory and `signature_of_eigen3_matrix_library`
with the contents of a new upstream release and refresh the `COPYING.*` files.
