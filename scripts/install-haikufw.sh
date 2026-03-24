#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

ARTIFACT_DIR="${ARTIFACT_DIR:-$PROJECT_DIR/out/haiku}"
BUILD_SCRIPT_PATH="${BUILD_SCRIPT_PATH:-$PROJECT_DIR/scripts/build-haikufw-haiku.sh}"

NON_PACKAGED_ROOT="${NON_PACKAGED_ROOT:-/boot/system/non-packaged}"
BIN_DIR="${BIN_DIR:-$NON_PACKAGED_ROOT/bin}"
PROTOCOL_DIR="${PROTOCOL_DIR:-$NON_PACKAGED_ROOT/add-ons/kernel/network/protocols}"
DRIVER_DIR="${DRIVER_DIR:-$NON_PACKAGED_ROOT/add-ons/kernel/drivers/dev/misc}"
GENERIC_DIR="${GENERIC_DIR:-$NON_PACKAGED_ROOT/add-ons/kernel/generic}"
STATE_DIR="${STATE_DIR:-$NON_PACKAGED_ROOT/data/haikufw}"
BACKUP_DIR="${BACKUP_DIR:-$STATE_DIR/backups}"
INSTALL_MANIFEST="${INSTALL_MANIFEST:-$STATE_DIR/install-manifest.tsv}"
CONFIG_CREATED_MARKER="${CONFIG_CREATED_MARKER:-$STATE_DIR/config-created}"
CONFIG_DIR="${CONFIG_DIR:-/boot/system/settings}"
BOOTSCRIPT_PATH="${BOOTSCRIPT_PATH:-/boot/home/config/settings/boot/UserBootscript}"

CTL_ARTIFACT="$ARTIFACT_DIR/haikufwctl"
CORE_ARTIFACT="$ARTIFACT_DIR/haikufw_core"
DRIVER_ARTIFACT="$ARTIFACT_DIR/haikufw_driver"
IPV4_ARTIFACT="$ARTIFACT_DIR/ipv4"
IPV6_ARTIFACT="$ARTIFACT_DIR/ipv6"

CTL_PATH="$BIN_DIR/haikufwctl"
BOOT_HELPER_PATH="$BIN_DIR/haikufw-boot.sh"
CONFIG_PATH="$CONFIG_DIR/haikufw.conf"
RULES_PATH="$CONFIG_DIR/haikufw.rules"
CORE_PATH="$GENERIC_DIR/haikufw_core"
DRIVER_PATH="$DRIVER_DIR/haikufw"
IPV4_PATH="$PROTOCOL_DIR/ipv4"
IPV6_PATH="$PROTOCOL_DIR/ipv6"
LEGACY_PROTOCOL_PATH="$PROTOCOL_DIR/haikufw"
LEGACY_TCP_PATH="$PROTOCOL_DIR/tcp"
LEGACY_UDP_PATH="$PROTOCOL_DIR/udp"

BOOT_MARKER_BEGIN="# BEGIN HAIKUFW BOOT"
BOOT_MARKER_END="# END HAIKUFW BOOT"

log() {
	printf '[haikufw-install] %s\n' "$*"
}

fail() {
	printf '[haikufw-install] ERROR: %s\n' "$*" >&2
	exit 1
}

need_haiku() {
	[ "$(uname -s)" = "Haiku" ] || fail "this installer must run on Haiku"
}

need_cmd() {
	command -v "$1" >/dev/null 2>&1 || fail "missing command: $1"
}

remove_marked_block() {
	local target_path="$1"
	local marker_begin="$2"
	local marker_end="$3"
	local tmp_path

	[ -e "$target_path" ] || return 0

	tmp_path="${target_path}.tmp.$$"
	awk -v begin="$marker_begin" -v end="$marker_end" '
BEGIN { skip = 0 }
$0 == begin { skip = 1; next }
$0 == end { skip = 0; next }
skip { next }
{ print }
' "$target_path" >"$tmp_path"
	mv "$tmp_path" "$target_path"
}

prepare_state_dirs() {
	mkdir -p "$BIN_DIR" "$PROTOCOL_DIR" "$DRIVER_DIR" "$GENERIC_DIR" "$CONFIG_DIR" \
		"$(dirname "$BOOTSCRIPT_PATH")" "$STATE_DIR"
}

require_artifacts() {
	local missing=0
	local path
	for path in \
		"$CTL_ARTIFACT" \
		"$CORE_ARTIFACT" \
		"$DRIVER_ARTIFACT" \
		"$IPV4_ARTIFACT" \
		"$IPV6_ARTIFACT"; do
		if [ ! -f "$path" ]; then
			printf '[haikufw-install] missing build artifact: %s\n' "$path" >&2
			missing=1
		fi
	done
	[ "$missing" -eq 0 ] || fail "build artifacts are missing"
}

build_if_needed() {
	if [ "${SKIP_BUILD:-0}" = "1" ]; then
		require_artifacts
		return
	fi

	if [ ! -f "$CTL_ARTIFACT" ] || [ ! -f "$CORE_ARTIFACT" ] \
		|| [ ! -f "$DRIVER_ARTIFACT" ] \
		|| [ ! -f "$IPV4_ARTIFACT" ] || [ ! -f "$IPV6_ARTIFACT" ]; then
		log "artifacts missing; invoking build"
		"$BUILD_SCRIPT_PATH"
	fi

	require_artifacts
}

manifest_append() {
	printf '%s\t%s\t%s\n' "$1" "$2" "$3" >>"$INSTALL_MANIFEST"
}

