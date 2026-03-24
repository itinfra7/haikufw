#include "compiler.hpp"

#include "../../kernel/core/haikufw_abi.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace haikufw {

namespace {

uint32_t
compile_default_action(Action action)
{
	return action == Action::Allow ? HAIKUFW_ACTION_ALLOW : HAIKUFW_ACTION_DENY;
}


uint8_t
compile_protocol(Protocol protocol)
{
	switch (protocol) {
		case Protocol::Tcp:
			return HAIKUFW_PROTOCOL_TCP;
		case Protocol::Udp:
			return HAIKUFW_PROTOCOL_UDP;
		case Protocol::All:
		default:
			return HAIKUFW_PROTOCOL_ALL;
	}
}


void
append_bytes(std::vector<uint8_t>& blob, const void* data, size_t size)
{
	const uint8_t* bytes = static_cast<const uint8_t*>(data);
	blob.insert(blob.end(), bytes, bytes + size);
}

} // namespace


std::vector<uint8_t>
compile_ruleset_blob(const FirewallConfig& config, uint32_t generation)
{
	std::vector<haikufw_rule> rules;
	std::vector<haikufw_port_span> spans;

	rules.reserve(config.rules.size());

	for (const Rule& configRule : config.rules) {
		haikufw_rule rule {};
		rule.action = configRule.action == Action::Allow
			? HAIKUFW_ACTION_ALLOW
			: HAIKUFW_ACTION_DENY;
		rule.protocol = compile_protocol(configRule.protocol);

		if (configRule.source.any) {
			rule.address_family = HAIKUFW_AF_ANY;
			rule.prefix_length = 0;
		} else if (configRule.source.family == AF_INET) {
			rule.address_family = HAIKUFW_AF_INET4;
			rule.prefix_length = configRule.source.prefix_length;
			std::memcpy(rule.address, configRule.source.bytes.data(), 4);
		} else if (configRule.source.family == AF_INET6) {
			rule.address_family = HAIKUFW_AF_INET6;
			rule.prefix_length = configRule.source.prefix_length;
			std::memcpy(rule.address, configRule.source.bytes.data(), 16);
		} else {
			throw std::runtime_error("unsupported source family in compiled ruleset");
		}

		if (configRule.any_port) {
			rule.port_match_mode = HAIKUFW_PORT_MATCH_ANY;
			rule.first_port_span_index = 0;
			rule.port_span_count = 0;
		} else {
			rule.port_match_mode = HAIKUFW_PORT_MATCH_LIST;
			rule.first_port_span_index = static_cast<uint32_t>(spans.size());
			rule.port_span_count = static_cast<uint32_t>(configRule.ports.size());
			for (const PortSpan& span : configRule.ports) {
				haikufw_port_span compiledSpan {};
				compiledSpan.first_port = span.first;
				compiledSpan.last_port = span.last;
				spans.push_back(compiledSpan);
			}
		}

		rules.push_back(rule);
	}

	haikufw_ruleset_header header {};
	header.magic = HAIKUFW_RULESET_MAGIC;
	header.abi_version = HAIKUFW_ABI_VERSION;
	header.generation = generation;
	header.default_action = compile_default_action(config.default_inbound);
	header.rule_count = static_cast<uint32_t>(rules.size());
	header.port_span_count = static_cast<uint32_t>(spans.size());
	header.total_size = static_cast<uint32_t>(sizeof(header)
		+ rules.size() * sizeof(haikufw_rule)
		+ spans.size() * sizeof(haikufw_port_span));

	std::vector<uint8_t> blob;
	blob.reserve(header.total_size);
	append_bytes(blob, &header, sizeof(header));
	if (!rules.empty())
		append_bytes(blob, rules.data(), rules.size() * sizeof(haikufw_rule));
	if (!spans.empty())
		append_bytes(blob, spans.data(),
			spans.size() * sizeof(haikufw_port_span));
	return blob;
}


void
write_compiled_ruleset_atomically(const std::vector<uint8_t>& blob,
	const std::string& path)
{
	std::string tmpPath = path + ".tmp";
	std::ofstream output(tmpPath, std::ios::binary | std::ios::trunc);
	if (!output)
		throw std::runtime_error("unable to write file: " + tmpPath);

	output.write(reinterpret_cast<const char*>(blob.data()), blob.size());
	output.close();
	if (!output)
		throw std::runtime_error("failed to flush file: " + tmpPath);

	if (std::rename(tmpPath.c_str(), path.c_str()) != 0)
		throw std::runtime_error("unable to replace file: " + path);
}

} // namespace haikufw
