# planegcs (vendored) — KiCad geometric constraint solver

2D geometric constraint solver from FreeCAD's Sketcher, vendored for KiCad
issue [#2329](https://gitlab.com/kicad/code/kicad/-/issues/2329) (parametric /
geometric drafting constraints). See the design doc for how it is used.

## Provenance

- **Upstream:** `FreeCAD/FreeCAD`, `src/Mod/Sketcher/App/planegcs/`
- **Pinned commit:** `64bc717c82c7188e9f102e5c121a2b5283320d17` (branch `main`)
- **License:** LGPL-2.1-or-later (unchanged; see file headers). Compatible with
  KiCad's GPL-3.0+. Kept out of permissive `libs/kimath`; the KiCad adapter that
  consumes it lives in `pcbnew/constraints/`.
- **Files (11, upstream-faithful):** `Constraints.{cpp,h}`, `GCS.{cpp,h}`,
  `Geo.{cpp,h}`, `SubSystem.{cpp,h}`, `Util.h`, `qp_eq.{cpp,h}`.

## Dependencies

- **Eigen** (header-only; modules: Core, LU, Cholesky, QR, SparseCore, SparseQR,
  OrderingMethods). **No BLAS/LAPACK/Fortran** — the external-backend macros
  (`EIGEN_USE_BLAS`/`EIGEN_USE_LAPACKE`) are not defined. Eigen ≥ 3.2.2 required
  for the sparse-QR diagnostics path (gated by `EIGEN_SPARSEQR_COMPATIBLE`).
- **Boost.Graph** + **Boost.Math** (header-only; already in KiCad's tree).
- C++20 (`<numbers>`, `<optional>`).

## KiCad modifications

Upstream pulls four FreeCAD-only headers. They are redirected to a single shim
(`planegcs_shim.h`, KiCad-authored) so we carry no FreeCAD build tree. The shim
provides the only FreeCAD symbols planegcs references: `Base::Console().log()/
.warning()` and `Base::TimeElapsed`, plus the `SketcherExport` macro. The exact
patch (re-apply on upstream re-sync):

| File | Upstream include | Replaced with |
|---|---|---|
| `GCS.cpp` | `#include <Base/Console.h>` | `#include "planegcs_shim.h"` |
| `GCS.cpp` | `#include <FCConfig.h>` | *(deleted — no FCConfig symbol used)* |
| `GCS.cpp` | `#include <boost_graph_adjacency_list.hpp>` | `#include <boost/graph/adjacency_list.hpp>` |
| `GCS.h`, `Constraints.h`, `Geo.h` | `#include "../../SketcherGlobal.h"` | `#include "planegcs_shim.h"` |

No other source edits. For production the shim's `Console`/`TimeElapsed` should
route to `wxLogTrace` / a `std::chrono` timer; the spike keeps them trivial.

## Phase-0 build verification (de-risk spike)

Standalone build, g++ 14.2.0, `-std=c++20 -O2`, system Eigen 3.4.0, Boost 1.83:

```
Constraints.cpp   2.1s   224K
GCS.cpp           8.8s   740K   (Eigen sparse-QR heavy)
Geo.cpp           1.6s    96K
SubSystem.cpp     1.9s    68K
qp_eq.cpp         3.7s   236K
-------------------------------
5 TUs            ~18s    1.4M objects, 0 warnings, 0 errors
adapter TU (#include <GCS.h>)  1.4s   (Eigen parse tax per consuming TU)
```

`kicad/smoke_solve.cpp` builds a 2-segment system (parallel + distance +
coordinate pins), solves DogLeg, and asserts `Success`, full constraint
satisfaction, and 0 DOF. **Passes.**

Build/run the smoke test:

```sh
g++ -std=c++20 -O2 -I/usr/include/eigen3 -I. -pthread -c *.cpp kicad/smoke_solve.cpp
g++ -std=c++20 -O2 -pthread *.o -o smoke && ./smoke
```
