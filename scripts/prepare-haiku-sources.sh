#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

VENDOR_ROOT="${VENDOR_ROOT:-$PROJECT_DIR/vendor/haiku}"
PATCH_DIR="${PATCH_DIR:-$PROJECT_DIR/patches}"
OUTPUT_ROOT="${1:-$PROJECT_DIR/work/haiku-src}"

log() {
	printf '[haikufw-prepare] %s\n' "$*"
}

need_cmd() {
	command -v "$1" >/dev/null 2>&1 || {
		printf '[haikufw-prepare] missing command: %s\n' "$1" >&2
		exit 1
	}
}

main() {
	need_cmd cp
	need_cmd patch

	[ -d "$VENDOR_ROOT/src/add-ons/kernel/network/protocols/ipv4" ] \
		|| {
			printf '[haikufw-prepare] missing vendored IPv4 sources under %s\n' \
				"$VENDOR_ROOT/src/add-ons/kernel/network/protocols/ipv4" >&2
			printf '[haikufw-prepare] this package is incomplete; ensure vendor/ is present in the release tree\n' >&2
			exit 1
		}
	[ -d "$VENDOR_ROOT/src/add-ons/kernel/network/protocols/ipv6" ] \
		|| {
			printf '[haikufw-prepare] missing vendored IPv6 sources under %s\n' \
				"$VENDOR_ROOT/src/add-ons/kernel/network/protocols/ipv6" >&2
			printf '[haikufw-prepare] this package is incomplete; ensure vendor/ is present in the release tree\n' >&2
			exit 1
		}

	rm -rf "$OUTPUT_ROOT"
	mkdir -p "$OUTPUT_ROOT"

	log "copying vendored Haiku network protocol sources"
	cp -R "$VENDOR_ROOT/src" "$OUTPUT_ROOT/"

	log "applying ipv4 receive hook patch"
	(
		cd "$OUTPUT_ROOT"
		patch --no-backup-if-mismatch -p1 < "$PATCH_DIR/ipv4-receive-hook.patch"
	)

	log "applying ipv6 receive hook patch"
	(
		cd "$OUTPUT_ROOT"
		patch --no-backup-if-mismatch -p1 < "$PATCH_DIR/ipv6-receive-hook.patch"
	)

	log "prepared sources at $OUTPUT_ROOT"
}

main "$@"
