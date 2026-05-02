#!/usr/bin/env bash
# minify.sh — Web asset minification pipeline for Elmahdy Relay firmware
#
# Minifies HTML/CSS/JS assets in firmware/data/www/, then gzip-compresses each
# file alongside the original (index.html → index.html + index.html.gz).
#
# Run from either firmware/ or firmware/scripts/; paths are always resolved
# relative to the location of this script.
#
# Required tools (all stdin→stdout):
#   html-minifier-terser   npm install -g html-minifier-terser
#   csso                   npm install -g csso-cli
#   terser                 npm install -g terser
#   gzip                   system package (pre-installed on Linux and macOS)

set -euo pipefail

# ---------------------------------------------------------------------------
# Path resolution — works regardless of call site (firmware/ or scripts/)
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIRMWARE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
WWW_DIR="${FIRMWARE_DIR}/data/www"

# ---------------------------------------------------------------------------
# Colour helpers — degrade gracefully when not a colour terminal
# ---------------------------------------------------------------------------
if [ -t 1 ] && command -v tput &>/dev/null && tput colors &>/dev/null \
   && [ "$(tput colors)" -ge 8 ]; then
    RED="$(tput setaf 1)"
    GREEN="$(tput setaf 2)"
    YELLOW="$(tput setaf 3)"
    CYAN="$(tput setaf 6)"
    BOLD="$(tput bold)"
    RESET="$(tput sgr0)"
else
    RED="" GREEN="" YELLOW="" CYAN="" BOLD="" RESET=""
fi

info()    { printf "%s[INFO]%s  %s\n"  "${CYAN}"   "${RESET}" "$*"; }
success() { printf "%s[OK]%s    %s\n"  "${GREEN}"  "${RESET}" "$*"; }
warn()    { printf "%s[WARN]%s  %s\n"  "${YELLOW}" "${RESET}" "$*" >&2; }
error()   { printf "%s[ERR]%s   %s\n"  "${RED}"    "${RESET}" "$*" >&2; }

# ---------------------------------------------------------------------------
# Portable byte-count (GNU stat on Linux, BSD stat on macOS)
# ---------------------------------------------------------------------------
file_bytes() {
    if stat --version &>/dev/null 2>&1; then
        stat --format="%s" "$1"   # GNU coreutils
    else
        stat -f "%z" "$1"         # BSD / macOS
    fi
}

# Human-readable size
human_size() {
    local bytes="$1"
    if [ "${bytes}" -ge 1024 ]; then
        printf "%d KB (%d B)" $(( bytes / 1024 )) "${bytes}"
    else
        printf "%d B" "${bytes}"
    fi
}

# ---------------------------------------------------------------------------
# 1. Dependency checks — fail fast with a clear install hint
# ---------------------------------------------------------------------------
MISSING_TOOLS=()
for tool in html-minifier-terser csso terser gzip; do
    if ! command -v "${tool}" &>/dev/null; then
        MISSING_TOOLS+=("${tool}")
    fi
done

if [ "${#MISSING_TOOLS[@]}" -gt 0 ]; then
    error "The following required tools are not installed or not in PATH:"
    for t in "${MISSING_TOOLS[@]}"; do
        error "  - ${t}"
    done
    printf "\n"
    error "Install missing npm tools with:"
    error "  npm install -g html-minifier-terser csso-cli terser"
    exit 1
fi

# ---------------------------------------------------------------------------
# 2. Verify the web-asset directory exists
# ---------------------------------------------------------------------------
if [ ! -d "${WWW_DIR}" ]; then
    error "Web assets directory not found: ${WWW_DIR}"
    error "Expected firmware/data/www/ to exist."
    exit 1
fi

info "Minification pipeline starting"
info "Source: ${WWW_DIR}"
printf "\n"

