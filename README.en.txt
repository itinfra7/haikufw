haikufw manual (English)

overview

haikufw is a real inbound firewall for Haiku. It enforces rules inside the
kernel/network stack before traffic is delivered to userland servers.

The current implementation:
- keeps the stock TCP and UDP protocol modules in place
- patches the Haiku IPv4 and IPv6 receive paths
- calls `haikufw_core` from patched `ipv4` and `ipv6` protocol modules
- exposes `/dev/misc/haikufw` for live control
- uses `haikufwctl` to parse text rules and load a compiled ruleset
- reloads the configured rules during boot with `haikufw-boot.sh`

what this package contains

- kernel rule engine:
  - `kernel/core/`
- control driver:
  - `kernel/control_driver/`
- receive-path hook patches:
  - `patches/ipv4-receive-hook.patch`
  - `patches/ipv6-receive-hook.patch`
- vendored Haiku IPv4/IPv6 protocol sources required for build:
  - `vendor/haiku/src/add-ons/kernel/network/protocols/ipv4/`
  - `vendor/haiku/src/add-ons/kernel/network/protocols/ipv6/`
- userland control tool:
  - `userland/haikufwctl/`
- build/install/test scripts:
  - `scripts/`
- example configurations:
  - `config/`

requirements

- Haiku x86_64
- build toolchain available on the target host
- root privileges or equivalent ability to install files under
  `/boot/system/non-packaged`

The release package already includes the Haiku IPv4/IPv6 protocol source subset
used for the patched module build. A separate full Haiku source checkout is not
required for the standard release workflow.

important behavior

- firewall decisions are inbound only
- matching is first-match-wins
- if no rule matches, `default_inbound` is applied
- reload is live and does not require reinstall
- install and uninstall both require reboot because patched kernel protocol
  modules must replace the stock modules during boot

rule syntax

The configuration file is `/boot/system/settings/haikufw.conf`.

Supported actions:
- `allow`
- `deny`

Supported protocols:
- `tcp`
- `udp`
- `all`

Supported source selectors:
- `any`
- IPv4 host
- IPv4 CIDR
- IPv6 host
- IPv6 CIDR

Supported destination ports:
- `any`
- single port: `22`
- comma-separated list: `80,443`
- range: `60000-61000`
- mixed list and range: `53,80,443,60000-61000`

Minimal example:

default_inbound = deny

allow tcp from any to port 22
allow udp from any to port 53

Exhaustive syntax matrix:

See `config/haikufw.conf.example` for the full protocol/source/port combination
matrix.

Representative generic examples:

default_inbound = deny

allow tcp from 203.0.113.0/24 to port 22
allow tcp from 203.0.113.10 to port 22
allow tcp from 2001:db8:100::/64 to port 80,443
allow udp from 203.0.113.0/24 to port 53,123
allow all from 198.51.100.0/24 to port 60000-61000
deny udp from 203.0.113.0/24 to port 53,123
deny all from 2001:db8:200::/64 to port 25,110,143

build

From the extracted release root on the Haiku machine:

1. `./scripts/build-haikufw-haiku.sh`

The build script:
- prepares patched Haiku network protocol sources
- builds `haikufw_core`
- builds `haikufw` control driver
- builds patched `ipv4` and `ipv6`
- builds `haikufwctl`
- writes artifacts under `out/haiku/`

install

1. `./scripts/install-haikufw.sh install`
2. reboot

Install writes:
- `/boot/system/non-packaged/add-ons/kernel/generic/haikufw_core`
- `/boot/system/non-packaged/add-ons/kernel/drivers/dev/misc/haikufw`
- `/boot/system/non-packaged/add-ons/kernel/network/protocols/ipv4`
- `/boot/system/non-packaged/add-ons/kernel/network/protocols/ipv6`
- `/boot/system/non-packaged/bin/haikufwctl`
- `/boot/system/non-packaged/bin/haikufw-boot.sh`

Install also:
- creates `/boot/system/settings/haikufw.conf` if it does not exist
- registers a boot block in `UserBootscript`
- stores backups under the installer state directory

post-install verification

After reboot:

1. check that the device exists:
   - `ls /dev/misc/haikufw`
2. inspect kernel state:
   - `haikufwctl status`
3. verify your management path still works:
   - SSH from the intended source network

live config workflow

1. edit `/boot/system/settings/haikufw.conf`
2. validate syntax:
   - `haikufwctl check /boot/system/settings/haikufw.conf`
3. reload without reinstall:
   - `haikufwctl reload /boot/system/settings/haikufw.conf`
4. inspect runtime state:
   - `haikufwctl status`

clear runtime rules

- `haikufwctl clear`

This clears the in-kernel ruleset only. It does not remove installed files and
does not modify the config file on disk.

uninstall

1. `./scripts/install-haikufw.sh uninstall`
2. reboot

Uninstall removes:
- installed runtime files under `/boot/system/non-packaged`
- boot registration block in `UserBootscript`

Uninstall restores:
- backed up stock `ipv4` and `ipv6` protocol modules
- previously backed up files from the installer manifest

testing

Useful scripts included in this package:

- host parser/compiler smoke:
  - `./scripts/test-host.sh`
- live loopback rule smoke on Haiku:
  - `./scripts/live-rule-smoke-haiku.sh`
- remote reboot smoke from a control host:
  - `python3 ./scripts/reboot-smoke-remote.py --host <host> --password-file <file> --iterations 3 --settle-seconds 30`

rollout guidance

Recommended production rollout:

1. start with `default_inbound = allow`
2. validate `check`, `reload`, and `status`
3. add explicit allow rules for:
   - SSH management paths
   - any service ports that must remain reachable
4. switch to `default_inbound = deny`
5. verify from independent clients that only intended ports remain reachable

known scope

The current validated scope is inbound TCP and UDP filtering through the Haiku
IPv4 and IPv6 receive paths. That is the implementation and test surface this
package currently ships and validates.
