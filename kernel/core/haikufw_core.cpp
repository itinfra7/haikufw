#include "haikufw_core.h"
#include "haikufw_match.h"

#include <KernelExport.h>
#include <kernel/OS.h>
#include <stdlib.h>
#include <string.h>

struct haikufw_ruleset_state {
	size_t size;
	uint8_t blob[];
};

static haikufw_ruleset_state* volatile sActiveRuleset;
static int32 sActiveReaders;
static uint32 sFallbackAction = HAIKUFW_ACTION_ALLOW;
static uint64 sPacketsAllowed;
static uint64 sPacketsDenied;


static inline void
haikufw_acquire_reader()
{
	__atomic_add_fetch(&sActiveReaders, 1, __ATOMIC_ACQUIRE);
}


static inline void
haikufw_release_reader()
{
	__atomic_sub_fetch(&sActiveReaders, 1, __ATOMIC_RELEASE);
}


static void
haikufw_wait_for_readers()
{
	while (__atomic_load_n(&sActiveReaders, __ATOMIC_ACQUIRE) != 0)
		snooze(1000);
}


static status_t
haikufw_load_ruleset(const void* data, size_t size)
{
	if (!haikufw_validate_ruleset_blob(data, size))
		return B_BAD_VALUE;

	haikufw_ruleset_state* copy = static_cast<haikufw_ruleset_state*>(
		malloc(sizeof(haikufw_ruleset_state) + size));
	if (copy == NULL)
		return B_NO_MEMORY;

	copy->size = size;
	memcpy(copy->blob, data, size);

	haikufw_ruleset_state* oldRuleset = __atomic_exchange_n(&sActiveRuleset,
		copy, __ATOMIC_ACQ_REL);
	if (oldRuleset != NULL) {
		haikufw_wait_for_readers();
		free(oldRuleset);
	}
	return B_OK;
}


static status_t
haikufw_clear_ruleset()
{
	haikufw_ruleset_state* oldRuleset = __atomic_exchange_n(&sActiveRuleset,
		NULL, __ATOMIC_ACQ_REL);
	if (oldRuleset != NULL) {
		haikufw_wait_for_readers();
		free(oldRuleset);
	}
	return B_OK;
}


static status_t
haikufw_get_status(haikufw_status* status)
{
	if (status == NULL)
		return B_BAD_VALUE;

	memset(status, 0, sizeof(*status));
	status->abi_version = HAIKUFW_ABI_VERSION;
	status->default_action = sFallbackAction;
	status->packets_allowed = __atomic_load_n(&sPacketsAllowed, __ATOMIC_RELAXED);
	status->packets_denied = __atomic_load_n(&sPacketsDenied, __ATOMIC_RELAXED);

	haikufw_ruleset_state* ruleset = __atomic_load_n(&sActiveRuleset,
		__ATOMIC_ACQUIRE);
	if (ruleset == NULL)
		return B_OK;

	const haikufw_ruleset_header* header
		= reinterpret_cast<const haikufw_ruleset_header*>(ruleset->blob);
	status->active_generation = header->generation;
	status->default_action = header->default_action;
	status->rule_count = header->rule_count;
	status->port_span_count = header->port_span_count;
	status->flags = HAIKUFW_STATUS_LOADED;
	return B_OK;
}


static bool
haikufw_decide_inbound(const haikufw_packet_meta* meta)
{
	if (meta == NULL)
		return false;

	haikufw_acquire_reader();

	haikufw_ruleset_state* ruleset = __atomic_load_n(&sActiveRuleset,
		__ATOMIC_ACQUIRE);
	if (ruleset == NULL) {
		bool allow = sFallbackAction == HAIKUFW_ACTION_ALLOW;
		if (allow)
			__atomic_add_fetch(&sPacketsAllowed, 1, __ATOMIC_RELAXED);
		else
			__atomic_add_fetch(&sPacketsDenied, 1, __ATOMIC_RELAXED);
		haikufw_release_reader();
		return allow;
	}

	const uint8_t* base = ruleset->blob;
	const haikufw_ruleset_header* header
		= reinterpret_cast<const haikufw_ruleset_header*>(base);
	const haikufw_rule* rules
		= reinterpret_cast<const haikufw_rule*>(base + sizeof(*header));
	const haikufw_port_span* spans
		= reinterpret_cast<const haikufw_port_span*>(
			base + sizeof(*header) + header->rule_count * sizeof(haikufw_rule));

	bool allow = haikufw_decide_packet(header, rules, spans, meta);
	if (allow)
		__atomic_add_fetch(&sPacketsAllowed, 1, __ATOMIC_RELAXED);
	else
		__atomic_add_fetch(&sPacketsDenied, 1, __ATOMIC_RELAXED);
	haikufw_release_reader();
	return allow;
}


static status_t
haikufw_core_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			sActiveRuleset = NULL;
			sActiveReaders = 0;
			sFallbackAction = HAIKUFW_ACTION_ALLOW;
			sPacketsAllowed = 0;
			sPacketsDenied = 0;
			return B_OK;

		case B_MODULE_UNINIT:
			{
				haikufw_ruleset_state* oldRuleset = __atomic_exchange_n(
					&sActiveRuleset, NULL, __ATOMIC_ACQ_REL);
				if (oldRuleset != NULL) {
					haikufw_wait_for_readers();
					free(oldRuleset);
				}
			}
			return B_OK;

		default:
			return B_BAD_VALUE;
	}
}


static haikufw_core_module_info sHaikuFWCoreModule = {
	{
		HAIKUFW_CORE_MODULE_NAME,
		0,
		haikufw_core_std_ops
	},
	haikufw_load_ruleset,
	haikufw_clear_ruleset,
	haikufw_get_status,
	haikufw_decide_inbound
};


module_info* modules[] = {
	reinterpret_cast<module_info*>(&sHaikuFWCoreModule),
	NULL
};