# ---------------------------------------------------------------------------
# 3. Core helper: minify a file in-place, then create a .gz sibling
#
# Usage: process_file <src_file> <label> <cmd> [args...]
#
# <cmd> and its optional [args] must accept the source content on stdin and
# write the minified result to stdout.  The function handles all temp-file
# management and size reporting so each per-tool call stays concise.
# ---------------------------------------------------------------------------
process_file() {
    local src="$1"
    local label="$2"
    shift 2
    # $@ is now the minifier command + any extra arguments

    if [ ! -f "${src}" ]; then
        warn "${label}: file not found (${src}) — skipping"
        return 0
    fi

    local before
    before="$(file_bytes "${src}")"

    # Stage output in a temp file; on failure the original is untouched
    local tmp_out
    tmp_out="$(mktemp)"

    # Run the minifier: stdin comes from the original file, stdout goes to tmp
    if ! "$@" < "${src}" > "${tmp_out}" 2>&1; then
        error "${label}: minifier exited with an error — original file unchanged"
        rm -f "${tmp_out}"
        return 1
    fi

    if [ ! -s "${tmp_out}" ]; then
        error "${label}: minifier produced empty output — original file unchanged"
        rm -f "${tmp_out}"
        return 1
    fi

    # Replace the original atomically
    mv "${tmp_out}" "${src}"

    local after gz_size
    after="$(file_bytes "${src}")"

    # Produce a .gz sibling (-k keeps the original, -f overwrites any prior .gz)
    gzip -9 -k -f "${src}"
    gz_size="$(file_bytes "${src}.gz")"

    local saved pct
    saved=$(( before - after ))
    pct=0
    if [ "${before}" -gt 0 ]; then
        pct=$(( saved * 100 / before ))
    fi

    printf "  %-14s  before: %-16s  after: %-16s  gzip: %-16s  saved: %d%%\n" \
        "${label}" \
        "$(human_size "${before}")" \
        "$(human_size "${after}")" \
        "$(human_size "${gz_size}")" \
        "${pct}"
    success "${label}: minification complete"
    printf "\n"
}

# ---------------------------------------------------------------------------
# 4. Minify each web asset
#
# Every tool below reads from stdin and writes to stdout — process_file
# handles the redirection.  No --file / --input flags are used here because
# those would cause the tools to ignore stdin and bypass the safety wrapper.
# ---------------------------------------------------------------------------

# index.html — html-minifier-terser (reads stdin when no positional arg given)
process_file "${WWW_DIR}/index.html" "index.html" \
    html-minifier-terser \
        --collapse-whitespace \
        --remove-comments \
        --remove-redundant-attributes \
        --remove-script-type-attributes \
        --remove-style-link-type-attributes \
        --use-short-doctype \
        --collapse-boolean-attributes \
        --minify-css true \
        --minify-js true

# style.css — csso reads from stdin when called with no positional arguments
process_file "${WWW_DIR}/style.css" "style.css" \
    csso

# app.js — terser reads from stdin when no input file is specified
process_file "${WWW_DIR}/app.js" "app.js" \
    terser \
        --compress \
        --mangle \
        --ecma 5

# ---------------------------------------------------------------------------
# 5. Summary — list every .gz file and report the total compressed size
# ---------------------------------------------------------------------------
printf "%s--- Summary ---%s\n" "${BOLD}" "${RESET}"

total_gz=0
while IFS= read -r -d '' gz_file; do
    gz_bytes="$(file_bytes "${gz_file}")"
    total_gz=$(( total_gz + gz_bytes ))
    printf "  %-40s  %s\n" \
        "${gz_file#"${FIRMWARE_DIR}/"}" \
        "$(human_size "${gz_bytes}")"
done < <(find "${WWW_DIR}" -maxdepth 1 -name "*.gz" -print0 | sort -z)

printf "\n"
printf "  %sTotal compressed www/ size : %s%s\n" \
    "${BOLD}" "$(human_size "${total_gz}")" "${RESET}"

# Soft budget: keep www/ .gz assets under 400 KB so the 512 KB LittleFS
# partition still has headroom for JSON config files and language packs.
BUDGET_BYTES=$(( 400 * 1024 ))
if [ "${total_gz}" -gt "${BUDGET_BYTES}" ]; then
    warn "Compressed assets exceed the soft budget of 400 KB"
    warn "(LittleFS partition limit is 512 KB — consider optimising further)"
else
    success "Assets are within the 400 KB soft budget (512 KB LittleFS limit)"
fi

printf "\n"
success "Minification pipeline complete."
