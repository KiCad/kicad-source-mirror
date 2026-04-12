# KiCad Custom Build — IATRT Internal

Last updated: 2026-04-10
Initial build: 2026-03-22

## Baseline

- Installed build version: `10.0.0`
- Source branch used: `10.0`
- Local checkout commit used for the installed build: `58775834`
- Upstream official release reference: tag `10.0.0`

This checkout is:

- 22 commits ahead of the official `10.0.0` release tag
- 2 commits behind the current `origin/10.0` branch tip at the time this note was written

## Upstream Changes Since Official 10.0.0

The checkout used for this build already includes upstream post-`10.0.0` changes from the `10.0` maintenance branch.

Main upstream deltas in `10.0.0..58775834`:

- Start of `10.0.1` development
- Zone fill improvements:
  - DAG scheduling and batching
  - zone/edge violation reporting on the zone layer
  - rejection of negative clearance values
- DRC rule editor and rule help fixes:
  - multiple rule-help corrections
  - `ShowMatches` button fix
  - rule area attributes exposed in the properties panel
- Library table refresh fixes after editing
- Translation updates across many languages

Representative upstream commits in this range:

- `1e9374af` Begin version 10.0.1 development.
- `f5f5ecad` Zone fill: DAG scheduling and batching
- `28af8875` Don't allow negative clearance values.
- `be04b2bf` Report zone <-> edge violations on zone's layer.
- `a3f9e9f7` add rule area attributes to properties panel
- `53f64d06` Make sure child tables get reloaded after editing

## Local Source Changes In This Checkout

These are the local source modifications on top of the upstream snapshot used for this build.

### 1. Symbol chooser footprint preview gained a 3D tab

Files:

- `common/CMakeLists.txt`
- `eeschema/dialogs/dialog_symbol_chooser.cpp`
- `eeschema/widgets/panel_symbol_chooser.cpp`
- `eeschema/widgets/panel_symbol_chooser.h`
- `include/frame_type.h`
- `pcbnew/CMakeLists.txt`
- `pcbnew/pcbnew.cpp`
- `common/widgets/footprint_3d_preview_widget.cpp`
- `include/widgets/footprint_3d_preview_widget.h`
- `pcbnew/footprint_3d_preview_panel.cpp`
- `pcbnew/footprint_3d_preview_panel.h`

Behavior change:

- The symbol chooser now uses a notebook for footprint preview.
- Existing 2D footprint preview remains.
- A new 3D footprint preview tab was added next to it.
- The 3D preview reports:
  - footprint not found
  - no 3D model linked
- The chooser now shuts down preview canvases explicitly on dialog teardown to avoid canvas/assert/shutdown issues.

Implementation summary:

- Added a new `FOOTPRINT_3D_PREVIEW_WIDGET` wrapper in common widgets.
- Added a new `FRAME_FOOTPRINT_3D_PREVIEW` frame type.
- Added a new `FOOTPRINT_3D_PREVIEW_PANEL` in `pcbnew` using `EDA_3D_CANVAS`.
- Routed the new frame type through `pcbnew` KIFACE creation.

### 2. LCSC / EasyEDA parts import panel (added 2026-04-10)

Files:

- `eeschema/widgets/panel_lcsc_import.cpp` (new)
- `eeschema/widgets/panel_lcsc_import.h` (new)
- `eeschema/dialogs/dialog_symbol_chooser.cpp`
- `eeschema/dialogs/dialog_symbol_chooser.h`
- `eeschema/CMakeLists.txt`
- `eeschema/sch_action_plugin.cpp` (new)
- `eeschema/sch_action_plugin.h` (new)
- `eeschema/python/scripting/eeschema_action_plugins.cpp` (new)
- `eeschema/python/scripting/eeschema_action_plugins.h` (new)
- `eeschema/menubar.cpp`
- `eeschema/sch_edit_frame.cpp`
- `eeschema/sch_edit_frame.h`
- `eeschema/toolbars_sch_editor.cpp`

Behavior change:

