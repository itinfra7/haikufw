#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
RELEASE_DIR="${RELEASE_DIR:-$PROJECT_DIR/release}"
PACKAGE_ROOT="$RELEASE_DIR/haikufw"

log() {
	printf '[haikufw-release] %s\n' "$*"
}

need_cmd() {
	command -v "$1" >/dev/null 2>&1 || {
		printf '[haikufw-release] missing command: %s\n' "$1" >&2
		exit 1
	}
}

copy_tree() {
	local src="$1"
	local dst="$2"
	mkdir -p "$(dirname "$dst")"
	cp -a "$src" "$dst"
}

write_manifest() {
	cat >"$PACKAGE_ROOT/MANIFEST.txt" <<'EOF'
haikufw release contents

top-level docs
- README.md
- README.en.txt
- README.ko.txt
- OPERATIONS.txt
- LICENSE

source directories
- config/
- kernel/
- patches/
- scripts/
- tests/
- userland/
- vendor/

notes
- This release directory is distribution-oriented.
- Internal planning and research notes are intentionally excluded.
- The required Haiku IPv4/IPv6 protocol source subset is bundled under `vendor/`.
- Build on the target Haiku host with:
  - ./scripts/build-haikufw-haiku.sh
- Install on the target Haiku host with:
  - ./scripts/install-haikufw.sh install
EOF
}

write_checksums() {
	(
		cd "$PACKAGE_ROOT"
		find . -type f | sort | sed 's#^\./##' | while read -r path; do
			sha256sum "$path"
		done
	) >"$RELEASE_DIR/SHA256SUMS.txt"
	(
		cd "$RELEASE_DIR"
		sha256sum haikufw-release.tar.gz
	) >>"$RELEASE_DIR/SHA256SUMS.txt"
}

write_tarball() {
	(
		cd "$RELEASE_DIR"
		tar -czf haikufw-release.tar.gz haikufw
	)
}

main() {
	need_cmd cp
	need_cmd find
	need_cmd sha256sum
	need_cmd tar

	rm -rf "$RELEASE_DIR"
	mkdir -p "$PACKAGE_ROOT"

	copy_tree "$PROJECT_DIR/config" "$PACKAGE_ROOT/config"
	copy_tree "$PROJECT_DIR/kernel" "$PACKAGE_ROOT/kernel"
	copy_tree "$PROJECT_DIR/patches" "$PACKAGE_ROOT/patches"
	copy_tree "$PROJECT_DIR/scripts" "$PACKAGE_ROOT/scripts"
	copy_tree "$PROJECT_DIR/tests" "$PACKAGE_ROOT/tests"
	copy_tree "$PROJECT_DIR/userland" "$PACKAGE_ROOT/userland"
	copy_tree "$PROJECT_DIR/vendor" "$PACKAGE_ROOT/vendor"

	cp "$PROJECT_DIR/README.md" "$PACKAGE_ROOT/README.md"
	cp "$PROJECT_DIR/MANUAL.en.txt" "$PACKAGE_ROOT/README.en.txt"
	cp "$PROJECT_DIR/MANUAL.ko.txt" "$PACKAGE_ROOT/README.ko.txt"
	cp "$PROJECT_DIR/OPERATIONS.txt" "$PACKAGE_ROOT/OPERATIONS.txt"
	cp "$PROJECT_DIR/LICENSE" "$PACKAGE_ROOT/LICENSE"

	write_manifest
	write_tarball
	write_checksums

	log "release assembled at $RELEASE_DIR"
}

main "$@"
