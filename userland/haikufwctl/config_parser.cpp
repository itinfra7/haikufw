#include "config_parser.hpp"

#include <arpa/inet.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace haikufw {

namespace {

std::string
trim(std::string value)
{
	auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
	value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
	value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(),
		value.end());
	return value;
}


std::string
strip_comment(const std::string& input)
{
	size_t pos = input.find('#');
	return pos == std::string::npos ? input : input.substr(0, pos);
}


std::string
to_lower(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(),
		[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	return value;
}


std::vector<std::string>
split_csv(const std::string& input)
{
	std::vector<std::string> parts;
	std::stringstream stream(input);
	std::string part;
	while (std::getline(stream, part, ',')) {
		parts.push_back(trim(part));
	}
	return parts;
}


Action
parse_action(const std::string& input)
{
	std::string lowered = to_lower(input);
	if (lowered == "allow")
		return Action::Allow;
	if (lowered == "deny")
		return Action::Deny;
	throw ParseError("invalid action: " + input);
}


Protocol
parse_protocol(const std::string& input)
{
	std::string lowered = to_lower(input);
	if (lowered == "tcp")
		return Protocol::Tcp;
	if (lowered == "udp")
		return Protocol::Udp;
	if (lowered == "all")
		return Protocol::All;
	throw ParseError("invalid protocol: " + input);
}


uint16_t
parse_port(const std::string& input)
{
	if (input.empty())
		throw ParseError("empty port");

	size_t end = 0;
	unsigned long value = std::stoul(input, &end, 10);
	if (end != input.size() || value == 0 || value > 65535ul)
		throw ParseError("invalid port: " + input);

	return static_cast<uint16_t>(value);
}


unsigned int
parse_bounded_number(const std::string& input, unsigned int maxValue,
	const char* label, bool allowZero)
{
	if (input.empty())
		throw ParseError(std::string("empty ") + label);

	size_t end = 0;
	unsigned long value = std::stoul(input, &end, 10);
	if (end != input.size() || value > maxValue || (!allowZero && value == 0))
		throw ParseError(std::string("invalid ") + label + ": " + input);

	return static_cast<unsigned int>(value);
}


std::vector<PortSpan>
parse_ports(const std::string& input, bool* anyPort)
{
	std::string lowered = to_lower(trim(input));
	std::vector<PortSpan> result;

	if (lowered == "any") {
		*anyPort = true;
		return result;
	}

	*anyPort = false;

	for (const std::string& item : split_csv(lowered)) {
		if (item.empty())
			throw ParseError("empty port entry");

		size_t dash = item.find('-');
		if (dash == std::string::npos) {
			uint16_t port = parse_port(item);
			result.push_back({port, port});
			continue;
		}

		uint16_t first = parse_port(trim(item.substr(0, dash)));
		uint16_t last = parse_port(trim(item.substr(dash + 1)));
		if (first > last)
			throw ParseError("invalid port range: " + item);

		result.push_back({first, last});
	}

	return result;
}


AddressMatcher
parse_source(const std::string& input)
{
	std::string lowered = to_lower(trim(input));
	AddressMatcher matcher;

	if (lowered == "any") {
		matcher.any = true;
		return matcher;
	}

	std::string addressText = lowered;
	std::string prefixText;
	size_t slash = lowered.find('/');
	if (slash != std::string::npos) {
		addressText = lowered.substr(0, slash);
		prefixText = lowered.substr(slash + 1);
	}

	in_addr addr4 {};
	if (inet_pton(AF_INET, addressText.c_str(), &addr4) == 1) {
		matcher.any = false;
		matcher.family = AF_INET;
		matcher.prefix_length = prefixText.empty()
			? 32
			: static_cast<uint8_t>(parse_bounded_number(prefixText, 32,
				"IPv4 prefix", true));
		const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&addr4);
		std::copy(bytes, bytes + 4, matcher.bytes.begin());
		return matcher;
	}

	in6_addr addr6 {};
	if (inet_pton(AF_INET6, addressText.c_str(), &addr6) == 1) {
		matcher.any = false;
		matcher.family = AF_INET6;
		matcher.prefix_length = prefixText.empty()
			? 128
			: static_cast<uint8_t>(parse_bounded_number(prefixText, 128,
				"IPv6 prefix", true));
		std::copy(addr6.s6_addr, addr6.s6_addr + 16, matcher.bytes.begin());
		return matcher;
	}

	throw ParseError("invalid source matcher: " + input);
}


