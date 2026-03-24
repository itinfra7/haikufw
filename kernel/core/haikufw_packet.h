#ifndef HAIKUFW_PACKET_H
#define HAIKUFW_PACKET_H

#include <stdint.h>

struct haikufw_packet_meta {
	uint8_t address_family;
	uint8_t protocol;
	uint16_t destination_port;
	uint8_t source[16];
};

#endif
