#!/usr/bin/env bash
#
# Phase 9 end-to-end smoke for the `kicad-cli mergetool` entry point.
#
# Verifies argument handling, exit-code translation, and the basic
# kicad-cli → kicad re-exec contract documented in Phase 8 of the
# diff/merge plan. Does NOT exercise a real PCB conflict-resolution
# flow — that requires a running display server and is gated on the
# GUI test runner.
#
# Exit code: 0 = all expectations met, non-zero = a failure with the
#            mismatched test name printed to stderr.
#
# Usage: mergetool_e2e.sh /path/to/kicad-cli
#

set -u
set -o pipefail

KICAD_CLI="${1:-}"

if [ -z "$KICAD_CLI" ] || [ ! -x "$KICAD_CLI" ]; then
    echo "usage: $0 /path/to/kicad-cli" >&2
    exit 2
fi

failed=0

# Run kicad-cli with the given args and verify it exits with the expected code.
# Stdout/stderr are captured so a failing case's output appears in the report.
expect_exit() {
    local name=$1
    local want=$2
    shift 2

    set +e
    output=$( "$KICAD_CLI" "$@" 2>&1 )
    got=$?
    set -e

    if [ "$got" -ne "$want" ]; then
        echo "FAIL: $name — expected exit $want, got $got" >&2
        echo "$output" >&2
        failed=$(( failed + 1 ))
    fi
}

# 1. --help should succeed (exit 0) and mention "mergetool" in its usage.
set +e
out=$( "$KICAD_CLI" mergetool --help 2>&1 )
got=$?
set -e

if [ "$got" -ne 0 ]; then
    echo "FAIL: mergetool --help expected exit 0, got $got" >&2
    echo "$out" >&2
    failed=$(( failed + 1 ))
fi

if ! echo "$out" | grep -qi mergetool; then
    echo "FAIL: mergetool --help did not mention 'mergetool'" >&2
    failed=$(( failed + 1 ))
fi

# 2. Missing --output → ERR_ARGS (1). argparse handles this before our doPerform.
expect_exit "missing --output" 1 mergetool ancestor.kicad_pcb ours.kicad_pcb theirs.kicad_pcb

# 3. Unsupported output extension → contract code 3 (I/O / parse error).
tmpdir=$( mktemp -d )
trap 'rm -rf "$tmpdir"' EXIT
touch "$tmpdir/a" "$tmpdir/b" "$tmpdir/c"
expect_exit "bad output extension" 3 mergetool "$tmpdir/a" "$tmpdir/b" "$tmpdir/c" \
            --output "$tmpdir/merged.txt"

# 4. .kicad_pcb extension dispatches to the kicad binary. If kicad doesn't exist
#    next to kicad-cli, the contract returns 4 (init failure). Otherwise we
#    can't predict the exit (depends on whether DISPLAY is set, etc.) so we
#    skip when the binary is present.
kicad_bin="$(dirname "$KICAD_CLI")/kicad"

if [ ! -x "$kicad_bin" ]; then
    expect_exit "missing kicad sibling" 4 mergetool \
                "$tmpdir/a" "$tmpdir/b" "$tmpdir/c" --output "$tmpdir/merged.kicad_pcb"
fi

# 5. Repository setup configures git mergetool for symbol libraries and
#    footprints too.  A standalone CLI copy with no sibling kicad binary should
#    accept those extensions far enough to fail with init code 4, not reject
#    them as unsupported input code 3.
solo_cli="$tmpdir/kicad-cli"
cp "$KICAD_CLI" "$solo_cli"
chmod +x "$solo_cli"

expect_solo_extension() {
    local ext=$1

    set +e
    output=$( "$solo_cli" mergetool "$tmpdir/a" "$tmpdir/b" "$tmpdir/c" \
              --output "$tmpdir/merged.$ext" 2>&1 )
    got=$?
    set -e

    if [ "$got" -ne 4 ]; then
        echo "FAIL: supported .$ext output should reach missing kicad sibling (4), got $got" >&2
        echo "$output" >&2
        failed=$(( failed + 1 ))
    fi
}

expect_solo_extension "kicad_sym"
expect_solo_extension "kicad_mod"

if [ "$failed" -gt 0 ]; then
    echo "$failed test(s) failed" >&2
    exit 1
fi

echo "All mergetool CLI smoke tests passed."
