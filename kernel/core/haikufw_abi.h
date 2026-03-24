#ifndef HAIKUFW_ABI_H
#define HAIKUFW_ABI_H

#include <stdint.h>

/*
 * Shared userland/kernel ABI for haikufw.
 *
 * The human configuration file is parsed in userland. The kernel does not
 * parse free-form text. Instead, userland compiles the rules into the packed
 * structures below and ships one blob through the control device.
 *
 * The blob layout is:
 *
 *   haikufw_ruleset_header
 *   haikufw_rule[ruleset_header.rule_count]
 *   haikufw_port_span[ruleset_header.port_span_count]
 *
 * Each rule references a contiguous subset of the trailing port-span array.
 * A rule with `HAIKUFW_PORT_MATCH_ANY` ignores the port span table.
 */

#define HAIKUFW_RULESET_MAGIC 0x48465731u /* "HFW1" */
#define HAIKUFW_ABI_VERSION   1u

#define HAIKUFW_PUBLISHED_DEVICE_NAME "misc/haikufw"
#define HAIKUFW_DEVICE_PATH "/dev/" HAIKUFW_PUBLISHED_DEVICE_NAME

#define HAIKUFW_IOCTL_BASE            0x48574600u
#define HAIKUFW_IOCTL_LOAD_RULESET    (HAIKUFW_IOCTL_BASE + 1u)
#define HAIKUFW_IOCTL_CLEAR_RULESET   (HAIKUFW_IOCTL_BASE + 2u)
#define HAIKUFW_IOCTL_GET_STATUS      (HAIKUFW_IOCTL_BASE + 3u)
#define HAIKUFW_IOCTL_GET_VERSION     (HAIKUFW_IOCTL_BASE + 4u)

enum haikufw_action {
	HAIKUFW_ACTION_DENY = 0,
	HAIKUFW_ACTION_ALLOW = 1
};

enum haikufw_protocol {
	HAIKUFW_PROTOCOL_ALL = 0,
	HAIKUFW_PROTOCOL_TCP = 6,
	HAIKUFW_PROTOCOL_UDP = 17
};

enum haikufw_address_family {
	HAIKUFW_AF_ANY = 0,
	HAIKUFW_AF_INET4 = 4,
	HAIKUFW_AF_INET6 = 6
};

enum haikufw_port_match_mode {
	HAIKUFW_PORT_MATCH_ANY = 0,
	HAIKUFW_PORT_MATCH_LIST = 1
};

struct haikufw_port_span {
	uint16_t first_port;
	uint16_t last_port;
};

/*
 * Address encoding rules:
 *
 * - `address_family == HAIKUFW_AF_ANY`
 *   - the address bytes and prefix are ignored
 * - `address_family == HAIKUFW_AF_INET4`
 *   - first four bytes of `address` hold the IPv4 address in network order
 *   - `prefix_length` is 0..32
 * - `address_family == HAIKUFW_AF_INET6`
 *   - all 16 bytes hold the IPv6 address in network order
 *   - `prefix_length` is 0..128
 */
struct haikufw_rule {
	uint8_t action;
	uint8_t protocol;
	uint8_t address_family;
	uint8_t prefix_length;
	uint8_t port_match_mode;
	uint8_t reserved0[3];
	uint32_t first_port_span_index;
	uint32_t port_span_count;
	uint8_t address[16];
};

struct haikufw_ruleset_header {
	uint32_t magic;
	uint32_t abi_version;
	uint32_t total_size;
	uint32_t generation;
	uint32_t default_action;
	uint32_t rule_count;
	uint32_t port_span_count;
	uint32_t reserved0;
};

struct haikufw_status {
	uint32_t abi_version;
	uint32_t active_generation;
	uint32_t default_action;
	uint32_t rule_count;
	uint32_t port_span_count;
	uint32_t flags;
	uint64_t packets_allowed;
	uint64_t packets_denied;
};

enum haikufw_status_flags {
	HAIKUFW_STATUS_LOADED = 0x00000001u
};

#endif /* HAIKUFW_ABI_H */
