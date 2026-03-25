#include "Device.hpp"

#include "ResultCheck.hpp"
#include "SwapchainSupport.hpp"

bool CheckDeviceExtensionSupport(VkPhysicalDevice device,
                                 const char* const* requestedDeviceExtensions,
                                 uint32_t requestedDeviceExtCount) noexcept
{
  uint32_t extensionCount = 0;
  if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                           nullptr) != VK_SUCCESS)
  {
    return false;
  }

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                           availableExtensions.data()) !=
      VK_SUCCESS)
  {
    return false;
  }

  std::set<std::string> requiredExtensions(requestedDeviceExtensions,
                                           requestedDeviceExtensions +
                                               requestedDeviceExtCount);

  for (const auto& extension : availableExtensions)
    requiredExtensions.erase(extension.extensionName);

  return requiredExtensions.empty();
}

bool FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface,
                       QueueFamilyIndices& queueFamilyIndices) noexcept
{
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  uint32_t i = 0;
  uint32_t computeFamily = UINT32_MAX;
  for (const auto& queueFamily : queueFamilies)
  {
    if (queueFamily.queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily = i;

      if (queueFamily.queueFlags & VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT)
        computeFamily = i;
    }

    VkBool32 presentSupport = VK_FALSE;
    if (vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &presentSupport) != VK_SUCCESS)
    {
      return false;
    }

    if (presentSupport)
      indices.presentFamily = i;

    if (indices.IsComplete())
      break;

    i++;
  }

  queueFamilyIndices = indices;
  return indices.graphicsFamily == computeFamily;
}

bool CheckDeviceFeatureSupport(
    VkPhysicalDevice device,
    const DeviceFeatureContainer& requestedDeviceFeatures)
{
  auto supportedFeatures{requestedDeviceFeatures};

  vkGetPhysicalDeviceFeatures2(device, &supportedFeatures.GetFeatures2());

  return requestedDeviceFeatures.IsSubset(supportedFeatures);
}

bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface,
                      const char* const* requestedDeviceExtensions,
                      uint32_t requestedDeviceExtCount,
                      const DeviceFeatureContainer& requestedDeviceFeatures,
                      QueueFamilyIndices& indices) noexcept
{
  if (!FindQueueFamilies(device, surface, indices))
    return false;

  IfNRetF(CheckDeviceExtensionSupport(device, requestedDeviceExtensions,
                                      requestedDeviceExtCount));

  IfNRetF(CheckDeviceFeatureSupport(device, requestedDeviceFeatures));

  SwapchainSupportDetails swapchainSupport;
  IfNRetF(QuerySwapchainSupport(device, surface, swapchainSupport));

  return indices.IsComplete();
}

VkPhysicalDevice
PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface,
                   const char* const* requestedDeviceExtensions,
                   uint32_t requestedDeviceExtCount,
                   const DeviceFeatureContainer& deviceFeatures,
                   QueueFamilyIndices& queueFamilyIndices)
{
  uint32_t physicalDeviceCount{0};
  IfNThrow(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr),
           "Failed to get count of physical devices!");

  std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
  IfNThrow(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
                                      physicalDevices.data()),
           "Failed to enumerate physical devices!");

  // physicalDevices.erase(physicalDevices.begin());

  for (const auto& device : physicalDevices)
  {
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(device, &props);
    COUT("Physical device name: " << props.deviceName);

    if (IsDeviceSuitable(device, surface, requestedDeviceExtensions,
                         requestedDeviceExtCount, deviceFeatures,
                         queueFamilyIndices))
    {
      return device;
    }
  }

  IfNThrow(false, "Failed to find suitable physical device!");
  return nullptr;
}

VkResult CreateDevice(const char* const* requestedDeviceExtensions,
                      uint32_t requestedDeviceExtCount, const void* pNext,
                      VkPhysicalDevice physicalDevice,
                      uint32_t graphicsQueueIndex, uint32_t presentQueueIndex,
                      VkDevice& device)
{
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {graphicsQueueIndex,
                                            presentQueueIndex};

  const float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    queueCreateInfos.push_back(queueCreateInfo);
  }

  uint32_t deviceExtCount{};
  IfNThrow(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                                &deviceExtCount, nullptr),
           "Failed to get device extensions property count!");

  std::vector<VkExtensionProperties> extensionProperties(deviceExtCount);
  IfNThrow(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                                &deviceExtCount,
                                                extensionProperties.data()),
           "Failed to enumerate device extensions properties!");

  std::vector<const char*> extensions;

  const char VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME[] =
      "VK_KHR_portability_subset";
  for (auto extension : extensionProperties)
  {
    if (std::memcmp(extension.extensionName,
                    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
                    sizeof(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) - 1) == 0)
    {
      extensions.resize(requestedDeviceExtCount + 1);
      for (uint32_t i = 0; i < requestedDeviceExtCount; i++)
      {
        extensions[i] = requestedDeviceExtensions[i];
      }

      extensions.back() = VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
    }
  }

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pNext = pNext;
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.queueCreateInfoCount = queueCreateInfos.size();
  createInfo.enabledExtensionCount =
      (extensions.size() == 0) ? requestedDeviceExtCount : extensions.size();
  createInfo.ppEnabledExtensionNames =
      (extensions.size() == 0) ? requestedDeviceExtensions : extensions.data();

  return vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
}
