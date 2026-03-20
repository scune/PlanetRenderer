#include "Images.hpp"

#include "Buffers.hpp"
#include "Context.hpp"
#include "GpuMemAlloc.hpp"
#include "ResultCheck.hpp"
#include "vulkan/vulkan_core.h"

bool CreateImage(Image& image, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags memProperties) noexcept
{
  assert(image.extent.width != 0 && image.extent.height != 0);
  assert(image.format != VK_FORMAT_UNDEFINED);
  assert((image.mipLevels > 1 && (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) &&
          (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) ||
         image.mipLevels == 1);

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = image.format;
  imageInfo.extent = {
      .width = image.extent.width, .height = image.extent.height, .depth = 1};
  imageInfo.mipLevels = image.mipLevels;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  IfNRetFM(
      vkCreateImage(gContext.GetDevice(), &imageInfo, nullptr, &image.handle),
      "Failed to create Image!");

  VkMemoryRequirements memRequirements{};
  vkGetImageMemoryRequirements(gContext.GetDevice(), image.handle,
                               &memRequirements);

  if (!AllocateDeviceMemory(memRequirements, memProperties, image.mem))
  {
    DestroyImage(image);
    return false;
  }

  const VkResult allocRes =
      vkBindImageMemory(gContext.GetDevice(), image.handle, image.mem, 0);
  if (allocRes != VK_SUCCESS)
  {
    DestroyImage(image);
    IfNRetFM(allocRes, "Failed to bind image memory!");
  }

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = image.format;
  viewInfo.subresourceRange = VkImageSubresourceRange{
      .aspectMask = image.aspectFlags,
      .baseMipLevel = 0,
      .levelCount = image.mipLevels,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };
  viewInfo.image = image.handle;

  const VkResult viewRes =
      vkCreateImageView(gContext.GetDevice(), &viewInfo, nullptr, &image.view);

  if (viewRes != VK_SUCCESS)
  {
    DestroyImage(image);
    IfNRetFM(viewRes, "Failed to create image view!");
  }

  if (image.layout != VK_IMAGE_LAYOUT_UNDEFINED &&
      (usage & VK_IMAGE_USAGE_HOST_TRANSFER_BIT))
  {
    const VkImageLayout newLayout = image.layout;
    image.layout = VK_IMAGE_LAYOUT_UNDEFINED;

    const VkResult transitionRes = ImageTransitionLayoutAuto(image, newLayout);

    if (transitionRes != VK_SUCCESS)
    {
      DestroyImage(image);
      IfNRetFM(transitionRes, "Failed to transition image layout!");
    }
  }
  else
  {
    image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
  }

  return true;
}

void DestroyImage(Image& image) noexcept
{
  if (image.handle != VK_NULL_HANDLE)
  {
    vkDestroyImage(gContext.GetDevice(), image.handle, nullptr);
    image.handle = VK_NULL_HANDLE;
  }

  if (image.view != VK_NULL_HANDLE)
  {
    vkDestroyImageView(gContext.GetDevice(), image.view, nullptr);
    image.view = VK_NULL_HANDLE;
  }

  if (image.mem != VK_NULL_HANDLE)
  {
    vkFreeMemory(gContext.GetDevice(), image.mem, nullptr);
    image.mem = VK_NULL_HANDLE;
  }

  if (image.sampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(gContext.GetDevice(), image.sampler, nullptr);
  }
}

bool ImageCopyToDevice(Image& image, VkCommandBuffer cmdBuffer,
                       Buffer& outStagingBuffer, const void* data,
                       VkDeviceSize size) noexcept
{
  assert(image.layout & VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  IfNRetF(CreateStagingBufferAndLoad(outStagingBuffer, data, size));

  VkBufferImageCopy bufferImageCopy{};
  bufferImageCopy.imageSubresource.aspectMask = image.aspectFlags;
  bufferImageCopy.imageSubresource.layerCount = 1;
  bufferImageCopy.imageExtent = {
      .width = image.extent.width, .height = image.extent.height, .depth = 1};

  vkCmdCopyBufferToImage(cmdBuffer, outStagingBuffer.handle, image.handle,
                         image.layout, 1, &bufferImageCopy);

  return true;
}

VkResult ImageCopyToDeviceWithEXT(const Image& image, const void* data) noexcept
{
  VkMemoryToImageCopy region{};
  region.sType = VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY;
  region.pHostPointer = data;
  region.imageSubresource.aspectMask = image.aspectFlags;
  region.imageSubresource.layerCount = 1;
  region.imageExtent = {
      .width = image.extent.width, .height = image.extent.height, .depth = 1};

  VkCopyMemoryToImageInfo memImageCopy{};
  memImageCopy.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO;
  memImageCopy.flags = VK_HOST_IMAGE_COPY_MEMCPY_BIT;
  memImageCopy.dstImage = image.handle;
  memImageCopy.dstImageLayout = image.layout;
  memImageCopy.regionCount = 1;
  memImageCopy.pRegions = &region;

  return vkCopyMemoryToImage(gContext.GetDevice(), &memImageCopy);
}

bool ImageCopyToDeviceAuto(Image& image, const void* data,
                           VkDeviceSize size) noexcept
{
  if (gContext.HostImageTransferSupp())
  {
    return ImageCopyToDeviceWithEXT(image, data) == VK_SUCCESS;
  }

  gContext.ResetAndBeginCmdBufferOneTime();

  const VkImageLayout oldLayout = image.layout;
  ImageInitTransitionLayoutCmd(gContext.GetCmdBufferOneTime(), image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  Buffer stagingBuffer;
  bool res = ImageCopyToDevice(image, gContext.GetCmdBufferOneTime(),
                               stagingBuffer, data, size);
  ImageInitTransitionLayoutCmd(gContext.GetCmdBufferOneTime(), image,
                               oldLayout);

  gContext.EndCmdBufferOneTime();
  if (res)
    gContext.OneTimeSubmit();

  DestroyBuffer(stagingBuffer);
  return res;
}

VkResult ImageTransitionLayoutEXT(Image& image, VkImageLayout layout) noexcept
{
  VkHostImageLayoutTransitionInfo info{};
  info.sType = VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO;
  info.image = image.handle;
  info.oldLayout = image.layout;
  info.newLayout = layout;
  info.subresourceRange.aspectMask = image.aspectFlags;
  info.subresourceRange.levelCount = image.mipLevels;
  info.subresourceRange.layerCount = 1;

  image.layout = layout;
  return vkTransitionImageLayout(gContext.GetDevice(), 1, &info);
}

void ImageInitTransitionLayoutCmd(VkCommandBuffer cmdBuffer, Image& image,
                                  VkImageLayout layout) noexcept
{
  VkImageSubresourceRange range{};
  range.aspectMask = image.aspectFlags;
  range.layerCount = 1;
  range.levelCount = image.mipLevels;

  VkImageMemoryBarrier imageBarrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_NONE,
      .dstAccessMask = VK_ACCESS_NONE,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image.handle,
      .subresourceRange = range};

  vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &imageBarrier);

  image.layout = layout;
}

VkResult ImageTransitionLayoutAuto(Image& image, VkImageLayout layout) noexcept
{
  if (gContext.HostImageTransferSupp())
  {
    return ImageTransitionLayoutEXT(image, layout);
  }

  gContext.ResetAndBeginCmdBufferOneTime();
  ImageInitTransitionLayoutCmd(gContext.GetCmdBufferOneTime(), image, layout);
  gContext.EndCmdBufferOneTime();
  gContext.OneTimeSubmit();
  return VK_SUCCESS;
}

VkResult CreateSampler(Image& image, VkBool32 unnormCoords,
                       VkBool32 anisotropy) noexcept
{
  VkSamplerCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  createInfo.magFilter = VK_FILTER_LINEAR;
  createInfo.minFilter = VK_FILTER_LINEAR;
  createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  createInfo.anisotropyEnable = anisotropy;
  createInfo.unnormalizedCoordinates = unnormCoords;
  createInfo.maxLod = (image.mipLevels > 0) ? image.mipLevels - 1 : 0;

  if (anisotropy)
  {
    createInfo.maxAnisotropy =
        gContext.GetDeviceProperties().limits.maxSamplerAnisotropy;
  }

  return vkCreateSampler(gContext.GetDevice(), &createInfo, nullptr,
                         &image.sampler);
}

bool ImageGenMipmaps(Image& image) noexcept
{
  assert(image.mipLevels > 1);

  VkImageBlit2 region{};
  region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
  region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.srcSubresource.layerCount = 1;
  region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.dstSubresource.layerCount = 1;

  VkBlitImageInfo2 blitInfo{};
  blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
  blitInfo.srcImage = image.handle;
  blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  blitInfo.dstImage = image.handle;
  blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  blitInfo.regionCount = 1;
  blitInfo.pRegions = &region;
  blitInfo.filter = VK_FILTER_LINEAR;

  VkImageMemoryBarrier2 imageBarriers[2]{};
  imageBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2; // Src
  imageBarriers[0].srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
  imageBarriers[0].srcAccessMask = VK_ACCESS_2_NONE;
  imageBarriers[0].dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
  imageBarriers[0].dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
  imageBarriers[0].oldLayout = image.layout;
  imageBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  imageBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarriers[0].image = image.handle;
  imageBarriers[0].subresourceRange.aspectMask = image.aspectFlags;
  imageBarriers[0].subresourceRange.levelCount = 1;
  imageBarriers[0].subresourceRange.layerCount = 1;

  imageBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2; // Dst
  imageBarriers[1].srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
  imageBarriers[1].srcAccessMask = VK_ACCESS_2_NONE;
  imageBarriers[1].dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
  imageBarriers[1].dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  imageBarriers[1].oldLayout = image.layout;
  imageBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imageBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageBarriers[1].image = image.handle;
  imageBarriers[1].subresourceRange.aspectMask = image.aspectFlags;
  imageBarriers[1].subresourceRange.levelCount = 1;
  imageBarriers[1].subresourceRange.layerCount = 1;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.imageMemoryBarrierCount = 2;
  dependencyInfo.pImageMemoryBarriers = imageBarriers;

  gContext.ResetAndBeginCmdBufferOneTime();
  auto cmdBuffer = gContext.GetCmdBufferOneTime();

  for (uint32_t i = 0; i < image.mipLevels - 1; i++)
  {
    imageBarriers[0].subresourceRange.baseMipLevel = i;
    imageBarriers[1].subresourceRange.baseMipLevel = i + 1;
    vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);

    region.srcSubresource.mipLevel = i;
    region.srcOffsets[1] = {.x = (int32_t)(image.extent.width >> i),
                            .y = (int32_t)(image.extent.height >> i),
                            .z = 1};
    region.dstSubresource.mipLevel = i + 1;
    region.dstOffsets[1] = {.x = (int32_t)(image.extent.width >> (i + 1)),
                            .y = (int32_t)(image.extent.height >> (i + 1)),
                            .z = 1};
    region.srcOffsets[1].x = std::max(1, region.srcOffsets[1].x);
    region.srcOffsets[1].y = std::max(1, region.srcOffsets[1].y);

    region.dstOffsets[1].x = std::max(1, region.dstOffsets[1].x);
    region.dstOffsets[1].y = std::max(1, region.dstOffsets[1].y);

    vkCmdBlitImage2(cmdBuffer, &blitInfo);

    imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarriers[0].srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
    imageBarriers[0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  }

  // Transition back
  {
    imageBarriers[0].srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
    imageBarriers[0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    imageBarriers[0].dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    imageBarriers[0].dstAccessMask = VK_ACCESS_2_NONE;
    imageBarriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    imageBarriers[0].newLayout = image.layout;
    imageBarriers[0].subresourceRange.baseMipLevel = 0;
    imageBarriers[0].subresourceRange.levelCount = image.mipLevels - 1;

    imageBarriers[1] = imageBarriers[0];
    imageBarriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageBarriers[1].subresourceRange.levelCount = 1;
    imageBarriers[1].subresourceRange.baseMipLevel = image.mipLevels - 1;

    vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
  }

  gContext.EndCmdBufferOneTime();
  gContext.OneTimeSubmit();

  return true;
}
