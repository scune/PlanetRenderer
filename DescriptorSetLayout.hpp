#pragma once

#include "Libs.hpp"

#include "Context.hpp"
#include "InitializerList.hpp"
#include "vulkan/vulkan_core.h"
#include <expected>

inline std::expected<VkDescriptorSetLayout, VkResult> CreateDescriptorSetLayout(
    InitializerList<VkDescriptorSetLayoutBinding> bindings,
    InitializerList<VkDescriptorBindingFlags> flags) noexcept
{
  VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
  bindingFlagsInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
  bindingFlagsInfo.bindingCount = flags.size();
  bindingFlagsInfo.pBindingFlags = flags.begin();

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = bindings.size();
  layoutInfo.pBindings = bindings.begin();
  layoutInfo.pNext = &bindingFlagsInfo;

  VkDescriptorSetLayout layout;
  const VkResult res = vkCreateDescriptorSetLayout(
      gContext.GetDevice(), &layoutInfo, nullptr, &layout);

  if (res == VK_SUCCESS)
  {
    return layout;
  }
  else
  {
    return std::unexpected(res);
  }
}
