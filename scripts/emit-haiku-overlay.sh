#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_ROOT="${1:-$PROJECT_DIR/work/overlay}"
TMP_SRC_ROOT="$PROJECT_DIR/work/haiku-src"

log() {
	printf '[haikufw-overlay] %s\n' "$*"
}

need_cmd() {
	command -v "$1" >/dev/null 2>&1 || {
		printf '[haikufw-overlay] missing command: %s\n' "$1" >&2
		exit 1
	}
}

copy_tree() {
	local src="$1"
	local dst="$2"
	mkdir -p "$dst"
	cp -R "$src"/. "$dst"/
}

main() {
	need_cmd cp
	need_cmd mkdir

	log "preparing patched ipv4/ipv6 protocol sources"
	"$SCRIPT_DIR/prepare-haiku-sources.sh" "$TMP_SRC_ROOT" >/tmp/haikufw_prepare_overlay.log

	log "resetting overlay root"
	rm -rf "$OUTPUT_ROOT"
	mkdir -p "$OUTPUT_ROOT/src/add-ons/kernel"

	log "copying haikufw core"
	copy_tree "$PROJECT_DIR/kernel/core" \
		"$OUTPUT_ROOT/src/add-ons/kernel/generic/haikufw_core"

	log "copying haikufw control driver"
	copy_tree "$PROJECT_DIR/kernel/control_driver" \
		"$OUTPUT_ROOT/src/add-ons/kernel/drivers/dev/misc/haikufw"

	log "copying standalone kernel build shims"
	copy_tree "$PROJECT_DIR/kernel/shims" \
		"$OUTPUT_ROOT/headers/haikufw_shims"

	log "copying patched ipv4 and ipv6"
	copy_tree "$TMP_SRC_ROOT/src/add-ons/kernel/network/protocols/ipv4" \
		"$OUTPUT_ROOT/src/add-ons/kernel/network/protocols/ipv4"
	copy_tree "$TMP_SRC_ROOT/src/add-ons/kernel/network/protocols/ipv6" \
		"$OUTPUT_ROOT/src/add-ons/kernel/network/protocols/ipv6"

	log "overlay ready at $OUTPUT_ROOT"
}

main "$@"
