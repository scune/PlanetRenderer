#include "Swapchain.hpp"

#include "Context.hpp"
#include "DescriptorBuilder.hpp"
#include "ResultCheck.hpp"
#include "SwapchainSupport.hpp"
#include "vulkan/vulkan_core.h"

void Swapchain::Init()
{
  CreateSwapchain();
  CreateSwapchainImageViews();

  mSrcQueueFamilyIndex =
      AreDeviceQueuesConcurrent()
          ? VK_QUEUE_FAMILY_IGNORED
          : gContext.GetQueueFamilyIndices().presentFamily.value();

  mDstQueueFamilyIndex =
      AreDeviceQueuesConcurrent()
          ? VK_QUEUE_FAMILY_IGNORED
          : gContext.GetQueueFamilyIndices().graphicsFamily.value();

  // Transition to present
  IfNThrow(gContext.ResetAndBeginCmdBufferOneTime(),
           "Failed to reset and begin cmd buffer!");
  for (uint32_t i = 0; i < mViews.size(); i++)
  {
    TransitionForPresent(gContext.GetCmdBufferOneTime(), i, VK_ACCESS_NONE,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_IMAGE_LAYOUT_UNDEFINED);
  }
  IfNThrow(gContext.EndCmdBufferOneTime(), "Failed to end cmd buffer!");
  IfNThrow(gContext.OneTimeSubmit(), "Failed to submit!");

  // Create semaphores
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  IfNThrow(vkCreateSemaphore(gContext.GetDevice(), &semaphoreInfo, nullptr,
                             &mImageAvailableSemaphore),
           "Failed to create image available semaphore!");

  mRenderFinishedSemaphores.resize(mImages.size());
  for (auto& semaphore : mRenderFinishedSemaphores)
  {
    IfNThrow(vkCreateSemaphore(gContext.GetDevice(), &semaphoreInfo, nullptr,
                               &semaphore),
             "Failed to create render finished semaphore!")
  }

  CreateDescriptor();
}

void Swapchain::Destroy() noexcept
{
  if (gContext.GetDevice() == VK_NULL_HANDLE)
    return;

  if (mImageAvailableSemaphore != VK_NULL_HANDLE)
  {
    vkDestroySemaphore(gContext.GetDevice(), mImageAvailableSemaphore, nullptr);
  }

  for (auto& semaphore : mRenderFinishedSemaphores)
  {
    if (semaphore != VK_NULL_HANDLE)
    {
      vkDestroySemaphore(gContext.GetDevice(), semaphore, nullptr);
    }
  }

  DestroySwapchain();

  if (mDescriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(gContext.GetDevice(), mDescriptorPool, nullptr);
  }
  if (mDescriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(gContext.GetDevice(), mDescriptorSetLayout,
                                 nullptr);
  }
}

void Swapchain::TransitionForWrite(VkCommandBuffer cmdBuffer, uint32_t imgIdx,
                                   VkAccessFlags accessFlags,
                                   VkPipelineStageFlags stageFlags,
                                   VkImageLayout newLayout) noexcept
{
  VkImageMemoryBarrier imageBarrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_NONE,
      .dstAccessMask = accessFlags,
      .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = mSrcQueueFamilyIndex,
      .dstQueueFamilyIndex = mDstQueueFamilyIndex,
      .image = mImages[imgIdx],
      .subresourceRange = mSubresourceRange};

  vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, stageFlags,
                       0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
}

void Swapchain::TransitionForPresent(VkCommandBuffer cmdBuffer, uint32_t imgIdx,
                                     VkAccessFlags accessFlags,
                                     VkPipelineStageFlags stageFlags,
                                     VkImageLayout oldLayout) noexcept
{
  VkImageMemoryBarrier imageBarrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = accessFlags,
      .dstAccessMask = VK_ACCESS_NONE,
      .oldLayout = oldLayout,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcQueueFamilyIndex = mSrcQueueFamilyIndex,
      .dstQueueFamilyIndex = mDstQueueFamilyIndex,
      .image = mImages[imgIdx],
      .subresourceRange = mSubresourceRange};

  vkCmdPipelineBarrier(cmdBuffer, stageFlags,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &imageBarrier);
}

bool Swapchain::AcquireNextImage()
{
  const VkResult result = vkAcquireNextImageKHR(
      gContext.GetDevice(), mSwapchain, UINT64_MAX, mImageAvailableSemaphore,
      VK_NULL_HANDLE, &mFrameImgIdx);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    // TODO: Implement resize
    std::cerr << "AcquireNextSwapchainImage(): RESIZE NOT IMPLEMENTED!\n";
    return false;
  }
  IfNThrow(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR,
           "Failed to acquire next swapchain image!");
  return true;
}

