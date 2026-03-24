#ifndef HAIKUFW_MATCH_H
#define HAIKUFW_MATCH_H

#include "haikufw_abi.h"
#include "haikufw_packet.h"

#include <stddef.h>
#include <stdint.h>

bool haikufw_validate_ruleset_blob(const void* data, size_t size);
bool haikufw_decide_packet(const haikufw_ruleset_header* header,
	const haikufw_rule* rules, const haikufw_port_span* spans,
	const haikufw_packet_meta* meta);

#endif
