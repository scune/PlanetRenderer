#pragma once

#include "Libs.hpp"
#include "vulkan/vulkan_core.h"

namespace AllowedDescriptors
{
template <typename T>
concept Infos =
    std::same_as<T, VkDescriptorImageInfo> ||
    std::same_as<T, VkDescriptorBufferInfo> || std::same_as<T, VkBufferView> ||
    std::same_as<T, VkWriteDescriptorSetAccelerationStructureKHR>;
}
