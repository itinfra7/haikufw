#!/usr/bin/env bash
set -euo pipefail

CTL_PATH="${CTL_PATH:-/boot/system/non-packaged/bin/haikufwctl}"
CONFIG_PATH="${CONFIG_PATH:-/boot/system/settings/haikufw.conf}"
RULES_PATH="${RULES_PATH:-/boot/system/settings/haikufw.rules}"
TMP_CONFIG="${TMP_CONFIG:-/tmp/haikufw-live-smoke.conf}"
TMP_RULES="${TMP_RULES:-/tmp/haikufw-live-smoke.rules}"
TCP_PORT="${TCP_PORT:-12345}"
UDP_PORT="${UDP_PORT:-12346}"

TCP_PID=""
UDP_PID=""

log() {
	printf '[haikufw-live-smoke] %s\n' "$*"
}

cleanup() {
	local exit_code="$?"
	if [ -n "$TCP_PID" ]; then
		kill "$TCP_PID" >/dev/null 2>&1 || true
	fi
	if [ -n "$UDP_PID" ]; then
		kill "$UDP_PID" >/dev/null 2>&1 || true
	fi
	rm -f "$TMP_CONFIG" "$TMP_RULES"
	if [ -x "$CTL_PATH" ] && [ -f "$CONFIG_PATH" ]; then
		"$CTL_PATH" reload "$CONFIG_PATH" "$RULES_PATH" "$(date +%s)" \
			>/tmp/haikufw-live-smoke-restore.out 2>/tmp/haikufw-live-smoke-restore.err || true
	fi
	exit "$exit_code"
}

trap cleanup EXIT INT TERM HUP

need_cmd() {
	command -v "$1" >/dev/null 2>&1 || {
		printf '[haikufw-live-smoke] missing command: %s\n' "$1" >&2
		exit 1
	}
}

start_tcp_server() {
	python3 - "$TCP_PORT" <<'PY' &
import socket
import sys

port = int(sys.argv[1])
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", port))
s.listen(16)

while True:
    conn, _addr = s.accept()
    try:
        conn.sendall(b"OK")
    finally:
        conn.close()
PY
	TCP_PID="$!"
}

start_udp_server() {
	python3 - "$UDP_PORT" <<'PY' &
import socket
import sys

port = int(sys.argv[1])
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.bind(("127.0.0.1", port))

while True:
    data, addr = s.recvfrom(2048)
    s.sendto(b"OK", addr)
PY
	UDP_PID="$!"
}

tcp_expect() {
	local mode="$1"
	local port="$2"
	python3 - "$mode" "$port" <<'PY'
import socket
import sys

mode = sys.argv[1]
port = int(sys.argv[2])

try:
    s = socket.create_connection(("127.0.0.1", port), timeout=3)
    try:
        s.recv(16)
    finally:
        s.close()
    ok = True
except Exception:
    ok = False

if mode == "allow" and not ok:
    raise SystemExit(1)
if mode == "deny" and ok:
    raise SystemExit(2)
PY
}

udp_expect() {
	local mode="$1"
	local port="$2"
	python3 - "$mode" "$port" <<'PY'
import socket
import sys

mode = sys.argv[1]
port = int(sys.argv[2])

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.settimeout(3)
ok = False
try:
    s.sendto(b"PING", ("127.0.0.1", port))
    data, _ = s.recvfrom(16)
    ok = (data == b"OK")
except Exception:
    ok = False
finally:
    s.close()

if mode == "allow" and not ok:
    raise SystemExit(1)
if mode == "deny" and ok:
    raise SystemExit(2)
PY
}

main() {
	[ "$(uname -s)" = "Haiku" ] || {
		printf '[haikufw-live-smoke] this script must run on Haiku\n' >&2
		exit 1
	}

	need_cmd python3
	[ -x "$CTL_PATH" ] || {
		printf '[haikufw-live-smoke] missing %s\n' "$CTL_PATH" >&2
		exit 1
	}

	log "starting loopback TCP/UDP echo servers"
	start_tcp_server
	start_udp_server
	sleep 1

	log "verifying baseline allow"
	tcp_expect allow "$TCP_PORT"
	udp_expect allow "$UDP_PORT"

	log "loading deny rules for loopback test ports"
	cat >"$TMP_CONFIG" <<EOF
default_inbound = allow
deny tcp from 127.0.0.1/32 to port $TCP_PORT
deny udp from 127.0.0.1/32 to port $UDP_PORT
EOF
	"$CTL_PATH" reload "$TMP_CONFIG" "$TMP_RULES" "$(date +%s)" \
		>/tmp/haikufw-live-smoke-reload.out 2>/tmp/haikufw-live-smoke-reload.err

	log "verifying deny path"
	tcp_expect deny "$TCP_PORT"
	udp_expect deny "$UDP_PORT"

	log "restoring configured rules"
	"$CTL_PATH" reload "$CONFIG_PATH" "$RULES_PATH" "$(date +%s)" \
		>/tmp/haikufw-live-smoke-restore.out 2>/tmp/haikufw-live-smoke-restore.err

	log "verifying allow after restore"
	tcp_expect allow "$TCP_PORT"
	udp_expect allow "$UDP_PORT"

	log "live rule smoke ok"
}

main "$@"
