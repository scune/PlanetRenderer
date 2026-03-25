#include "Context.hpp"

#include "Device.hpp"
#include "DeviceFeatureContainer.hpp"
#include "ResultCheck.hpp"
#include "Swapchain.hpp"
#include <thread>

void Context::Init(bool force_x11)
{
  CreateWindow(force_x11);
  CreateVulkan();
  std::array<const char*, 4> extensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
      VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
      VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME};

  VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR
      enabledExecutableFeatures{};
  enabledExecutableFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR;
  enabledExecutableFeatures.pipelineExecutableInfo = VK_TRUE;

  VkPhysicalDeviceSynchronization2Features enabledSynchronization2Features{};
  enabledSynchronization2Features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
  enabledSynchronization2Features.synchronization2 = VK_TRUE;

  VkPhysicalDeviceHostImageCopyFeatures enabledHostImageCopyFeatures{};
  enabledHostImageCopyFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES;
  enabledHostImageCopyFeatures.hostImageCopy = VK_TRUE;

  VkPhysicalDeviceDynamicRenderingFeatures enabledDynamicRenderingFeatures{};
  enabledDynamicRenderingFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
  enabledDynamicRenderingFeatures.dynamicRendering = VK_TRUE;

  VkPhysicalDeviceMaintenance9FeaturesKHR enabledMaintenance9Features{};
  enabledMaintenance9Features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_9_FEATURES_KHR;
  enabledMaintenance9Features.maintenance9 = VK_TRUE;

  VkPhysicalDeviceMaintenance4Features enabledMaintenance4Features{};
  enabledMaintenance4Features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
  enabledMaintenance4Features.maintenance4 = VK_TRUE;

  VkPhysicalDeviceShaderObjectFeaturesEXT enabledShaderObjectFeatures{};
  enabledShaderObjectFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
  enabledShaderObjectFeatures.shaderObject = VK_TRUE;

  VkPhysicalDeviceRobustness2FeaturesEXT enabledRobustness2Features{};
  enabledRobustness2Features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
  enabledRobustness2Features.nullDescriptor = VK_TRUE;

  VkPhysicalDeviceVulkan11Features enabledVulkan11Features{};
  enabledVulkan11Features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
  enabledVulkan11Features.storageBuffer16BitAccess = VK_TRUE;

  VkPhysicalDeviceVulkan12Features enabledVulkan12Features{};
  enabledVulkan12Features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  enabledVulkan12Features.descriptorIndexing = VK_TRUE;
  enabledVulkan12Features.runtimeDescriptorArray = VK_TRUE;
  enabledVulkan12Features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
  enabledVulkan12Features.bufferDeviceAddress = VK_TRUE;
  enabledVulkan12Features.shaderFloat16 = VK_TRUE;
  enabledVulkan12Features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
  enabledVulkan12Features.scalarBlockLayout = VK_TRUE;
  enabledVulkan12Features.shaderBufferInt64Atomics = VK_TRUE;
  enabledVulkan12Features.shaderSharedInt64Atomics = VK_TRUE;

  DeviceFeatureContainer deviceFeatures{};
  deviceFeatures.GetFeatures2().features.samplerAnisotropy = VK_TRUE;
  deviceFeatures.GetFeatures2().features.shaderInt16 = VK_TRUE;
  deviceFeatures.GetFeatures2().features.shaderInt64 = VK_TRUE;

  deviceFeatures.Reserve(11);

  deviceFeatures.AddFeature(enabledExecutableFeatures);
  deviceFeatures.AddFeature(enabledSynchronization2Features);
  // deviceFeatures.AddFeature(enabledHostImageCopyFeatures);
  deviceFeatures.AddFeature(enabledDynamicRenderingFeatures);
  // deviceFeatures.AddFeature(enabledMaintenance9Features);
  deviceFeatures.AddFeature(enabledMaintenance4Features);
  deviceFeatures.AddFeature(enabledShaderObjectFeatures);
  deviceFeatures.AddFeature(enabledRobustness2Features);
  deviceFeatures.AddFeature(enabledVulkan11Features);
  deviceFeatures.AddFeature(enabledVulkan12Features);

  mPhysicalDevice = PickPhysicalDevice(mInstance, mSurface, extensions.data(),
                                       extensions.size(), deviceFeatures,
                                       mQueueFamilyIndices);

  IfNThrow(CreateDevice(extensions.data(), extensions.size(),
                        &deviceFeatures.GetFeatures2(), mPhysicalDevice,
                        mQueueFamilyIndices.graphicsFamily.value(),
                        mQueueFamilyIndices.presentFamily.value(), mDevice),
           "Failed to create device!");

  vkGetDeviceQueue(mDevice, mQueueFamilyIndices.graphicsFamily.value(), 0,
                   &mGraphicsQueue);
  vkGetDeviceQueue(mDevice, mQueueFamilyIndices.presentFamily.value(), 0,
                   &mPresentQueue);

  GetDeviceProps();

  CreateCommandBuffer();

  VkFenceCreateInfo fenceCreateInfo{};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mFenceOneTime);
  vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mFencePreRec);
}

