#pragma once

#include "config_types.hpp"

#include <stdexcept>
#include <string>

namespace haikufw {

class ParseError : public std::runtime_error {
public:
	explicit ParseError(const std::string& message);
};

FirewallConfig parse_config_text(const std::string& text);
FirewallConfig parse_config_file(const std::string& path);
std::string render_canonical_config(const FirewallConfig& config);

} // namespace haikufw
