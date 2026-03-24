#ifndef HAIKUFW_CORE_H
#define HAIKUFW_CORE_H

#include "haikufw_abi.h"
#include "haikufw_packet.h"

#include <module.h>

#define HAIKUFW_CORE_MODULE_NAME "generic/haikufw_core/v1"

typedef struct haikufw_core_module_info {
	module_info info;
	status_t (*load_ruleset)(const void* data, size_t size);
	status_t (*clear_ruleset)(void);
	status_t (*get_status)(haikufw_status* status);
	bool (*decide_inbound)(const haikufw_packet_meta* meta);
} haikufw_core_module_info;

#endif