void Context::CreateWindow(bool force_x11)
{
  if (force_x11)
  {
    COUT("Forced X11");
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
  }

  IfNThrow(glfwInit() == GLFW_TRUE, "Failed to initialize glfw!");
  IfNThrow(glfwVulkanSupported() == GLFW_TRUE, "Glfw: no vulkan supported!");

  m_bGlfwInit = true;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);

  const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
  // uint32_t width = mode->width >> 1;
  // uint32_t height = mode->height >> 1;
  uint32_t width = mode->width;
  uint32_t height = mode->height;
  // width -= mode->width >> 2;
  // height -= mode->height >> 2;

  mWindow = glfwCreateWindow(width, height, mWindowTitle, nullptr, nullptr);
  IfNThrow(mWindow, "Failed to create glfw window!");
}

#ifdef DEBUG
bool Context::CheckValidationLayerSupport() noexcept
{
  uint32_t layerCount = 0;
  IfNRetF(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

  std::vector<VkLayerProperties> availableLayers(layerCount);
  IfNRetF(
      vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

  for (const auto& layerProperties : availableLayers)
  {
    if (strcmp("VK_LAYER_KHRONOS_validation", layerProperties.layerName) == 0)
    {
      return true;
    }
  }

  return false;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    const char* colorEnd = "";

    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      std::cerr << "\033[33m[VALIDATION_VERBOSE]: ";
      colorEnd = "\033[0m";
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      std::cerr << "[VALIDATION_INFO]: ";
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      std::cerr << "\033[33m[VALIDATION_WARNING]: ";
      colorEnd = "\033[0m";
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      std::cerr << "\033[31m[VALIDATION_ERROR]: ";
      colorEnd = "\033[0m";
      break;

    default:
      std::cerr << "\033[31mVALIDATION MESSAGE NOT HANDLED!\033[0m"
                << std::endl;
      return VK_FALSE;
    }

    std::cerr << pCallbackData->pMessage << colorEnd << std::endl;
  }

  return VK_FALSE;
}

bool Context::SetupDebugMessenger() noexcept
{
  auto CreateDebugUtilsMessenger =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          mInstance, "vkCreateDebugUtilsMessengerEXT");

  IfNRetFM(
      CreateDebugUtilsMessenger,
      "Failed to get \"vkCreateDebugUtilsMessengerEXT\" function pointer!");

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = DebugCallback;

  IfNRetFM(CreateDebugUtilsMessenger(mInstance, &createInfo, nullptr,
                                     &mDebugMessenger),
           "Failed to create debug utils messenger!");
  return true;
}
#endif // DEBUG

void Context::CreateVulkan()
{
  bool enableValidationLayers = false;

#ifdef DEBUG
  if (!(enableValidationLayers = CheckValidationLayerSupport()))
  {
    std::cerr << "Validation layer unavailable!\n";
  }
#endif // DEBUG

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pEngineName = "VEGA";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_4;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions,
                                      glfwExtensions + glfwExtensionCount);
  COUT("Glfw extensions:");
  for (auto str : extensions)
  {
    COUT(str);
  }

  if (enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  const char* validationLayerNames[] = {"VK_LAYER_KHRONOS_validation"};

  VkValidationFeatureEnableEXT enabledValidationFeatures[] = {
      VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};
  VkValidationFeaturesEXT validationFeatures{};
  validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
  validationFeatures.enabledValidationFeatureCount = 1;
  validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

  VkInstanceCreateInfo instanceInfo{};
  instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pApplicationInfo = &appInfo;
  instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  instanceInfo.ppEnabledExtensionNames = extensions.data();
  if (enableValidationLayers)
  {
    instanceInfo.enabledLayerCount = std::size(validationLayerNames);
    instanceInfo.ppEnabledLayerNames = validationLayerNames;
    instanceInfo.pNext = &validationFeatures;
  }
  IfNThrow(vkCreateInstance(&instanceInfo, nullptr, &mInstance),
           "Failed to create vulkan instance!");

#ifdef DEBUG
  if (enableValidationLayers)
  {
    if (!SetupDebugMessenger())
    {
      std::cerr << "Failed to setup vulkan debug messenger!" << std::endl;
    }
  }
#endif // DEBUG

  IfNThrow(glfwCreateWindowSurface(mInstance, mWindow, nullptr, &mSurface),
           "Failed to create vulkan surface!");
}

