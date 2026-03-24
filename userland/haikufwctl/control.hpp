#pragma once

#include "../../kernel/core/haikufw_abi.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace haikufw {

void load_ruleset_into_kernel(const std::vector<uint8_t>& blob);
void clear_kernel_ruleset();
haikufw_status fetch_kernel_status();

} // namespace haikufw
