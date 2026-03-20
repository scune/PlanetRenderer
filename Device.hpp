#pragma once

#include <Libs.hpp>

#include "DeviceFeatureContainer.hpp"
#include <optional>

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  inline bool IsComplete() const noexcept
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

VkPhysicalDevice
PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
                   const char* const* requestedDeviceExtensions,
                   uint32_t requestedDeviceExtCount,
                   const DeviceFeatureContainer& requestedDeviceFeatures,
                   QueueFamilyIndices& queueFamilyIndices);

VkResult CreateDevice(const char* const* requestedDeviceExtensions,
                      uint32_t requestedDeviceExtCount, const void* pNext,
                      VkPhysicalDevice physicalDevice,
                      uint32_t graphicsQueueIndex, uint32_t presentQueueIndex,
                      VkDevice& device);
