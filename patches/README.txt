haikufw patches

current files

- `ipv4-receive-hook.patch`
- `ipv6-receive-hook.patch`

meaning

- `ipv4-receive-hook.patch`
  - adds `haikufw_kernel_hook.h`
  - calls `haikufw_should_allow_inbound_buffer()` in `ipv4_receive_data()`
  - drops denied TCP/UDP packets before transport handoff
- `ipv6-receive-hook.patch`
  - adds `haikufw_kernel_hook.h`
  - calls `haikufw_should_allow_inbound_buffer()` in `ipv6_receive_data()`
  - drops denied TCP/UDP packets before transport handoff
