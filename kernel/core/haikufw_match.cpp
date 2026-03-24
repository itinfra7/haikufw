#include "haikufw_match.h"

#include <string.h>

namespace {

static bool
match_prefix(const uint8_t* lhs, const uint8_t* rhs, uint8_t prefixLength)
{
	uint8_t wholeBytes = prefixLength / 8;
	uint8_t remainder = prefixLength % 8;

	if (wholeBytes > 0 && memcmp(lhs, rhs, wholeBytes) != 0)
		return false;

	if (remainder > 0) {
		uint8_t mask = static_cast<uint8_t>(0xff << (8 - remainder));
		if ((lhs[wholeBytes] & mask) != (rhs[wholeBytes] & mask))
			return false;
	}

	return true;
}


static bool
match_source(const haikufw_rule& rule, const haikufw_packet_meta& meta)
{
	if (rule.address_family == HAIKUFW_AF_ANY)
		return true;

	if (rule.address_family != meta.address_family)
		return false;

	if (rule.address_family == HAIKUFW_AF_INET4)
		return match_prefix(rule.address, meta.source, rule.prefix_length);

	if (rule.address_family == HAIKUFW_AF_INET6)
		return match_prefix(rule.address, meta.source, rule.prefix_length);

	return false;
}


static bool
match_protocol(const haikufw_rule& rule, const haikufw_packet_meta& meta)
{
	return rule.protocol == HAIKUFW_PROTOCOL_ALL || rule.protocol == meta.protocol;
}


static bool
match_port(const haikufw_rule& rule, const haikufw_port_span* spans,
	const haikufw_packet_meta& meta)
{
	if (rule.port_match_mode == HAIKUFW_PORT_MATCH_ANY)
		return true;

	for (uint32_t i = 0; i < rule.port_span_count; i++) {
		const haikufw_port_span& span = spans[rule.first_port_span_index + i];
		if (span.first_port <= meta.destination_port
			&& meta.destination_port <= span.last_port) {
			return true;
		}
	}

	return false;
}

} // namespace


bool
haikufw_validate_ruleset_blob(const void* data, size_t size)
{
	if (size < sizeof(haikufw_ruleset_header))
		return false;

	const haikufw_ruleset_header* header
		= static_cast<const haikufw_ruleset_header*>(data);

	if (header->magic != HAIKUFW_RULESET_MAGIC)
		return false;
	if (header->abi_version != HAIKUFW_ABI_VERSION)
		return false;
	if (header->total_size != size)
		return false;

	size_t rulesOffset = sizeof(haikufw_ruleset_header);
	size_t rulesSize = header->rule_count * sizeof(haikufw_rule);
	size_t spansOffset = rulesOffset + rulesSize;
	size_t spansSize = header->port_span_count * sizeof(haikufw_port_span);

	if (rulesOffset > size || rulesSize > size - rulesOffset)
		return false;
	if (spansOffset > size || spansSize > size - spansOffset)
		return false;
	if (spansOffset + spansSize != size)
		return false;

	const haikufw_rule* rules
		= reinterpret_cast<const haikufw_rule*>(
			static_cast<const uint8_t*>(data) + rulesOffset);

	for (uint32_t i = 0; i < header->rule_count; i++) {
		const haikufw_rule& rule = rules[i];

		if (rule.action != HAIKUFW_ACTION_ALLOW
			&& rule.action != HAIKUFW_ACTION_DENY) {
			return false;
		}

		if (rule.protocol != HAIKUFW_PROTOCOL_ALL
			&& rule.protocol != HAIKUFW_PROTOCOL_TCP
			&& rule.protocol != HAIKUFW_PROTOCOL_UDP) {
			return false;
		}

		if (rule.address_family != HAIKUFW_AF_ANY
			&& rule.address_family != HAIKUFW_AF_INET4
			&& rule.address_family != HAIKUFW_AF_INET6) {
			return false;
		}

		if (rule.address_family == HAIKUFW_AF_INET4 && rule.prefix_length > 32)
			return false;
		if (rule.address_family == HAIKUFW_AF_INET6 && rule.prefix_length > 128)
			return false;
		if (rule.address_family == HAIKUFW_AF_ANY && rule.prefix_length != 0)
			return false;

		if (rule.port_match_mode == HAIKUFW_PORT_MATCH_ANY) {
			if (rule.port_span_count != 0)
				return false;
			continue;
		}

		if (rule.port_match_mode != HAIKUFW_PORT_MATCH_LIST)
			return false;

		if (rule.first_port_span_index > header->port_span_count)
			return false;
		if (rule.port_span_count > header->port_span_count
			|| rule.first_port_span_index + rule.port_span_count
				> header->port_span_count) {
			return false;
		}
	}

	const haikufw_port_span* spans
		= reinterpret_cast<const haikufw_port_span*>(
			static_cast<const uint8_t*>(data) + spansOffset);
	for (uint32_t i = 0; i < header->port_span_count; i++) {
		if (spans[i].first_port == 0 || spans[i].last_port == 0)
			return false;
		if (spans[i].first_port > spans[i].last_port)
			return false;
	}

	return true;
}


bool
haikufw_decide_packet(const haikufw_ruleset_header* header,
	const haikufw_rule* rules, const haikufw_port_span* spans,
	const haikufw_packet_meta* meta)
{
	for (uint32_t i = 0; i < header->rule_count; i++) {
		const haikufw_rule& rule = rules[i];
		if (!match_protocol(rule, *meta))
			continue;
		if (!match_source(rule, *meta))
			continue;
		if (!match_port(rule, spans, *meta))
			continue;
		return rule.action == HAIKUFW_ACTION_ALLOW;
	}

	return header->default_action == HAIKUFW_ACTION_ALLOW;
}
