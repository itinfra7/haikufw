#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

log() {
	printf '[haikufw-test] %s\n' "$*"
}

log "building haikufwctl"
make -C "$PROJECT_DIR/userland/haikufwctl" clean
make -C "$PROJECT_DIR/userland/haikufwctl"

log "building host smoke test"
c++ -std=c++17 -Wall -Wextra -O2 \
	-I"$PROJECT_DIR/kernel/core" \
	-I"$PROJECT_DIR/userland/haikufwctl" \
	"$PROJECT_DIR/tests/host_smoke_test.cpp" \
	"$PROJECT_DIR/kernel/core/haikufw_match.cpp" \
	"$PROJECT_DIR/userland/haikufwctl/config_parser.cpp" \
	"$PROJECT_DIR/userland/haikufwctl/compiler.cpp" \
	-o /tmp/haikufw_host_smoke_test

log "running host smoke test"
/tmp/haikufw_host_smoke_test

log "cleaning userland build artifacts"
make -C "$PROJECT_DIR/userland/haikufwctl" clean >/dev/null
rm -f /tmp/haikufw_host_smoke_test
