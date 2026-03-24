#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

OVERLAY_ROOT="${OVERLAY_ROOT:-$PROJECT_DIR/work/overlay}"
BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/work/haiku-build}"
ARTIFACT_DIR="${ARTIFACT_DIR:-$PROJECT_DIR/out/haiku}"

CXX="${CXX:-g++}"
CXXFLAGS_COMMON=(
	-O2
	-fPIC
	-ffreestanding
	-Wall
	-Wextra
	-Wno-unused-parameter
	-Wno-deprecated-copy
	-fno-strict-aliasing
	-fno-delete-null-pointer-checks
	-fno-stack-protector
	-finline
	-fno-builtin
	-fno-exceptions
	-fno-rtti
	-fno-threadsafe-statics
	-fno-use-cxa-atexit
	-D_KERNEL_MODE
)

INCLUDE_DIRS=(
	"$OVERLAY_ROOT/headers/haikufw_shims"
	"/boot/system/develop/headers/os"
	"/boot/system/develop/headers/os/drivers"
	"/boot/system/develop/headers/private"
	"/boot/system/develop/headers/private/kernel"
	"/boot/system/develop/headers/private/kernel/arch"
	"/boot/system/develop/headers/private/kernel/arch/x86"
	"/boot/system/develop/headers/private/kernel/arch/x86/64"
	"/boot/system/develop/headers/private/net"
	"/boot/system/develop/headers/private/shared"
	"$OVERLAY_ROOT/src/add-ons/kernel/generic/haikufw_core"
	"$OVERLAY_ROOT/src/add-ons/kernel/drivers/dev/misc/haikufw"
	"$OVERLAY_ROOT/src/add-ons/kernel/network/protocols/ipv4"
	"$OVERLAY_ROOT/src/add-ons/kernel/network/protocols/ipv6"
)

log() {
	printf '[haikufw-build] %s\n' "$*"
}

fail() {
	printf '[haikufw-build] ERROR: %s\n' "$*" >&2
	exit 1
}

need_cmd() {
	command -v "$1" >/dev/null 2>&1 || fail "missing command: $1"
}

need_haiku() {
	[ "$(uname -s)" = "Haiku" ] || fail "this build script must run on Haiku"
}

append_include_flags() {
	local dir
	for dir in "${INCLUDE_DIRS[@]}"; do
		CXXFLAGS_COMPILE+=("-I$dir")
	done
}

resolve_toolchain_paths() {
	local libgcc_path
	libgcc_path="$("$CXX" -print-libgcc-file-name)"
	GCC_LIB_DIR="$(dirname "$libgcc_path")"
	CRTBEGIN="$GCC_LIB_DIR/crtbeginS.o"
	CRTEND="$GCC_LIB_DIR/crtendS.o"
	LIBGCC_KERNEL="$GCC_LIB_DIR/libgcc-kernel.a"
	HAIKU_VERSION_GLUE="/boot/system/develop/lib/haiku_version_glue.o"
	KERNEL_IMPORT_LIB="/boot/system/develop/lib/_KERNEL_"

	[ -f "$CRTBEGIN" ] || fail "missing $CRTBEGIN"
	[ -f "$CRTEND" ] || fail "missing $CRTEND"
	[ -f "$LIBGCC_KERNEL" ] || fail "missing $LIBGCC_KERNEL"
	[ -f "$HAIKU_VERSION_GLUE" ] || fail "missing $HAIKU_VERSION_GLUE"
	[ -f "$KERNEL_IMPORT_LIB" ] || fail "missing $KERNEL_IMPORT_LIB"
}

append_arch_flags() {
	local arch
	arch="$(uname -m)"
	case "$arch" in
		x86_64)
			CXXFLAGS_COMPILE+=(
				-m64
				-mno-red-zone
			)
			;;
	esac
}

prepare_tree() {
	log "emitting patched overlay"
	"$SCRIPT_DIR/emit-haiku-overlay.sh" "$OVERLAY_ROOT"
	rm -rf "$BUILD_DIR"
	mkdir -p "$BUILD_DIR"
	mkdir -p "$ARTIFACT_DIR"
}