Rule
parse_rule_line(const std::string& line)
{
	static const std::regex kRule(
		R"(^(allow|deny)\s+(tcp|udp|all)\s+from\s+(\S+)(?:\s+to(?:\s+any)?)?\s+port\s+(.+)$)",
		std::regex::icase);
	std::smatch match;

	if (!std::regex_match(line, match, kRule))
		throw ParseError("invalid rule syntax: " + line);

	Rule rule;
	rule.action = parse_action(match[1].str());
	rule.protocol = parse_protocol(match[2].str());
	rule.source = parse_source(match[3].str());
	rule.ports = parse_ports(match[4].str(), &rule.any_port);
	return rule;
}


bool
parse_default_line(const std::string& line, Action* action)
{
	static const std::regex kDefault(
		R"(^default_inbound\s*=\s*(allow|deny)$)", std::regex::icase);
	std::smatch match;
	if (!std::regex_match(line, match, kDefault))
		return false;

	*action = parse_action(match[1].str());
	return true;
}

} // namespace


ParseError::ParseError(const std::string& message)
	:
	std::runtime_error(message)
{
}


const char*
to_string(Action action)
{
	return action == Action::Allow ? "allow" : "deny";
}


const char*
to_string(Protocol protocol)
{
	switch (protocol) {
		case Protocol::Tcp:
			return "tcp";
		case Protocol::Udp:
			return "udp";
		case Protocol::All:
		default:
			return "all";
	}
}


FirewallConfig
parse_config_text(const std::string& text)
{
	FirewallConfig config;
	bool sawDefault = false;
	std::stringstream stream(text);
	std::string rawLine;
	size_t lineNumber = 0;

	while (std::getline(stream, rawLine)) {
		++lineNumber;
		std::string line = trim(strip_comment(rawLine));
		if (line.empty())
			continue;

		try {
			Action action;
			if (parse_default_line(line, &action)) {
				config.default_inbound = action;
				sawDefault = true;
				continue;
			}

			config.rules.push_back(parse_rule_line(line));
		} catch (const std::exception& error) {
			throw ParseError("line " + std::to_string(lineNumber) + ": "
				+ error.what());
		}
	}

	if (!sawDefault)
		throw ParseError("missing default_inbound directive");

	return config;
}


FirewallConfig
parse_config_file(const std::string& path)
{
	std::ifstream input(path);
	if (!input)
		throw ParseError("unable to open file: " + path);

	std::ostringstream buffer;
	buffer << input.rdbuf();
	return parse_config_text(buffer.str());
}


std::string
render_canonical_config(const FirewallConfig& config)
{
	std::ostringstream out;
	out << "default_inbound = " << to_string(config.default_inbound);
	if (!config.rules.empty())
		out << "\n\n";
	else
		out << '\n';

	for (size_t i = 0; i < config.rules.size(); i++) {
		const Rule& rule = config.rules[i];
		out << to_string(rule.action) << ' ' << to_string(rule.protocol)
			<< " from ";
		if (rule.source.any) {
			out << "any";
		} else if (rule.source.family == AF_INET) {
			char address[INET_ADDRSTRLEN] = {};
			inet_ntop(AF_INET, rule.source.bytes.data(), address, sizeof(address));
			out << address << '/' << static_cast<unsigned int>(rule.source.prefix_length);
		} else {
			char address[INET6_ADDRSTRLEN] = {};
			inet_ntop(AF_INET6, rule.source.bytes.data(), address, sizeof(address));
			out << address << '/' << static_cast<unsigned int>(rule.source.prefix_length);
		}
		out << " to port ";

		if (rule.any_port) {
			out << "any";
		} else {
			for (size_t j = 0; j < rule.ports.size(); j++) {
				if (j > 0)
					out << ',';
				if (rule.ports[j].first == rule.ports[j].last)
					out << rule.ports[j].first;
				else
					out << rule.ports[j].first << '-' << rule.ports[j].last;
			}
		}

		if (i + 1 < config.rules.size())
			out << '\n';
	}

	return out.str();
}

} // namespace haikufw