void Context::GetDeviceProps() noexcept
{
  VkPhysicalDeviceHostImageCopyProperties hostImageProps{};
  hostImageProps.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_PROPERTIES;

  VkPhysicalDeviceSubgroupProperties subgroupProps{};
  subgroupProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
  subgroupProps.pNext = &hostImageProps;

  VkPhysicalDeviceProperties2 props2{};
  props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  props2.pNext = &subgroupProps;

  vkGetPhysicalDeviceProperties2(mPhysicalDevice, &props2);

  mWorkgroupSize1D = subgroupProps.subgroupSize;
  mWorkgroupSize2D =
      uint32_t(std::roundf(std::sqrtf(float(subgroupProps.subgroupSize))));

  std::cout << "Device name: " << props2.properties.deviceName << "\n";

  mDeviceProps = props2.properties;

  mHostImageTransfer =
      (hostImageProps.copyDstLayoutCount > 0 && hostImageProps.pCopyDstLayouts);

  if (mHostImageTransfer)
  {
    COUT("Host image dst image layouts: (" << hostImageProps.copyDstLayoutCount
                                           << "):");
    for (uint32_t i = 0; i < hostImageProps.copyDstLayoutCount; i++)
    {
      COUT(string_VkImageLayout(hostImageProps.pCopyDstLayouts[i]));
    }
  }
}

void Context::CreateCommandBuffer()
{
  VkCommandPoolCreateInfo poolCreateInfo{};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolCreateInfo.queueFamilyIndex = mQueueFamilyIndices.graphicsFamily.value();
  poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  IfNThrow(
      vkCreateCommandPool(mDevice, &poolCreateInfo, nullptr, &mCmdPoolOneTime),
      "Failed to create command pool!");

  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = mCmdPoolOneTime;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1;
  IfNThrow(vkAllocateCommandBuffers(mDevice, &allocateInfo, &mCmdBufferOneTime),
           "Failed to allocate command buffer!");
}

void Context::CreateCmdBufferPreRec(uint32_t count)
{
  VkCommandPoolCreateInfo poolCreateInfo{};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolCreateInfo.queueFamilyIndex = mQueueFamilyIndices.graphicsFamily.value();
  IfNThrow(
      vkCreateCommandPool(mDevice, &poolCreateInfo, nullptr, &mCmdPoolPreRec),
      "Failed to create command pool!");

  mCmdBufferPreRec.resize(count);

  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = mCmdPoolPreRec;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = count;
  IfNThrow(
      vkAllocateCommandBuffers(mDevice, &allocateInfo, mCmdBufferPreRec.data()),
      "Failed to allocate command buffer!");
}

void Context::Destroy() noexcept
{
  if (mDevice != VK_NULL_HANDLE)
  {
    if (mFenceOneTime != VK_NULL_HANDLE)
      vkDestroyFence(mDevice, mFenceOneTime, nullptr);

    if (mFencePreRec != VK_NULL_HANDLE)
      vkDestroyFence(mDevice, mFencePreRec, nullptr);

    if (mCmdPoolOneTime != VK_NULL_HANDLE)
      vkDestroyCommandPool(mDevice, mCmdPoolOneTime, nullptr);

    if (mCmdPoolPreRec != VK_NULL_HANDLE)
      vkDestroyCommandPool(mDevice, mCmdPoolPreRec, nullptr);

    vkDestroyDevice(mDevice, nullptr);
  }

  if (mInstance != VK_NULL_HANDLE)
  {
#ifdef DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            mInstance, "vkDestroyDebugUtilsMessengerEXT");

    if (mDebugMessenger != VK_NULL_HANDLE &&
        vkDestroyDebugUtilsMessengerEXT != nullptr)
      vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
#endif // DEBUG

    if (mSurface != VK_NULL_HANDLE)
      vkDestroySurfaceKHR(mInstance, mSurface, nullptr);

    vkDestroyInstance(mInstance, nullptr);
  }

  if (mWindow)
    glfwDestroyWindow(mWindow);
  if (m_bGlfwInit)
    glfwTerminate();
}

