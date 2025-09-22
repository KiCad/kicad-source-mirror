#!/usr/bin/env bash

# Build KiCad inside the Fedora CI container with the same parameters as CI.
#
# Usage:
#   tools/build-fedora-ci.sh
#
# Environment overrides (optional):
#   IMAGE=registry.gitlab.com/.../fedora:41   # Override CI image
#   CONTAINER_TOOL=podman|docker              # Force container runtime
#   REPO_DIR=/path/to/kicad_repo              # Defaults to current dir
#   BUILD_DIR=/path/to/build/linux            # Defaults to REPO_DIR/build/linux
#   CCACHE_DIR_HOST=/path/to/ccache           # Defaults to REPO_DIR/ccache
#
# Notes:
# - Prefers podman if available; falls back to docker.
# - Preserves your UID/GID in the container so outputs aren't root-owned.
# - Uses SELinux-friendly volume labels when using podman.
# - Matches CI flags from .gitlab/Fedora-Linux-CI.yml and image from .gitlab-ci.yml.

set -euo pipefail

here() { builtin cd "$(dirname "$0")" >/dev/null 2>&1 && pwd -P; }
SCRIPT_DIR="$(here)"
REPO_DIR_DEFAULT="$(builtin cd "${SCRIPT_DIR}/.." >/dev/null 2>&1 && pwd -P)"

REPO_DIR="${REPO_DIR:-${REPO_DIR_DEFAULT}}"
BUILD_DIR="${BUILD_DIR:-${REPO_DIR}/build/linux}"
CCACHE_DIR_HOST="${CCACHE_DIR_HOST:-${REPO_DIR}/ccache}"

# Try to read the image from .gitlab-ci.yml DEFAULT_FEDORA_IMAGE; fallback to known tag
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
echo "Host ccache dir: ${CCACHE_DIR_HOST}"

mkdir -p "${CCACHE_DIR_HOST}" "${BUILD_DIR}"

# Common environment
ENV_VARS=(
  -e CCACHE_BASEDIR=/work
  -e CCACHE_DIR=/work/ccache
)

# Volume mappings and user flags
if [[ "${CONTAINER_TOOL}" == "podman" ]]; then
  VOLUMES=(
    -v "${REPO_DIR}:/work:Z"
    -v "${CCACHE_DIR_HOST}:/work/ccache:Z"
  )
  USERFLAGS=(--userns=keep-id)
else
  VOLUMES=(
    -v "${REPO_DIR}:/work"
    -v "${CCACHE_DIR_HOST}:/work/ccache"
  )
  USERFLAGS=(-u "$(id -u):$(id -g)")
fi

# Build commands mirror .gitlab/Fedora-Linux-CI.yml
read -r -d '' BUILD_CMDS <<'EOF' || true
set -euo pipefail
mkdir -p ccache
export CCACHE_BASEDIR=/work
export CCACHE_DIR=/work/ccache

mkdir -p build/linux
cd build/linux

cmake \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=QABUILD \
  -DKICAD_STDLIB_LIGHT_DEBUG=ON \
  -DKICAD_BUILD_I18N=ON \
  -DKICAD_TEST_XML_OUTPUT=ON \
  -DKICAD_BUILD_PNS_DEBUG_TOOL=ON \
  -DKICAD_USE_CMAKE_FINDPROTOBUF=ON \
  ../../

ninja 2>&1 | tee compilation_log.txt
EOF

# Run the container and execute build
if [[ "${CONTAINER_TOOL}" == "podman" ]]; then
  exec podman run --rm -it \
    --name kicad-fedora-ci-build \
    "${USERFLAGS[@]}" \
    "${VOLUMES[@]}" \
    -w /work \
    "${ENV_VARS[@]}" \
    "${IMAGE}" \
    bash -lc "${BUILD_CMDS}"
else
  exec docker run --rm -it \
    --name kicad-fedora-ci-build \
    "${USERFLAGS[@]}" \
    "${VOLUMES[@]}" \
    -w /work \
    "${ENV_VARS[@]}" \
    "${IMAGE}" \
    bash -lc "${BUILD_CMDS}"
fi
