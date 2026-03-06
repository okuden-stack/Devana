#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# docker-entrypoint.sh — runs inside the Arch container
#
# Mounts expected:
#   /src  — project source (read-only from host)
#   /out  — output directory for the built binary
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

BUILD_DIR="/build"
SRC_DIR="/src"
OUT_DIR="/out"

echo ""
echo "══════════════════════════════════════════════════"
echo "  Devana — Arch Linux Container Build"
echo "  ZMQ harness: ${DEVANA_ENABLE_ZMQ:-OFF}"
echo "══════════════════════════════════════════════════"
echo ""

# Copy source into a writable build staging area
# (source is mounted read-only so CMake can't write into it)
echo "▶ Copying source..."
cp -r "${SRC_DIR}/." /staging
cd /staging

# Generate build files via the devana CLI
echo "▶ Generating CMake files..."
python3 tools/devana build --regen 2>/dev/null || true

# Configure
echo "▶ Configuring..."
cmake \
    -S . \
    -B "${BUILD_DIR}" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DDEVANA_ENABLE_ZMQ="${DEVANA_ENABLE_ZMQ:-OFF}"

# Build
echo "▶ Building..."
cmake --build "${BUILD_DIR}" --parallel "$(nproc)"

# Copy output
echo "▶ Copying binary to /out..."
mkdir -p "${OUT_DIR}"
find "${BUILD_DIR}" -name "Devana" -type f -exec cp {} "${OUT_DIR}/Devana" \;

if [[ -f "${OUT_DIR}/Devana" ]]; then
    echo ""
    echo "✓ Build complete: /out/Devana"
    echo "  Size: $(du -sh "${OUT_DIR}/Devana" | cut -f1)"
    echo "  Type: $(file "${OUT_DIR}/Devana")"
    echo ""
else
    echo ""
    echo "✗ Binary not found after build — check output above"
    exit 1
fi
