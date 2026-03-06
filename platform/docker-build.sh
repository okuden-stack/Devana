#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# docker-build.sh — build Devana for Arch Linux using Docker
#
# Usage:
#   ./tools/scripts/docker-build.sh              # build, output → dist/linux/
#   ./tools/scripts/docker-build.sh --zmq        # build with ZMQ harness enabled
#   ./tools/scripts/docker-build.sh --rebuild    # force image rebuild first
#   ./tools/scripts/docker-build.sh --shell      # drop into container shell
#
# Output:
#   dist/linux/Devana    — compiled Linux/x86_64 binary
#
# Requirements:
#   Docker (docker.com/get-docker or 'brew install --cask docker')
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
IMAGE_NAME="devana-arch-builder"
DOCKERFILE="${SCRIPT_DIR}/Dockerfile.arch"
OUT_DIR="${PROJECT_ROOT}/dist/linux"
ZMQ_ENABLED="OFF"
FORCE_REBUILD=false
SHELL_MODE=false

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --zmq)      ZMQ_ENABLED="ON"; shift ;;
        --rebuild)  FORCE_REBUILD=true; shift ;;
        --shell)    SHELL_MODE=true; shift ;;
        -h|--help)
            grep '^#' "$0" | grep -v '#!/' | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

cd "${PROJECT_ROOT}"

# ── Preflight ─────────────────────────────────────────────────────────────────
if ! command -v docker &>/dev/null; then
    echo "✗ Docker not found."
    echo "  Install: brew install --cask docker"
    exit 1
fi

if ! docker info &>/dev/null 2>&1; then
    echo "✗ Docker daemon is not running. Start Docker.app first."
    exit 1
fi

# ── Build image if needed ─────────────────────────────────────────────────────
IMAGE_EXISTS=$(docker images -q "${IMAGE_NAME}" 2>/dev/null)

if [[ -z "${IMAGE_EXISTS}" ]] || [[ "${FORCE_REBUILD}" == "true" ]]; then
    echo "▶ Building Docker image: ${IMAGE_NAME}"
    echo "  (This takes ~3 min on first run, packages are cached after that)"
    echo ""
    docker build \
        -f "${DOCKERFILE}" \
        -t "${IMAGE_NAME}" \
        "${PROJECT_ROOT}"
    echo ""
    echo "✓ Image built"
else
    echo "✓ Using existing image: ${IMAGE_NAME}"
    echo "  (Pass --rebuild to force a fresh image)"
fi

echo ""
mkdir -p "${OUT_DIR}"

# ── Shell mode ────────────────────────────────────────────────────────────────
if [[ "${SHELL_MODE}" == "true" ]]; then
    echo "▶ Dropping into container shell (source mounted at /src)..."
    docker run --rm -it \
        -v "${PROJECT_ROOT}":/src:ro \
        -v "${OUT_DIR}":/out \
        -w /staging \
        --entrypoint /bin/bash \
        "${IMAGE_NAME}"
    exit 0
fi

# ── Build ─────────────────────────────────────────────────────────────────────
echo "▶ Building Devana for Linux (Arch)..."
echo "  ZMQ harness: ${ZMQ_ENABLED}"
echo ""

docker run --rm \
    -v "${PROJECT_ROOT}":/src:ro \
    -v "${OUT_DIR}":/out \
    -e DEVANA_ENABLE_ZMQ="${ZMQ_ENABLED}" \
    "${IMAGE_NAME}"

# ── Result ────────────────────────────────────────────────────────────────────
if [[ -f "${OUT_DIR}/Devana" ]]; then
    echo ""
    echo "══════════════════════════════════════════════════"
    echo "  ✓ Linux binary ready: dist/linux/Devana"
    echo "══════════════════════════════════════════════════"

    # Optionally sync to home server if configured and reachable
    if [[ -n "${DEVANA_DEPLOY_HOST:-}" ]]; then
        echo ""
        echo "▶ DEVANA_DEPLOY_HOST is set (${DEVANA_DEPLOY_HOST})"
        echo "  Run './tools/scripts/sync-server.sh --deploy-only' to push the binary."
    fi
else
    echo "✗ Build failed — binary not found at dist/linux/Devana"
    exit 1
fi
