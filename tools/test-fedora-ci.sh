#!/usr/bin/env bash

# Run KiCad QA inside the Fedora CI container, similar to the CI .fedora_qa job.
#
# Usage:
#   tools/test-fedora-ci.sh [TEST]
#
# TEST is optional; if provided, it selects the CI suite name used in ctest regex (qa_${TEST}), e.g.:
#   python, cli, common, gerbview, pcbnew, pns_regressions, eeschema, kimath, sexpr, kicad2step, spice, api.
# If omitted, runs the full QA suite (equivalent to ctest in build/linux/qa).
#
# Environment overrides (optional):
#   IMAGE=registry.gitlab.com/.../fedora:41   # Override CI image
#   CONTAINER_TOOL=podman|docker              # Force container runtime
#   REPO_DIR=/path/to/kicad_repo              # Defaults to current dir
#   BUILD_DIR=/path/to/build/linux            # Defaults to REPO_DIR/build/linux
#   CCACHE_DIR_HOST=/path/to/ccache           # Defaults to REPO_DIR/ccache (not required for tests)
#   TEST=<label>                              # Alternative to positional arg
#
# Notes:
# - For TEST=cli (like CI), installs 'gerbv' inside the container before running tests.
# - For TEST=python or cli, installs Python requirements from qa/tests/requirements.txt.
# - Runs tests the same way CI does: from build/linux/qa with `ctest -R qa_${TEST}`.

set -euo pipefail

here() { builtin cd "$(dirname "$0")" >/dev/null 2>&1 && pwd -P; }
SCRIPT_DIR="$(here)"
REPO_DIR_DEFAULT="$(builtin cd "${SCRIPT_DIR}/.." >/dev/null 2>&1 && pwd -P)"

REPO_DIR="${REPO_DIR:-${REPO_DIR_DEFAULT}}"
BUILD_DIR="${BUILD_DIR:-${REPO_DIR}/build/linux}"
CCACHE_DIR_HOST="${CCACHE_DIR_HOST:-${REPO_DIR}/ccache}"

TEST_LABEL_INPUT="${1:-${TEST:-}}"
TEST_LABEL="${TEST_LABEL_INPUT:-}"
BUILD_BEFORE_TEST="${BUILD_BEFORE_TEST:-}"

# Read CI image default
IMAGE_FROM_CI=""
if [[ -f "${REPO_DIR}/.gitlab-ci.yml" ]]; then
  IMAGE_FROM_CI=$(grep -Po 'DEFAULT_FEDORA_IMAGE:\s*\K.*' "${REPO_DIR}/.gitlab-ci.yml" || true)
fi
IMAGE="${IMAGE:-${IMAGE_FROM_CI:-registry.gitlab.com/kicad/kicad-ci/source_containers/master/fedora:41}}"

# Choose container tool
if [[ -z "${CONTAINER_TOOL:-}" ]]; then
  if command -v podman >/dev/null 2>&1; then
    CONTAINER_TOOL=podman
  elif command -v docker >/dev/null 2>&1; then
    CONTAINER_TOOL=docker
  else
    echo "Error: neither podman nor docker is installed." >&2
    exit 1
  fi
fi

echo "Using container runtime: ${CONTAINER_TOOL}"
echo "Using image: ${IMAGE}"
echo "Repository dir: ${REPO_DIR}"
echo "Build dir: ${BUILD_DIR}"
if [[ -n "${TEST_LABEL}" ]]; then
  echo "Selected TEST label: ${TEST_LABEL}"
fi

# Volumes
if [[ "${CONTAINER_TOOL}" == "podman" ]]; then
  VOLUMES=(
    -v "${REPO_DIR}:/work:Z"
    -v "${REPO_DIR}:${REPO_DIR}:Z"    # mount source at its absolute host path for __FILE__ lookups
    -v "${CCACHE_DIR_HOST}:/work/ccache:Z"
  )
else
  VOLUMES=(
    -v "${REPO_DIR}:/work"
    -v "${REPO_DIR}:${REPO_DIR}"      # mount source at its absolute host path for __FILE__ lookups
    -v "${CCACHE_DIR_HOST}:/work/ccache"
  )
fi
  # Map host passwd to give the UID a name and HOME resolution
  if [[ -r /etc/passwd ]]; then
    VOLUMES+=( -v "/etc/passwd:/etc/passwd:ro" )
  fi
  USERFLAGS=(-u "$(id -u):$(id -g)" -e HOME=/work)

