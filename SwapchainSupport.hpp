#pragma once

#include <Libs.hpp>

struct SwapchainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

static inline bool
QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                      SwapchainSupportDetails& details) noexcept
{
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          physicalDevice, surface, &details.capabilities) != VK_SUCCESS)
  {
    return false;
  }

  uint32_t formatCount = 0;
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                           &formatCount, nullptr) != VK_SUCCESS)
  {
    return false;
  }
  details.formats.resize(formatCount);
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(
          physicalDevice, surface, &formatCount, details.formats.data()) !=
      VK_SUCCESS)
  {
    return false;
  }

  uint32_t presentModeCount = 0;
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(
          physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS)
  {
    return false;
  }
  details.presentModes.resize(presentModeCount);
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(
          physicalDevice, surface, &presentModeCount,
          details.presentModes.data()) != VK_SUCCESS)
  {
    return false;
  }

  return true;
}
