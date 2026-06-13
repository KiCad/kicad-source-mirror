#!/usr/bin/env bash
#
# End-to-end workflow test for `kicad-cli mergetool` with auto-resolution.
#
# Generates an ancestor / ours / theirs .kicad_pcb triple by copying and
# mutating a fixture board, writes a KICAD_MERGETOOL_AUTO resolution file
# (TAKE_OURS on every doc-level conflict, plus a sentinel auto-resolver
# strategy for item-level conflicts), and invokes kicad-cli mergetool
# expecting the merged board to be written to disk.
#
# Because kicad-cli mergetool re-execs into the kicad GUI binary (the
# resolution UI lives in eeschema/pcbnew kifaces), this test needs a
# display server.  When xvfb-run is available the test runs under it;
# otherwise it skips cleanly so CI without xvfb still passes.
#
# Usage: mergetool_workflow_e2e.sh /path/to/kicad-cli /path/to/qa/data
#

set -u
set -o pipefail

KICAD_CLI="${1:-}"
QA_DATA_DIR="${2:-}"

if [ -z "$KICAD_CLI" ] || [ ! -x "$KICAD_CLI" ]; then
    echo "usage: $0 /path/to/kicad-cli /path/to/qa/data" >&2
    exit 2
fi

# Skip if the kicad GUI binary isn't next to kicad-cli.  The CLI mergetool
# entry point re-execs into it; without that this workflow can't complete.
kicad_bin="$(dirname "$KICAD_CLI")/kicad"

if [ ! -x "$kicad_bin" ]; then
    echo "SKIP: kicad GUI binary not present next to kicad-cli ($kicad_bin)" >&2
    exit 0
fi

# Skip if no display server is reachable.  We try DISPLAY first (a normal
# desktop session), then xvfb-run (CI virtual display).
RUNNER=""

if [ -n "${DISPLAY:-}" ]; then
    RUNNER=""   # use the existing display
elif command -v xvfb-run >/dev/null 2>&1; then
    RUNNER="xvfb-run -a --server-args=-screen 0 1024x768x24"
else
    echo "SKIP: no DISPLAY and xvfb-run not available; cannot exercise GUI re-exec" >&2
    exit 0
fi

# Skip if the QA data dir isn't reachable (test is sometimes invoked
# standalone without the build infrastructure).
if [ -z "$QA_DATA_DIR" ] || [ ! -d "$QA_DATA_DIR" ]; then
    echo "SKIP: qa/data directory not provided or missing" >&2
    exit 0
fi

# Locate a tiny fixture board.  complex_hierarchy is shared across other
# diff/merge tests so it's a known-good baseline.
FIXTURE_BOARD="$QA_DATA_DIR/pcbnew/complex_hierarchy.kicad_pcb"

if [ ! -f "$FIXTURE_BOARD" ]; then
    echo "SKIP: fixture board not found at $FIXTURE_BOARD" >&2
    exit 0
fi

tmpdir=$( mktemp -d )
trap 'rm -rf "$tmpdir"' EXIT

ANCESTOR="$tmpdir/ancestor.kicad_pcb"
OURS="$tmpdir/ours.kicad_pcb"
THEIRS="$tmpdir/theirs.kicad_pcb"
MERGED="$tmpdir/merged.kicad_pcb"
AUTO_RES="$tmpdir/auto_resolution.json"

cp "$FIXTURE_BOARD" "$ANCESTOR"
cp "$FIXTURE_BOARD" "$OURS"
cp "$FIXTURE_BOARD" "$THEIRS"

# Make ours and theirs differ from ancestor: bump a numeric value in each.
# Use sed to mutate a trace width on each side (different values) so the
# auto-resolver has to choose.  Robust to fixture changes -- if neither
# mutation fires we still get a no-op merge which is also a valid test
# outcome (exit 0 with no diff).
sed -i 's/(width 0\.2)/(width 0.21)/' "$OURS"     || true
sed -i 's/(width 0\.2)/(width 0.22)/' "$THEIRS"   || true

# Write an auto-resolution file selecting TAKE_OURS as the default strategy.
cat > "$AUTO_RES" <<'EOF'
{
    "schema_version": 1,
    "strategy": "take_ours"
}
EOF

export KICAD_MERGETOOL_AUTO="$AUTO_RES"

set +e
$RUNNER "$KICAD_CLI" mergetool "$ANCESTOR" "$OURS" "$THEIRS" --output "$MERGED" 2>&1
exit_code=$?
set -e

# Exit code 0 = clean resolution; 5 = resolved with warnings; both are
# acceptable for this smoke test.  Anything else (1/2/3/4) indicates a
# pipeline regression.
if [ "$exit_code" -ne 0 ] && [ "$exit_code" -ne 5 ]; then
    echo "FAIL: kicad-cli mergetool exited $exit_code (expected 0 or 5)" >&2
    exit 1
fi

if [ ! -f "$MERGED" ]; then
    echo "FAIL: merged output file not created at $MERGED" >&2
    exit 1
fi

# Verify the merged file is a valid s-expression with the kicad_pcb header.
if ! head -c 200 "$MERGED" | grep -q "kicad_pcb"; then
    echo "FAIL: merged output doesn't look like a .kicad_pcb file" >&2
    head -c 500 "$MERGED" >&2
    exit 1
fi

echo "OK: mergetool workflow E2E passed (exit code $exit_code)."

# Also verify the git-configured per-file footprint path.  It uses the same
# mergetool entry point but must dispatch to the footprint-library merger in
# single-file mode instead of rejecting `.kicad_mod` or parsing it as a board.
FIXTURE_FOOTPRINT="$QA_DATA_DIR/libraries/Resistor_SMD.pretty/R_0402_1005Metric.kicad_mod"

if [ ! -f "$FIXTURE_FOOTPRINT" ]; then
    echo "SKIP: fixture footprint not found at $FIXTURE_FOOTPRINT" >&2
    exit 0
fi

FP_ANCESTOR="$tmpdir/ancestor.kicad_mod"
FP_OURS="$tmpdir/ours.kicad_mod"
FP_THEIRS="$tmpdir/theirs.kicad_mod"
FP_MERGED="$tmpdir/merged.kicad_mod"

cp "$FIXTURE_FOOTPRINT" "$FP_ANCESTOR"
cp "$FIXTURE_FOOTPRINT" "$FP_OURS"
cp "$FIXTURE_FOOTPRINT" "$FP_THEIRS"

set +e
$RUNNER "$KICAD_CLI" mergetool "$FP_ANCESTOR" "$FP_OURS" "$FP_THEIRS" --output "$FP_MERGED" 2>&1
exit_code=$?
set -e

if [ "$exit_code" -ne 0 ] && [ "$exit_code" -ne 5 ]; then
    echo "FAIL: kicad-cli mergetool footprint merge exited $exit_code (expected 0 or 5)" >&2
    exit 1
fi

if [ ! -f "$FP_MERGED" ]; then
    echo "FAIL: merged footprint output file not created at $FP_MERGED" >&2
    exit 1
fi

if ! head -c 200 "$FP_MERGED" | grep -q "footprint"; then
    echo "FAIL: merged output doesn't look like a .kicad_mod file" >&2
    head -c 500 "$FP_MERGED" >&2
    exit 1
fi

echo "OK: mergetool footprint workflow E2E passed (exit code $exit_code)."
exit 0