bool Context::WindowShouldClose() noexcept
{
  return glfwGetKey(mWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
         glfwWindowShouldClose(gContext.GetWindow());
}

inline void FpsToString(uint32_t fps, char* str) noexcept
{
  uint32_t temp = fps / 1000;
  str[0] = temp + '0';
  // str[0] = (str[0] == '0') ? ' ' : str[0];

  uint32_t temp2 = fps / 100;
  str[1] = temp2 - temp * 10 + '0';
  temp = temp2;
  // str[1] = (str[1] == '0') ? ' ' : str[1];

  temp2 = fps / 10;
  str[2] = temp2 - temp * 10 + '0';
  temp = temp2;
  // str[2] = (str[2] == '0') ? ' ' : str[2];

  str[3] = fps - temp * 10 + '0';
}

void Context::UpdateWindowTitle() noexcept
{
  mTitleUpdateTimer += mDeltaTime;
  if (mTitleUpdateTimer >= 1. / 8.)
  {
    uint32_t fps =
        std::min<uint32_t>(uint32_t(std::lround(1. / mDeltaTime)), 9999u);

    const char fpsTitle[]{"Fps"};
    char fpsStr[9] = " -      ";

    char title[sizeof(mWindowTitle) - 1 + sizeof(fps) - 1 + sizeof(fpsStr)]{};
    memcpy(title, mWindowTitle, sizeof(mWindowTitle) - 1);

    FpsToString(fps, fpsStr + 3);
    memcpy(title + sizeof(mWindowTitle) - 1, fpsStr, sizeof(fpsStr) - 1);

    memcpy(title + sizeof(mWindowTitle) - 1 + sizeof(fpsStr) - 1, fpsTitle,
           sizeof(fpsTitle));

    glfwSetWindowTitle(mWindow, title);

    mTitleUpdateTimer = 0.;
  }
}

void Context::NewDeltaTime() noexcept
{
  const double time = glfwGetTime();
  mDeltaTime = time - mLastFrameTime;
  mLastFrameTime = time;
}

double Context::UpdateDeltaTime() noexcept
{
  mDeltaTime = glfwGetTime() - mLastFrameTime;
  return mDeltaTime;
}

void Context::WaitForFence(VkFence fence)
{
  IfNThrow(vkWaitForFences(mDevice, 1, &fence, VK_TRUE, UINT64_MAX),
           "Failed to wait for fence!");
}

void Context::ResetFence(VkFence fence)
{
  IfNThrow(vkResetFences(mDevice, 1, &fence), "Failed to reset fence!");
}

void Context::ResetAndBeginCmdBufferOneTime()
{
  IfNThrow(vkResetCommandBuffer(mCmdBufferOneTime, 0),
           "Failed to reset \"one time\" command buffer!");

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  IfNThrow(vkBeginCommandBuffer(mCmdBufferOneTime, &beginInfo),
           "Failed to begin \"one time\" command buffer!");
}

void Context::EndCmdBufferOneTime()
{
  IfNThrow(vkEndCommandBuffer(mCmdBufferOneTime),
           "Failed to end \"one time\" command buffer!");
}

void Context::BeginCmdBufferPreRec(uint32_t idx)
{
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  IfNThrow(vkBeginCommandBuffer(mCmdBufferPreRec[idx], &beginInfo),
           "Failed to begin \"prerecording\" command buffer!");
}

void Context::EndCmdBufferPreRec(uint32_t idx)
{
  IfNThrow(vkEndCommandBuffer(mCmdBufferPreRec[idx]),
           "Failed to end \"prerecording\" command buffer!");
}

void Context::SubmitWorkPreRec(uint32_t idx)
{
  std::array<VkPipelineStageFlags, 1> waitStages = {
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &gSwapchain.GetImageAvailableSemaphore();
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &mCmdBufferPreRec[idx];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores =
      &gSwapchain.GetRenderFinishedSemaphores()[gSwapchain.GetFrameImgIndex()];

  IfNThrow(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mFencePreRec),
           "Failed to submit work to graphics queue!");

  NewDeltaTime();
  mFrameCounter++;
}

void Context::OneTimeSubmit()
{
  ResetFence(mFenceOneTime);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &mCmdBufferOneTime;
  IfNThrow(vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mFenceOneTime),
           "Failed to submit work to graphics queue!");

  WaitForFence(mFenceOneTime);
}

bool Context::FrameCap() noexcept
{
  UpdateDeltaTime();
  const double timeRemaining = mMaxFrameTime - mDeltaTime;
  if (timeRemaining > 0.)
  {
    if (timeRemaining > 0.002)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    else if (timeRemaining > 0.0005)
    {
      std::this_thread::yield();
    }

    return mMaxFrameTime - UpdateDeltaTime() > 0.;
  }
  return false;
}

VkFormat
Context::FindSupportedFormat(InitializerList<VkFormat> candidates,
                             VkImageTiling tiling,
                             VkFormatFeatureFlags features) const noexcept
{
  for (VkFormat format : candidates)
  {
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features)
    {
      return format;
    }
    else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
             (props.optimalTilingFeatures & features) == features)
    {
      return format;
    }
  }

  std::cerr << "Failed to find supported format!\n";
  return VK_FORMAT_UNDEFINED;
}
