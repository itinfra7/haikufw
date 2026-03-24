#ifndef HAIKUFW_KERNEL_HOOK_H
#define HAIKUFW_KERNEL_HOOK_H

#include "haikufw_core.h"

#include <module.h>

#include <net_buffer.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <string.h>


struct haikufw_udp_wire_header {
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t length;
	uint16_t checksum;
};


struct haikufw_tcp_wire_header {
	uint16_t source_port;
	uint16_t destination_port;
	uint32_t sequence;
	uint32_t acknowledgement;
	uint8_t data_offset_reserved;
	uint8_t flags;
	uint16_t window;
	uint16_t checksum;
	uint16_t urgent_pointer;
};

static inline haikufw_core_module_info*
haikufw_try_acquire_core(haikufw_core_module_info** slot)
{
	if (*slot != NULL)
		return *slot;

	if (get_module(HAIKUFW_CORE_MODULE_NAME,
			reinterpret_cast<module_info**>(slot)) != B_OK) {
		return NULL;
	}

	return *slot;
}


static inline void
haikufw_put_core(haikufw_core_module_info** slot)
{
	if (*slot == NULL)
		return;

	put_module(HAIKUFW_CORE_MODULE_NAME);
	*slot = NULL;
}


static inline bool
haikufw_build_packet_meta(const sockaddr* source, uint8_t protocol,
	uint16_t destinationPort, haikufw_packet_meta* meta)
{
	if (source == NULL || meta == NULL)
		return false;

	memset(meta, 0, sizeof(*meta));
	meta->protocol = protocol;
	meta->destination_port = destinationPort;

	if (source->sa_family == AF_INET) {
		const sockaddr_in* in = reinterpret_cast<const sockaddr_in*>(source);
		meta->address_family = HAIKUFW_AF_INET4;
		memcpy(meta->source, &in->sin_addr, 4);
		return true;
	}

	if (source->sa_family == AF_INET6) {
		const sockaddr_in6* in6 = reinterpret_cast<const sockaddr_in6*>(source);
		meta->address_family = HAIKUFW_AF_INET6;
		memcpy(meta->source, in6->sin6_addr.s6_addr, 16);
		return true;
	}

	return false;
}


static inline bool
haikufw_extract_destination_port(net_buffer_module_info* bufferModule,
	net_buffer* buffer, uint8_t protocol, uint16_t headerOffset,
	uint16_t* destinationPort)
{
	if (bufferModule == NULL || buffer == NULL || destinationPort == NULL)
		return false;

	if (protocol == IPPROTO_TCP) {
		haikufw_tcp_wire_header header;
		if (bufferModule->read(buffer, headerOffset, &header, sizeof(header))
				!= B_OK) {
			return false;
		}

		*destinationPort = ntohs(header.destination_port);
		return true;
	}

	if (protocol == IPPROTO_UDP) {
		haikufw_udp_wire_header header;
		if (bufferModule->read(buffer, headerOffset, &header, sizeof(header))
				!= B_OK) {
			return false;
		}

		*destinationPort = ntohs(header.destination_port);
		return true;
	}

	return false;
}


static inline bool
haikufw_should_allow_inbound_buffer(net_buffer_module_info* bufferModule,
	net_buffer* buffer, const sockaddr* source, uint8_t protocol,
	uint16_t headerOffset, haikufw_core_module_info** slot)
{
	if (protocol != IPPROTO_TCP && protocol != IPPROTO_UDP)
		return true;

	haikufw_core_module_info* core = haikufw_try_acquire_core(slot);
	if (core == NULL)
		return true;

	uint16_t destinationPort = 0;
	if (!haikufw_extract_destination_port(bufferModule, buffer, protocol,
			headerOffset, &destinationPort)) {
		return true;
	}

	haikufw_packet_meta meta;
	if (!haikufw_build_packet_meta(source,
			protocol == IPPROTO_TCP ? HAIKUFW_PROTOCOL_TCP : HAIKUFW_PROTOCOL_UDP,
			destinationPort, &meta)) {
		return true;
	}

	return core->decide_inbound(&meta);
}

#endif
