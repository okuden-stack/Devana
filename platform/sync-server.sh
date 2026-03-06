#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# sync-server.sh — sync source to home server and optionally build there
#
# Modes:
#   (default)        rsync source → server, trigger remote build, pull binary
#   --source-only    rsync source only, don't build
#   --build-only     trigger remote build without rsyncing (source already there)
#   --deploy-only    push the local dist/linux/Devana binary to the server
#   --pull           pull the binary from the server without building
#   --status         show last build status on the server
#
# Configuration (set in tools/scripts/setup/env.local.zsh or export in shell):
#   DEVANA_DEPLOY_HOST   SSH host, e.g. "homeserver" or "192.168.1.10"
#   DEVANA_DEPLOY_USER   SSH user (default: current user)
#   DEVANA_DEPLOY_PORT   SSH port (default: 22)
#   DEVANA_DEPLOY_PATH   Remote path for the project (default: ~/devana)
#   DEVANA_DEPLOY_KEY    Path to SSH private key (default: ~/.ssh/id_ed25519)
#
# Example env.local.zsh entries:
#   export DEVANA_DEPLOY_HOST="homeserver"
#   export DEVANA_DEPLOY_USER="aleks"
#   export DEVANA_DEPLOY_PATH="~/projects/devana"
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# ── Load local env if present ─────────────────────────────────────────────────
LOCAL_ENV="${PROJECT_ROOT}/tools/scripts/setup/env.local.zsh"
if [[ -f "${LOCAL_ENV}" ]]; then
    # shellcheck disable=SC1090
    source "${LOCAL_ENV}"
fi

# ── Config with defaults ──────────────────────────────────────────────────────
HOST="${DEVANA_DEPLOY_HOST:-}"
USER="${DEVANA_DEPLOY_USER:-$(whoami)}"
PORT="${DEVANA_DEPLOY_PORT:-22}"
REMOTE_PATH="${DEVANA_DEPLOY_PATH:-~/devana}"
KEY="${DEVANA_DEPLOY_KEY:-${HOME}/.ssh/id_ed25519}"

MODE="full"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --source-only)  MODE="source"; shift ;;
        --build-only)   MODE="build"; shift ;;
        --deploy-only)  MODE="deploy"; shift ;;
        --pull)         MODE="pull"; shift ;;
        --status)       MODE="status"; shift ;;
        -h|--help)
            grep '^#' "$0" | grep -v '#!/' | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

# ── Preflight ─────────────────────────────────────────────────────────────────
if [[ -z "${HOST}" ]]; then
    echo "✗ DEVANA_DEPLOY_HOST is not set."
    echo ""
    echo "  Add to tools/scripts/setup/env.local.zsh:"
    echo "    export DEVANA_DEPLOY_HOST=\"homeserver\""
    echo "    export DEVANA_DEPLOY_USER=\"aleks\""
    echo "    export DEVANA_DEPLOY_PATH=\"~/projects/devana\""
    echo ""
    echo "  Or export in your shell before running."
    exit 1
fi

SSH_OPTS="-p ${PORT} -i ${KEY} -o ConnectTimeout=5 -o BatchMode=yes"
SSH_TARGET="${USER}@${HOST}"

ssh_run() {
    # shellcheck disable=SC2086
    ssh ${SSH_OPTS} "${SSH_TARGET}" "$@"
}

# ── Connectivity check ────────────────────────────────────────────────────────
echo "▶ Checking connection to ${SSH_TARGET}:${PORT}..."
if ! ssh_run "true" 2>/dev/null; then
    echo "✗ Cannot reach ${SSH_TARGET} — are you on a network with access to it?"
    echo "  Use './tools/scripts/docker-build.sh' to build locally when offline."
    exit 1
fi
echo "✓ Connected"

# ── rsync helper ──────────────────────────────────────────────────────────────
do_sync() {
    echo ""
    echo "▶ Syncing source → ${SSH_TARGET}:${REMOTE_PATH}..."
    # shellcheck disable=SC2086
    rsync -avz --progress \
        -e "ssh ${SSH_OPTS}" \
        --exclude='.git' \
        --exclude='build/' \
        --exclude='dist/' \
        --exclude='.build_cache/' \
        --exclude='logs/' \
        --exclude='__pycache__/' \
        --exclude='*.pyc' \
        --exclude='.DS_Store' \
        "${PROJECT_ROOT}/" \
        "${SSH_TARGET}:${REMOTE_PATH}/"
    echo "✓ Sync complete"
}

# ── remote build helper ───────────────────────────────────────────────────────
do_remote_build() {
    echo ""
    echo "▶ Triggering remote build on ${HOST}..."
    ssh_run "
        set -e
        cd ${REMOTE_PATH}
        echo '  Regenerating CMake files...'
        python3 tools/devana build --regen 2>/dev/null || true
        echo '  Building...'
        python3 tools/devana rebuild
        echo '  Done.'
    "
    echo "✓ Remote build complete"
}

# ── pull binary helper ────────────────────────────────────────────────────────
do_pull() {
    echo ""
    echo "▶ Pulling binary from ${HOST}..."
    mkdir -p "${PROJECT_ROOT}/dist/linux"
    # shellcheck disable=SC2086
    rsync -avz \
        -e "ssh ${SSH_OPTS}" \
        "${SSH_TARGET}:${REMOTE_PATH}/build/bin/Devana" \
        "${PROJECT_ROOT}/dist/linux/Devana"

    if [[ -f "${PROJECT_ROOT}/dist/linux/Devana" ]]; then
        echo "✓ Binary pulled to dist/linux/Devana"
        echo "  Type: $(file "${PROJECT_ROOT}/dist/linux/Devana")"
    else
        echo "✗ Pull failed — binary not found on server"
        exit 1
    fi
}

# ── deploy binary helper ──────────────────────────────────────────────────────
do_deploy_binary() {
    local local_bin="${PROJECT_ROOT}/dist/linux/Devana"
    if [[ ! -f "${local_bin}" ]]; then
        echo "✗ No binary found at dist/linux/Devana"
        echo "  Build first with: ./tools/scripts/docker-build.sh"
        exit 1
    fi
    echo ""
    echo "▶ Pushing binary to ${HOST}:${REMOTE_PATH}/dist/linux/..."
    # shellcheck disable=SC2086
    ssh_run "mkdir -p ${REMOTE_PATH}/dist/linux"
    rsync -avz \
        -e "ssh ${SSH_OPTS}" \
        "${local_bin}" \
        "${SSH_TARGET}:${REMOTE_PATH}/dist/linux/Devana"
    echo "✓ Binary deployed"
}

# ── Execute mode ──────────────────────────────────────────────────────────────
case "${MODE}" in
    full)
        do_sync
        do_remote_build
        do_pull
        ;;
    source)
        do_sync
        ;;
    build)
        do_remote_build
        do_pull
        ;;
    deploy)
        do_deploy_binary
        ;;
    pull)
        do_pull
        ;;
    status)
        echo ""
        echo "▶ Remote build directory:"
        ssh_run "ls -lh ${REMOTE_PATH}/build/bin/ 2>/dev/null || echo '  (no build output yet)'"
        echo ""
        echo "▶ Last build log (tail -20):"
        ssh_run "tail -20 ${REMOTE_PATH}/logs/build.log 2>/dev/null || echo '  (no log found)'"
        ;;
esac