compile_one() {
	local source_path="$1"
	local object_path="$2"
	log "compiling $(basename "$source_path")"
	"$CXX" "${CXXFLAGS_COMPILE[@]}" -c "$source_path" -o "$object_path"
}

link_module() {
	local output_path="$1"
	local soname
	shift
	soname="$(basename "$output_path")"
	log "linking $(basename "$output_path")"
	"$CXX" -nostdlib -shared -o "$output_path" \
		-Wl,--no-undefined \
		-Wl,-soname,"$soname" \
		"$CRTBEGIN" \
		"$@" \
		"$HAIKU_VERSION_GLUE" \
		"$LIBGCC_KERNEL" \
		"$CRTEND" \
		"$KERNEL_IMPORT_LIB"
}

build_kernel_artifacts() {
	local core_src="$OVERLAY_ROOT/src/add-ons/kernel/generic/haikufw_core"
	local driver_src="$OVERLAY_ROOT/src/add-ons/kernel/drivers/dev/misc/haikufw"
	local ipv4_src="$OVERLAY_ROOT/src/add-ons/kernel/network/protocols/ipv4"
	local ipv6_src="$OVERLAY_ROOT/src/add-ons/kernel/network/protocols/ipv6"

	compile_one "$core_src/haikufw_core.cpp" "$BUILD_DIR/haikufw_core.o"
	compile_one "$core_src/haikufw_match.cpp" "$BUILD_DIR/haikufw_match.o"
	compile_one "$driver_src/haikufw_control_driver.cpp" "$BUILD_DIR/haikufw_control_driver.o"
	compile_one "$ipv4_src/ipv4.cpp" "$BUILD_DIR/ipv4.o"
	compile_one "$ipv4_src/ipv4_address.cpp" "$BUILD_DIR/ipv4_address.o"
	compile_one "$ipv4_src/multicast.cpp" "$BUILD_DIR/ipv4_multicast.o"
	compile_one "$ipv6_src/ipv6.cpp" "$BUILD_DIR/ipv6.o"
	compile_one "$ipv6_src/ipv6_address.cpp" "$BUILD_DIR/ipv6_address.o"
	compile_one "$ipv6_src/ipv6_utils.cpp" "$BUILD_DIR/ipv6_utils.o"
	compile_one "$ipv6_src/multicast.cpp" "$BUILD_DIR/ipv6_multicast.o"

	link_module "$ARTIFACT_DIR/haikufw_core" \
		"$BUILD_DIR/haikufw_core.o" \
		"$BUILD_DIR/haikufw_match.o"
	link_module "$ARTIFACT_DIR/haikufw_driver" \
		"$BUILD_DIR/haikufw_control_driver.o"
	link_module "$ARTIFACT_DIR/ipv4" \
		"$BUILD_DIR/ipv4.o" \
		"$BUILD_DIR/ipv4_address.o" \
		"$BUILD_DIR/ipv4_multicast.o"
	link_module "$ARTIFACT_DIR/ipv6" \
		"$BUILD_DIR/ipv6.o" \
		"$BUILD_DIR/ipv6_address.o" \
		"$BUILD_DIR/ipv6_utils.o" \
		"$BUILD_DIR/ipv6_multicast.o"
}

build_userland_artifacts() {
	log "building haikufwctl"
	make -C "$PROJECT_DIR/userland/haikufwctl" clean >/tmp/haikufwctl-clean.log
	make -C "$PROJECT_DIR/userland/haikufwctl" CXX="$CXX" >/tmp/haikufwctl-build.log
	cp "$PROJECT_DIR/userland/haikufwctl/haikufwctl" "$ARTIFACT_DIR/haikufwctl"
	chmod 755 "$ARTIFACT_DIR/haikufwctl"
}

main() {
	need_haiku
	need_cmd "$CXX"
	need_cmd make
	need_cmd cp
	need_cmd mkdir
	need_cmd rm

	CXXFLAGS_COMPILE=("${CXXFLAGS_COMMON[@]}")
	append_include_flags
	append_arch_flags
	resolve_toolchain_paths
	prepare_tree
	build_kernel_artifacts
	build_userland_artifacts
	log "artifacts ready in $ARTIFACT_DIR"
}

main "$@"
