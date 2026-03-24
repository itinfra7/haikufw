#include "../kernel/core/haikufw_match.h"
#include "../userland/haikufwctl/compiler.hpp"
#include "../userland/haikufwctl/config_parser.hpp"

#include <arpa/inet.h>

#include <cstring>
#include <iostream>

int
main()
{
	const char* text =
		"default_inbound = deny\n"
		"allow tcp from 10.43.0.0/16 to port 22,80\n"
		"allow udp from 2001:db8:43:1::/64 to port 53\n"
		"allow all from 198.51.100.0/24 to port 443,60000-61000\n";

	haikufw::FirewallConfig config = haikufw::parse_config_text(text);
	std::vector<uint8_t> blob = haikufw::compile_ruleset_blob(config, 9);

	if (!haikufw_validate_ruleset_blob(blob.data(), blob.size())) {
		std::cerr << "ruleset validation failed\n";
		return 1;
	}

	const auto* header
		= reinterpret_cast<const haikufw_ruleset_header*>(blob.data());
	const auto* rules = reinterpret_cast<const haikufw_rule*>(
		blob.data() + sizeof(*header));
	const auto* spans = reinterpret_cast<const haikufw_port_span*>(
		blob.data() + sizeof(*header) + header->rule_count * sizeof(haikufw_rule));

	haikufw_packet_meta meta4 {};
	meta4.address_family = HAIKUFW_AF_INET4;
	meta4.protocol = HAIKUFW_PROTOCOL_TCP;
	meta4.destination_port = 22;
	in_addr addr4 {};
	inet_pton(AF_INET, "10.43.0.142", &addr4);
	std::memcpy(meta4.source, &addr4, 4);

	if (!haikufw_decide_packet(header, rules, spans, &meta4)) {
		std::cerr << "expected IPv4 tcp/22 allow\n";
		return 2;
	}

	meta4.destination_port = 443;
	if (haikufw_decide_packet(header, rules, spans, &meta4)) {
		std::cerr << "expected IPv4 tcp/443 deny\n";
		return 3;
	}

	haikufw_packet_meta meta6 {};
	meta6.address_family = HAIKUFW_AF_INET6;
	meta6.protocol = HAIKUFW_PROTOCOL_UDP;
	meta6.destination_port = 53;
	in6_addr addr6 {};
	inet_pton(AF_INET6, "2001:db8:43:1::84d8", &addr6);
	std::memcpy(meta6.source, addr6.s6_addr, 16);

	if (!haikufw_decide_packet(header, rules, spans, &meta6)) {
		std::cerr << "expected IPv6 udp/53 allow\n";
		return 4;
	}

	haikufw_packet_meta metaAll {};
	metaAll.address_family = HAIKUFW_AF_INET4;
	metaAll.destination_port = 60005;
	in_addr addrAll {};
	inet_pton(AF_INET, "198.51.100.7", &addrAll);
	std::memcpy(metaAll.source, &addrAll, 4);

	metaAll.protocol = HAIKUFW_PROTOCOL_TCP;
	if (!haikufw_decide_packet(header, rules, spans, &metaAll)) {
		std::cerr << "expected protocol-all tcp/60005 allow\n";
		return 5;
	}

	metaAll.protocol = HAIKUFW_PROTOCOL_UDP;
	metaAll.destination_port = 443;
	if (!haikufw_decide_packet(header, rules, spans, &metaAll)) {
		std::cerr << "expected protocol-all udp/443 allow\n";
		return 6;
	}

	metaAll.protocol = HAIKUFW_PROTOCOL_TCP;
	metaAll.destination_port = 59999;
	if (haikufw_decide_packet(header, rules, spans, &metaAll)) {
		std::cerr << "expected tcp/59999 deny outside allowed spans\n";
		return 7;
	}

	in6_addr addr6NoMatch {};
	inet_pton(AF_INET6, "2001:db8:43:2::1", &addr6NoMatch);
	std::memcpy(meta6.source, addr6NoMatch.s6_addr, 16);
	if (haikufw_decide_packet(header, rules, spans, &meta6)) {
		std::cerr << "expected IPv6 source outside prefix deny\n";
		return 8;
	}

	std::cout << "host smoke test ok\n";
	return 0;
}
