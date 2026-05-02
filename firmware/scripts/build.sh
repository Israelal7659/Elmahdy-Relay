#!/usr/bin/env bash
# =============================================================================
# build.sh — Full production build for Elmahdy Relay IoT Smart Controller
#
# Pipeline:
#   1. Minify and gzip web assets (data/www/) via minify.sh
#   2. Build LittleFS filesystem image (pio buildfs)
#   3. Compile firmware (pio run)
#   4. Merge firmware.bin + littlefs.bin into a single flashable firmware.bin
#   5. Validate merged binary size (must be < 1MB)
#   6. Copy merged binary to project root as firmware_release.bin
#
# Flash map (eagle.flash.4m2m.ld — 4MB flash, 2MB LittleFS):
#   0x000000  firmware.bin   (app code, max ~1MB OTA slot)
#   0x200000  littlefs.bin   (filesystem image, 2MB)
# =============================================================================

set -e
set -o pipefail

# ---------------------------------------------------------------------------
# Paths — all relative to the firmware/ project root
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIRMWARE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${FIRMWARE_DIR}/.pio/build/nodemcuv2"
MINIFY_SCRIPT="${SCRIPT_DIR}/minify.sh"
FIRMWARE_BIN="${BUILD_DIR}/firmware.bin"
LITTLEFS_BIN="${BUILD_DIR}/littlefs.bin"
MERGED_BIN="${BUILD_DIR}/firmware_merged.bin"
RELEASE_BIN="${FIRMWARE_DIR}/../firmware_release.bin"

# LittleFS flash offset for the 4m2m partition scheme
LITTLEFS_OFFSET="0x200000"

# Size limit for the merged binary: 1MB = 1048576 bytes
MAX_MERGED_BYTES=1048576

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
step() {
    echo ""
    echo "============================================================"
    echo "  $*"
    echo "============================================================"
}

check_tool() {
    local tool="$1"
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "ERROR: '${tool}' not found in PATH." >&2
        echo "       Install it and ensure it is on your PATH before running this script." >&2
        exit 1
    fi
}

# ---------------------------------------------------------------------------
# Prerequisite checks
# ---------------------------------------------------------------------------
step "Checking prerequisites"

check_tool pio
echo "  pio      : $(pio --version 2>&1 | head -1)"

check_tool esptool.py
echo "  esptool  : $(esptool.py version 2>&1 | head -1)"

if [ ! -f "${MINIFY_SCRIPT}" ]; then
    echo "ERROR: minify.sh not found at ${MINIFY_SCRIPT}" >&2
    exit 1
fi

echo "  minify.sh: ${MINIFY_SCRIPT}"
echo ""
echo "  Firmware project : ${FIRMWARE_DIR}"
echo "  Build output dir : ${BUILD_DIR}"

# ---------------------------------------------------------------------------
# Step 1 — Minify and gzip web assets
# ---------------------------------------------------------------------------
step "Step 1/4 — Minifying and gzipping web assets"

bash "${MINIFY_SCRIPT}"

# ---------------------------------------------------------------------------
# Step 2 — Build LittleFS filesystem image
# ---------------------------------------------------------------------------
step "Step 2/4 — Building LittleFS filesystem image"

cd "${FIRMWARE_DIR}"
pio run -e nodemcuv2 -t buildfs

if [ ! -f "${LITTLEFS_BIN}" ]; then
    echo "ERROR: LittleFS image not produced at ${LITTLEFS_BIN}" >&2
    exit 1
fi

LITTLEFS_SIZE=$(wc -c < "${LITTLEFS_BIN}")
echo ""
echo "  LittleFS image : ${LITTLEFS_BIN}"
printf "  LittleFS size  : %d bytes (%.1f KB)\n" \
    "${LITTLEFS_SIZE}" "$(echo "scale=1; ${LITTLEFS_SIZE}/1024" | bc)"

# ---------------------------------------------------------------------------
# Step 3 — Compile firmware
# ---------------------------------------------------------------------------
step "Step 3/4 — Compiling firmware"

cd "${FIRMWARE_DIR}"
pio run -e nodemcuv2

if [ ! -f "${FIRMWARE_BIN}" ]; then
    echo "ERROR: Firmware binary not produced at ${FIRMWARE_BIN}" >&2
    exit 1
fi

FW_SIZE=$(wc -c < "${FIRMWARE_BIN}")
echo ""
echo "  Firmware binary : ${FIRMWARE_BIN}"
printf "  Firmware size   : %d bytes (%.1f KB)\n" \
    "${FW_SIZE}" "$(echo "scale=1; ${FW_SIZE}/1024" | bc)"

# ---------------------------------------------------------------------------
# Step 4 — Merge firmware + LittleFS into a single flashable binary
# ---------------------------------------------------------------------------
step "Step 4/4 — Merging firmware.bin + littlefs.bin"

echo "  Flash map:"
echo "    0x000000  -> firmware.bin  (app code)"
echo "    ${LITTLEFS_OFFSET}  -> littlefs.bin (filesystem)"
echo ""

esptool.py --chip esp8266 merge_bin \
    --output "${MERGED_BIN}" \
    --target-offset 0x0 \
    0x0          "${FIRMWARE_BIN}" \
    "${LITTLEFS_OFFSET}" "${LITTLEFS_BIN}"

if [ ! -f "${MERGED_BIN}" ]; then
    echo "ERROR: Merged binary not produced at ${MERGED_BIN}" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Size validation
# ---------------------------------------------------------------------------
MERGED_SIZE=$(wc -c < "${MERGED_BIN}")
printf "\n  Merged binary   : %s\n" "${MERGED_BIN}"
printf "  Merged size     : %d bytes (%.1f KB)\n" \
    "${MERGED_SIZE}" "$(echo "scale=1; ${MERGED_SIZE}/1024" | bc)"

if [ "${MERGED_SIZE}" -gt "${MAX_MERGED_BYTES}" ]; then
    echo "" >&2
    echo "ERROR: Merged binary exceeds the 1MB size limit." >&2
    printf "       Size: %d bytes  Limit: %d bytes\n" \
        "${MERGED_SIZE}" "${MAX_MERGED_BYTES}" >&2
    echo "       Reduce firmware or web asset footprint before releasing." >&2
    exit 1
fi

REMAINING=$((MAX_MERGED_BYTES - MERGED_SIZE))
printf "  Size check      : PASS (%d bytes remaining under 1MB limit)\n" "${REMAINING}"

# ---------------------------------------------------------------------------
# Copy to project root as firmware_release.bin
# ---------------------------------------------------------------------------
cp "${MERGED_BIN}" "${RELEASE_BIN}"
echo ""
echo "  Release binary  : ${RELEASE_BIN}"

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo ""
echo "============================================================"
echo "  BUILD COMPLETE"
echo "============================================================"
printf "  Firmware   : %6d bytes  (%5.1f KB)\n" \
    "${FW_SIZE}" "$(echo "scale=1; ${FW_SIZE}/1024" | bc)"
printf "  LittleFS   : %6d bytes  (%5.1f KB)\n" \
    "${LITTLEFS_SIZE}" "$(echo "scale=1; ${LITTLEFS_SIZE}/1024" | bc)"
printf "  Merged     : %6d bytes  (%5.1f KB)  [limit: 1024 KB]\n" \
    "${MERGED_SIZE}" "$(echo "scale=1; ${MERGED_SIZE}/1024" | bc)"
echo ""
echo "  Flash with Tasmotizer:"
echo "    firmware_release.bin  @0x0  (single-file full flash)"
echo "============================================================"