bool Swapchain::Present() noexcept
{
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[mFrameImgIdx];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &mSwapchain;
  presentInfo.pImageIndices = &mFrameImgIdx;

  const VkResult result =
      vkQueuePresentKHR(gContext.GetPresentQueue(), &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
  {
    // TODO: Implement resize queue
    std::cerr << "Swapchain::Present(): RESIZE NOT IMPLEMENTED!\n";
    return false;
  }
  IfNThrow(result, "Failed to present swapchain!");
  return true;
}

void Swapchain::DestroySwapchain() noexcept
{
  for (auto& view : mViews)
  {
    if (view != VK_NULL_HANDLE)
    {
      vkDestroyImageView(gContext.GetDevice(), view, nullptr);
      view = VK_NULL_HANDLE;
    }
  }

  if (mSwapchain != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(gContext.GetDevice(), mSwapchain, nullptr);
    mSwapchain = VK_NULL_HANDLE;
  }
}

void Swapchain::CreateDescriptor()
{
  if (!(mUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT))
    return;

  std::vector<VkDescriptorImageInfo> infos{mImages.size()};
  for (uint32_t i = 0; i < mImages.size(); i++)
  {
    infos[i].imageView = mViews[i];
    infos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  }

  DescriptorBuilder<1> builder;
  builder.AddBinding<VkDescriptorImageInfo>(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                            VK_SHADER_STAGE_COMPUTE_BIT, infos,
                                            true);

  const auto descriptorPoolRes = builder.BuildDescriptorPool(mImages.size());
  if (descriptorPoolRes.has_value())
  {
    mDescriptorPool = descriptorPoolRes.value();
  }
  else
  {
    IfNThrow(descriptorPoolRes.error(), "Failed to create descriptor pool!");
  }

  const auto descriptorSetLayoutRes = builder.BuildDescriptorSetLayout();
  if (descriptorSetLayoutRes.has_value())
  {
    mDescriptorSetLayout = descriptorSetLayoutRes.value();
  }
  else
  {
    IfNThrow(descriptorSetLayoutRes.error(),
             "Failed to create descriptor set layout!");
  }

  mDescriptorSets.resize(mImages.size());
  for (uint32_t i = 0; i < mImages.size(); i++)
  {
    IfNThrow(builder.BuildDescriptorSet(mDescriptorPool, mDescriptorSetLayout,
                                        mDescriptorSets[i]),
             "Failed to create descriptor set!");

    IfNThrow(mDescriptorSets[i].Update(builder.GetResourceInfos(), i),
             "Failed to update descriptor set!");
  }
}

VkSurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) noexcept
{
  for (const auto& availableFormat : availableFormats)
  {
    if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
    else if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
             availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
    else if (availableFormat.format == VK_FORMAT_R8G8B8A8_SNORM &&
             availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
    else if (availableFormat.format == VK_FORMAT_B8G8R8A8_SNORM &&
             availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }
  return availableFormats.front();
}

VkPresentModeKHR Swapchain::ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) noexcept
{
  for (const auto availablePresentMode : availablePresentModes)
  {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      return availablePresentMode;
    // if (availablePresentMode == vk::PresentModeKHR::eImmediate)
    //     return availablePresentMode;
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::ChooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities) noexcept
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }
  else
  {
    int width, height;
    glfwGetFramebufferSize(gContext.GetWindow(), &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width =
        glm::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        glm::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

VkImageUsageFlags
Swapchain::ChooseUsage(VkImageUsageFlags supportedUsageFlags) const noexcept
{
  return (supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) |
         (supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
}

void Swapchain::CreateSwapchain()
{
  SwapchainSupportDetails swapchainSupport{};
  IfNThrow(QuerySwapchainSupport(gContext.GetPhysicalDevice(),
                                 gContext.GetSurface(), swapchainSupport),
           "No swapchain support!");

  VkSurfaceFormatKHR surfaceFormat =
      ChooseSwapSurfaceFormat(swapchainSupport.formats);
  VkPresentModeKHR presentMode =
      ChooseSwapPresentMode(swapchainSupport.presentModes);
  mExtent = ChooseSwapExtent(swapchainSupport.capabilities);
  mUsageFlags = ChooseUsage(swapchainSupport.capabilities.supportedUsageFlags);

  mFormat = surfaceFormat.format;

  // Image count
  uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
  if (swapchainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapchainSupport.capabilities.maxImageCount)
  {
    imageCount = swapchainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = gContext.GetSurface();
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = mExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = mUsageFlags;

  if (AreDeviceQueuesConcurrent())
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  else
  {
    const uint32_t indices[] = {
        gContext.GetQueueFamilyIndices().graphicsFamily.value(),
        gContext.GetQueueFamilyIndices().presentFamily.value()};

    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = indices;
  }

  createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  IfNThrow(vkCreateSwapchainKHR(gContext.GetDevice(), &createInfo, nullptr,
                                &mSwapchain),
           "Failed to create swapchain!\n");

  uint32_t swapchainImageCount{0};
  IfNThrow(vkGetSwapchainImagesKHR(gContext.GetDevice(), mSwapchain,
                                   &swapchainImageCount, nullptr),
           "Failed to get swapchain image count!\n");

  mImages.resize(swapchainImageCount);
  IfNThrow(vkGetSwapchainImagesKHR(gContext.GetDevice(), mSwapchain,
                                   &swapchainImageCount, mImages.data()),
           "Failed to get swapchain images!\n");

  mSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  mSubresourceRange.baseMipLevel = 0;
  mSubresourceRange.levelCount = 1;
  mSubresourceRange.baseArrayLayer = 0;
  mSubresourceRange.layerCount = 1;
}

void Swapchain::CreateSwapchainImageViews()
{
  VkImageViewCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = mFormat;
  createInfo.components =
      VkComponentMapping{.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                         .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                         .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                         .a = VK_COMPONENT_SWIZZLE_IDENTITY};
  createInfo.subresourceRange = mSubresourceRange;

  mViews.resize(mImages.size());
  for (uint32_t i = 0; i < mViews.size(); i++)
  {
    createInfo.image = mImages[i];
    IfNThrow(vkCreateImageView(gContext.GetDevice(), &createInfo, nullptr,
                               &mViews[i]),
             "Failed to create swapchain image view!");
  }
}