- The symbol chooser dialog now has an "LCSC Import" tab alongside the "Library" tab.
- Enter an LCSC part number (e.g. C25804) to import the symbol, footprint, and 3D model via the `easyeda2kicad` Python package.
- Imported parts get live symbol, 2D footprint, and 3D model preview immediately.
- The "LCSC Part" field is renamed to "LCSC" so it appears in the chooser column.
- LCSC part numbers are appended to `ki_keywords` so parts are searchable by LCSC number in the Library tab.
- Files are saved to `~/Documents/Kicad/LCSC_Parts/` under `LCSC_Parts.kicad_sym`, `LCSC_Parts.pretty/`, and `LCSC_Parts.3dshapes/`.

Dependencies:

- Requires `easyeda2kicad` Python package (`pip install easyeda2kicad`).
- Requires `EASYEDA2KICAD` environment variable pointing to `~/Documents/Kicad/LCSC_Parts/` (set in kicad_common.json).
- Requires `LCSC_Parts` entries in both `sym-lib-table` and `fp-lib-table` using `${EASYEDA2KICAD}`.

Bugs fixed during development:

- Footprint name extraction: easyeda2kicad writes multi-line properties; the parser was only matching single-line format, so footprint preview never worked. Fixed with a state-machine parser.
- LCSC field name: easyeda2kicad writes `"LCSC Part"` but the chooser column expects `"LCSC"`. Fixed by renaming during the copy step.
- Search: LCSC part numbers were not in `ki_keywords`, so searching by part number (e.g. "C25804") didn't find the part. Fixed by appending the LCSC ID to keywords.

## Wayland / EGL Build Configuration Changes

The installed system build was configured as a native Wayland/EGL-oriented KiCad build.

Key configuration points:

- `KICAD_WAYLAND=ON`
- wxWidgets config used:
  - `/home/user/.local/opt/wx-3.2.8-egl/bin/wx-config`
- EGL and Wayland libraries resolved from system libraries
- Install prefix changed to:
  - `/usr/local`

Installed binary linkage confirmed against:

- `libwayland-client`
- `libwayland-egl`
- `libEGL`
- wx EGL-enabled GTK3 libraries from:
  - `/home/user/.local/opt/wx-3.2.8-egl/lib`

## System Install / Runtime Changes

These changes are outside the source tree but are part of the working Wayland deployment.

### 1. Replaced the old manual `/usr/local` KiCad install

- The previous KiCad manual install under `/usr/local` was removed/replaced.
- The new Wayland/EGL build was installed into `/usr/local`.

### 2. Fixed Python pcbnew install location

- Installed Python bindings were aligned to the active interpreter path:
  - `/usr/local/lib/python3.13/dist-packages`
- Stale older copies under other Python install paths were removed.

### 3. Forced desktop launchers to the intended binaries

Per-user desktop file overrides were added in:

- `/home/user/.local/share/applications`

These overrides use absolute `Exec=` targets so GNOME does not resolve KiCad through an unexpected PATH entry or stale launcher.


## Short Summary

Compared with the official `10.0.0` release, this installed build differs in four ways:

1. It is based on a newer `10.0` maintenance-branch snapshot containing 22 upstream post-`10.0.0` fixes.
2. It includes a 3D footprint preview tab in the symbol chooser with improved preview shutdown behavior.
3. It includes an LCSC/EasyEDA parts import panel in the symbol chooser with live preview and keyword-based search.
4. Its system installation and launch behavior were adjusted specifically for direct native Wayland/EGL use, with desktop launchers pinned to the correct binaries.

## Rebuilding

```bash
cd ~/src/kicad/build-wayland-egl
cmake --build . -- -j$(nproc)
sudo cmake --install .
```

## Source Location

- Build source: `~/src/kicad` (branch `10.0`, commit `58775834` + local changes)
- Build directory: `~/src/kicad/build-wayland-egl`
- Reference copy: `~/Documents/IATRT/Local/IATRT IT/internal projects/kicad/`
- Patch file: `kicad-custom-modifications.patch` (apply with `git apply` on a clean `10.0` checkout at commit `58775834`)
