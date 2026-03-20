#pragma once

#include "Libs.hpp"

bool AllocateDeviceMemory(VkMemoryRequirements& memRequirements,
                          VkMemoryPropertyFlags memProperties,
                          VkDeviceMemory& mem,
                          const void* memAllocPNext = nullptr) noexcept;
