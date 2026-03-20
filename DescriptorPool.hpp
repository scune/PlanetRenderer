#pragma once

#include "Libs.hpp"

#include "Context.hpp"
#include "InitializerList.hpp"
#include <expected>

inline std::expected<VkDescriptorPool, VkResult>
CreateDescriptorPool(InitializerList<VkDescriptorPoolSize> poolSizes,
                     uint32_t maxSets) noexcept
{
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.pPoolSizes = poolSizes.begin();
  poolInfo.poolSizeCount = poolSizes.size();
  poolInfo.maxSets = maxSets;

  VkDescriptorPool pool;
  const VkResult res =
      vkCreateDescriptorPool(gContext.GetDevice(), &poolInfo, nullptr, &pool);

  if (res == VK_SUCCESS)
  {
    return pool;
  }
  else
  {
    return std::unexpected(res);
  }
}
