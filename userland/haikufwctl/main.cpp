#include "compiler.hpp"
#include "config_parser.hpp"
#include "control.hpp"

#include <fstream>
#include <iostream>
#include <string>

namespace {

constexpr const char* kDefaultConfigPath = "/boot/system/settings/haikufw.conf";
constexpr const char* kDefaultCompiledRulesPath = "/boot/system/settings/haikufw.rules";


std::string
example_config()
{
	return
		"default_inbound = allow\n"
		"\n"
		"# haikufw syntax matrix\n"
		"#\n"
		"# This default file is intentionally permissive and is meant to document rule syntax.\n"
		"# Every example below is commented out on purpose.\n"
		"#\n"
		"# tcp + any source\n"
		"# allow tcp from any to port any\n"
		"# allow tcp from any to port 22\n"
		"# allow tcp from any to port 80,443\n"
		"# allow tcp from any to port 60000-61000\n"
		"# allow tcp from any to port 53,80,443,60000-61000\n"
		"#\n"
		"# tcp + IPv4 host\n"
		"# allow tcp from 203.0.113.10 to port any\n"
		"# allow tcp from 203.0.113.10 to port 22\n"
		"# allow tcp from 203.0.113.10 to port 80,443\n"
		"# allow tcp from 203.0.113.10 to port 60000-61000\n"
		"# allow tcp from 203.0.113.10 to port 53,80,443,60000-61000\n"
		"#\n"
		"# tcp + IPv4 network\n"
		"# allow tcp from 203.0.113.0/24 to port any\n"
		"# allow tcp from 203.0.113.0/24 to port 22\n"
		"# allow tcp from 203.0.113.0/24 to port 80,443\n"
		"# allow tcp from 203.0.113.0/24 to port 60000-61000\n"
		"# allow tcp from 203.0.113.0/24 to port 53,80,443,60000-61000\n"
		"#\n"
		"# tcp + IPv6 host\n"
		"# allow tcp from 2001:db8:100::10 to port any\n"
		"# allow tcp from 2001:db8:100::10 to port 22\n"
		"# allow tcp from 2001:db8:100::10 to port 80,443\n"
		"# allow tcp from 2001:db8:100::10 to port 60000-61000\n"
		"# allow tcp from 2001:db8:100::10 to port 53,80,443,60000-61000\n"
		"#\n"
		"# tcp + IPv6 network\n"
		"# allow tcp from 2001:db8:100::/64 to port any\n"
		"# allow tcp from 2001:db8:100::/64 to port 22\n"
		"# allow tcp from 2001:db8:100::/64 to port 80,443\n"
		"# allow tcp from 2001:db8:100::/64 to port 60000-61000\n"
		"# allow tcp from 2001:db8:100::/64 to port 53,80,443,60000-61000\n"
		"#\n"
		"# udp + any source\n"
		"# allow udp from any to port any\n"
		"# allow udp from any to port 53\n"
		"# allow udp from any to port 53,123\n"
		"# allow udp from any to port 60000-61000\n"
		"# allow udp from any to port 53,123,161,60000-61000\n"
		"#\n"
		"# udp + IPv4 host\n"
		"# allow udp from 203.0.113.10 to port any\n"
		"# allow udp from 203.0.113.10 to port 53\n"
		"# allow udp from 203.0.113.10 to port 53,123\n"
		"# allow udp from 203.0.113.10 to port 60000-61000\n"
		"# allow udp from 203.0.113.10 to port 53,123,161,60000-61000\n"
		"#\n"
		"# udp + IPv4 network\n"
		"# allow udp from 203.0.113.0/24 to port any\n"
		"# allow udp from 203.0.113.0/24 to port 53\n"
		"# allow udp from 203.0.113.0/24 to port 53,123\n"
		"# allow udp from 203.0.113.0/24 to port 60000-61000\n"
		"# allow udp from 203.0.113.0/24 to port 53,123,161,60000-61000\n"
		"#\n"
		"# udp + IPv6 host\n"
		"# allow udp from 2001:db8:100::10 to port any\n"
		"# allow udp from 2001:db8:100::10 to port 53\n"
		"# allow udp from 2001:db8:100::10 to port 53,123\n"
		"# allow udp from 2001:db8:100::10 to port 60000-61000\n"
		"# allow udp from 2001:db8:100::10 to port 53,123,161,60000-61000\n"
		"#\n"
		"# udp + IPv6 network\n"
		"# allow udp from 2001:db8:100::/64 to port any\n"
		"# allow udp from 2001:db8:100::/64 to port 53\n"
		"# allow udp from 2001:db8:100::/64 to port 53,123\n"
		"# allow udp from 2001:db8:100::/64 to port 60000-61000\n"
		"# allow udp from 2001:db8:100::/64 to port 53,123,161,60000-61000\n"
		"#\n"
		"# all + any source\n"
		"# allow all from any to port any\n"
		"# allow all from any to port 22\n"
		"# allow all from any to port 80,443\n"
		"# allow all from any to port 60000-61000\n"
		"# allow all from any to port 53,80,443,60000-61000\n"
		"#\n"
		"# all + IPv4 host\n"
		"# allow all from 203.0.113.10 to port any\n"
		"# allow all from 203.0.113.10 to port 22\n"
		"# allow all from 203.0.113.10 to port 80,443\n"
		"# allow all from 203.0.113.10 to port 60000-61000\n"
		"# allow all from 203.0.113.10 to port 53,80,443,60000-61000\n"
		"#\n"
		"# all + IPv4 network\n"
		"# allow all from 203.0.113.0/24 to port any\n"
		"# allow all from 203.0.113.0/24 to port 22\n"
		"# allow all from 203.0.113.0/24 to port 80,443\n"
		"# allow all from 203.0.113.0/24 to port 60000-61000\n"
		"# allow all from 203.0.113.0/24 to port 53,80,443,60000-61000\n"
		"#\n"
		"# all + IPv6 host\n"
		"# allow all from 2001:db8:100::10 to port any\n"
		"# allow all from 2001:db8:100::10 to port 22\n"
		"# allow all from 2001:db8:100::10 to port 80,443\n"
		"# allow all from 2001:db8:100::10 to port 60000-61000\n"
		"# allow all from 2001:db8:100::10 to port 53,80,443,60000-61000\n"
		"#\n"
		"# all + IPv6 network\n"
		"# allow all from 2001:db8:100::/64 to port any\n"
		"# allow all from 2001:db8:100::/64 to port 22\n"
		"# allow all from 2001:db8:100::/64 to port 80,443\n"
		"# allow all from 2001:db8:100::/64 to port 60000-61000\n"
		"# allow all from 2001:db8:100::/64 to port 53,80,443,60000-61000\n"
		"#\n"
		"# deny examples\n"
		"# deny tcp from any to port 60000-61000\n"
		"# deny udp from 203.0.113.0/24 to port 53,123\n"
		"# deny all from 2001:db8:200::/64 to port 25,110,143\n";
}


void
print_usage()
{
	std::cout
		<< "Usage: haikufwctl <command> [args]\n"
		<< "\n"
		<< "Commands:\n"
		<< "  check [config_path]\n"
		<< "  compile [config_path] [compiled_rules_path] [generation]\n"
		<< "  reload [config_path] [compiled_rules_path] [generation]\n"
		<< "  status\n"
		<< "  clear\n"
		<< "  install-default-config [config_path]\n";
}


int
do_check(const std::string& configPath)
{
	haikufw::FirewallConfig config = haikufw::parse_config_file(configPath);
	std::cout << "OK default_inbound=" << haikufw::to_string(config.default_inbound)
		<< " rules=" << config.rules.size() << "\n";
	return 0;
}


uint32_t
parse_generation_arg(const char* arg)
{
	if (arg == NULL)
		return 1;

	size_t end = 0;
	unsigned long value = std::stoul(arg, &end, 10);
	if (end == 0 || arg[end] != '\0' || value > 0xfffffffful)
		throw std::runtime_error("invalid generation");
	return static_cast<uint32_t>(value);
}


int
do_compile(const std::string& configPath, const std::string& compiledRulesPath,
	uint32_t generation)
{
	haikufw::FirewallConfig config = haikufw::parse_config_file(configPath);
	std::vector<uint8_t> blob = haikufw::compile_ruleset_blob(config, generation);
	haikufw::write_compiled_ruleset_atomically(blob, compiledRulesPath);
	std::cout << "compiled " << compiledRulesPath << " bytes=" << blob.size()
		<< " rules=" << config.rules.size() << " generation=" << generation
		<< "\n";
	return 0;
}


int
do_reload(const std::string& configPath, const std::string& compiledRulesPath,
	uint32_t generation)
{
	haikufw::FirewallConfig config = haikufw::parse_config_file(configPath);
	std::vector<uint8_t> blob = haikufw::compile_ruleset_blob(config, generation);
	haikufw::write_compiled_ruleset_atomically(blob, compiledRulesPath);
	haikufw::load_ruleset_into_kernel(blob);
	std::cout << "reloaded generation=" << generation << " rules="
		<< config.rules.size() << " bytes=" << blob.size() << "\n";
	return 0;
}


int
do_status()
{
	haikufw_status status = haikufw::fetch_kernel_status();
	std::cout << "abi_version=" << status.abi_version << "\n";
	std::cout << "active_generation=" << status.active_generation << "\n";
	std::cout << "default_action="
		<< (status.default_action == HAIKUFW_ACTION_ALLOW ? "allow" : "deny")
		<< "\n";
	std::cout << "rule_count=" << status.rule_count << "\n";
	std::cout << "port_span_count=" << status.port_span_count << "\n";
	std::cout << "flags=" << status.flags << "\n";
	std::cout << "packets_allowed=" << status.packets_allowed << "\n";
	std::cout << "packets_denied=" << status.packets_denied << "\n";
	return 0;
}


int
do_clear()
{
	haikufw::clear_kernel_ruleset();
	std::cout << "cleared kernel ruleset\n";
	return 0;
}


int
do_install_default_config(const std::string& configPath)
{
	std::ifstream input(configPath);
	if (input.good()) {
		std::cout << "config already exists: " << configPath << "\n";
		return 0;
	}

	std::ofstream output(configPath, std::ios::trunc);
	if (!output) {
		std::cerr << "unable to write default config: " << configPath << "\n";
		return 1;
	}

	output << example_config();
	std::cout << "wrote default config: " << configPath << "\n";
	return 0;
}

} // namespace


int
main(int argc, char** argv)
{
	try {
		if (argc < 2) {
			print_usage();
			return 1;
		}

		std::string command = argv[1];

		if (command == "check")
			return do_check(argc >= 3 ? argv[2] : kDefaultConfigPath);

		if (command == "compile") {
			return do_compile(argc >= 3 ? argv[2] : kDefaultConfigPath,
				argc >= 4 ? argv[3] : kDefaultCompiledRulesPath,
				parse_generation_arg(argc >= 5 ? argv[4] : NULL));
		}

		if (command == "reload") {
			return do_reload(argc >= 3 ? argv[2] : kDefaultConfigPath,
				argc >= 4 ? argv[3] : kDefaultCompiledRulesPath,
				parse_generation_arg(argc >= 5 ? argv[4] : NULL));
		}

		if (command == "status")
			return do_status();

		if (command == "clear")
			return do_clear();

		if (command == "install-default-config")
			return do_install_default_config(argc >= 3 ? argv[2] : kDefaultConfigPath);

		print_usage();
		return 1;
	} catch (const std::exception& error) {
		std::cerr << "haikufwctl: " << error.what() << "\n";
		return 1;
	}
}
