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
# display server. On macOS it uses the desktop session. On other systems
# it uses DISPLAY or xvfb-run and skips if neither is available.
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

run_mergetool()
{
    if [ "${#RUNNER[@]}" -gt 0 ]; then
        "${RUNNER[@]}" "$@"
    else
        "$@"
    fi
}

# Skip if the kicad GUI binary isn't next to kicad-cli.  The CLI mergetool
# entry point re-execs into it; without that this workflow can't complete.
kicad_bin="$(dirname "$KICAD_CLI")/kicad"

if [ ! -x "$kicad_bin" ]; then
    echo "SKIP: kicad GUI binary not present next to kicad-cli ($kicad_bin)" >&2
    exit 0
fi

# Use the desktop session or a virtual display when one is available.
RUNNER=()

if [ "$(uname -s)" = "Darwin" ] || [ -n "${DISPLAY:-}" ]; then
    RUNNER=()   # use the existing display
elif command -v xvfb-run >/dev/null 2>&1; then
    RUNNER=(xvfb-run -a "--server-args=-screen 0 1024x768x24")
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

export KICAD_CONFIG_HOME="$tmpdir/config"

ANCESTOR="$tmpdir/ancestor.kicad_pcb"
OURS="$tmpdir/ours.kicad_pcb"
THEIRS="$tmpdir/theirs.kicad_pcb"
MERGED="$tmpdir/merged.kicad_pcb"
AUTO_RES="$tmpdir/auto_resolution.json"

cp "$FIXTURE_BOARD" "$ANCESTOR"
cp "$FIXTURE_BOARD" "$OURS"
cp "$FIXTURE_BOARD" "$THEIRS"

# Make ours and theirs differ from ancestor: bump a numeric value in each.
# The two divergent Edge.Cuts stroke widths require the auto-resolver to choose a side.
sed '/1546372c-4518-4c49-b994-211020ea3f32/s/(width 0\.2032)/(width 0.21)/' "$OURS" > "$OURS.next" && mv "$OURS.next" "$OURS"
sed '/1546372c-4518-4c49-b994-211020ea3f32/s/(width 0\.2032)/(width 0.22)/' "$THEIRS" > "$THEIRS.next" && mv "$THEIRS.next" "$THEIRS"

if cmp -s "$ANCESTOR" "$OURS" || cmp -s "$ANCESTOR" "$THEIRS" || cmp -s "$OURS" "$THEIRS"; then
    echo "FAIL: fixture did not produce divergent merge inputs" >&2
    exit 1
fi

# Resolve the conflict on the modified Edge.Cuts line by its fixture UUID.
cat > "$AUTO_RES" <<'EOF'
{
    "/1546372c-4518-4c49-b994-211020ea3f32": "take_ours"
}
EOF

export KICAD_MERGETOOL_AUTO="$AUTO_RES"

set +e
run_mergetool "$KICAD_CLI" mergetool "$ANCESTOR" "$OURS" "$THEIRS" --output "$MERGED" 2>&1
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

if ! grep -q '(width 0.21)' "$MERGED" || grep -q '(width 0.22)' "$MERGED"; then
    echo "FAIL: merged board did not select ours for the divergent Edge.Cuts stroke" >&2
    exit 1
fi

echo "OK: mergetool workflow E2E passed (exit code $exit_code)."

# An empty resolution map cancels the dialog when a conflict needs a choice.
CANCEL_RES="$tmpdir/cancel_resolution.json"
printf '{}\n' > "$CANCEL_RES"

check_cancelled_merge()
{
    local name=$1
    local ancestor=$2
    local ours=$3
    local theirs=$4
    local extension=$5
    local existing="$tmpdir/cancelled_$name.$extension"
    local missing="$tmpdir/absent_$name.$extension"
    local project="$tmpdir/cancelled_$name.kicad_pro"
    local output
    local messages
    local result

    cp "$ours" "$existing"
    cp "$existing" "$existing.before"
    printf '{"text_variables":{"cancel_test":"unchanged"}}\n' > "$project"
    cp "$project" "$project.before"

    for output in "$existing" "$missing"; do
        set +e
        messages=$( KICAD_MERGETOOL_AUTO="$CANCEL_RES" run_mergetool "$KICAD_CLI" mergetool \
                    "$ancestor" "$ours" "$theirs" --output "$output" 2>&1 )
        result=$?
        set -e

        if [ "$result" -ne 1 ]; then
            echo "FAIL: cancelled $name merge exited $result (expected 1)" >&2
            echo "$messages" >&2
            exit 1
        fi

        if printf '%s\n' "$messages" | grep -Eq '^(RefdesUniqueness|ConnectivityRebuild|SchemaVersion):'; then
            echo "FAIL: cancelled $name merge reported validator diagnostics" >&2
            echo "$messages" >&2
            exit 1
        fi
    done

    if ! cmp -s "$existing.before" "$existing" || ! cmp -s "$project.before" "$project"; then
        echo "FAIL: cancelled $name merge changed an existing output or project file" >&2
        exit 1
    fi

    if [ -e "$missing" ] || [ -e "$tmpdir/absent_$name.kicad_pro" ]; then
        echo "FAIL: cancelled $name merge created an output or project file" >&2
        exit 1
    fi

    echo "OK: cancelled $name merge left output and project files untouched."
}

check_cancelled_merge pcb "$ANCESTOR" "$OURS" "$THEIRS" kicad_pcb

# Different paper sizes create a schematic conflict without external libraries.
SCH_FIXTURE="$QA_DATA_DIR/diff_merge/visual_diff/sch_a.kicad_sch"
SCH_ANCESTOR="$tmpdir/ancestor.kicad_sch"
SCH_OURS="$tmpdir/ours.kicad_sch"
SCH_THEIRS="$tmpdir/theirs.kicad_sch"

cp "$SCH_FIXTURE" "$SCH_ANCESTOR"
# The older schema would produce a validator warning if cancellation reached the applier.
sed -e 's/(paper "A4")/(paper "A3")/' \
    -e 's/(version [0-9]*)/(version 20250114)/' "$SCH_ANCESTOR" > "$SCH_OURS"
sed 's/(paper "A4")/(paper "A5")/' "$SCH_ANCESTOR" > "$SCH_THEIRS"

if cmp -s "$SCH_ANCESTOR" "$SCH_OURS" || cmp -s "$SCH_ANCESTOR" "$SCH_THEIRS"; then
    echo "FAIL: schematic fixture did not produce divergent merge inputs" >&2
    exit 1
fi

check_cancelled_merge sch "$SCH_ANCESTOR" "$SCH_OURS" "$SCH_THEIRS" kicad_sch

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
run_mergetool "$KICAD_CLI" mergetool "$FP_ANCESTOR" "$FP_OURS" "$FP_THEIRS" --output "$FP_MERGED" 2>&1
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
