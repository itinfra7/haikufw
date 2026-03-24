haikufw_core

implemented now

- shared ABI header: `haikufw_abi.h`
- shared packet metadata header: `haikufw_packet.h`
- matcher implementation: `haikufw_match.cpp`
- core module skeleton: `haikufw_core.cpp`

current responsibilities

- validate compiled ruleset blobs
- own active ruleset blob in kernel memory
- expose atomic load/clear/status/decide functions through the core module
- match:
  - source CIDR
  - protocol
  - destination port sets/ranges
  - default policy
