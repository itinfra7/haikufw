#!/usr/bin/env bash
set -euo pipefail

CTL_PATH="${CTL_PATH:-/boot/system/non-packaged/bin/haikufwctl}"
CONFIG_PATH="${CONFIG_PATH:-/boot/system/settings/haikufw.conf}"
RULES_PATH="${RULES_PATH:-/boot/system/settings/haikufw.rules}"
DEVICE_PATH="${DEVICE_PATH:-/dev/misc/haikufw}"
WAIT_SECONDS="${WAIT_SECONDS:-60}"

log() {
	printf '[haikufw-boot] %s\n' "$*" >>/tmp/haikufw-boot.log
}

wait_for_device() {
	local remaining="$WAIT_SECONDS"
	while [ "$remaining" -gt 0 ]; do
		if [ -e "$DEVICE_PATH" ]; then
			return 0
		fi
		sleep 1
		remaining=$((remaining - 1))
	done
	return 1
}

main() {
	: > /tmp/haikufw-boot.log

	if [ ! -x "$CTL_PATH" ]; then
		log "haikufwctl not found"
		exit 0
	fi

	if [ ! -f "$CONFIG_PATH" ]; then
		log "config missing: $CONFIG_PATH"
		exit 0
	fi

	log "waiting for $DEVICE_PATH"
	if ! wait_for_device; then
		log "device did not appear"
		exit 0
	fi

	log "reloading rules"
	"$CTL_PATH" reload "$CONFIG_PATH" "$RULES_PATH" "$(date +%s)" \
		>/tmp/haikufw-reload.out 2>/tmp/haikufw-reload.err
	log "reload complete"
}

main "$@"