backup_target() {
	local target_path="$1"
	local backup_path

	[ -e "$target_path" ] || return 1

	backup_path="$BACKUP_DIR$target_path"
	mkdir -p "$(dirname "$backup_path")"
	cp -p "$target_path" "$backup_path"
	manifest_append restore "$target_path" "$backup_path"
	return 0
}

install_target() {
	local source_path="$1"
	local target_path="$2"
	local mode="$3"

	if ! backup_target "$target_path"; then
		manifest_append remove "$target_path" ""
	fi

	mkdir -p "$(dirname "$target_path")"
	cp "$source_path" "$target_path"
	chmod "$mode" "$target_path"
}

install_runtime_files() {
	rm -rf "$BACKUP_DIR"
	rm -f "$INSTALL_MANIFEST"

	install_target "$CTL_ARTIFACT" "$CTL_PATH" 755
	install_target "$CORE_ARTIFACT" "$CORE_PATH" 644
	install_target "$DRIVER_ARTIFACT" "$DRIVER_PATH" 644
	install_target "$IPV4_ARTIFACT" "$IPV4_PATH" 644
	install_target "$IPV6_ARTIFACT" "$IPV6_PATH" 644
	install_target "$PROJECT_DIR/scripts/haikufw-boot.sh" "$BOOT_HELPER_PATH" 755

	if ! backup_target "$LEGACY_PROTOCOL_PATH"; then
		manifest_append remove "$LEGACY_PROTOCOL_PATH" ""
	fi
	rm -f "$LEGACY_PROTOCOL_PATH"

	if ! backup_target "$LEGACY_TCP_PATH"; then
		manifest_append remove "$LEGACY_TCP_PATH" ""
	fi
	rm -f "$LEGACY_TCP_PATH"

	if ! backup_target "$LEGACY_UDP_PATH"; then
		manifest_append remove "$LEGACY_UDP_PATH" ""
	fi
	rm -f "$LEGACY_UDP_PATH"

	rm -f "$CONFIG_CREATED_MARKER"
	if [ ! -f "$CONFIG_PATH" ]; then
		cp "$PROJECT_DIR/config/haikufw.conf.example" "$CONFIG_PATH"
		printf '%s\n' "$CONFIG_PATH" >"$CONFIG_CREATED_MARKER"
	fi
}

register_boot() {
	local tmp_path="${BOOTSCRIPT_PATH}.tmp.$$"
	mkdir -p "$(dirname "$BOOTSCRIPT_PATH")"
	touch "$BOOTSCRIPT_PATH"
	remove_marked_block "$BOOTSCRIPT_PATH" "$BOOT_MARKER_BEGIN" "$BOOT_MARKER_END"
	cp "$BOOTSCRIPT_PATH" "$tmp_path"
	cat >>"$tmp_path" <<EOF

$BOOT_MARKER_BEGIN
if [ -x "$BOOT_HELPER_PATH" ]; then
	/bin/bash "$BOOT_HELPER_PATH" >/tmp/haikufw-boot-run.out 2>/tmp/haikufw-boot-run.err </dev/null &
fi
$BOOT_MARKER_END
EOF
	mv "$tmp_path" "$BOOTSCRIPT_PATH"
}

unregister_boot() {
	remove_marked_block "$BOOTSCRIPT_PATH" "$BOOT_MARKER_BEGIN" "$BOOT_MARKER_END"
}

restore_installed_files() {
	[ -f "$INSTALL_MANIFEST" ] || return 0

	local action target backup
	while IFS="$(printf '\t')" read -r action target backup; do
		case "$action" in
			restore)
				if [ -f "$backup" ]; then
					mkdir -p "$(dirname "$target")"
					cp -p "$backup" "$target"
				else
					rm -f "$target"
				fi
				;;
			remove)
				rm -f "$target"
				;;
		esac
	done <"$INSTALL_MANIFEST"

	rm -rf "$BACKUP_DIR"
	rm -f "$INSTALL_MANIFEST"
}

remove_generated_config() {
	if [ -f "$CONFIG_CREATED_MARKER" ]; then
		rm -f "$CONFIG_PATH"
	fi
	rm -f "$CONFIG_CREATED_MARKER"
}

try_clear_runtime_rules() {
	if [ -x "$CTL_PATH" ] && [ -e "/dev/misc/haikufw" ]; then
		"$CTL_PATH" clear >/tmp/haikufw-clear.out 2>/tmp/haikufw-clear.err || true
	fi
}

do_install() {
	need_haiku
	need_cmd awk
	need_cmd cp
	need_cmd chmod
	need_cmd mkdir
	need_cmd rm
	prepare_state_dirs
	build_if_needed
	install_runtime_files
	register_boot
	if [ -e "/dev/misc/haikufw" ]; then
		"$CTL_PATH" reload "$CONFIG_PATH" "$RULES_PATH" "$(date +%s)" \
			>/tmp/haikufw-install-reload.out 2>/tmp/haikufw-install-reload.err || true
	fi
	sync
	log "kernel firewall installed; reboot required for patched ipv4/ipv6 modules"
}

do_uninstall() {
	need_haiku
	unregister_boot
	try_clear_runtime_rules
	restore_installed_files
	remove_generated_config
	rm -f "$RULES_PATH"
	rmdir "$STATE_DIR" >/dev/null 2>&1 || true
	sync
	log "kernel firewall files removed; reboot required to restore stock ipv4/ipv6 modules"
}

main() {
	local action="${1:-install}"
	case "$action" in
		install)
			do_install
			;;
		uninstall)
			do_uninstall
			;;
		*)
			fail "usage: $0 [install|uninstall]"
			;;
	esac
}

main "$@"
