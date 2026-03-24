#pragma once

#include "config_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace haikufw {

std::vector<uint8_t> compile_ruleset_blob(const FirewallConfig& config,
	uint32_t generation);
void write_compiled_ruleset_atomically(const std::vector<uint8_t>& blob,
	const std::string& path);

} // namespace haikufw
