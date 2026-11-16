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
- **Files (11, upstream apart from the edits under KiCad modifications):**
  `Constraints.{cpp,h}`, `GCS.{cpp,h}`, `Geo.{cpp,h}`, `SubSystem.{cpp,h}`, `Util.h`,
  `qp_eq.{cpp,h}`.

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

`SubSystem::initialize()` and the two-subsystem `System::solve()` order their unknowns
as the caller declared them instead of by address. Upstream intersects or sorts sets of
`double*`, so the variable order, and with it the rounding of every solve, follows heap
layout. MSVC's `std::deque<double>` allocates two doubles per block, so the same edit
landed a few nanometres apart from one session to the next. Re-apply on upstream re-sync.

The two-subsystem `System::solve()` seeds its BFGS matrix with the soft subsystem's
Gauss-Newton curvature `JB^T JB`, plus `1e-10` of its largest diagonal so directions no
soft constraint sees stay bounded. Upstream starts from the identity. KiCad's stay-put pins
weigh `1e-4`, so their curvature is `1e-8` and the identity overstates it so far that the
`1e-10` step test ends the solve short of the optimum, at a point the unknown order picks.
Re-apply on upstream re-sync.

The same solve returns `Failed` as soon as the soft gradient or the hard Jacobian is no
longer finite, at the start or after any step, and hands back the parameters it started
from. A contradiction can be met by collapsing a segment, where a distance constraint's
derivative is 0/0. Upstream then lets the NaN through the `yTh != 0` test into the BFGS
matrix and every later step. Stopping at the collapsed iterate instead is no better, because
it can satisfy every hard constraint and be written back as a solution. Re-apply on upstream
re-sync.

The penalty weight `mu` (Eq. 18.36) is left unchanged on an iterate that already meets
every hard constraint. Upstream divides by that zero violation, so `mu` becomes infinite,
the merit becomes `inf * 0`, and every line-search test passes. The perpendicular(2 seg)
Sweep solve then runs to the iteration limit instead of converging in 29 iterations.
Re-apply on upstream re-sync.

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
