#include "Textures.hpp"

#include "Context.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::expected<Image, Textures::Error>
Textures::LoadFromMem(const void* data, VkExtent2D extent, uint8_t components,
                      VkImageLayout layout,
                      VkImageUsageFlags additionalUsageFlags) noexcept
{
  assert(layout != VK_IMAGE_LAYOUT_UNDEFINED);

  Image texture{};
  texture.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  texture.extent = extent;
  texture.mipLevels = CalcMipLevels(extent);

  switch (components)
  {
  case 1:
    texture.format = VK_FORMAT_R8_UNORM;
    break;

  case 2:
    texture.format = VK_FORMAT_R8G8_UNORM;
    break;

  case 4:
    texture.format = VK_FORMAT_R8G8B8A8_UNORM;
    break;

  default:
    return std::unexpected(Error::NO_SUPPORTED_COMPONENTS);
  }

  VkImageUsageFlags usageFlags =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if (gContext.HostImageTransferSupp())
  {
    usageFlags |= VK_IMAGE_USAGE_HOST_TRANSFER_BIT;
  }

  usageFlags |= additionalUsageFlags;

  if (!CreateImage(texture, usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
  {
    return std::unexpected(Error::FAILED_TO_CREATE_IMAGE);
  }

  if (!ImageCopyToDeviceAuto(
          texture, data, extent.width * extent.height * uint32_t(components)))
  {
    DestroyImage(texture);
    return std::unexpected(Error::FAILED_TO_COPY_TO_IMAGE);
  }

  if (CreateSampler(texture, VK_FALSE, VK_TRUE) != VK_SUCCESS)
  {
    DestroyImage(texture);
    return std::unexpected(Error::FAILED_TO_CREATE_SAMPLER);
  }

  if (layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    if (ImageTransitionLayoutAuto(texture, layout) != VK_SUCCESS)
    {
      DestroyImage(texture);
      return std::unexpected(Error::FAILED_TO_TRANSITION_IMAGE);
    }
  }

  if (texture.mipLevels > 1)
  {
    ImageGenMipmaps(texture);
  }

  return texture;
}

std::expected<Image, Textures::Error>
Textures::LoadFromFile(const char* path, VkImageLayout layout,
                       VkImageUsageFlags additionalUsageFlags) noexcept
{
  VkExtent2D extent{};

  int32_t components{};
  if (stbi_info(path, (int*)&extent.width, (int*)&extent.height, &components) !=
      1)
  {
    return std::unexpected(Error::FILE_COULD_NOT_BE_OPENED);
  }

  int32_t requestedComps = (components == 3) ? 4 : 0;

  auto imageData = stbi_load(path, (int*)&extent.width, (int*)&extent.height,
                             &components, requestedComps);
  if (!imageData)
  {
    return std::unexpected(Error::FILE_COULD_NOT_BE_OPENED);
  }

  components = (requestedComps != 0) ? requestedComps : components;

  auto res =
      LoadFromMem(imageData, extent, components, layout, additionalUsageFlags);
  stbi_image_free(imageData);
  return res;
}
