#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <sys/socket.h>

namespace haikufw {

enum class Action {
	Allow,
	Deny,
};

enum class Protocol {
	Tcp,
	Udp,
	All,
};

struct PortSpan {
	uint16_t first = 0;
	uint16_t last = 0;

	bool
	matches(uint16_t port) const
	{
		return first <= port && port <= last;
	}
};

struct AddressMatcher {
	bool any = true;
	int family = AF_UNSPEC;
	std::array<uint8_t, 16> bytes {};
	uint8_t prefix_length = 0;
};

struct Rule {
	Action action = Action::Deny;
	Protocol protocol = Protocol::All;
	AddressMatcher source;
	bool any_port = true;
	std::vector<PortSpan> ports;
};

struct FirewallConfig {
	Action default_inbound = Action::Deny;
	std::vector<Rule> rules;
};

const char* to_string(Action action);
const char* to_string(Protocol protocol);

} // namespace haikufw