# Decide whether to run as root (needed to dnf install gerbv for cli)
RUN_AS_ROOT=false
if [[ "${TEST_LABEL}" == "cli" ]]; then
  RUN_AS_ROOT=true
fi
# Allow override via env var RUN_AS_ROOT=1
if [[ -n "${RUN_AS_ROOT_OVERRIDE:-}" ]]; then
  if [[ "${RUN_AS_ROOT_OVERRIDE}" == "1" || "${RUN_AS_ROOT_OVERRIDE}" == "true" ]]; then
    RUN_AS_ROOT=true
  fi
fi

if [[ "${CONTAINER_TOOL}" == "podman" ]]; then
  if [[ "${RUN_AS_ROOT}" == true ]]; then
    USERFLAGS=() # root by default
  else
    USERFLAGS=(--userns=keep-id)
  fi
else
  if [[ "${RUN_AS_ROOT}" == true ]]; then
    USERFLAGS=() # root by default
  else
    USERFLAGS=(-u "$(id -u):$(id -g)")
  fi
fi

# Build test commands to run inside container
read -r -d '' TEST_CMDS <<'EOF' || true
set -euo pipefail
cd /work

# Ensure a writable HOME and explicit KiCad config/data roots so wx doesn't touch /home
export HOME=/work
export KICAD_CONFIG_HOME=/work/qa/tests/config
export QA_DATA_ROOT=/work/qa/data
export KICAD_TEST_EESCHEMA_DATA_DIR=/work/qa/data/eeschema
mkdir -p "$KICAD_CONFIG_HOME"

# Pre-install like CI before_script
if [[ "${TEST_LABEL}" == "python" || "${TEST_LABEL}" == "cli" ]]; then
  python3 -m pip install -r qa/tests/requirements.txt
fi
if [[ "${TEST_LABEL}" == "cli" ]]; then
  # Install gerbv (requires root)
  if command -v dnf >/dev/null 2>&1; then
    dnf -y install gerbv
  else
    echo "Warning: dnf not available; skipping gerbv install" >&2
  fi
fi

# Optional tools to satisfy some QA checks (PDF rasterization and fonts)
if command -v dnf >/dev/null 2>&1; then
  if [[ "$(id -u)" == "0" ]]; then
    dnf -y install poppler-utils fontconfig || echo "Optional deps not installed; some checks may be skipped"
  else
    echo "Optional deps not installed; some checks may be skipped"
  fi
fi



# Optionally rebuild tests in the container if requested
if [[ -n "${BUILD_BEFORE_TEST}" ]]; then
  cd /work/build/linux
  ninja -j"$(nproc)" qa
  cd /work
fi

cd /work/build/linux/qa
export KICAD_RUN_FROM_BUILD_DIR=1
# CI env for junit and verbosity
export BOOST_TEST_LOGGER='JUNIT,warning,test_results.'"${TEST_LABEL}"'.xml:HRF,message'
export CTEST_OUTPUT_ON_FAILURE=1
export BOOST_TEST_CATCH_SYSTEM_ERRORS='no'

if [[ -n "${TEST_LABEL}" ]]; then
  ctest -j"$(nproc)" -R "qa_${TEST_LABEL}"
else
  ctest -j"$(nproc)"
fi
EOF

# Inject TEST_LABEL into the heredoc by substituting placeholder
TEST_CMDS=${TEST_CMDS//'${TEST_LABEL}'/${TEST_LABEL}}
TEST_CMDS=${TEST_CMDS//'${BUILD_BEFORE_TEST}'/${BUILD_BEFORE_TEST}}

if [[ "${CONTAINER_TOOL}" == "podman" ]]; then
  exec podman run --rm -it \
    --name kicad-fedora-ci-test \
    "${USERFLAGS[@]}" \
    "${VOLUMES[@]}" \
    -w /work \
    "${IMAGE}" \
    bash -lc "${TEST_CMDS}"
else
  exec docker run --rm -it \
    --name kicad-fedora-ci-test \
    "${USERFLAGS[@]}" \
    "${VOLUMES[@]}" \
    -w /work \
    "${IMAGE}" \
    bash -lc "${TEST_CMDS}"
fi
