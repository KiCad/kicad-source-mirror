#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
IMAGE_NAME="kicad-fedora-rawhide-qa"
CONTAINER_BUILD_DIR="/src/build/fedora-rawhide"

QA_TESTS=(kimath sexpr common eeschema pcbnew pns_regressions gerbview kicad2step spice api)

usage() {
    echo "Usage: $0 [build-image|build|test|shell|all]"
    echo ""
    echo "Commands:"
    echo "  build-image   Build the Fedora Rawhide Docker image"
    echo "  build         Configure and build QA tests inside container"
    echo "  test          Run QA tests inside container"
    echo "  shell         Open interactive shell in container"
    echo "  all           Build image, build tests, run tests (default)"
    exit 0
}

docker_run() {
    docker run --rm \
        -v "${SRC_DIR}:/src:rw" \
        -w /src \
        -e CCACHE_DIR=/src/build/fedora-rawhide-ccache \
        "$IMAGE_NAME" \
        bash -c "$1"
}

cmd_build_image() {
    echo "=== Building Fedora Rawhide Docker image ==="
    docker build -t "$IMAGE_NAME" "$SCRIPT_DIR"
}

cmd_build() {
    echo "=== Configuring and building QA tests for Fedora Rawhide ==="
    docker_run "
        mkdir -p ${CONTAINER_BUILD_DIR} && cd ${CONTAINER_BUILD_DIR} &&
        cmake -G Ninja \
            -DCMAKE_BUILD_TYPE=QABUILD \
            -DKICAD_BUILD_QA_TESTS=ON \
            -DKICAD_SCRIPTING=OFF \
            -DKICAD_SCRIPTING_WXPYTHON=OFF \
            -DKICAD_BUILD_I18N=OFF \
            -DKICAD_UPDATE_CHECK=OFF \
            -DKICAD_USE_CMAKE_FINDPROTOBUF=ON \
            /src 2>&1 | tee cmake_log.txt &&
        echo '=== CMake configuration complete ===' &&
        ninja 2>&1 | tee build_log.txt
    "
}

cmd_test() {
    echo "=== Running QA tests for Fedora Rawhide ==="
    local failed=()

    for t in "${QA_TESTS[@]}"; do
        echo "--- Running qa_${t} ---"

        if docker_run "cd ${CONTAINER_BUILD_DIR}/qa && ctest -R qa_${t} --output-on-failure 2>&1"; then
            echo "--- qa_${t}: PASSED ---"
        else
            echo "--- qa_${t}: FAILED ---"
            failed+=("$t")
        fi
    done

    echo ""
    echo "=== Test Summary ==="

    if [ ${#failed[@]} -eq 0 ]; then
        echo "All tests passed."
    else
        echo "Failed tests: ${failed[*]}"
        exit 1
    fi
}

cmd_shell() {
    echo "=== Opening shell in Fedora Rawhide container ==="
    docker run --rm -it \
        -v "${SRC_DIR}:/src:rw" \
        -w /src \
        "$IMAGE_NAME" \
        bash
}

CMD="${1:-all}"

case "$CMD" in
    build-image) cmd_build_image ;;
    build)       cmd_build ;;
    test)        cmd_test ;;
    shell)       cmd_shell ;;
    all)         cmd_build_image && cmd_build && cmd_test ;;
    -h|--help)   usage ;;
    *)           echo "Unknown command: $CMD"; usage ;;
esac
