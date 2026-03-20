#pragma once

#include <Libs.hpp>

#include "DescriptorSet.hpp"

class Swapchain
{
public:
  Swapchain() = default;

  Swapchain(Swapchain&&) = delete;
  Swapchain(const Swapchain&) = delete;

  void Init();
  void Destroy() noexcept;

  void TransitionForWrite(VkCommandBuffer cmdBuffer, uint32_t imgIdx,
                          VkAccessFlags accessFlags,
                          VkPipelineStageFlags stageFlags,
                          VkImageLayout newLayout) noexcept;
  void TransitionForPresent(VkCommandBuffer cmdBuffer, uint32_t imgIdx,
                            VkAccessFlags accessFlags,
                            VkPipelineStageFlags stageFlags,
                            VkImageLayout oldLayout) noexcept;
  inline void TransitionForCompute(VkCommandBuffer cmdBuffer,
                                   uint32_t imgIdx) noexcept
  {
    TransitionForWrite(cmdBuffer, imgIdx, VK_ACCESS_SHADER_WRITE_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_IMAGE_LAYOUT_GENERAL);
  }
  inline void TransitionAfterCompute(VkCommandBuffer cmdBuffer,
                                     uint32_t imgIdx) noexcept
  {
    TransitionForPresent(cmdBuffer, imgIdx, VK_ACCESS_SHADER_WRITE_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_IMAGE_LAYOUT_GENERAL);
  }
  inline void TransitionForGraphics(VkCommandBuffer cmdBuffer,
                                    uint32_t imgIdx) noexcept
  {
    TransitionForWrite(cmdBuffer, imgIdx, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  }
  inline void TransitionAfterGraphics(VkCommandBuffer cmdBuffer,
                                      uint32_t imgIdx) noexcept
  {
    TransitionForPresent(cmdBuffer, imgIdx,
                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  }

  bool AcquireNextImage();
  bool Present() noexcept;

  inline const std::vector<VkImage>& GetImages() const noexcept
  {
    return mImages;
  }
  inline const std::vector<VkImageView>& GetViews() const noexcept
  {
    return mViews;
  }
  inline VkFormat GetFormat() const noexcept { return mFormat; }
  inline VkExtent2D GetExtent() const noexcept { return mExtent; }

  inline void SetFrameImgIndex(uint32_t frameImgIndex) noexcept
  {
    mFrameImgIdx = frameImgIndex;
  }
  inline uint32_t GetFrameImgIndex() const noexcept { return mFrameImgIdx; }

  inline const VkSemaphore& GetImageAvailableSemaphore() const noexcept
  {
    return mImageAvailableSemaphore;
  }
  inline const std::vector<VkSemaphore>&
  GetRenderFinishedSemaphores() const noexcept
  {
    return mRenderFinishedSemaphores;
  }

  inline VkDescriptorSetLayout GetDescriptorSetLayout() const noexcept
  {
    return mDescriptorSetLayout;
  }
  inline VkDescriptorSet GetDescriptorSet(uint32_t index) const noexcept
  {
    return mDescriptorSets[index].Get();
  }

private:
  inline bool AreDeviceQueuesConcurrent()
  {
    return gContext.GetQueueFamilyIndices().graphicsFamily.value() ==
           gContext.GetQueueFamilyIndices().presentFamily.value();
  }

  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR>& availableFormats) noexcept;
  VkPresentModeKHR ChooseSwapPresentMode(
      const std::vector<VkPresentModeKHR>& availablePresentModes) noexcept;
  VkExtent2D
  ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) noexcept;
  VkImageUsageFlags
  ChooseUsage(VkImageUsageFlags supportedUsageFlags) const noexcept;

  void CreateSwapchain();
  void CreateSwapchainImageViews();
  void DestroySwapchain() noexcept;
  VkSwapchainKHR mSwapchain{VK_NULL_HANDLE};
  std::vector<VkImage> mImages;
  std::vector<VkImageView> mViews;

  VkFormat mFormat{VK_FORMAT_UNDEFINED};
  VkExtent2D mExtent;
  VkImageUsageFlags mUsageFlags;
  VkImageSubresourceRange mSubresourceRange;

  VkSemaphore mImageAvailableSemaphore;
  std::vector<VkSemaphore> mRenderFinishedSemaphores;

  uint32_t mSrcQueueFamilyIndex{VK_QUEUE_FAMILY_IGNORED};
  uint32_t mDstQueueFamilyIndex{VK_QUEUE_FAMILY_IGNORED};

  uint32_t mFrameImgIdx{0};

  // Descriptor
  void CreateDescriptor();
  VkDescriptorPool mDescriptorPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout mDescriptorSetLayout{VK_NULL_HANDLE};
  std::vector<DescriptorSet> mDescriptorSets{};
};

inline Swapchain gSwapchain;
