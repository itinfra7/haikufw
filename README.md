# haikufw

haikufw is a real inbound firewall for Haiku. It enforces rules inside the
kernel/network stack before traffic is delivered to userland servers.

## Features

- Inbound TCP and UDP filtering on Haiku `x86_64`
- Patched Haiku `ipv4` and `ipv6` receive paths
- Shared in-kernel rule engine through `haikufw_core`
- Live control through `/dev/misc/haikufw` and `haikufwctl`
- Standalone source release that includes the vendored Haiku IPv4/IPv6
  protocol source subset needed for local builds on Haiku

## Repository Layout

- `config/`: example configurations
- `kernel/`: in-kernel rule engine, hook support, and control driver
- `patches/`: receive-path patches for Haiku `ipv4` and `ipv6`
- `scripts/`: build, install, uninstall, and smoke-test helpers
- `tests/`: host-side parser/compiler/matcher smoke coverage
- `userland/haikufwctl/`: config parser, compiler, and runtime control tool
- `vendor/`: vendored Haiku protocol source subset used for patched module
  builds

## Requirements

- Haiku `x86_64`
- A working build toolchain on the target host
- Privileges to install files under `/boot/system/non-packaged`

The standard release workflow does not require a separate full Haiku source
checkout. The release package already includes the vendored `ipv4` and `ipv6`
protocol source subset used for the patched build.

## Build And Install

From a writable directory on the target Haiku host:

```sh
wget https://github.com/itinfra7/haikufw/releases/latest/download/haikufw-release.tar.gz
tar -xzf haikufw-release.tar.gz
cd haikufw
./scripts/build-haikufw-haiku.sh
./scripts/install-haikufw.sh install
```

Reboot is required after install because the patched `ipv4` and `ipv6`
protocol modules must replace the stock modules during boot.

## Rule Syntax

- First match wins
- If no rule matches, `default_inbound` is applied
- Actions: `allow`, `deny`
- Protocols: `tcp`, `udp`, `all`
- Sources: `any`, IPv4 host/CIDR, IPv6 host/CIDR
- Destination ports: `any`, single port, list, range, or mixed list/range

Canonical examples:

```txt
default_inbound = deny

allow tcp from any to port 22
allow tcp from 203.0.113.0/24 to port 80,443
allow udp from 2001:db8:100::/64 to port 53,123
allow all from 198.51.100.0/24 to port 60000-61000
deny tcp from any to port 60000-61000
```

For the exhaustive syntax matrix, see `config/haikufw.conf.example`.

## Live Operations

```sh
haikufwctl check /boot/system/settings/haikufw.conf
haikufwctl reload /boot/system/settings/haikufw.conf /boot/system/settings/haikufw.rules "$(date +%s)"
haikufwctl status
```

To clear only the active in-kernel ruleset:

```sh
haikufwctl clear
```

## Uninstall

```sh
./scripts/install-haikufw.sh uninstall
```

Reboot is required after uninstall so the stock `ipv4` and `ipv6` modules are
restored.

## Included Docs

- `README.en.txt`
- `README.ko.txt`
- `OPERATIONS.txt`

## License

MIT. See `LICENSE`.
