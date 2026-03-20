#pragma once

#include "Libs.hpp"

#include "Buffers.hpp"

struct Image
{
  VkImage handle{VK_NULL_HANDLE};
  VkDeviceMemory mem{VK_NULL_HANDLE};
  VkImageView view{VK_NULL_HANDLE};
  VkSampler sampler{VK_NULL_HANDLE};

  VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
  VkFormat format{VK_FORMAT_UNDEFINED};
  VkImageAspectFlags aspectFlags{VK_IMAGE_ASPECT_COLOR_BIT};
  VkExtent2D extent{.width = 0, .height = 0};
  uint32_t mipLevels{1};
};

bool CreateImage(Image& image, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags memProperties) noexcept;

void DestroyImage(Image& image) noexcept;

bool ImageCopyToDevice(Image& image, VkCommandBuffer cmdBuffer,
                       Buffer& outStagingBuffer, const void* data,
                       VkDeviceSize size) noexcept;

VkResult ImageCopyToDeviceWithEXT(const Image& image,
                                  const void* data) noexcept;

bool ImageCopyToDeviceAuto(Image& image, const void* data,
                           VkDeviceSize size) noexcept;

VkResult ImageTransitionLayoutEXT(Image& image, VkImageLayout layout) noexcept;
void ImageInitTransitionLayoutCmd(VkCommandBuffer cmdBuffer, Image& image,
                                  VkImageLayout layout) noexcept;

VkResult ImageTransitionLayoutAuto(Image& image, VkImageLayout layout) noexcept;

VkResult CreateSampler(Image& image, VkBool32 unnormCoords,
                       VkBool32 anisotropy) noexcept;

bool ImageGenMipmaps(Image& image) noexcept;

constexpr inline uint32_t CalcMipLevels(const VkExtent2D& extent) noexcept
{
  return uint32_t(std::log2f((float)std::max(extent.width, extent.height)));
}

constexpr inline VkDescriptorImageInfo
ImageGetDescriptorInfo(const Image& image) noexcept
{
  VkDescriptorImageInfo info{};
  info.sampler = image.sampler;
  info.imageView = image.view;
  info.imageLayout = image.layout;
  return info;
}
